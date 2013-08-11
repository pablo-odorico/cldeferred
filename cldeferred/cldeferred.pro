TARGET = cldeferred
TEMPLATE = app

QT += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS_RELEASE = -march=native -O3 -fPIC

DESTDIR = bin
OBJECTS_DIR = obj
MOC_DIR = obj

LIBS += -lQt53D -lGLEW

INCLUDEPATH += src/

SOURCES += \
    src/main.cpp \
    src/fbo.cpp \
    src/openglwindow.cpp \
    src/glwindow.cpp

HEADERS += \
    src/fbo.h \
    src/openglwindow.h \
    src/glwindow.h

OTHER_FILES += \
    bin/shader.frag \
    bin/shader.vert

