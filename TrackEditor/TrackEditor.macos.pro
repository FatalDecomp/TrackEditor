TEMPLATE = app
TARGET = TrackEditor
DESTDIR = ../bin/TrackEditor

CONFIG += c++17 console sdk_no_version_check
QT += widgets xml opengl

MOC_DIR = lmoc-macos
UI_DIR = lui-macos
RCC_DIR = lrcc-macos
OBJECTS_DIR = obj/macos

GLEW_PREFIX = $$system(brew --prefix glew)
GLM_PREFIX = $$system(brew --prefix glm)
LIBXML2_PREFIX = $$system(brew --prefix libxml2)
ENABLE_FBX = $$(ENABLE_FBX)
FBX_LIB_DIR = $$(FBX_LIB_DIR)
MACOSX_DEPLOYMENT_TARGET_ENV = $$(MACOSX_DEPLOYMENT_TARGET)

isEmpty(ENABLE_FBX): ENABLE_FBX = 0
!isEmpty(MACOSX_DEPLOYMENT_TARGET_ENV): QMAKE_MACOSX_DEPLOYMENT_TARGET = $$MACOSX_DEPLOYMENT_TARGET_ENV

DEFINES += GLM_ENABLE_EXPERIMENTAL
DEFINES += GL_SILENCE_DEPRECATION
DEFINES += WHIPLIB_ENABLE_FBX=$$ENABLE_FBX

INCLUDEPATH += . \
    ../WhipLib \
    $$GLEW_PREFIX/include \
    $$GLEW_PREFIX/include/GL \
    $$GLM_PREFIX/include \
    $$GLM_PREFIX/include/glm

LIBS += ../lib/WhipLib.a \
    -L$$GLEW_PREFIX/lib \
    -lGLEW \
    -framework OpenGL

equals(ENABLE_FBX, 1) {
    INCLUDEPATH += ../external/FBX/include
    LIBS += $$FBX_LIB_DIR/libalembic.a \
        $$FBX_LIB_DIR/libfbxsdk.a \
        -L$$LIBXML2_PREFIX/lib \
        -lxml2 \
        -lz
}

SOURCES += $$files(*.cpp)
HEADERS += $$files(*.h)
FORMS += $$files(*.ui)
RESOURCES += resource.qrc
