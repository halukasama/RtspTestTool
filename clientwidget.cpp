#include "clientwidget.h"

#include <QLayout>
#include <QTimer>

MyRtspClientWidget::MyRtspClientWidget(QWidget *parent)
    : QWidget(parent)
    , m_pkgEdit(new QTextEdit)
    , m_connIp(new QLineEdit)
    , m_connPort(new QLineEdit)
    , m_startBtn(new QPushButton)
    , m_testBtn(new QPushButton)
    , m_user(new QLineEdit)
    , m_pwd(new QLineEdit)
    , m_listenPort(new QLineEdit)
    , m_info(new MyDeviceInfo)
    , m_engine()
    , m_client(new MyRtspClient(m_engine, m_info))
    , m_runner(m_engine)
{

    InitWidget();

    m_runner.start();
    connect(m_testBtn, SIGNAL(clicked()), this, SLOT(onTerminationBtnClicked()));
    connect(m_startBtn, SIGNAL(clicked()), this, SLOT(onStratBtnClicked()));
    m_testBtn->setEnabled(false);
}
MyRtspClientWidget::~MyRtspClientWidget()
{
    m_runner.stop();
}

void MyRtspClientWidget::onStratBtnClicked()
{
    if (m_client == NULL) {
        getInteractiveInfo();
        m_client = new MyRtspClient(m_engine, m_info);
    }
    connect(m_client, SIGNAL(getClientResp(QString,QString)), this, SLOT(onGetResponse(QString,QString)));
    m_client->start();
    m_startBtn->setEnabled(false);
    m_testBtn->setEnabled(true);
}

void MyRtspClientWidget::onTerminationBtnClicked()
{
    if (m_client != NULL) {
        m_client->stop();

        QTimer timer;
        timer.singleShot(100, this, SLOT(destroyClient()));
    }
    m_testBtn->setEnabled(false);
    m_startBtn->setEnabled(true);
}

void MyRtspClientWidget::onGetResponse(QString header, QString resp)
{
    if (header != "")
        m_pkgEdit->append(header);
    m_pkgEdit->append(resp);
}

void MyRtspClientWidget::destroyClient()
{
    disconnect(m_client, SIGNAL(getClientResp(QString,QString)), this, SLOT(onGetResponse(QString,QString)));
    delete m_client;
    m_client = NULL;
}

void MyRtspClientWidget::InitWidget()
{
    QGridLayout *layout = new QGridLayout(this);

    layout->addWidget(m_pkgEdit,0,0,1,4);
    layout->addWidget(new QLabel("设备地址:"),1,0,1,1);
    layout->addWidget(m_connIp,1,1,1,1);
    layout->addWidget(new QLabel("端口号:"),1,2,1,1);
    layout->addWidget(m_connPort,1,3,1,1);
    layout->addWidget(new QLabel("用户名:"),2,0,1,1);
    layout->addWidget(m_user, 2,1,1,1);
    layout->addWidget(new QLabel("密码:"),2,2,1,1);
    layout->addWidget(m_pwd, 2,3,1,1);
    layout->addWidget(new QLabel("监听端口:"), 3,0,1,1);
    layout->addWidget(m_listenPort,3,1,1,1);
    QHBoxLayout *btnlayout = new QHBoxLayout(this);
    btnlayout->addWidget(m_startBtn);
    btnlayout->addWidget(m_testBtn);
    layout->addLayout(btnlayout,3,3,1,1);

    m_pkgEdit->setText("输出信息：\n");
    m_pkgEdit->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_pkgEdit->setReadOnly(true);

    m_testBtn->setText("结束");
    m_startBtn->setText("开始");
}

void MyRtspClientWidget::getInteractiveInfo()
{
    if (!m_connIp->text().isEmpty())
        m_info->m_deviceIp = m_connIp->text();
    if (!m_connPort->text().isEmpty())
        m_info->m_serverPort = m_connPort->text().toUInt();
    if (!m_user->text().isEmpty())
        m_info->m_username = m_user->text();
    if (!m_pwd->text().isEmpty())
        m_info->m_password = m_pwd->text();
    if (!m_listenPort->text().isEmpty())
        m_info->m_listenPort = m_listenPort->text().toUInt();
}

