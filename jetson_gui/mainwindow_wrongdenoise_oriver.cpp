#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "include/cyusb.h"
#include <QDebug>
#include <QMessageBox>
#include "include/cyusb.h"
#include <cstring>
#include <QTime>
#include <QtGlobal>
#include <QDir>
#include <QFileInfo>
#include <cmath>

// 2025/02/15 - diplay_1
// 2025/02/16 - diplay_2

// int GUI_display_height = 1000;
// int GUI_display_width = 2048;

// MainWindow::MainWindow(QWidget *parent)
//     : QMainWindow(parent), ui(new Ui::MainWindow),
//       m_workerMutex(QMutex::NonRecursive), m_loopMutex(QMutex::NonRecursive),
//       currentScene(new QGraphicsScene(this)), m_display2Scene(new QGraphicsScene(this)),
//       m_display3Scene(new QGraphicsScene(this)), h(nullptr)
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
    // unsigned char out_endpoint = 0x01;
    unsigned char in_endpoint = 0x81;
    const size_t buffer_size = 4096 * 1000;
    unsigned char *aligned_chunk = static_cast<unsigned char *>(aligned_alloc(64, buffer_size));
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
    free(aligned_chunk);
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

// qint64 testImageProcessing(MainWindow *mw, QByteArray &chunkData)
// {
//     QElapsedTimer timer;
//     timer.start();

//     QVector<float> floatData = mw->convertToFloat32LittleEndian(chunkData);

//     float minVal = *std::min_element(floatData.constBegin(), floatData.constEnd());
//     float maxVal = *std::max_element(floatData.constBegin(), floatData.constEnd());

//     QImage img(1024, 1000, QImage::Format_Grayscale8);
//     for (int y = 0; y < 1000; ++y)
//     {
//         uchar *scanLine = img.scanLine(y);
//         for (int x = 0; x < 1024; ++x)
//         {
//             int idx = y * 1024 + x;
//             float val = floatData[idx];
//             int gray = static_cast<int>(255.0f * (val - minVal) / (maxVal - minVal + 1e-6f));
//             scanLine[x] = static_cast<uchar>(qBound(0, gray, 255));
//         }
//     }

//     return timer.elapsed();
// }

void CaptureThread::run()
{
    const int one_frame_size = 4096 * 1000;
    int width = 1024, height = 1000;
    int iii = 0;
    int total_frames = 120;
    quint64 total_elapsed_time = 0;
    int quant_num = 0;

    selectedChunks.clear();
    selectedIndices.clear();
    qsrand(QTime::currentTime().msec()); // 初始化隨機種子，只需要呼叫一次

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
        // qDebug() << "r_i = " << r_i;
        timer.start();
        if (r_i != 0 || transferred_i != one_frame_size)
        {
            emit captureError(QString("Receive failed: %1").arg(r_i));
            // qDebug() << "Calling saveSelectedChunks, selectedChunks.size() =" << selectedChunks.size();
            // saveSelectedChunks("quant_raw");
            // return;
            qDebug() << "[CaptureThread] Transfer failed! r_i =" << r_i
                     << ", transferred_i =" << transferred_i
                     << ", expected =" << one_frame_size;
            break; // 不 return，保留已收到的資料
        }

        // chunk.resize(transferred_i);
        // data_buffer.insert(data_buffer.end(), chunk.begin(), chunk.end());

        QByteArray chunkData(reinterpret_cast<char *>(chunk.data()), transferred_i);
        emit rawDataReceived_cap(chunkData);

        QVector<float> floatData = m_mainwindow->convertToFloat32LittleEndian(chunkData);

        float minVal = *std::min_element(floatData.constBegin(), floatData.constEnd());
        float maxVal = *std::max_element(floatData.constBegin(), floatData.constEnd());

        QImage img(width, height, QImage::Format_Grayscale8);
        for (int y = 0; y < height; ++y)
        {
            uchar *scanLine = img.scanLine(y);
            for (int x = 0; x < width; ++x)
            {
                int idx = y * width + x;
                float val = floatData[idx];
                int gray = static_cast<int>(255.0f * (val - minVal) / (maxVal - minVal + 1e-6f));
                scanLine[x] = static_cast<uchar>(qBound(0, gray, 255));
            }
        }

        // qint64 elapsed_ms_test = testImageProcessing(m_mainwindow, chunkData); // 123
        // emit oneFrameCaptured(img, elapsed_ms_test);                           // 123
        // QVector<float> data4model = m_mainwindow->normalizeAndCropFloatImage(floatData, width, height); // for training

        QVector<float> data4model = m_mainwindow->normalizeAndRotateFloatImage(floatData, width, height); // for training

        if (selectedIndices.contains(iii)) // random save 10 frames
        // if (iii < 70 && iii > 60)
        {
            
            QByteArray byteArray4model(reinterpret_cast<const char *>(floatData.constData()), floatData.size() * sizeof(float));
            // QByteArray byteArray4model(reinterpret_cast<const char *>(data4model.constData()), data4model.size() * sizeof(float));
            selectedChunks.append(byteArray4model);

            QString fileName = QString("quant_raw/frame_%1.bin").arg(quant_num);
            // QString fileName = QString("quant_raw/frame_%1.bin").arg(iii);
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

        qint64 elapsed_ms = timer.elapsed(); // time calculate
        emit oneFrameCaptured(img, elapsed_ms);
        total_elapsed_time = total_elapsed_time + elapsed_ms; // elapsed_ms_test;
        iii++;
      //  qDebug() << "No." << iii << " frame data received.";
    }
    //qDebug() << "total " << iii << " frames average processing time: " << (total_elapsed_time / iii) << "ms";
    emit captureFinished();

    // saveSelectedChunks("quant_raw");
    // qDebug() << "Final selectedIndices: " << selectedIndices;
}

