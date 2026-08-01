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
	PLATFORM_INCLUDES = -I$(LIBXML2_PREFIX)/include/libxml2 \
			-I$(GLEW_PREFIX)/include \
			-I$(GLEW_PREFIX)/include/GL \
			-I$(GLM_PREFIX)/include \
			-I$(GLM_PREFIX)/include/glm
	PLATFORM_CXXFLAGS = -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
	PLATFORM_LDFLAGS =
else
	OPENGL_LIBS = -lGL
	GLEW_LIBS = ../external/glew/lib/libGLEW.a
	PLATFORM_INCLUDES = -I/usr/include/libxml2 \
			-I../external/glew/include \
			-I../external/glew/include/GL \
			-I../external/glm
	PLATFORM_CXXFLAGS =
	PLATFORM_LDFLAGS = -Wl,-R\$$ORIGIN -static-libgcc -static-libstdc++
endif

LIBS = 	$(OPENGL_LIBS) \
		$(GLEW_LIBS)

INCLUDES = 	-I. \
			$(PLATFORM_INCLUDES) \
			-I../external/stb \
			-I./CarPlans \
			-I./SignPlans

ifeq ($(ENABLE_FBX),1)
	LIBS += $(FBX_LIB_DIR)/libalembic.a \
			$(FBX_LIB_DIR)/libfbxsdk.a \
			-L$(LIBXML2_PREFIX)/lib \
			-lxml2 \
			-lz
	INCLUDES += -I$(FBX_ROOT)/include
	FBX_DEFINES = -D WHIPLIB_ENABLE_FBX=1 -DmLefttChild=mLeftChild
	FBX_SRC = FBXExporter.cpp
else
	FBX_DEFINES = -D WHIPLIB_ENABLE_FBX=0
	FBX_SRC =
endif

ifeq ($(BUILDCONFIG), debug)
	DEBUGFLAGS = -D _DEBUG -D DEBUG
else ifeq ($(BUILDCONFIG), release)
	DEBUGFLAGS = -D NDEBUG -O
endif

ARCH = x64
ARCHFLAGS = -m64
OUTPUTDIR = ../lib
TARGET = ../lib/WhipLib

FULLCONFIG=$(BUILDCONFIG)-$(ARCH)
CXXFLAGS = -std=c++17 -g $(DEBUGFLAGS) $(INCLUDES) $(PLATFORM_CXXFLAGS) -fPIC -D GLM_ENABLE_EXPERIMENTAL -D GL_SILENCE_DEPRECATION $(FBX_DEFINES)
LDFLAGS = $(PLATFORM_LDFLAGS)

.SUFFIXES: .o

SRC = 	Camera.cpp \
		CarHelpers.cpp \
		Clock.cpp \
		DisasmHelpers.cpp \
		DriveComponent.cpp \
		Entity.cpp \
		$(FBX_SRC) \
		GameClock.cpp \
		GameInput.cpp \
		IndexBuffer.cpp \
		Logging.cpp \
		MathHelpers.cpp \
		NoclipComponent.cpp \
		ObjExporter.cpp \
		ObjImporter.cpp \
		Palette.cpp \
		PhysicsComponent.cpp \
		Renderer.cpp \
		Scene.cpp \
		SceneManager.cpp \
		Shader.cpp \
		ShapeComponent.cpp \
		ShapeData.cpp \
		ShapeFactory.cpp \
		Texture.cpp \
		Track.cpp \
		TrackComponent.cpp \
		Unmangler.cpp \
		VertexArray.cpp \
		VertexBuffer.cpp \
		WhipLib.cpp

default: debug
debug: clean
	make -j 4 build BUILDCONFIG=debug ENABLE_FBX=$(ENABLE_FBX)
release: clean
	make -j 4 build BUILDCONFIG=release ENABLE_FBX=$(ENABLE_FBX)

build: WhipLib.a

obj/$(FULLCONFIG)/%.o: %.cpp
	$(CXX) $(ARCHFLAGS) $(CXXFLAGS) -c -o $@ $<

OBJ = $(patsubst %.cpp,obj/$(FULLCONFIG)/%.o, $(SRC))
$(OBJ): | CREATE_OBJ_DIR

CREATE_OBJ_DIR:
	mkdir -p obj
	mkdir -p obj/$(FULLCONFIG)

WhipLib.a: $(OBJ) Makefile
	mkdir -p $(OUTPUTDIR)
	ar rc $(OUTPUTDIR)/$@ $(OBJ)
	ranlib $(OUTPUTDIR)/$@

clean:
	rm -f ../lib/WhipLib.a
	rm -f obj/*/*.o
