PLUGIN_TARGET_TYPE=.so# .dll
EXECUTABLE_TARGET_TYPE=#.exe
override CXXFLAGS+= -Wno-unused-parameter -Wno-implicit-fallthrough -Wall -Wextra -Werror -pedantic -std=c++20
CXX=g++#clang++
COMMON=common/math.h common/onepole.h

.PHONY: all
all:
	@echo
	@echo "This is the Makefile for the mnamp projects."
	@echo
	@echo "An alternative C++ compiler can be selected by giving the make argument variable assignment CXX=whatever. "
	@echo "For example: make target CXX=clang++ "
	@echo "Also one may provide a platform specific PLUGIN_TARGET_TYPE (=.so on Linux and =.dll on Windows)"
	@echo "Similarly one may provide the variable EXECUTABLE_TARGET_TYPE (=.exe on Windows on Linux this can be left empty.) "
	@echo "*Additional* compiler flags such as -O3 -mtune=native -mavx -mfpmath=sse can be provided on the command line by running: "
	@echo "	make target CXXFLAGS=\"-O3\" "
	@echo "I recommend building with internal double precision processing type and a lookup table in addition to any optimization flags: "
	@echo "	make target CXXFLAGS=\"-DUSE_DOUBLE -DUSE_LUT\" "
	@echo
	@echo "The possible targets are: "
	@echo "	mnamp - Plugin "
	@echo

mnamp: build/mnamp.lv2/manifest.ttl build/mnamp.lv2/mnamp.ttl build/mnamp.lv2/mnamp${PLUGIN_TARGET_TYPE}

# mnamp
build/mnamp.lv2/manifest.ttl: mnamp/manifest.ttl
	mkdir -vp build/mnamp.lv2
	cp mnamp/manifest.ttl build/mnamp.lv2/manifest.ttl

build/mnamp.lv2/mnamp.ttl: mnamp/mnamp.ttl
	mkdir -vp build/mnamp.lv2
	cp mnamp/mnamp.ttl build/mnamp.lv2/mnamp.ttl

build/mnamp.lv2/mnamp${PLUGIN_TARGET_TYPE}: mnamp/mnamp.cc ${COMMON}
	mkdir -vp build/mnamp.lv2
	${CXX} -shared -fPIC ${CXXFLAGS} mnamp/mnamp.cc -o build/mnamp.lv2/mnamp${PLUGIN_TARGET_TYPE}