void CaptureThread::saveSelectedChunks(const QString &folderPath) // not using anymore //use for pre-save in buffer, save it after all frame captured
{
    QDir().mkpath(folderPath);
    for (int i = 0; i < selectedChunks.size(); ++i)
    {
        QString fileName = QString("%1/frame_%2.bin").arg(folderPath).arg(i);
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(selectedChunks[i]);
            qDebug() << "Saved:" << fileName;
        }
        else
        {
            qDebug() << "Failed to write file:" << fileName;
        }
    }
}

// QVector<quint16> MainWindow::convertTo16BitLittleEndian(const QByteArray &data)      //0701
// {
//     QVector<quint16> buffer(data.size() / 2);
//     const unsigned char *raw = reinterpret_cast<const unsigned char *>(data.constData());

//     for (int i = 0; i < buffer.size(); ++i)
//     {
//         quint16 low = raw[i * 2];
//         quint16 high = raw[i * 2 + 1];
//         buffer[i] = (high << 8) | low;
//     }
//     return buffer;
// }

// QImage MainWindow::scale16BitTo8Bit(const QVector<quint16> &data, int width, int height)     //0701
// {
//     QImage img(width, height, QImage::Format_Grayscale8);
//     if (data.size() != width * height)
//     {
//         qWarning() << "Data size mismatch";
//         return img;
//     }

//     quint16 minVal = *std::min_element(data.constBegin(), data.constEnd());
//     quint16 maxVal = *std::max_element(data.constBegin(), data.constEnd());
//     float range = maxVal - minVal + 1;

//     for (int i = 0; i < data.size(); ++i)
//     {
//         img.bits()[i] = static_cast<uchar>((data[i] - minVal) * 255 / range);
//     }
//     return img;
// }

//****************************************************************************************************************** */
//****************************************************************************************************************** */     //ver.1 use img.bits()[i] write in (1D)
QImage MainWindow::scaleFloatTo8Bit(const QVector<float> &data, int width, int height)
{
    QImage img(width, height, QImage::Format_Grayscale8);
    if (data.size() != width * height)
    {
        qWarning() << "Data size mismatch";
        return img;
    }

    float minVal = *std::min_element(data.constBegin(), data.constEnd());
    float maxVal = *std::max_element(data.constBegin(), data.constEnd());
    float range = maxVal - minVal + 1e-6;

    for (int i = 0; i < data.size(); ++i)
    {
        img.bits()[i] = static_cast<uchar>((data[i] - minVal) * 255.0f / range);
    }
    return img;
}

// QVector<float> MainWindow::normalizeAndCropFloatImage(const QVector<float> &inputData, int inputWidth, int inputHeight)      //crop and normalize fp image
// {
//     const int cropWidth = 370;
//     const int cropHeight = 944;

