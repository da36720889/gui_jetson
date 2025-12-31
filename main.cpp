#include "mainwindow.h"
#include "include/cyusb.h"
#include <pthread.h>
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QMetaObject>
#include <QTimer>
#include <queue>
#include <QGraphicsPixmapItem>
#include <QProcess>
#include <QElapsedTimer>
#include <QDir>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDialog>

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
    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput, [=]()
            { qDebug() << process->readAllStandardOutput(); });

    connect(process, &QProcess::readyReadStandardError, [=]()
            { qDebug() << process->readAllStandardError(); });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus exitStatus)
            {
                Q_UNUSED(exitCode);
                Q_UNUSED(exitStatus);

                // Read PNG
                QString folderPath_result = "modelresult/eval_overlay/";
                QDir dir_result(folderPath_result);
                dir_result.setNameFilters(QStringList() << "*.png");
                dir_result.setSorting(QDir::Time | QDir::Reversed);

                QFileInfoList fileList = dir_result.entryInfoList();
                if (fileList.isEmpty())
                {
                    qWarning() << "No PNG files found in" << folderPath_result;
                    return;
                }

                QFileInfo latestFile = fileList.last();
                QImage image(latestFile.absoluteFilePath());
                if (image.isNull()) {
                    qWarning() << "Failed to load image:" << latestFile.absoluteFilePath();
                } else {
                    qDebug() << "Loaded PNG:" << latestFile.fileName();
                    updateDisplay4(image); 
                }

                // Read TXT
                QString folderPath_txt = "modelresult/Analysis/";
                QDir dir_txt(folderPath_txt);
                dir_txt.setNameFilters(QStringList() << "*.txt");
                dir_txt.setSorting(QDir::Time | QDir::Reversed);

                QFileInfoList txtFileList = dir_txt.entryInfoList();
                if (txtFileList.isEmpty())
                {
                    qWarning() << "No TXT files found in" << folderPath_txt;
                    return;
                }

                QFileInfo latestTxtFile = txtFileList.last();
                QFile txtFile(latestTxtFile.absoluteFilePath());
                if (txtFile.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    QString content = txtFile.readAll();
                    txtFile.close();

                    QDialog *dialog = new QDialog(this);
                    dialog->setWindowTitle("Quantitative analysis results");

                    QVBoxLayout *layout = new QVBoxLayout(dialog);
                    QTextEdit *textEdit = new QTextEdit(dialog);
                    textEdit->setPlainText(content);
                    textEdit->setReadOnly(true);
                    layout->addWidget(textEdit);

                    QPushButton *closeBtn = new QPushButton("Close");
                    layout->addWidget(closeBtn);
                    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

                    dialog->resize(450, 160);
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    dialog->show();
                }
                else
                {
                    qWarning() << "Failed to open latest txt file:" << latestTxtFile.absoluteFilePath();
                }
            });

    process->start("curl", QStringList() << "http://0.0.0.0:8000/run/test");
}

void MainWindow::updateDisplay4(const QImage &img)
{
    if (img.isNull())
        return;

    m_display4Scene->clear();

    QPixmap rotatedPixmap = QPixmap::fromImage(img);

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

    ui->display_4->centerOn(sceneRect.center().x(), sceneRect.top() + sceneRect.height() / 4);
    ui->display_4->viewport()->update();
}

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
    int rst_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)rst.constData(), rst.size(), &transferred_o, 10000);
    if (rst_o)
    {
        qDebug() << "rst set";
    }

    int dummy_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)dummy.constData(), dummy.size(), &transferred_o, 10000);
    if (dummy_o)
    {
        qDebug() << "rst set";
    }

    int initialize_o = cyusb_bulk_transfer(h, out_endpoint, (unsigned char *)initialize.constData(), initialize.size(), &transferred_o, 10000);
    if (initialize_o)
    {
        qDebug() << "initialize set";
    }
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

