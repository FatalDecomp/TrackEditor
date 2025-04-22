LIBS = 	-lGL \
		../external/glew/lib/libGLEW.a \
		../external/FBX/lib/release/libalembic.a \
		../external/FBX/lib/release/libfbxsdk.a \
		-lxml2 \
		-lz

INCLUDES = 	-I. \
			-I/usr/include/libxml2 \
			-I../external/glew/include \
			-I../external/glew/include/GL \
			-I../external/glm \
			-I../external/FBX/include \
			-I../external/stb \
			-I./CarPlans \
			-I./SignPlans

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
CXXFLAGS = -g $(DEBUGFLAGS) $(INCLUDES) -fPIC -D GLM_ENABLE_EXPERIMENTAL
LDFLAGS = -Wl,-R\$$ORIGIN -static-libgcc -static-libstdc++

.SUFFIXES: .o

SRC = 	Camera.cpp \
		CarHelpers.cpp \
		Clock.cpp \
		DisasmHelpers.cpp \
		DriveComponent.cpp \
		Entity.cpp \
		FBXExporter.cpp \
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
	make -j 4 build BUILDCONFIG=debug
release: clean
	make -j 4 build BUILDCONFIG=release

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
	rm -f ../lib/WhipLib
	rm -f obj/*/*.o
