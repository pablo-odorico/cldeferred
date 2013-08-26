TARGET = cldeferred
TEMPLATE = app

QT += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS_RELEASE = -march=native -O3 -fPIC

DESTDIR = bin
OBJECTS_DIR = obj
MOC_DIR = obj
RCC_DIR = obj

LIBS += -lGLEW -lOpenCL -lQt53D

INCLUDEPATH += src/
# Include path for shared kernel structs
INCLUDEPATH += res/kernels/

SOURCES += \
    src/main.cpp \
    src/fbo.cpp \
    src/clglwindow.cpp \
    src/cldeferred.cpp \
    src/clutilfunctions.cpp \
    src/fbocl.cpp \
    src/camera.cpp \
    src/cameracl.cpp

HEADERS += \
    src/fbo.h \
    src/clglwindow.h \
    src/cldeferred.h \
    src/clutilfunctions.h \
    src/fbocl.h \
    src/camera.h \
    src/cameracl.h \
    res/kernels/cltypes.h \
    res/kernels/cl_camera.h

OTHER_FILES += \
    TODO.txt \
    res/shaders/firstpass.frag \
    res/shaders/firstpass.vert \
    res/shaders/outputTex.frag \
    res/shaders/outputTex.vert \
    res/kernels/deferredPass.cl

RESOURCES += \
    res/shaders.qrc \
    res/kernels.qrc

