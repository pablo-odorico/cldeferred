TARGET = cldeferred
TEMPLATE = app

QT += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS_RELEASE = -march=native -O3 -fPIC
QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder -Wno-cpp

DESTDIR = bin
OBJECTS_DIR = obj
MOC_DIR = obj
RCC_DIR = obj

LIBS += -lGLU -lGLEW -lOpenCL -lQt53D

INCLUDEPATH += src/ src/scene/ src/utils/
# Include path for shared kernel structs
INCLUDEPATH += res/kernels/

SOURCES += \
    src/main.cpp \
    src/fbo.cpp \
    src/clglwindow.cpp \
    src/cldeferred.cpp \
    src/fbocl.cpp \
    src/occlusionbuffer.cpp \
    src/scene/camera.cpp \
    src/scene/cameracl.cpp \
    src/utils/debug.cpp \
    src/scene/light.cpp \
    src/scene/lightmanager.cpp \
    src/scene/scene.cpp \
    src/scene/spotlight.cpp \
    src/utils/clutils.cpp \
    src/utils/glutils.cpp \
    src/scene/dirlight.cpp \
    src/utils/analytics.cpp

HEADERS += \
    src/fbo.h \
    src/clglwindow.h \
    src/cldeferred.h \
    src/fbocl.h \
    res/kernels/cltypes.h \
    res/kernels/cl_camera.h \
    res/kernels/cl_spotlight.h \
    src/occlusionbuffer.h \
    src/scene/camera.h \
    src/scene/cameracl.h \
    src/utils/debug.h \
    src/scene/light.h \
    src/scene/lightmanager.h \
    src/scene/scene.h \
    src/scene/spotlight.h \
    src/utils/clutils.h \
    src/utils/glutils.h \
    res/kernels/cl_dirlight.h \
    src/scene/dirlight.h \
    res/kernels/cl_material.h \
    src/utils/analytics.h

OTHER_FILES += \
    TODO.txt \
    res/shaders/firstpass.frag \
    res/shaders/firstpass.vert \
    res/kernels/deferredPass.cl \
    res/kernels/clutils.cl \
    res/shaders/shadowmapping.vert \
    res/shaders/shadowmapping.frag \
    res/kernels/occlusionPass.cl \
    res/shaders/outputQuad.frag \
    res/shaders/outputQuad.vert \
    res/kernels/fxaa.cl \
    res/kernels/depthDownsample.cl

RESOURCES += \
    res/shaders.qrc \
    res/kernels.qrc

