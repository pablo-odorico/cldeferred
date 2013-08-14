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

#include "clglwindow.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QResizeEvent>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>
#include <QDebug>

CLGLWindow::CLGLWindow(QWindow* parent)
    : QWindow(parent)
    , _updatePending(false)
    , _glContext(0)
    , _glDevice(0)
{
    setSurfaceType(QWindow::OpenGLSurface);
}

CLGLWindow::~CLGLWindow()
{
    delete _glDevice;
}

void CLGLWindow::initialize()
{
    // OpenGL init
    glewInit();
    // OpenGL user init
    initializeGL();

    // OpenCL init
    bool ok= setupOpenCLGL(_clContext, _clQueue, _clDevice);
    if(!ok) {
        qWarning() << "Could not initialize OpenCL";
        return;
    }
    // OpenCL user init
    initializeCL();

    qDebug() << "OpenGL Info";
    qDebug() << "   Version :" << (char*)glGetString(GL_VERSION);
    qDebug() << "   GLSL    :" << (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    qDebug() << "   GPU     :" << (char*)glGetString(GL_RENDERER);
}

void CLGLWindow::renderLater()
{
    if (!_updatePending) {
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
    _updatePending = false;

    bool needsInitialize = false;
    if (!_glContext) {
        _glContext = new QOpenGLContext(this);
        _glContext->setFormat(requestedFormat());
        _glContext->create();
        needsInitialize = true;
    }

    _glContext->makeCurrent(this);

    if (needsInitialize)
        initialize();

    if(isExposed()) {
        renderGL();
        _glContext->swapBuffers(this);
    }
}
