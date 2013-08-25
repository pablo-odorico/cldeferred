// This classed is based on GPL code by Qt

#ifndef CLGLWINDOW_H
#define CLGLWINDOW_H

#include <GL/glew.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include "clutilfunctions.h"
#include <QtOpenGL>

#include <QtGui/QWindow>

class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;

class CLGLWindow : public QWindow, protected CLUtilFunctions
{
    Q_OBJECT
public:
    explicit CLGLWindow(QWindow* parent = 0);
    ~CLGLWindow();

    virtual void renderGL() = 0;
    virtual void initializeGL() = 0;
    virtual void initializeCL() = 0;
    virtual void resizeGL(QSize size) = 0;

    QOpenGLContext* glCtx() { return _glContext; }

    cl_context clCtx() { return _clContext; }
    cl_device_id clDevice() { return _clDevice; }
    cl_command_queue clQueue() { return _clQueue; }

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

    QOpenGLContext* _glContext;
    QOpenGLPaintDevice* _glDevice;

    cl_context _clContext;
    cl_device_id _clDevice;
    cl_command_queue _clQueue;

    QTimer _renderTimer;
};

#endif // CLGLWINDOW_H
