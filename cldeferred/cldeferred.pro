TARGET = cldeferred
TEMPLATE = app

QT += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS_RELEASE = -march=native -O3 -fPIC

DESTDIR = bin
OBJECTS_DIR = obj
MOC_DIR = obj

LIBS += -lGLEW -lOpenCL -lQt53D

INCLUDEPATH += src/

SOURCES += \
    src/main.cpp \
    src/fbo.cpp \
    src/glwindow.cpp \
    src/clglwindow.cpp \
    src/clutils.cpp

HEADERS += \
    src/fbo.h \
    src/glwindow.h \
    src/clglwindow.h \
    src/clutils.h

OTHER_FILES += \
    TODO.txt \
    bin/shaders/firstpass.frag \
    bin/shaders/firstpass.vert \
    bin/shaders/outputTex.frag \
    bin/shaders/outputTex.vert \
    bin/kernels/output.cl

