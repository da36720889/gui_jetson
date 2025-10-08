#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "include/cyusb.h"
#include <QDebug>
#include <QMessageBox>
#include "include/cyusb.h"
#include <cstring>

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
      m_worker(nullptr),
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
    if (m_worker)
    {
        m_worker->stop();
        m_worker->wait();
        delete m_worker;
    }
    if (h)
    {
        cyusb_release_interface(h, 0);
        cyusb_close();
    }
    delete ui;
}

WorkerThread::WorkerThread(cyusb_handle *usbHandle, MainWindow *parent)
    : QThread(parent), m_stop(false), m_usbHandle(usbHandle), m_parent(parent) {}

void WorkerThread::stop()
{
    m_stop = true;
    qDebug() << "WorkerThread stop requested";
}

void WorkerThread::run()
{
    const QByteArray ABOIL = QByteArray::fromHex("41424F494C00000A");
    unsigned char out_endpoint = 0x01;
    unsigned char in_endpoint = 0x81;
    const size_t buffer_size = 4096 * 1000;
    unsigned char *aligned_chunk = static_cast<unsigned char *>(aligned_alloc(64, buffer_size));

    qDebug() << "WorkerThread started";
    while (!m_stop && m_usbHandle)
    {
        int transferred_o = 0;
        int r_o = cyusb_bulk_transfer(m_usbHandle, out_endpoint, (unsigned char *)ABOIL.constData(), ABOIL.size(), &transferred_o, 1000);
        qDebug() << "r_o:" << r_o;
        if (r_o != 0)
        {
            emit errorOccurred(QString("Send failed: %1").arg(r_o));
            break;
        }

        std::vector<unsigned char> chunk(4096 * 1000);
        int transferred_i = 0;
        int r_i = cyusb_bulk_transfer(m_usbHandle, in_endpoint, chunk.data(), chunk.size(), &transferred_i, 10000);
        qDebug() << "r_i:" << r_i << "transferred_i:" << transferred_i;

        if (r_i == 0 && transferred_i > 0)
        {
            QByteArray data(reinterpret_cast<const char *>(chunk.data()), transferred_i);
            emit dataReceived(data);
        }
        msleep(1100);
    }
    free(aligned_chunk);
    qDebug() << "WorkerThread exited";
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
    float range = maxVal - minVal + 1e-6; // 防止除以 0

    for (int i = 0; i < data.size(); ++i)
    {
        img.bits()[i] = static_cast<uchar>((data[i] - minVal) * 255.0f / range);
    }
    return img;
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

QImage MainWindow::scaleTo8Bit(const QVector<float> &data)
{
    QImage img(GUI_display_width, GUI_display_height, QImage::Format_Grayscale8);

    float minVal = *std::min_element(data.constBegin(), data.constEnd());
    float maxVal = *std::max_element(data.constBegin(), data.constEnd());
    float range = maxVal - minVal + 1;

    for (int y = 0; y < GUI_display_height; ++y)
    {
        const float *src = data.constData() + y * GUI_display_width;
        uchar *dst = img.scanLine(y);
        for (int x = 0; x < GUI_display_width; ++x)
        {
            float normalized = (src[x] - minVal) / range;
            dst[x] = static_cast<uchar>(normalized * 255.0f);
        }
    }
    return img;
}

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
    if (m_worker) {
        m_worker->stop();
        m_worker->wait();
        delete m_worker;
    }
    if (h) {
        cyusb_release_interface(h, 0);
        cyusb_close();
    }
    delete ui;
}

WorkerThread::WorkerThread(cyusb_handle *usbHandle, MainWindow *parent)
    : QThread(parent), m_stop(false), m_usbHandle(usbHandle), m_parent(parent) {}

void WorkerThread::stop()
{
    m_stop = true;
    qDebug() << "WorkerThread stop requested";
}
*/
