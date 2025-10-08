#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <libusb-1.0/libusb.h>
#include "ui_mainwindow.h"
#include "include/cyusb.h"

#include <QList>
#include <QImage>
#include <QGraphicsView>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QButtonGroup>

extern int GUI_display_height; // 4096
extern int GUI_display_width;  // 1000

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow;
class WorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit WorkerThread(cyusb_handle *m_usbHandle, MainWindow *parent);
    void stop();

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &msg);

protected:
    void run() override;

private:
    volatile bool m_stop;
    cyusb_handle *m_usbHandle;
    MainWindow *m_parent;
    QMutex m_workerMutex;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void pb_start_clicked();
    void pb_save_clicked();
    void pb_capture_clicked();
    void pb_stop_clicked();
    void pb_quantize_clicked(); // 0701

    void rb_2d_clicked();
    void rb_3d_clicked();
    void rb_cross_clicked();
    void rb_wang_clicked();

    void pb_initialize_clicked();
    void pb_quit_clicked();

    void updateDisplay(const QImage &img);   // display1
    void updateDisplay2(const QImage &img);  // display2
    void updateDisplay3(const QImage &img);  // display3 for enface
    void updateDisplay4(const QImage &img);  // 0701
    void slider_x_valueChanged(int value_x); // for display2
    void slider_y_valueChanged(int value_y); // for display3 (enface)
    void clearImageStack();

    void handleData(const QByteArray &data);
    void showError(const QString &msg);

private:
    Ui::MainWindow *ui;
    std::vector<unsigned char> data_buffer;

    QButtonGroup *radioGroup;

    QGraphicsScene *currentScene;    // for display1
    QGraphicsScene *m_display2Scene; // for display2
    QGraphicsScene *m_display3Scene; // for display3 (enface)
    QList<QImage> m_imageStack;      // store all captured images
    int m_currentIndex = -1;         // for slider_x

    QGraphicsScene *m_display4Scene; // 0701

    // Preview stop
    bool m_loopRunning = false;
    bool m_stopRequested = false;
    QMutex m_loopMutex;

    // Thread for preview stop (loop version)
    WorkerThread *m_worker;
    cyusb_handle *h;
    QMutex m_workerMutex;

    // // Little endian processing      //0701
    // QImage scaleTo8Bit(const QVector<quint16> &data);
    // QVector<quint16> convertTo16BitLittleEndian(const QByteArray &data);
    // QImage scale16BitTo8Bit(const QVector<quint16> &data, int width, int height);
    // QImage generateEnfaceImage(int height); // Generate enface image for display3

    QImage scaleTo8Bit(const QVector<float> &data);
    QVector<float> convertTo16BitLittleEndian(const QByteArray &data);
    QImage scale16BitTo8Bit(const QVector<float> &data, int width, int height);
    QImage generateEnfaceImage(int height); // Generate enface image for display3

    QImage scaleFloatTo8Bit(const QVector<float> &data, int width, int height);
    QVector<float> convertToFloat32LittleEndian(const QByteArray &data);

    // bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // MAINWINDOW_H

/*
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <libusb-1.0/libusb.h>
#include "ui_mainwindow.h"
#include "include/cyusb.h"

#include <QList>  //2025/02/16
#include <QImage> //2025/02/16
#include <QGraphicsView>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE
// ************************************************** //
class MainWindow;
class RawDataWidget;
class WorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit WorkerThread(cyusb_handle *m_usbHandle, MainWindow *parent);

    void stop();

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &msg);

protected:
    void run() override;

private:
    volatile bool m_stop;
    cyusb_handle *m_usbHandle;
    MainWindow *m_parent;
    QMutex m_workerMutex;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = NULL);
    ~MainWindow();

private slots:
    void pb_start_clicked();
    void pb_save_clicked();
    void pb_capture_clicked();
    void pb_stop_clicked();

    void updateDisplay(const QImage &img);   // 2025/03/16
    void updateDisplay2(const QImage &img);  // 2025/02/16
    void slider_x_valueChanged(int value_x); // 2025/02/16
    void clearImageStack();

    void handleData(const QByteArray &data);
    void showError(const QString &msg);

private:
    static MainWindow *mainwin; // 2025/01/26
    Ui::MainWindow *ui;
    std::vector<unsigned char> data_buffer;

    QGraphicsScene *currentScene; // 2025/02/15

    QList<QImage> m_imageStack;      // 2025/02/16
    QGraphicsScene *m_display2Scene; // 2025/02/16
    int m_currentIndex = -1;         // 2025/02/16

    // ====================================================== //
    // preview stop
    bool m_loopRunning = false;
    bool m_stopRequested = false;
    QMutex m_loopMutex;
    // =============== thread for preview stop(loop ver.) ================= //
    WorkerThread *m_worker;
    cyusb_handle *h;
    // ==================================================================== //
    QMutex m_workerMutex;

    // =============== Little endian ======================================= //
    QImage scaleTo8Bit(const QVector<quint16> &data);
    QVector<quint16> convertTo16BitLittleEndian(const QByteArray &data);          // 0316 little endian
    QImage scale16BitTo8Bit(const QVector<quint16> &data, int width, int height); // 0316 little endian
};

class RawDataWidget : public QWidget
{
public:
    explicit RawDataWidget(QWidget *parent = 0)
        : QWidget(parent), m_width(0), m_height(0), m_minValue(0), m_maxValue(65535)
    {
        updateLUT();
    }
    void setRawData(const quint16 *data, int width, int height)
    {
        m_rawData.resize(width * height);
        memcpy(m_rawData.data(), data, width * height * sizeof(quint16));
        m_width = width;
        m_height = height;
        m_minValue = *std::min_element(m_rawData.constBegin(), m_rawData.constEnd());
        m_maxValue = *std::max_element(m_rawData.constBegin(), m_rawData.constEnd());
        update();
    }

    void setDisplayRange(quint16 minVal, quint16 maxVal)
    {
        m_minValue = minVal;
        m_maxValue = maxVal;
        updateLUT();
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        paintraw(painter);
    }

private:
    QVector<quint16> m_rawData;
    int m_width;
    int m_height;
    quint16 m_minValue;
    quint16 m_maxValue;
    QVector<QRgb> m_colorLUT;

    void paintraw(QPainter &painter)
    {
        if (m_rawData.isEmpty() || m_width <= 0 || m_height <= 0)
        {
            painter.fillRect(rect(), Qt::black);
            return;
        }

        double xScale = static_cast<double>(width()) / m_width;
        double yScale = static_cast<double>(height()) / m_height;

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                quint16 value = m_rawData[y * m_width + x];
                int level = qBound(0, (value - m_minValue) * 255 / (m_maxValue - m_minValue + 1), 255);
                painter.fillRect(
                    QRectF(x * xScale, y * yScale, xScale, yScale),
                    QColor(m_colorLUT[level]));
            }
        }
    }

    void updateLUT()
    {
        m_colorLUT.resize(256);
        for (int i = 0; i < 256; ++i)
        {
            m_colorLUT[i] = qRgb(i, i, i);
        }
    }
};

#endif // MAINWINDOW_H

*/