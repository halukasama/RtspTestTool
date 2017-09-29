#ifndef MYMAINWINDOW_H
#define MYMAINWINDOW_H

#include <QMainWindow>
#include "clientwidget.h"

class MyMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MyMainWindow(QWidget *parent = 0);

signals:

public slots:

private:
    MyRtspClientWidget *m_clientwidget;
};

#endif // MYMAINWINDOW_H