void MainWindow::updateDisplay(const QImage &img)
{
    if (img.isNull())
    {
        qWarning() << "Received null image for display1";
        return;
    }
    QElapsedTimer timer_paint;
    timer_paint.start();

    currentScene->clear();
    QPixmap pix = QPixmap::fromImage(img).transformed(QTransform().rotate(90));
    currentScene->addPixmap(pix);

    ui->display_1->setScene(currentScene);

    QRectF sceneRect = currentScene->itemsBoundingRect();
    currentScene->setSceneRect(sceneRect);

    qreal viewWidth = ui->display_1->viewport()->width();
    qreal zoomFactor = viewWidth / sceneRect.width();
    ui->display_1->resetTransform();
    ui->display_1->scale(zoomFactor, zoomFactor);

    ui->display_1->centerOn(sceneRect.center().x(),
                            sceneRect.top() + sceneRect.height() / 4);

    ui->display_1->viewport()->update();

    QTimer::singleShot(0, this, [timer_paint]() mutable {
        qDebug() << "[Display1] Paint cost:" << timer_paint.elapsed() << "ms";
    });
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

    QRectF sceneRect = m_display3Scene->itemsBoundingRect();
    m_display3Scene->setSceneRect(sceneRect);

    // 設置 QGraphicsView 的屬性
    ui->display_3->setRenderHint(QPainter::SmoothPixmapTransform); // 高質量縮放
    ui->display_3->setDragMode(QGraphicsView::ScrollHandDrag);     // 允許拖動
    ui->display_3->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // 計算縮放比例，使圖像適配視窗
    // QRectF sceneRect = m_display3Scene->itemsBoundingRect();
    
    // QSize viewSize = ui->display_3->viewport()->size();
    // qDebug() << "[INFO] Display3 viewport size:" << viewSize;
    // qDebug() << "[INFO] Enface image size:" << img.size() << ", scene rect:" << sceneRect;

    // 重置縮放並適配視圖
    ui->display_3->resetTransform();
    // ui->display_3->fitInView(sceneRect, Qt::KeepAspectRatio);
    ui->display_3->fitInView(sceneRect, Qt::IgnoreAspectRatio);


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

void MainWindow::clearImageStack()
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
}

void MainWindow::pb_start_clicked()
{
    clearImageStack();
    QMutexLocker lock(&m_workerMutex);
    qDebug() << "generate worker thread";

    if (scanningThread)
    {
        qDebug() << "Stopping existing worker thread";
        scanningThread->stop();
        scanningThread->wait();
        delete scanningThread;
        scanningThread = nullptr;
    }

    scanningThread = new StartThread(h, this);
    connect(scanningThread, &StartThread::dataReceived, this, &MainWindow::handleData, Qt::QueuedConnection);
    connect(scanningThread, &StartThread::errorOccurred, this, &MainWindow::showError, Qt::QueuedConnection);
    scanningThread->start();
    qDebug() << "scanning thread started";
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
    QImage img = scaleFloatTo8Bit(rawfloat, width, height);

    m_imageStack.append(img);
    updateDisplay(img);
    QCoreApplication::processEvents();
}

void MainWindow::showError(const QString &msg)
{
    QMessageBox::critical(this, "Error", msg);
    pb_stop_clicked();
}

void MainWindow::pb_stop_clicked()
{
    QElapsedTimer timer_stop;
    timer_stop.start();
    clearImageStack();
    qDebug() << "scanningThread = " << scanningThread;
    qDebug() << "captureThread = " << captureThread;
    if (scanningThread)
    {
        qDebug() << "scanningThread is not null";
        if (scanningThread->isRunning())
        {
            qDebug() << "scanningThread is running, stopping...";
            scanningThread->stop();
            scanningThread->wait();
        }
        delete scanningThread;
        scanningThread = nullptr;
    }

    if (captureThread)
    {
        qDebug() << "captureThread is not null";
        if (captureThread->isRunning())
        {
            qDebug() << "captureThread is running, stopping...";
            captureThread->stop();
            captureThread->wait();
        }
        // delete captureThread;
        captureThread->deleteLater();
        captureThread = nullptr;
    }
    qint64 elapsed_stop_ms = timer_stop.elapsed();
    qDebug() << "time to process stop: " << elapsed_stop_ms << "ms";
}

void MainWindow::pb_capture_clicked()
{
    QMutexLocker lock(&m_workerMutex);
    if (scanningThread)
    {
        scanningThread->stop();
        scanningThread->wait();
        delete scanningThread;
        scanningThread = nullptr;
    }

    clearImageStack();

    static CaptureThread *captureThread = nullptr;
    if (captureThread)
    {
        captureThread->stop();
        captureThread->wait();
        delete captureThread;
        captureThread = nullptr;
    }

    if (!h)
    {
        QMessageBox::critical(this, "Error", "USB device not connected");
        return;
    }

    captureThread = new CaptureThread(h, nullptr);

    connect(captureThread, &CaptureThread::oneFrameCaptured, this, [=](const QImage &img, qint64 elapsed_ms)
            {
                m_imageStack.append(img);
                ui->slider_x->setMaximum(m_imageStack.size() - 1);
                ui->slider_x->setEnabled(true);
                ui->slider_y->setEnabled(true);
                updateDisplay(img); 
                qDebug() << "Frame" << m_imageStack.size() << "process time:" << elapsed_ms << "ms"; }, Qt::QueuedConnection);

    connect(captureThread, &CaptureThread::rawDataReceived_cap, this, [=](const QByteArray &data)
            { data_buffer.insert(data_buffer.end(), data.begin(), data.end()); });

    connect(captureThread, &CaptureThread::captureFinished, this, [=]()
            { qDebug() << "Capture completed."; });

    captureThread->start();
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
