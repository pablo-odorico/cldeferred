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
    src/clglwindow.cpp \
    src/cldeferred.cpp \
    src/clutilfunctions.cpp

HEADERS += \
    src/fbo.h \
    src/clglwindow.h \
    src/cldeferred.h \
    src/clutilfunctions.h

OTHER_FILES += \
    TODO.txt \
    bin/shaders/firstpass.frag \
    bin/shaders/firstpass.vert \
    bin/shaders/outputTex.frag \
    bin/shaders/outputTex.vert \
    bin/kernels/output.cl

