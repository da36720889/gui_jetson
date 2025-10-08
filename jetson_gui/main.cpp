#include "mainwindow.h"
// need to add Cypress library in order to call functions
#include "include/cyusb.h" //problem libusb.h is only for linux system!
#include <pthread.h>       //2thread need to be added, 1for sending ABOIL014, the other one is for connect to camera(?)
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QMutex>
#include <QMutexLocker>
// #include <QtConcurrent>   //no such lib in Qt4, only Qt5 has it
#include <QThread>
#include <QMetaObject>
#include <QTimer>
#include <queue>
#include <QGraphicsPixmapItem>
// #include <libusb.h>
// #include <QDir>

cyusb_handle *h = NULL;

int GUI_display_height = 1000; // 4096
int GUI_display_width = 2048;  // 1000

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

void MainWindow::pb_quantize_clicked()
{
    QFile readfile("quant_raw/processed_golden.bin");
    // qDebug() << "Current working dir:" << QDir::currentPath();

    if (!readfile.open(QFile::ReadOnly))
    {
        qWarning() << "Cannot open file!";
        return;
    }
    QByteArray rawdata_quant = readfile.readAll();
    readfile.close();

    int width = 1024;
    int height = 1000;

    const float *floatData = reinterpret_cast<const float *>(rawdata_quant.constData());

    float minVal = floatData[0];
    float maxVal = floatData[0];
    for (int i = 1; i < width * height; ++i)
    {
        if (floatData[i] < minVal)
            minVal = floatData[i];
        if (floatData[i] > maxVal)
            maxVal = floatData[i];
    }

    QImage image(width, height, QImage::Format_Grayscale8);
    for (int y = 0; y < height; ++y)
    {
        uchar *scanLine = image.scanLine(y);
        for (int x = 0; x < width; ++x)
        {
            int idx = y * width + x;
            // normalize to 0-255
            float val = floatData[idx];
            int gray = static_cast<int>(255.0f * (val - minVal) / (maxVal - minVal));
            gray = qBound(0, gray, 255);
            scanLine[x] = static_cast<uchar>(gray);
        }
    }
    updateDisplay4(image);
}

void MainWindow::updateDisplay4(const QImage &img) // 預設顯示上半部，可以用滾輪看底下的部分
{
    if (img.isNull())
        return;

    m_display4Scene->clear();

    QPixmap rotatedPixmap = QPixmap::fromImage(img).transformed(QTransform().rotate(90));

    QGraphicsPixmapItem *item = m_display4Scene->addPixmap(rotatedPixmap);
    item->setTransformationMode(Qt::SmoothTransformation);

    ui->display_4->setScene(m_display4Scene);
    ui->display_4->resetTransform();

    ui->display_4->setDragMode(QGraphicsView::NoDrag);
    ui->display_4->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->display_4->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->display_4->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    QRectF sceneRect = m_display4Scene->itemsBoundingRect();
    m_display4Scene->setSceneRect(sceneRect);

    qreal viewWidth = ui->display_4->viewport()->width();
    qreal zoomFactor = viewWidth / sceneRect.width();
    ui->display_4->scale(zoomFactor, zoomFactor);

    // move the viewpoint to upper part
    ui->display_4->centerOn(sceneRect.center().x(), sceneRect.top() + sceneRect.height() / 4);

    ui->display_4->viewport()->update();
}

// void MainWindow::updateDisplay4(const QImage &img)       //只顯示上半部
// {
//     m_display4Scene->clear();
//     int width = 1024;
//     int height = 1000;
//     QImage upperHalf = img.copy(0, 0, img.width()/2, img.height());
//     QPixmap pix = QPixmap::fromImage(upperHalf).scaled(ui->display_4->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation).transformed(QTransform().rotate(90));
//     m_display4Scene->addPixmap(pix);
//     ui->display_4->fitInView(m_display4Scene->itemsBoundingRect(), Qt::KeepAspectRatio);
//     ui->display_4->viewport()->update();
// }

