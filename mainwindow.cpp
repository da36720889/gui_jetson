#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "include/cyusb.h"
#include <QDebug>
#include <QMessageBox>
#include <cstring>
#include <QTime>
#include <QtGlobal>
#include <QDir>
#include <QFileInfo>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      data_buffer(),
      currentScene(new QGraphicsScene(this)),
      m_display2Scene(new QGraphicsScene(this)),
      m_display3Scene(new QGraphicsScene(this)),
      m_display4Scene(new QGraphicsScene(this)), // 0701
      m_imageStack(),
      m_currentIndex(-1),
      m_loopRunning(false),
      m_stopRequested(false),
      m_loopMutex(QMutex::NonRecursive),
      scanningThread(nullptr),
      h(nullptr),
      m_workerMutex(QMutex::NonRecursive)
{
    ui->setupUi(this);

    // Open USB device
    int device_count = cyusb_open();
    if (device_count < 1)
    {
        qCritical() << "[ERROR] No USB device found";
        h = nullptr;
        return;
    }

    // Obtain device handle
    h = cyusb_gethandle(0);
    if (!h)
    {
        qCritical() << "[ERROR] Failed to obtain USB device handle";
        cyusb_close();
        h = nullptr;
        return;
    }

    // Detach kernel driver
    int interface = 0;
    if (cyusb_kernel_driver_active(h, interface))
    {
        if (cyusb_detach_kernel_driver(h, interface) != 0)
        {
            qCritical() << "[ERROR] Failed to detach kernel driver";
            cyusb_close();
            h = nullptr;
            return;
        }
    }

    // Claim interface
    if (cyusb_claim_interface(h, interface) != 0)
    {
        qCritical() << "[ERROR] Failed to claim interface";
        cyusb_close();
        h = nullptr;
        return;
    }

    qDebug() << "[INFO] USB initialized successfully";

    // Initialize displays
    ui->display_1->setScene(currentScene);
    ui->display_2->setScene(m_display2Scene);
    ui->display_3->setScene(m_display3Scene);
    ui->display_3->viewport()->installEventFilter(this);

    ui->display_4->setScene(m_display4Scene); // 0701

    // Initialize sliders
    ui->slider_x->setEnabled(false);
    ui->slider_x->setRange(0, 0);
    ui->slider_y->setEnabled(false);
    ui->slider_y->setRange(0, GUI_display_height - 1);

    // Connect signals and slots
    connect(ui->pb_start, &QPushButton::clicked, this, &MainWindow::pb_start_clicked);
    connect(ui->pb_stop, &QPushButton::clicked, this, &MainWindow::pb_stop_clicked);
    connect(ui->pb_capture, &QPushButton::clicked, this, &MainWindow::pb_capture_clicked);
    connect(ui->pb_save, &QPushButton::clicked, this, &MainWindow::pb_save_clicked);
    connect(ui->slider_x, &QSlider::valueChanged, this, &MainWindow::slider_x_valueChanged);
    connect(ui->slider_y, &QSlider::valueChanged, this, &MainWindow::slider_y_valueChanged);

    connect(ui->pb_initialize, &QPushButton::clicked, this, &MainWindow::pb_initialize_clicked);
    connect(ui->pb_quit, &QPushButton::clicked, this, &MainWindow::pb_quit_clicked);

    connect(ui->pb_quantize, &QPushButton::clicked, this, &MainWindow::pb_quantize_clicked); // 0701

    radioGroup = new QButtonGroup(this);
    radioGroup->addButton(ui->rb_2d);
    radioGroup->addButton(ui->rb_3d);
    radioGroup->addButton(ui->rb_cross);
    radioGroup->addButton(ui->rb_wang);

    connect(ui->rb_2d, &QRadioButton::clicked, this, &MainWindow::rb_2d_clicked);
    connect(ui->rb_3d, &QRadioButton::clicked, this, &MainWindow::rb_3d_clicked);
    connect(ui->rb_cross, &QRadioButton::clicked, this, &MainWindow::rb_cross_clicked);
    connect(ui->rb_wang, &QRadioButton::clicked, this, &MainWindow::rb_wang_clicked);

    
    // **************** python fastapi ************************* //
    pythonFastApi = new QProcess(this);
    pythonFastApi->start("python3", QStringList() << "workspace/Inference/eval_fastapi_tensorrt_multiprocess_opencv.py");

    connect(pythonFastApi, &QProcess::readyReadStandardOutput, [=]() {
        QByteArray output = pythonFastApi->readAllStandardOutput();
        qDebug() << "[Python STDOUT]" << output.trimmed();
    });

    connect(pythonFastApi, &QProcess::readyReadStandardError, [=]() {
        QByteArray error = pythonFastApi->readAllStandardError();
        qWarning() << "[Python STDERR]" << error.trimmed();
    });

    connect(pythonFastApi, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus status) {
                qDebug() << "[Python Process Finished] Exit code:" << exitCode << ", Status:" << status;
            });

    pythonFastApi->start();

    if (!pythonFastApi->waitForStarted(1000000)) {
        qCritical() << "[ERROR] Failed to start eval_fastapi_tensorrt_multiprocess_opencv.py";
    } else {
        qDebug() << "[INFO] Background Python process started.";
    }
}

