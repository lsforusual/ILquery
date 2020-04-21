#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <QTime>

int main(int argc, char *argv[])
{


    QApplication a(argc, argv);

    QSplashScreen splash(QPixmap(":/splash.png"));
    splash.show();
    splash.showMessage("<b>Thoughts, Composition and Faith</b>",Qt::AlignLeft|Qt::AlignBottom,Qt::white);

    QTime t;
    t.start();
    while(t.elapsed()<3000)
        a.processEvents();


    MainWindow w;
    w.show();

    splash.finish(&w);

    return a.exec();
}
