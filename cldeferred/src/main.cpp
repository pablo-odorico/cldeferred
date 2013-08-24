#include "cldeferred.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    CLDeferred window;
    window.resize(1280,720);
    //window.resize(100, 100);
    window.show();

    return app.exec();
}
