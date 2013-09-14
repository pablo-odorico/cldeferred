#include "cldeferred.h"
#include <QApplication>

#include "debug.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    CLDeferred window;
    //window.resize(854, 480);
    window.resize(1280, 720);
    window.show();
    //window.showFullScreen();

    return app.exec();
}