void MainWindow::rb_2d_clicked()
{
    QByteArray scan_2d = QByteArray::fromHex("4D454D5300000011"); // 0x4D, 0x45, 0x4D, 0x53, mode, Range, Filp, Scan
    unsigned char out_endpoint = 0x01;
    int transferred_o = 0;
    int scan_2d_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)scan_2d.constData(), scan_2d.size(), &transferred_o, 3000);
    if (scan_2d_o)
    {
        qDebug() << "scanning mode - 2D selected!";
    }
}
void MainWindow::rb_3d_clicked()
{
    QByteArray scan_3d = QByteArray::fromHex("4D454D5301000011");
    unsigned char out_endpoint = 0x01;
    int transferred_o = 0;
    int scan_3d_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)scan_3d.constData(), scan_3d.size(), &transferred_o, 3000);
    if (scan_3d_o)
    {
        qDebug() << "scanning mode - 3D selected!";
    }
}
void MainWindow::rb_cross_clicked()
{
    QByteArray scan_cross = QByteArray::fromHex("4D454D5302000011");
    unsigned char out_endpoint = 0x01;
    int transferred_o = 0;
    int scan_cross_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)scan_cross.constData(), scan_cross.size(), &transferred_o, 3000);
    if (scan_cross_o)
    {
        qDebug() << "scanning mode - cross selected!";
    }
}
void MainWindow::rb_wang_clicked()
{
    QByteArray scan_wang = QByteArray::fromHex("4D454D5303000011");
    unsigned char out_endpoint = 0x01;
    int transferred_o = 0;
    int scan_wang_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)scan_wang.constData(), scan_wang.size(), &transferred_o, 3000);
    if (scan_wang_o)
    {
        qDebug() << "scanning mode - 2D selected!";
    }
}

void MainWindow::pb_initialize_clicked()
{
    QByteArray rst = QByteArray::fromHex("00525354");
    QByteArray dummy = QByteArray::fromHex("0000000000000000");
    QByteArray initialize = QByteArray::fromHex("4D454D5300000001");
    unsigned char out_endpoint = 0x01;
    int transferred_o = 0;
    int rst_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)rst.constData(), rst.size(), &transferred_o, 3000);
    if (rst_o)
    {
        qDebug() << "rst set";
    }
    // msleep(1000);

    int dummy_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)dummy.constData(), dummy.size(), &transferred_o, 3000);
    if (dummy_o)
    {
        qDebug() << "rst set";
    }
    // msleep(1000);

    int initialize_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)initialize.constData(), initialize.size(), &transferred_o, 3000);
    if (initialize_o)
    {
        qDebug() << "initialize set";
    }
    // msleep(1000);
}

int is_endpoint_halted(cyusb_handle *handle, unsigned char endpoint)
{
    unsigned char data[2] = {0};
    int r = libusb_control_transfer(
        (libusb_device_handle *)handle,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_ENDPOINT,
        LIBUSB_REQUEST_GET_STATUS,
        0,
        endpoint,
        data,
        sizeof(data),
        1000 // timeout
    );
    if (r < 0)
    {
        fprintf(stderr, "GET_STATUS failed: %s\n", libusb_error_name(r));
        return -1;
    }
    return (data[0] & 1); // bit 0 表示是否 halt
}

void MainWindow::pb_quit_clicked()
{
    qDebug() << "quit clicked";
    // clearImageStack();
    // if (m_worker && m_worker->isRunning())
    // {
    //     m_worker = nullptr;
    //     delete m_worker;
    // }
    // qDebug() << "stack n thread cleared";

    int status = 0;
    unsigned char out_endpoint = 0x01;
    unsigned char in_endpoint = 0x81;

    if (is_endpoint_halted(h, out_endpoint))
    {
        printf(" out_endpoint is halted, clearing it\n");
        cyusb_clear_halt(h, out_endpoint);
    }
    qDebug() << "out_endpoint finished ";

    if (is_endpoint_halted(h, in_endpoint))
    {
        printf(" in_endpoint is halted, clearing it\n");
        cyusb_clear_halt(h, in_endpoint);
    }
    qDebug() << "in_endpoint finished ";
}

// void MainWindow::updateDisplay(const QImage &img)
// {
//     if (img.isNull())
//     {
//         qWarning() << "Received null image for display1";
//         return;
//     }

//     currentScene->clear();
//     QPixmap pix = QPixmap::fromImage(img).scaled(ui->display_1->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
//     currentScene->addPixmap(pix);
//     ui->display_1->fitInView(currentScene->itemsBoundingRect(), Qt::KeepAspectRatio);
//     ui->display_1->viewport()->update(); // force to update display     //0417
// }

