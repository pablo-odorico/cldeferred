// This classed is based on GPL code by Qt

#ifndef CLGLWINDOW_H
#define CLGLWINDOW_H

#include <GL/glew.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include "clutilfunctions.h"

#include <QtGui/QWindow>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <Qt3D/QGLPainter>

class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;
class QGLPainter;

class CLGLWindow : public QWindow, protected CLUtilFunctions
{
    Q_OBJECT
public:
    explicit CLGLWindow(QWindow* parent = 0);
    virtual ~CLGLWindow();

    virtual void renderGL() = 0;
    virtual void initializeGL() = 0;
    virtual void initializeCL() = 0;
    virtual void finalizeInit() = 0; // Called after OpenCL and OpenGL are set up
    virtual void resizeGL(QSize size) = 0;

    QOpenGLContext* glCtx() { return _glContext; }
    QGLPainter* glPainter() { return _glPainter; }

    cl_context clCtx() const { return _clContext; }
    cl_device_id clDevice() const { return _clDevice; }
    cl_command_queue clQueue() const { return _clQueue; }

public slots:
    void grabMouse();
    void releaseMouse();

    void renderLater();
    void renderNow();

    void startRenderTimer() { _renderTimer.start(); }
    void startRenderTimer(int targetFps) { _renderTimer.start(1000/targetFps); }
    void stopRenderTimer() { _renderTimer.stop(); }

protected:
    bool event(QEvent* event);

    void exposeEvent(QExposeEvent* event);
    void resizeEvent(QResizeEvent* event);

    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    virtual void grabbedMouseMoveEvent(QPointF delta) { Q_UNUSED(delta); }
    virtual void grabbedKeyPressEvent(int key) { Q_UNUSED(key); }
    virtual void grabbedKeyReleaseEvent(int key) { Q_UNUSED(key); }

private:
    void initialize();

    bool _updatePending;

    bool _mouseGrabbed;
    QPoint _mouseGrabPosition;
    QPoint _mouseLockPosition;

    QGLPainter* _glPainter;
    QOpenGLContext* _glContext;
    QOpenGLPaintDevice* _glDevice;

    cl_context _clContext;
    cl_device_id _clDevice;
    cl_command_queue _clQueue;

    QTimer _renderTimer;
};

#endif // CLGLWINDOW_H
