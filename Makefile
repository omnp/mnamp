PLUGIN_TARGET_TYPE=.so# .dll
EXECUTABLE_TARGET_TYPE=#.exe
override CXXFLAGS+= -Wno-unused-parameter -Wall -Wextra -Werror -pedantic -std=c++20
CXX=g++#clang++
COMMON=common/math.h common/bqfilter.h common/svfilter.h common/functions.h
LOOKUP=lookup/lookup.h

.PHONY: all
all:
	@echo "Additional compiler flags such as -O3 -mtune=native -mavx -mfpmath=sse "
	@echo "can be provided on the command line by running:"
	@echo "	make target CXXFLAGS=\"-O3\""
	@echo "I recommend building with internal double precision processing type "
	@echo "and a lookup table:"
	@echo "	make target CXXFLAGS=\"-DUSE_DOUBLE -DUSE_LUT\""
	@echo "in addition to any optimization flags."
	@echo
	@echo "The possible targets are:"
	@echo "	mnamp - Plugin"
	@echo "	mndist - Distortion plugin"
	@echo "	generate_lookup - An executable file that when run generates a new lookup.h file "
	@echo "		in *the current working directory* that it is run from."
	@echo "		Currently the generating parameters need to be changed by editing main in "
	@echo "		the file lookup/generate_lookup.cc"
	@echo

mnamp: build/mnamp.lv2/manifest.ttl build/mnamp.lv2/mnamp.ttl build/mnamp.lv2/mnamp${PLUGIN_TARGET_TYPE}
mndist: build/mndist.lv2/manifest.ttl build/mndist.lv2/mndist.ttl build/mndist.lv2/mndist${PLUGIN_TARGET_TYPE}
generate_lookup: build/generate_lookup${EXECUTABLE_TARGET_TYPE}

# mnamp
build/mnamp.lv2/manifest.ttl: mnamp/manifest.ttl
	mkdir -vp build/mnamp.lv2
	cp mnamp/manifest.ttl build/mnamp.lv2/manifest.ttl

build/mnamp.lv2/mnamp.ttl: mnamp/mnamp.ttl
	mkdir -vp build/mnamp.lv2
	cp mnamp/mnamp.ttl build/mnamp.lv2/mnamp.ttl

build/mnamp.lv2/mnamp${PLUGIN_TARGET_TYPE}: mnamp/mnamp.cc ${COMMON} ${LOOKUP}
	mkdir -vp build/mnamp.lv2
	${CXX} -shared -fPIC ${CXXFLAGS} mnamp/mnamp.cc -o build/mnamp.lv2/mnamp${PLUGIN_TARGET_TYPE}

# mndist
build/mndist.lv2/manifest.ttl: mndist/manifest.ttl
	mkdir -vp build/mndist.lv2
	cp mndist/manifest.ttl build/mndist.lv2/manifest.ttl

build/mndist.lv2/mndist.ttl: mndist/mndist.ttl
	mkdir -vp build/mndist.lv2
	cp mndist/mndist.ttl build/mndist.lv2/mndist.ttl

build/mndist.lv2/mndist${PLUGIN_TARGET_TYPE}: mndist/mndist.cc ${COMMON} ${LOOKUP}
	mkdir -vp build/mndist.lv2
	${CXX} -shared -fPIC ${CXXFLAGS} mndist/mndist.cc -o build/mndist.lv2/mndist${PLUGIN_TARGET_TYPE}

# generate_lookup
build/generate_lookup${EXECUTABLE_TARGET_TYPE}: lookup/generate_lookup.cc ${COMMON}
	mkdir -vp build
	${CXX} ${CXXFLAGS} lookup/generate_lookup.cc -o build/generate_lookup${EXECUTABLE_TARGET_TYPE}
