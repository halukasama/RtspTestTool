#include "mymainwindow.h"
#include <QLayout>

MyMainWindow::MyMainWindow(QWidget *parent) : QMainWindow(parent)
  , m_clientwidget(new MyRtspClientWidget(this))
{
    resize(500,600);

    setCentralWidget(m_clientwidget);
}
