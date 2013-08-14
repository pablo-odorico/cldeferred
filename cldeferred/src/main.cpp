#include "cldeferred.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    CLDeferred window;
    window.resize(854, 480);
    window.show();
    //window.showFullScreen();

    return app.exec();
}