void MainWindow::updateDisplay(const QImage &img)
{
    if (img.isNull())
    {
        qWarning() << "Received null image for display1";
        return;
    }

    currentScene->clear();
    QPixmap pix = QPixmap::fromImage(img).scaled(ui->display_1->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation).transformed(QTransform().rotate(90));
    currentScene->addPixmap(pix);
    ui->display_1->fitInView(currentScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    ui->display_1->viewport()->update(); // force to update display     //0417
}

void MainWindow::updateDisplay2(const QImage &img)
{
    if (img.isNull())
    {
        qWarning() << "Received null image for display2";
        return;
    }

    m_display2Scene->clear();
    QPixmap pix = QPixmap::fromImage(img).transformed(QTransform().rotate(90));

    QGraphicsPixmapItem *item = m_display2Scene->addPixmap(pix);
    item->setTransformationMode(Qt::SmoothTransformation);

    ui->display_2->setScene(m_display2Scene);
    ui->display_2->resetTransform();

    ui->display_2->setDragMode(QGraphicsView::NoDrag);
    ui->display_2->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->display_2->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->display_2->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    QRectF sceneRect = m_display2Scene->itemsBoundingRect();
    m_display2Scene->setSceneRect(sceneRect);

    qreal viewWidth = ui->display_2->viewport()->width();
    qreal zoomFactor = viewWidth / sceneRect.width();
    ui->display_2->scale(zoomFactor, zoomFactor);

    // move the viewpoint to upper part
    ui->display_2->centerOn(sceneRect.center().x(), sceneRect.top() + sceneRect.height() / 4);

    ui->display_2->viewport()->update();
}

void MainWindow::updateDisplay3(const QImage &img)
{
    if (img.isNull())
    {
        qWarning() << "[ERROR] Received null image for display3";
        return;
    }

    m_display3Scene->clear();
    QPixmap pix = QPixmap::fromImage(img.transformed(QTransform().rotate(90)));
    m_display3Scene->addPixmap(pix);

    // 設置 QGraphicsView 的屬性
    ui->display_3->setRenderHint(QPainter::SmoothPixmapTransform); // 高質量縮放
    ui->display_3->setDragMode(QGraphicsView::ScrollHandDrag);     // 允許拖動
    ui->display_3->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // 計算縮放比例，使圖像適配視窗
    QRectF sceneRect = m_display3Scene->itemsBoundingRect();
    // QSize viewSize = ui->display_3->viewport()->size();
    // qDebug() << "[INFO] Display3 viewport size:" << viewSize;
    // qDebug() << "[INFO] Enface image size:" << img.size() << ", scene rect:" << sceneRect;

    // 重置縮放並適配視圖
    ui->display_3->resetTransform();
    ui->display_3->fitInView(sceneRect, Qt::KeepAspectRatio);

    ui->display_3->viewport()->update();

    // qDebug() << "[INFO] Display3 updated: image size=" << img.width() << "x" << img.height();
}

void MainWindow::slider_x_valueChanged(int value)
{
    if (m_imageStack.isEmpty())
    {
        qDebug() << "Image stack is empty";
        return;
    }
    if (value >= 0 && value < m_imageStack.size())
    {
        m_currentIndex = value;
        updateDisplay2(m_imageStack[value]);
    }
    else
    {
        qDebug() << "Invalid slider value:" << value;
    }
}

void MainWindow::slider_y_valueChanged(int value_y)
{
    int width = 1024; // fp: 1024pixel
    if (m_imageStack.isEmpty())
    {
        qDebug() << "Image stack is empty for display3";
        return;
    }

    if (value_y >= 0 && value_y < width)
    {
        QImage enfaceImg = generateEnfaceImage(value_y);
        updateDisplay3(enfaceImg);
    }
    else
    {
        qDebug() << "Invalid slider_y value:" << value_y;
    }
}

void MainWindow::clearImageStack() // 0417
{
    int width = 1024;
    m_imageStack.clear();
    ui->slider_x->setMaximum(0);
    ui->slider_x->setEnabled(false);
    ui->slider_y->setMaximum(width - 1);
    ui->slider_y->setEnabled(false);

    currentScene->clear();
    m_display2Scene->clear();
    m_display3Scene->clear();
    ui->display_1->viewport()->update();
    ui->display_2->viewport()->update();
    ui->display_3->viewport()->update();
    // ui->display_4->viewport()->update();
}

void MainWindow::pb_start_clicked()
{
    clearImageStack();
    QMutexLocker lock(&m_workerMutex);
    qDebug() << "generate worker thread";

    if (m_worker)
    {
        qDebug() << "Stopping existing worker thread";
        m_worker->stop();
        m_worker->wait();
        delete m_worker;
        m_worker = nullptr;
    }

    m_worker = new WorkerThread(h, this);
    // connect(m_worker, SIGNAL(dataReceived(QByteArray)), this, SLOT(handleData(QByteArray)), Qt::QueuedConnection); // add QueuedConnection
    connect(m_worker, &WorkerThread::dataReceived, this, &MainWindow::handleData, Qt::QueuedConnection); //, Qt::QueuedConnection); //
    connect(m_worker, &WorkerThread::errorOccurred, this, &MainWindow::showError, Qt::QueuedConnection);
    m_worker->start();
    qDebug() << "New worker thread started";
}

void MainWindow::handleData(const QByteArray &data)
{
    int width = 1024;
    int height = 1000;
    const int expectedSize = 4096000;
    if (data.size() != expectedSize)
    {
        qWarning() << "Invalid data size:" << data.size() << "expected:" << expectedSize;
        return;
    }

    QVector<float> rawfloat = convertToFloat32LittleEndian(data);
    QImage img = scaleFloatTo8Bit(rawfloat, width, height).transformed(QTransform().rotate(90));

    m_imageStack.append(img);
    ui->slider_x->setMaximum(qMax(0, m_imageStack.size() - 1));
    ui->slider_x->setEnabled(true);
    ui->slider_y->setEnabled(true);
    updateDisplay(img);
    QCoreApplication::processEvents();
}

// void MainWindow::handleData(const QByteArray &data)      //0701
// {
//     const int expectedSize = 4096000;
//     if (data.size() != expectedSize)
//     {
//         qWarning() << "Invalid data size:" << data.size() << "expected:" << expectedSize;
//         return;
//     }

//     QVector<quint16> raw16 = convertTo16BitLittleEndian(data);
//     QImage img = scaleTo8Bit(raw16).transformed(QTransform().rotate(90));

//     m_imageStack.append(img);
//     ui->slider_x->setMaximum(qMax(0, m_imageStack.size() - 1));
//     ui->slider_x->setEnabled(true);
//     ui->slider_y->setEnabled(true); // 0417
//     updateDisplay(img);
//     QCoreApplication::processEvents(); // 0417
// }

void MainWindow::showError(const QString &msg)
{
    QMessageBox::critical(this, "Error", msg);
    pb_stop_clicked();
}

void MainWindow::pb_stop_clicked()
{
    clearImageStack();
    if (m_worker && m_worker->isRunning())
    {
        m_worker->stop();
        m_worker->wait();
        delete m_worker;
        m_worker = nullptr;
    }
}

void MainWindow::pb_capture_clicked()
{
    // if (!cyusb_kernel_driver_active(h, 0))
    // {
    //     cyusb_reset_device(h);
    //     cyusb_close();
    //     cyusb_open();
    //     h = cyusb_gethandle(0);
    // }
    // QThread::msleep(500);

    {
        QMutexLocker lock(&m_workerMutex);
        if (m_worker)
        {
            m_worker->stop();
            m_worker->wait();
            delete m_worker;
            m_worker = nullptr;
        }
    }

    QMutexLocker lock(&m_loopMutex);
    m_stopRequested = true;
    clearImageStack();

    if (!h)
    {
        QMessageBox::critical(this, "Error", "USB device not connected");
        return;
    }

    QByteArray ABOIL = QByteArray::fromHex("41424F494C000078"); // number of fig   //20:000014  //500:0001F4 //120:000078
    unsigned char out_endpoint = 0x01;
    unsigned char in_endpoint = 0x81;
    int transferred_o = 0;

    // cyusb_clear_halt(h, out_endpoint);
    // cyusb_clear_halt(h, in_endpoint);

    int r_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)ABOIL.constData(), ABOIL.size(), &transferred_o, 1000);
    qDebug() << "r_o:" << r_o;

    if (r_o != 0)
    {
        QMessageBox::critical(this, "Error", QString("Failed sending: %1").arg(r_o));
        return;
    }
    else if (r_o == -1)
    {
        qDebug() << "Error IO ";
        return;
    }
    else if (r_o == -7)
    {
        qDebug() << "Error timeout ";
        return;
    };

    size_t length = 4096 * 1000;
    int totalsize = 4096 * 1000 * 500; // number of fig
    size_t remaining = static_cast<size_t>(totalsize);
    data_buffer.clear();
    int iii = 0;

    int width = 1024;
    int height = 1000;
    // msleep(1000);   //0704

    while (remaining > 0)
    {
        size_t data_read = std::min(length, remaining);
        std::vector<unsigned char> chunk(data_read);
        int transferred_i = 0;
        int r_i = cyusb_bulk_transfer(h, in_endpoint, chunk.data(), data_read, &transferred_i, 30000);

        if (r_i == -7)
        {
            qDebug() << "Timeout error code:" << r_i;
            break;
        }
        else if (r_i != 0)
        {
            qDebug() << "Failed receiving, error code:" << r_i;
            break;
        }

        if (transferred_i <= 0)
        {
            qDebug() << "No data received";
            break;
        }

        chunk.resize(transferred_i);
        data_buffer.insert(data_buffer.end(), chunk.begin(), chunk.end());

        //// ***********************************************************************************************
        //// ***********************************************************************************************    //ver.1 Qvector<float>
        // if (transferred_i == 4096000)
        // {
        //     QByteArray chunkData(reinterpret_cast<char *>(chunk.data()), transferred_i);
        //     QVector<float> rawFloat = convertToFloat32LittleEndian(chunkData);
        //     QImage img = scaleFloatTo8Bit(rawFloat, GUI_display_width, GUI_display_height);
        //     m_imageStack.append(img);
        //     ui->slider_x->setMaximum(m_imageStack.size() - 1);
        //     ui->slider_x->setEnabled(true);
        //     ui->slider_y->setEnabled(true);
        //     updateDisplay(img);
        //     QCoreApplication::processEvents();
        //     iii = iii + 1;
        // }
        //// ***********************************************************************************************
        //// ***********************************************************************************************

        // ***********************************************************************************************
        // ***********************************************************************************************      //ver.2 reinterpret_cast<const float *>
        if (transferred_i == 4096000)
        {
            QByteArray chunkData(reinterpret_cast<char *>(chunk.data()), transferred_i);
            QVector<float> floatData = convertToFloat32LittleEndian(chunkData);

            float minVal = floatData[0];
            float maxVal = floatData[0];
            for (int i = 1; i < width * height; ++i)
            {
                if (floatData[i] < minVal)
                    minVal = floatData[i];
                if (floatData[i] > maxVal)
                    maxVal = floatData[i];
            }

            QImage img(width, height, QImage::Format_Grayscale8);
            for (int y = 0; y < height; ++y)
            {
                uchar *scanLine = img.scanLine(y);
                for (int x = 0; x < width; ++x)
                {
                    int idx = y * width + x;
                    float val = floatData[idx];
                    int gray = static_cast<int>(255.0f * (val - minVal) / (maxVal - minVal + 1e-6f));
                    gray = qBound(0, gray, 255);
                    scanLine[x] = static_cast<uchar>(gray);
                }
            }

            m_imageStack.append(img);
            ui->slider_x->setMaximum(m_imageStack.size() - 1);
            ui->slider_x->setEnabled(true);
            ui->slider_y->setEnabled(true);
            updateDisplay(img);
            QCoreApplication::processEvents();
            iii = iii + 1;
        }
        // ***********************************************************************************************
        // ***********************************************************************************************

        remaining -= transferred_i;
        // qDebug() << "Chunk read:" << (totalsize - remaining) << "bytes, transferred:" << transferred_i;
        qDebug() << "No. " << iii << "image acquired";
    }
}

void MainWindow::pb_save_clicked()
{
    QString filename = ui->savepath->toPlainText();
    QString filepath = "/home/aboil/jetson_gui/raw_data/" + filename + ".bin";
    if (filepath.isEmpty())
    {
        return;
    }

    FILE *fp = fopen(filepath.toStdString().c_str(), "wb");
    if (!fp)
    {
        printf("failed to open file %s for writing\n", filename.toStdString().c_str());
        return;
    }

    size_t original_size = data_buffer.size();
    fwrite(data_buffer.data(), sizeof(unsigned char), data_buffer.size(), fp);
    fclose(fp);
    qDebug() << "Data written to file:" << filepath << ", bytes saved:" << original_size;
    data_buffer.clear();
}