MainWindow::~MainWindow()
{
    if (scanningThread)
    {
        scanningThread->stop();
        scanningThread->wait();
        delete scanningThread;
        scanningThread = nullptr;
    }
    if (captureThread)
    {
        captureThread->stop();
        captureThread->wait();
        delete captureThread;
        captureThread = nullptr;
    }
    if (h)
    {
        cyusb_release_interface(h, 0);
        cyusb_close();
        h = nullptr;
    }
    if (pythonFastApi)
    {
        pythonFastApi->kill();  
        pythonFastApi->waitForFinished(5000);
        delete pythonFastApi;
        pythonFastApi = nullptr;
    }
    delete ui;
}

StartThread::StartThread(cyusb_handle *usbHandle, MainWindow *mainwindow)
    : QThread(mainwindow), m_stop(false), m_usbHandle(usbHandle), m_mainwindow(mainwindow) {}

void StartThread::stop()
{
    m_stop = true;
    qDebug() << "StartThread stop requested";
}

void StartThread::run()
{
    QByteArray scan_2d = QByteArray::fromHex("4D454D5300000011"); // 0x4D, 0x45, 0x4D, 0x53, mode, Range, Filp, Scan
    unsigned char out_endpoint = 0x01;
    int transferred_o_scan = 0;
    int scan_2d_o = cyusb_bulk_transfer(m_usbHandle, out_endpoint, (unsigned char *)scan_2d.constData(), scan_2d.size(), &transferred_o_scan, 3000);

    const QByteArray ABOIL = QByteArray::fromHex("41424F494C000014");
    unsigned char in_endpoint = 0x81;
    std::vector<unsigned char> chunk(4096 * 1000);
    qDebug() << "StartThread started";
    while (!m_stop && m_usbHandle)
    {
        int transferred_o = 0;
        int r_o = cyusb_bulk_transfer(m_usbHandle, out_endpoint, (unsigned char *)ABOIL.constData(), ABOIL.size(), &transferred_o, 5000);
        qDebug() << "r_o:" << r_o;
        if (r_o != 0)
        {
            emit errorOccurred(QString("Send failed: %1").arg(r_o));
            break;
        }

        int transferred_i = 0;
        int r_i = cyusb_bulk_transfer(m_usbHandle, in_endpoint, chunk.data(), chunk.size(), &transferred_i, 10000);
        qDebug() << "r_i:" << r_i << "transferred_i:" << transferred_i;

        if (r_i == 0 && transferred_i > 0)
        {
            QByteArray data(reinterpret_cast<const char *>(chunk.data()), transferred_i);
            emit dataReceived(data);
        }
        msleep(10);
    }
    qDebug() << "StartThread exited";
}

