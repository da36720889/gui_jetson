/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGraphicsView>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QStatusBar>
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionskin;
    QAction *actioneye;
    QAction *actionteeth;
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
    QMenu *menuGUI;
    QMenu *menuJET;
    QMenu *menumode;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1920, 1080);
        MainWindow->setMinimumSize(QSize(1198, 0));
        actionskin = new QAction(MainWindow);
        actionskin->setObjectName(QString::fromUtf8("actionskin"));
        actioneye = new QAction(MainWindow);
        actioneye->setObjectName(QString::fromUtf8("actioneye"));
        actionteeth = new QAction(MainWindow);
        actionteeth->setObjectName(QString::fromUtf8("actionteeth"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        pb_start = new QPushButton(centralwidget);
        pb_start->setObjectName(QString::fromUtf8("pb_start"));
        pb_start->setGeometry(QRect(10, 110, 151, 31));
        pb_stop = new QPushButton(centralwidget);
        pb_stop->setObjectName(QString::fromUtf8("pb_stop"));
        pb_stop->setGeometry(QRect(10, 150, 151, 31));
        pb_capture = new QPushButton(centralwidget);
        pb_capture->setObjectName(QString::fromUtf8("pb_capture"));
        pb_capture->setGeometry(QRect(10, 190, 151, 31));
        graphicsView = new QGraphicsView(centralwidget);
        graphicsView->setObjectName(QString::fromUtf8("graphicsView"));
        graphicsView->setGeometry(QRect(180, 30, 830, 411));
        graphicsView_2 = new QGraphicsView(centralwidget);
        graphicsView_2->setObjectName(QString::fromUtf8("graphicsView_2"));
        graphicsView_2->setGeometry(QRect(1020, 30, 830, 411));
        graphicsView_3 = new QGraphicsView(centralwidget);
        graphicsView_3->setObjectName(QString::fromUtf8("graphicsView_3"));
        graphicsView_3->setGeometry(QRect(180, 460, 830, 541));
        graphicsView_4 = new QGraphicsView(centralwidget);
        graphicsView_4->setObjectName(QString::fromUtf8("graphicsView_4"));
        graphicsView_4->setGeometry(QRect(1020, 460, 830, 541));
        horizontalSlider_x = new QSlider(centralwidget);
        horizontalSlider_x->setObjectName(QString::fromUtf8("horizontalSlider_x"));
        horizontalSlider_x->setGeometry(QRect(10, 420, 151, 22));
        horizontalSlider_x->setOrientation(Qt::Horizontal);
        horizontalSlider_y = new QSlider(centralwidget);
        horizontalSlider_y->setObjectName(QString::fromUtf8("horizontalSlider_y"));
        horizontalSlider_y->setGeometry(QRect(10, 480, 151, 22));
        horizontalSlider_y->setOrientation(Qt::Horizontal);
        pb_save = new QPushButton(centralwidget);
        pb_save->setObjectName(QString::fromUtf8("pb_save"));
        pb_save->setGeometry(QRect(10, 690, 151, 31));
        label_Xaxis = new QLabel(centralwidget);
        label_Xaxis->setObjectName(QString::fromUtf8("label_Xaxis"));
        label_Xaxis->setGeometry(QRect(10, 400, 131, 16));
        label_Yaxis = new QLabel(centralwidget);
        label_Yaxis->setObjectName(QString::fromUtf8("label_Yaxis"));
        label_Yaxis->setGeometry(QRect(10, 460, 111, 16));
        label_savepath = new QLabel(centralwidget);
        label_savepath->setObjectName(QString::fromUtf8("label_savepath"));
        label_savepath->setGeometry(QRect(10, 610, 101, 16));
        textEdit = new QTextEdit(centralwidget);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        textEdit->setGeometry(QRect(10, 630, 151, 31));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1920, 21));
        menuGUI = new QMenu(menubar);
        menuGUI->setObjectName(QString::fromUtf8("menuGUI"));
        menuJET = new QMenu(menubar);
        menuJET->setObjectName(QString::fromUtf8("menuJET"));
        menumode = new QMenu(menuJET);
        menumode->setObjectName(QString::fromUtf8("menumode"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuGUI->menuAction());
        menubar->addAction(menuJET->menuAction());
        menuJET->addAction(menumode->menuAction());
        menumode->addAction(actionskin);

        retranslateUi(MainWindow);
        QObject::connect(pb_start, SIGNAL(clicked()), graphicsView, SLOT(invalidateScene()));
        QObject::connect(pb_stop, SIGNAL(clicked()), graphicsView_2, SLOT(invalidateScene()));

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        actionskin->setText(QApplication::translate("MainWindow", "skin", 0, QApplication::UnicodeUTF8));
        actioneye->setText(QApplication::translate("MainWindow", "eye", 0, QApplication::UnicodeUTF8));
        actionteeth->setText(QApplication::translate("MainWindow", "teeth", 0, QApplication::UnicodeUTF8));
        pb_start->setText(QApplication::translate("MainWindow", "START", 0, QApplication::UnicodeUTF8));
        pb_stop->setText(QApplication::translate("MainWindow", "STOP", 0, QApplication::UnicodeUTF8));
        pb_capture->setText(QApplication::translate("MainWindow", "CAPTURE", 0, QApplication::UnicodeUTF8));
        pb_save->setText(QApplication::translate("MainWindow", "SAVE", 0, QApplication::UnicodeUTF8));
        label_Xaxis->setText(QApplication::translate("MainWindow", "X axis", 0, QApplication::UnicodeUTF8));
        label_Yaxis->setText(QApplication::translate("MainWindow", "Y axis", 0, QApplication::UnicodeUTF8));
        label_savepath->setText(QApplication::translate("MainWindow", "save path", 0, QApplication::UnicodeUTF8));
        menuGUI->setTitle(QApplication::translate("MainWindow", "save", 0, QApplication::UnicodeUTF8));
        menuJET->setTitle(QApplication::translate("MainWindow", "setup", 0, QApplication::UnicodeUTF8));
        menumode->setTitle(QApplication::translate("MainWindow", "mode", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
