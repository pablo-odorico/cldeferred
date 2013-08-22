/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

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

    void startRenderTimer(int targetFps=30) { _renderTimer.start(1000/targetFps); }
    void stopRenderTimer() { _renderTimer.stop(); }

signals:
    void grabbedMouseMove(QPointF delta);

protected:
    bool event(QEvent* event);

    void exposeEvent(QExposeEvent* event);
    void resizeEvent(QResizeEvent* event);

    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

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

#endif // OPENGLWINDOW_H