QVector<float> MainWindow::convertToFloat32LittleEndian(const QByteArray &data)
{
    QVector<float> buffer(data.size() / 4);
    const unsigned char *raw = reinterpret_cast<const unsigned char *>(data.constData());

    for (int i = 0; i < buffer.size(); ++i)
    {
        quint32 b0 = raw[i * 4];
        quint32 b1 = raw[i * 4 + 1];
        quint32 b2 = raw[i * 4 + 2];
        quint32 b3 = raw[i * 4 + 3];
        quint32 intBits = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;

        float f;
        std::memcpy(&f, &intBits, sizeof(float)); // reinterpret bits as float
        buffer[i] = f;
    }
    return buffer;
}

CaptureThread::CaptureThread(cyusb_handle *handle, MainWindow *mainwindow) //, QObject *parent)
    : QThread(mainwindow), h(handle), m_stop(false), m_mainwindow(mainwindow)
{
}

void CaptureThread::stop()
{
    m_stop = true;
}

void CaptureThread::run()
{
    const int one_frame_size = 4096 * 1000;
    int width = 1024, height = 1000;
    int iii = 0;
    int total_frames = 120;
    int quant_num = 0;

    selectedChunks.clear();
    selectedIndices.clear();
    qsrand(QTime::currentTime().msec()); 

    while (selectedIndices.size() < 1)		//number for model
    {
        selectedIndices.insert(qrand() % 100);
    }
    qDebug() << "selectedIndices : " << selectedIndices;
    QByteArray ABOIL = QByteArray::fromHex("41424F494C000078"); // 78 : 120, 6E : 110
    unsigned char out_endpoint = 0x01;
    unsigned char in_endpoint = 0x81;
    int transferred_o = 0;

    int r_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)ABOIL.constData(), ABOIL.size(), &transferred_o, 1000);
    if (r_o != 0)
    {
        emit captureError(QString("Send failed: %1").arg(r_o));
        return;
    }

    while (!m_stop && iii < total_frames)
    {
        QElapsedTimer timer; // time calculate

        std::vector<unsigned char> chunk(one_frame_size);
        int transferred_i = 0;
        int r_i = cyusb_bulk_transfer(h, in_endpoint, chunk.data(), one_frame_size, &transferred_i, 30000);
        timer.start();
        if (r_i != 0 || transferred_i != one_frame_size)
        {
            emit captureError(QString("Receive failed: %1").arg(r_i));
            qDebug() << "[CaptureThread] Transfer failed! r_i =" << r_i
                     << ", transferred_i =" << transferred_i
                     << ", expected =" << one_frame_size;
            break; 
        }

        QByteArray chunkData(reinterpret_cast<char *>(chunk.data()), transferred_i);
        emit rawDataReceived_cap(chunkData);

        QVector<float> floatData = m_mainwindow->convertToFloat32LittleEndian(chunkData);

        QImage img(width, height, QImage::Format_Grayscale8);
        double sum = 0.0;
        int num = 0;
        for (int y = height/2; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                sum += floatData[y * width + x];
                ++num;
            }
        }
        float half_noise = (num > 0) ? static_cast<float>(sum / num) : 0.0f;
        float maxAfter = 0.0f;
        for (int i = 0; i < floatData.size(); ++i) {
            float v = floatData[i] - half_noise;
            if (v > maxAfter) maxAfter = v;
        }
        float denom = (maxAfter > 1e-6f) ? maxAfter : 1e-6f;

        for (int y = 0; y < height; ++y) {
            uchar *scanLine = img.scanLine(y);
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                float v = floatData[idx] - half_noise;
                if (v < 0.0f) v = 0.0f;
                int gray = static_cast<int>(255.0f * (v / denom));
                scanLine[x] = static_cast<uchar>(qBound(0, gray, 255));
            }
        }

        QVector<float> data4model = m_mainwindow->normalizeAndRotateFloatImage(floatData, width, height); // for training

        if (selectedIndices.contains(iii))
        {
            QByteArray byteArray4model(reinterpret_cast<const char *>(floatData.constData()), floatData.size() * sizeof(float));
            selectedChunks.append(byteArray4model);

            QString fileName = QString("quant_raw/test/frame_%1.bin").arg(quant_num);
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly))
            {
                file.write(byteArray4model);
                file.close();
                qDebug() << "Saved instantly:" << fileName;
            }
            else
            {
                qDebug() << "Failed to open:" << fileName;
            }
            quant_num = quant_num + 1;
        }

        qint64 elapsed_ms = timer.elapsed();
        emit oneFrameCaptured(img, elapsed_ms);
        iii++;
    }

    emit captureFinished();
}