//     if (inputData.size() != inputWidth * inputHeight)
//     {
//         qWarning() << "Data size mismatch";
//         return {};
//     }

//     // Normalize 到 0~255（float）
//     float minVal = *std::min_element(inputData.constBegin(), inputData.constEnd());
//     float maxVal = *std::max_element(inputData.constBegin(), inputData.constEnd());
//     float range = maxVal - minVal + 1e-6f;

//     QVector<float> normalized(inputData.size());
//     for (int i = 0; i < inputData.size(); ++i)
//     {
//         normalized[i] = (inputData[i] - minVal) * 255.0f / range;
//     }

//     // 做裁剪：假設從左上角開始裁剪（x=0, y=0）
//     QVector<float> cropped;
//     cropped.reserve(cropWidth * cropHeight);

//     for (int y = 0; y < cropHeight; ++y)
//     {
//         for (int x = 0; x < cropWidth; ++x)
//         {
//             int index = y * inputWidth + x; // 注意是原始寬度
//             cropped.append(normalized[index]);
//         }
//     }

//     QVector<float> rotated;
//     rotated.resize(cropWidth * cropHeight);

//     for (int y = 0; y < cropHeight; ++y)
//     {
//         for (int x = 0; x < cropWidth; ++x)
//         {
//             int srcIndex = y * cropWidth + x;
//             int dstIndex = x * cropHeight + (cropHeight - 1 - y); // 順時針轉90度
//             rotated[dstIndex] = cropped[srcIndex];
//         }
//     }
//     return rotated;
// }

