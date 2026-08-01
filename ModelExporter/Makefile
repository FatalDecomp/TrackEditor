LIBS = 	../lib/WhipLib.a \
		../external/FBX/lib/release/libalembic.a \
		../external/FBX/lib/release/libfbxsdk.a \
		-lxml2 \
		-lz

INCLUDES = 	-I. \
			-I../WhipLib \
			-I../external/glm

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
CXXFLAGS = -g $(DEBUGFLAGS) $(INCLUDES) -fPIC -D GLM_ENABLE_EXPERIMENTAL
LDFLAGS = -Wl,-R\$$ORIGIN -static-libgcc -static-libstdc++

.SUFFIXES: .o

SRC = 	Main.cpp

default: debug
debug: clean
	make -j 4 build BUILDCONFIG=debug
release: clean
	make -j 4 build BUILDCONFIG=release

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