QImage MainWindow::scaleFloatTo8Bit(const QVector<float> &data, int width, int height)
{
    QImage img(width, height, QImage::Format_Grayscale8);
    if (data.size() != width * height)
    {
        qWarning() << "Data size mismatch";
        return img;
    }

    double sum = 0.0;
    int num = 0;
    for (int y = height / 2; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = y * width + x;
            sum += data[idx];
            num++;
        }
    }
    float half_noise = (num > 0) ? (sum / num) : 0.0f;

    QVector<float> noiseRemoved(data.size());
    for (int i = 0; i < data.size(); ++i)
    {
        float v = data[i] - half_noise;
        noiseRemoved[i] = (v < 0) ? 0.0f : v;
    }

    float maxVal = *std::max_element(noiseRemoved.constBegin(), noiseRemoved.constEnd());

    for (int i = 0; i < data.size(); ++i)
    {
        img.bits()[i] = (maxVal > 1e-6f) ? static_cast<uchar>(noiseRemoved[i] * 255.0f / maxVal) : 0;
    }

    return img;
}


QVector<float> MainWindow::normalizeAndRotateFloatImage(const QVector<float> &inputData, int width, int height)
{

    if (inputData.size() != width * height)
    {
        qWarning() << "Data size mismatch";
        return {};
    }

    double sum = 0.0;
    int num = 0;
    for (int y = height / 2; y < height; ++y) 
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = y * width + x;
            sum += inputData[idx];
            num++;
        }
    }
    float half_noise = (num > 0) ? (sum / num) : 0.0f;

    QVector<float> noiseRemoved(inputData.size());
    for (int i = 0; i < inputData.size(); ++i)
    {
        float v = inputData[i] - half_noise;
        noiseRemoved[i] = (v < 0) ? 0.0f : v;
    }

    float maxVal = *std::max_element(noiseRemoved.constBegin(), noiseRemoved.constEnd());

    QVector<float> normalized(inputData.size());
    for (int i = 0; i < inputData.size(); ++i)
    {
        normalized[i] = (maxVal > 1e-6f) ? (noiseRemoved[i] * 255.0f / maxVal) : 0.0f;
    }

    QVector<float> rotated(inputData.size());
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int srcIndex = y * width + x;
            int dstIndex = x * height + (height - 1 - y);
            rotated[dstIndex] = normalized[srcIndex];
        }
    }

    return rotated;
}


QImage MainWindow::generateEnfaceImage(int column)
{
    QElapsedTimer timer_enface; // time calculate
    timer_enface.start();

    int width = 1024;
    if (m_imageStack.isEmpty() || column < 0 || column >= width)
    {
        qDebug() << "Invalid parameters for enface image generation";
        return QImage();
    }

    QImage enfaceImg(m_imageStack.size(), GUI_display_height, QImage::Format_Grayscale8);
    enfaceImg.fill(0);

    for (int y = 0; y < GUI_display_height; ++y)
    {
        uchar *dstLine = enfaceImg.scanLine(y);
        for (int i = 0; i < m_imageStack.size(); ++i)
        {
            const QImage &srcImg = m_imageStack[i];
            if (srcImg.width() != width || srcImg.height() != GUI_display_height)
            {
                qWarning() << "Image size mismatch in enface generation";
                return QImage();
            }
            const uchar *srcLine = srcImg.scanLine(y);
            dstLine[i] = srcLine[column];
        }
    }
    qint64 elapsed_enface_ms = timer_enface.elapsed();
    qDebug() << "time to generate enface: " << elapsed_enface_ms << "ms";
    return enfaceImg;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->display_3->viewport() && event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        qreal scaleFactor = (wheelEvent->angleDelta().y() > 0) ? 1.05 : 0.95;
        ui->display_3->scale(scaleFactor, scaleFactor);
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}
