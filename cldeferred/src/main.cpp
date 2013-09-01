#include "cldeferred.h"
#include <QApplication>

#include "debug.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Debug::init();

    CLDeferred window;
    window.resize(854, 480);
    //window.resize(1280, 720);
    window.show();

    return app.exec();
}
