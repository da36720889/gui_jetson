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
#include <QPointer>
#include <QElapsedTimer>
#include <QProcess>

extern int GUI_display_height; // 4096
extern int GUI_display_width;  // 1000

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow;

class StartThread : public QThread
{
    Q_OBJECT
public:
    explicit StartThread(cyusb_handle *m_usbHandle, MainWindow *mainwindow);
    void stop();

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &msg);

protected:
    void run() override;

private:
    volatile bool m_stop;
    cyusb_handle *m_usbHandle;
    MainWindow *m_mainwindow;
    QMutex m_workerMutex;
};

// thread for capture!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
class CaptureThread : public QThread
{
    Q_OBJECT
public:
    explicit CaptureThread(cyusb_handle *handle, MainWindow *mainwindow); //, QObject *parent = nullptr);
    void stop();

signals:
    void oneFrameCaptured(const QImage &img, qint64 elapsed_ms);
    void captureFinished();
    void captureError(const QString &msg);
    void rawDataReceived_cap(const QByteArray &data);

protected:
    void run() override;

private:
    cyusb_handle *h;
    bool m_stop;
    MainWindow *m_mainwindow;

    QVector<QByteArray> selectedChunks; // save 10 frames data
    QSet<int> selectedIndices;          // random index

    void saveSelectedChunks(const QString &folderPath);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QVector<float> convertToFloat32LittleEndian(const QByteArray &data);
    QVector<float> normalizeAndRotateFloatImage(const QVector<float> &inputData, int inputWidth, int inputHeight);

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
    StartThread *scanningThread;
    cyusb_handle *h;
    QMutex m_workerMutex;
    QPointer<CaptureThread> captureThread;

    QImage generateEnfaceImage(int height);
    QImage scaleFloatTo8Bit(const QVector<float> &data, int width, int height);


    QProcess *pythonFastApi;
};

#endif // MAINWINDOW_H