QVector<float> MainWindow::normalizeAndRotateFloatImage(const QVector<float> &inputData, int width, int height)
{

    // int width = 1024;
    // int height = 1000;
    if (inputData.size() != width * height)
    {
        qWarning() << "Data size mismatch";
        return {};
    }

    // QVector<float> recovered(inputData.size());
    // for (int i = 0; i < inputData.size(); ++i)
    // {
    //     if (inputData[i] > -std::numeric_limits<float>::infinity())
    //         // recovered[i] = std::exp(inputData[i]); // 如果原來是 log10，請改用 std::pow(10, inputData[i])
    //         recovered[i] = std::pow(10.0f, inputData[i]);
    //     else
    //         recovered[i] = 0.0f;
    // }

    float minVal = *std::min_element(inputData.constBegin(), inputData.constEnd());
    float maxVal = *std::max_element(inputData.constBegin(), inputData.constEnd());

    // float minVal = *std::min_element(recovered.constBegin(), recovered.constEnd());
    // float maxVal = *std::max_element(recovered.constBegin(), recovered.constEnd());
    float range = maxVal - minVal + 1e-6f;

    QVector<float> normalized(inputData.size());
    // for (int i = 0; i < recovered.size(); ++i)
    for (int i = 0; i < inputData.size(); ++i)
    {
        normalized[i] = (inputData[i] - minVal) * 256.0f / range;
        // normalized[i] = (recovered[i] - minVal) * 255.0f / range;
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

//****************************************************************************************************************** */
//****************************************************************************************************************** */

//****************************************************************************************************************** */
//****************************************************************************************************************** */     //ver.2 use scanline(y) write in(2D)
// QImage MainWindow::scaleFloatTo8Bit(const QVector<float> &data, int width, int height)
// {
//     QImage img(width, height, QImage::Format_Grayscale8);
//     if (data.size() != width * height)
//     {
//         qWarning() << "Data size mismatch";
//         return img;
//     }

//     float minVal = *std::min_element(data.constBegin(), data.constEnd());
//     float maxVal = *std::max_element(data.constBegin(), data.constEnd());
//     float range = maxVal - minVal + 1e-6;

//     for (int y = 0; y < height; ++y)
//     {
//         const float *src = data.constData() + y * width;
//         uchar *dst = img.scanLine(y);
//         for (int x = 0; x < width; ++x)
//         {
//             float normalized = (src[x] - minVal) / range;
//             dst[x] = static_cast<uchar>(normalized * 255.0f);
//         }
//     }
//     return img;
// }
//****************************************************************************************************************** */
//****************************************************************************************************************** */

// QImage MainWindow::scaleTo8Bit(const QVector<quint16> &data)     //0701
// {
//     QImage img(GUI_display_width, GUI_display_height, QImage::Format_Grayscale8);

//     quint16 minVal = *std::min_element(data.constBegin(), data.constEnd());
//     quint16 maxVal = *std::max_element(data.constBegin(), data.constEnd());
//     float range = maxVal - minVal + 1;

//     for (int y = 0; y < GUI_display_height; ++y)
//     {
//         const quint16 *src = data.constData() + y * GUI_display_width;
//         uchar *dst = img.scanLine(y);
//         for (int x = 0; x < GUI_display_width; ++x)
//         {
//             float normalized = (src[x] - minVal) / range;
//             dst[x] = static_cast<uchar>(normalized * 255.0f);
//         }
//     }
//     return img;
// }

// QImage MainWindow::scaleTo8Bit(const QVector<float> &data)
// {
//     QImage img(GUI_display_width, GUI_display_height, QImage::Format_Grayscale8);

//     float minVal = *std::min_element(data.constBegin(), data.constEnd());
//     float maxVal = *std::max_element(data.constBegin(), data.constEnd());
//     float range = maxVal - minVal + 1;

//     for (int y = 0; y < GUI_display_height; ++y)
//     {
//         const float *src = data.constData() + y * GUI_display_width;
//         uchar *dst = img.scanLine(y);
//         for (int x = 0; x < GUI_display_width; ++x)
//         {
//             float normalized = (src[x] - minVal) / range;
//             dst[x] = static_cast<uchar>(normalized * 255.0f);
//         }
//     }
//     return img;
// }

// QImage MainWindow::generateEnfaceImage(int height)
// {
//     if (m_imageStack.isEmpty() || height < 0 || height >= GUI_display_height)
//     {
//         qDebug() << "Invalid parameters for enface image generation";
//         return QImage();
//     }

//     QImage enfaceImg(GUI_display_width, m_imageStack.size(), QImage::Format_Grayscale8);
//     enfaceImg.fill(0);

//     for (int i = 0; i < m_imageStack.size(); ++i)
//     {
//         const QImage &srcImg = m_imageStack[i];
//         if (srcImg.width() != GUI_display_width || srcImg.height() != GUI_display_height)
//         {
//             qWarning() << "Image size mismatch in enface generation";
//             return QImage();
//         }

//         const uchar *srcLine = srcImg.scanLine(height);
//         uchar *dstLine = enfaceImg.scanLine(i);
//         memcpy(dstLine, srcLine, GUI_display_width);
//     }

//     return enfaceImg;
// }

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
    qint64 elapsed_enface_ms = timer_enface.elapsed(); // time calculate
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
        // qDebug() << "[INFO] Display3 scaled by factor:" << scaleFactor;
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

/*
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      m_workerMutex(QMutex::NonRecursive), m_loopMutex(QMutex::NonRecursive),
      currentScene(new QGraphicsScene(this)), m_display2Scene(new QGraphicsScene(this)),
      m_display3Scene(new QGraphicsScene(this)), h(nullptr)
{
    ui->setupUi(this);

    // Open USB device
    int device_count = cyusb_open();
    if (device_count < 1) {
        qCritical() << "[ERROR] No USB device found";
        h = nullptr;
        return;
    }

    // Obtain device handle
    h = cyusb_gethandle(0);
    if (!h) {
        qCritical() << "[ERROR] Failed to obtain USB device handle";
        cyusb_close();
        h = nullptr;
        return;
    }

    // Detach kernel driver
    int interface = 0;
    if (cyusb_kernel_driver_active(h, interface)) {
        if (cyusb_detach_kernel_driver(h, interface) != 0) {
            qCritical() << "[ERROR] Failed to detach kernel driver";
            cyusb_close();
            h = nullptr;
            return;
        }
    }

    // Claim interface
    if (cyusb_claim_interface(h, interface) != 0) {
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
}

MainWindow::~MainWindow()
{
    if (scanningThread) {
        scanningThread->stop();
        scanningThread->wait();
        delete scanningThread;
    }
    if (h) {
        cyusb_release_interface(h, 0);
        cyusb_close();
    }
    delete ui;
}

StartThread::StartThread(cyusb_handle *usbHandle, MainWindow *parent)
    : QThread(parent), m_stop(false), m_usbHandle(usbHandle), m_parent(parent) {}

void StartThread::stop()
{
    m_stop = true;
    qDebug() << "StartThread stop requested";
}
*/
