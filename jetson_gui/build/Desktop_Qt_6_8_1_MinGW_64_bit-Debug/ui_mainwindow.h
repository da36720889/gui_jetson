/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *pb_start;
    QPushButton *pb_stop;
    QPushButton *pb_capture;
    QGraphicsView *graphicsView;
    QGraphicsView *graphicsView_2;
    QGraphicsView *graphicsView_3;
    QGraphicsView *graphicsView_4;
    QSlider *horizontalSlider_x;
    QSlider *horizontalSlider_y;
    QPushButton *pb_save;
    QLabel *label_Xaxis;
    QLabel *label_Yaxis;
    QLabel *label_savepath;
    QTextEdit *textEdit;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1920, 1080);
        MainWindow->setMinimumSize(QSize(1198, 0));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        centralwidget->setStyleSheet(QString::fromUtf8("color: rgb(0, 0, 0);\n"
"background-color: rgb(202, 202, 202);"));
        pb_start = new QPushButton(centralwidget);
        pb_start->setObjectName("pb_start");
        pb_start->setGeometry(QRect(10, 110, 151, 31));
        pb_stop = new QPushButton(centralwidget);
        pb_stop->setObjectName("pb_stop");
        pb_stop->setGeometry(QRect(10, 150, 151, 31));
        pb_capture = new QPushButton(centralwidget);
        pb_capture->setObjectName("pb_capture");
        pb_capture->setGeometry(QRect(10, 190, 151, 31));
        graphicsView = new QGraphicsView(centralwidget);
        graphicsView->setObjectName("graphicsView");
        graphicsView->setGeometry(QRect(180, 30, 830, 411));
        graphicsView_2 = new QGraphicsView(centralwidget);
        graphicsView_2->setObjectName("graphicsView_2");
        graphicsView_2->setGeometry(QRect(1020, 30, 830, 411));
        graphicsView_3 = new QGraphicsView(centralwidget);
        graphicsView_3->setObjectName("graphicsView_3");
        graphicsView_3->setGeometry(QRect(180, 460, 830, 541));
        graphicsView_4 = new QGraphicsView(centralwidget);
        graphicsView_4->setObjectName("graphicsView_4");
        graphicsView_4->setGeometry(QRect(1020, 460, 830, 541));
        horizontalSlider_x = new QSlider(centralwidget);
        horizontalSlider_x->setObjectName("horizontalSlider_x");
        horizontalSlider_x->setGeometry(QRect(10, 420, 151, 22));
        horizontalSlider_x->setOrientation(Qt::Orientation::Horizontal);
        horizontalSlider_y = new QSlider(centralwidget);
        horizontalSlider_y->setObjectName("horizontalSlider_y");
        horizontalSlider_y->setGeometry(QRect(10, 480, 151, 22));
        horizontalSlider_y->setOrientation(Qt::Orientation::Horizontal);
        pb_save = new QPushButton(centralwidget);
        pb_save->setObjectName("pb_save");
        pb_save->setGeometry(QRect(10, 690, 151, 31));
        label_Xaxis = new QLabel(centralwidget);
        label_Xaxis->setObjectName("label_Xaxis");
        label_Xaxis->setGeometry(QRect(10, 400, 131, 16));
        label_Yaxis = new QLabel(centralwidget);
        label_Yaxis->setObjectName("label_Yaxis");
        label_Yaxis->setGeometry(QRect(10, 460, 111, 16));
        label_savepath = new QLabel(centralwidget);
        label_savepath->setObjectName("label_savepath");
        label_savepath->setGeometry(QRect(10, 610, 101, 16));
        textEdit = new QTextEdit(centralwidget);
        textEdit->setObjectName("textEdit");
        textEdit->setGeometry(QRect(10, 630, 151, 31));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1920, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        pb_start->setText(QCoreApplication::translate("MainWindow", "START", nullptr));
        pb_stop->setText(QCoreApplication::translate("MainWindow", "STOP", nullptr));
        pb_capture->setText(QCoreApplication::translate("MainWindow", "CAPTURE", nullptr));
        pb_save->setText(QCoreApplication::translate("MainWindow", "SAVE", nullptr));
        label_Xaxis->setText(QCoreApplication::translate("MainWindow", "X axis", nullptr));
        label_Yaxis->setText(QCoreApplication::translate("MainWindow", "Y axis", nullptr));
        label_savepath->setText(QCoreApplication::translate("MainWindow", "save path", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
