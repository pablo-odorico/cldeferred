#include "clglwindow.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QResizeEvent>

#include <QtGui>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>
#include <QDebug>

CLGLWindow::CLGLWindow(QWindow* parent)
    : QWindow(parent)
    , _glPainter(0)
    , _glContext(0)
    , _glDevice(0)
    , _updatePending(false)
    , _mouseGrabbed(false)
{
    setSurfaceType(QWindow::OpenGLSurface);

    _renderTimer.setTimerType(Qt::PreciseTimer);
    connect(&_renderTimer, SIGNAL(timeout()), this, SLOT(renderLater()));
}

CLGLWindow::~CLGLWindow()
{
    delete _glDevice;
}

void CLGLWindow::initialize()
{
    // OpenGL init
    glewInit();
    // Create GL painter
    _glPainter= new QGLPainter(this);
    _glPainter->setStandardEffect(QGL::LitModulateTexture2D);

    // OpenCL init
    bool ok= setupOpenCLGL(&_clContext, &_clQueue, &_clDevice);
    if(!ok) {
        qWarning() << "Could not initialize OpenCL";
        exit(EXIT_FAILURE);
        return;
    }

    // OpenGL user init
    initializeGL();
    // OpenCL user init
    initializeCL();

    qDebug() << "OpenGL";
    qDebug() << "   Version :" << (char*)glGetString(GL_VERSION);
    qDebug() << "   GLSL    :" << (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    qDebug() << "   Device  :" << (char*)glGetString(GL_RENDERER);
    qDebug() << " ";

    const int dataSize= 1024;
    char data[dataSize];
    data[0]= 0;

    qDebug() << "OpenCL";
    clGetDeviceInfo(_clDevice, CL_DEVICE_VERSION, dataSize, data, NULL);
    qDebug() << "   Version :" << data;
    clGetDeviceInfo(_clDevice, CL_DEVICE_NAME, dataSize, data, NULL);
    qDebug() << "   Device  :" << data;
    clGetDeviceInfo(_clDevice, CL_DEVICE_VENDOR, dataSize, data, NULL);
    qDebug() << "   Vendor  :" << data;
    qDebug() << " ";

    // Final initialization
    finalizeInit();
}

void CLGLWindow::renderLater()
{
    if(!_updatePending) {
        _updatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool CLGLWindow::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void CLGLWindow::exposeEvent(QExposeEvent* event)
{
    Q_UNUSED(event);

    renderNow();
}

void CLGLWindow::resizeEvent(QResizeEvent* event)
{
    // First call renderNow() to create the context if needed
    renderNow();
    resizeGL(event->size());
    // Now call renderNow to actually render the scene
    renderNow();
}

void CLGLWindow::renderNow()
{
    bool needsInitialize = false;
    if (!_glContext) {
        _glContext= new QOpenGLContext(this);
        _glContext->setFormat(requestedFormat());
        _glContext->create();
        needsInitialize = true;
    }

    _glContext->makeCurrent(this);

    if(needsInitialize)
        initialize();

    if(isExposed()) {
        renderGL();
        _glContext->swapBuffers(this);
    }

    _updatePending = false;
}

void CLGLWindow::mousePressEvent(QMouseEvent*)
{
    if(_mouseGrabbed)
        releaseMouse();
    else
        grabMouse();
}

void CLGLWindow::mouseMoveEvent(QMouseEvent* event)
{
    if(!_mouseGrabbed)
        return;

    QPointF delta= event->globalPos() - _mouseLockPosition;
    delta.rx() /=  screen()->size().width();
    delta.ry() /= -screen()->size().height();

    if(!delta.x() and !delta.y())
        return;

    QCursor::setPos(_mouseLockPosition);

    grabbedMouseMoveEvent(delta);
}

void CLGLWindow::grabMouse()
{
    if(_mouseGrabbed)
        return;

    setCursor(Qt::BlankCursor);
    setMouseGrabEnabled(true);
    setKeyboardGrabEnabled(true);

    _mouseGrabPosition= QCursor::pos();

    const QPoint screenSize(screen()->size().width(), screen()->size().height());
    _mouseLockPosition= screenSize / 2;

    QCursor::setPos(_mouseLockPosition);
    _mouseGrabbed= true;
}

void CLGLWindow::releaseMouse()
{
    if(!_mouseGrabbed)
        return;

    setCursor(Qt::ArrowCursor);
    setMouseGrabEnabled(false);
    setKeyboardGrabEnabled(false);

    QCursor::setPos(_mouseGrabPosition);
    _mouseGrabbed= false;
}

void CLGLWindow::keyPressEvent(QKeyEvent* event)
{
    switch(event->key()) {
    case Qt::Key_Escape:
        releaseMouse();
        if(windowState() == Qt::WindowFullScreen)
            showNormal();
        break;
    case Qt::Key_F:
    case Qt::Key_F11:
        if(windowState() == Qt::WindowFullScreen)
            showNormal();
        else
            showFullScreen();
        break;
    case Qt::Key_Space:
        if(_renderTimer.isActive())
            stopRenderTimer();
        else
            startRenderTimer();
        break;
    default:
        break;
    }

    if(_mouseGrabbed and !event->isAutoRepeat())
        grabbedKeyPressEvent(event->key());
}

void CLGLWindow::keyReleaseEvent(QKeyEvent* event)
{
    if(_mouseGrabbed and !event->isAutoRepeat())
        grabbedKeyReleaseEvent(event->key());
}
