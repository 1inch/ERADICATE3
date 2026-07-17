CC=g++
CDEFINES=
SOURCES=Dispatcher.cpp eradicate2.cpp hexadecimal.cpp ModeFactory.cpp Speed.cpp sha3.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=ERADICATE2.x64

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# -mmmx is x86-only; Apple Silicon and other ARM hosts reject it.
ifeq ($(filter x86_64 amd64 i386 i686,$(UNAME_M)),)
	ARCH_CFLAGS=
else
	ARCH_CFLAGS=-mmmx
endif

ifeq ($(UNAME_S),Darwin)
	LDFLAGS=-framework OpenCL
	CFLAGS=-c -std=c++11 -Wall $(ARCH_CFLAGS) -O2
else ifneq (,$(findstring _NT,$(UNAME_S)))
	# Windows (MSYS2/MinGW): -mcmodel=large is not supported by PE targets.
	# GCC runtime libs are linked statically so the exe runs without MinGW
	# DLLs, but OpenCL must stay dynamic (system OpenCL.dll from GPU driver).
	EXECUTABLE=ERADICATE2.x64.exe
	LDFLAGS=-s -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive,-Bdynamic -lOpenCL
	CFLAGS=-c -std=c++11 -Wall $(ARCH_CFLAGS) -O2
else
	LDFLAGS=-s -lOpenCL -mcmodel=large
	CFLAGS=-c -std=c++11 -Wall $(ARCH_CFLAGS) -O2 -mcmodel=large
endif

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(CDEFINES) $< -o $@

clean:
	rm -rf *.o
