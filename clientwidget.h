#ifndef __CLIENT_WIDGET_H__
#define __CLIENT_WIDGET_H__

#include "testclient.h"
#include <swift/thread/engine_runner.hpp>

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

class MyRtspClientWidget: public QWidget
{
    Q_OBJECT

public:
    MyRtspClientWidget(QWidget *parent = 0);
    ~MyRtspClientWidget();

public slots:
    void onStratBtnClicked();
    void onTerminationBtnClicked();

    void onGetResponse(QString header, QString resp);

private slots:
    void destroyClient();
private:
    void InitWidget();

    void getInteractiveInfo();

private:
    QTextEdit *m_pkgEdit;
    QLineEdit *m_connIp;
    QLineEdit *m_connPort;
    QLineEdit *m_pwd;
    QLineEdit *m_user;
    QLineEdit *m_listenPort;
    QPushButton *m_startBtn;
    QPushButton *m_testBtn;
    bool m_bFirstStart;
    MyDeviceInfo *m_info;

    swift::engine m_engine;
    MyRtspClient *m_client;
    swift::engine_runner m_runner;
};

#endif /// __CLIENT_WIDGET_H__
