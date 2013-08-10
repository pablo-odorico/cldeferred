#include "glwindow.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    GLWindow window;
    window.resize(854, 480);
    window.show();

    return app.exec();
}
