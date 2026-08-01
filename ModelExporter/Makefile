UNAME_S := $(shell uname -s)

ENABLE_FBX ?= 1

GLEW_PREFIX ?= $(shell brew --prefix glew 2>/dev/null)
GLM_PREFIX ?= $(shell brew --prefix glm 2>/dev/null)
LIBXML2_PREFIX ?= $(shell brew --prefix libxml2 2>/dev/null)
FBX_ROOT ?= ../external/FBX
FBX_LIB_DIR ?= $(FBX_ROOT)/lib/release

ifeq ($(UNAME_S),Darwin)
	MACOSX_DEPLOYMENT_TARGET ?= $(shell sw_vers -productVersion | awk -F. '{print $$1 ".0"}')
	OPENGL_LIBS = -framework OpenGL
	GLEW_LIBS = -L$(GLEW_PREFIX)/lib -lGLEW
	PLATFORM_INCLUDES = -I$(GLEW_PREFIX)/include/GL \
			-I$(GLM_PREFIX)/include \
			-I$(GLM_PREFIX)/include/glm
	PLATFORM_CXXFLAGS = -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
	PLATFORM_LDFLAGS =
else
	OPENGL_LIBS = -lGL
	GLEW_LIBS = ../external/glew/lib/libGLEW.a
	PLATFORM_INCLUDES = -I../external/glew/include/GL \
			-I../external/glm
	PLATFORM_CXXFLAGS =
	PLATFORM_LDFLAGS = -Wl,-R\$$ORIGIN -static-libgcc -static-libstdc++
endif

LIBS = 	../lib/WhipLib.a \
		$(GLEW_LIBS) \
		$(OPENGL_LIBS)

INCLUDES = 	-I. \
			-I../WhipLib \
			$(PLATFORM_INCLUDES)

ifeq ($(ENABLE_FBX),1)
	LIBS += $(FBX_LIB_DIR)/libalembic.a \
			$(FBX_LIB_DIR)/libfbxsdk.a \
			-L$(LIBXML2_PREFIX)/lib \
			-lxml2 \
			-lz
	FBX_DEFINES = -D WHIPLIB_ENABLE_FBX=1
else
	FBX_DEFINES = -D WHIPLIB_ENABLE_FBX=0
endif

ifeq ($(BUILDCONFIG), debug)
	DEBUGFLAGS = -D _DEBUG -D DEBUG
else ifeq ($(BUILDCONFIG), release)
	DEBUGFLAGS = -D NDEBUG -O
endif

ARCH = x64
ARCHFLAGS = -m64
OUTPUTDIR = ../bin/TrackEditor
TARGET = ../bin/TrackEditor/ModelExporter

FULLCONFIG=$(BUILDCONFIG)-$(ARCH)
CXXFLAGS = -std=c++17 -g $(DEBUGFLAGS) $(INCLUDES) $(PLATFORM_CXXFLAGS) -fPIC -D GLM_ENABLE_EXPERIMENTAL -D GL_SILENCE_DEPRECATION $(FBX_DEFINES)
LDFLAGS = $(PLATFORM_LDFLAGS)

.SUFFIXES: .o

SRC = 	Main.cpp

default: debug
debug: clean
	make -j 4 build BUILDCONFIG=debug ENABLE_FBX=$(ENABLE_FBX)
release: clean
	make -j 4 build BUILDCONFIG=release ENABLE_FBX=$(ENABLE_FBX)

build: ModelExporter

obj/$(FULLCONFIG)/%.o: %.cpp
	$(CXX) $(ARCHFLAGS) $(CXXFLAGS) -c -o $@ $<

OBJ = $(patsubst %.cpp,obj/$(FULLCONFIG)/%.o, $(SRC))
$(OBJ): | CREATE_OBJ_DIR

CREATE_OBJ_DIR:
	mkdir -p obj
	mkdir -p obj/$(FULLCONFIG)

ModelExporter: $(OBJ) Makefile
	mkdir -p $(OUTPUTDIR)
	$(CXX) $(ARCHFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $(OUTPUTDIR)/$@ $(OBJ) $(LIBS)

clean:
	rm -f ../bin/TrackEditor/ModelExporter
	rm -f obj/*/*.o
