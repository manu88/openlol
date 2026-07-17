CCFLAGS= -g `pkg-config --cflags sdl2` `pkg-config --cflags SDL2_image` `pkg-config --cflags SDL2_ttf` `pkg-config --cflags sndfile` -Wpedantic -Wall -MD -fsanitize=address -Isrc/common -Isrc/game -Isrc/dbg
CCFLAGS+=-Wno-unknown-pragmas


CPPFLAGS=--std=c++14 -Isrc/ymfm/src/ -g `pkg-config --cflags sdl2`

LDFLAGS=  `pkg-config --libs SDL2_image` `pkg-config --libs SDL2_ttf` `pkg-config --libs sndfile`

SOURCES=$(wildcard src/*.c) $(wildcard src/common/*.c) $(wildcard src/common/formats/*.c) $(wildcard src/game/*.c) $(wildcard src/dbg/*.c)

OBJECTS=$(filter %.o,$(SOURCES:.c=.o))

YMFM_SOURCES=$(wildcard src/ymfm/src/*.cpp) $(wildcard src/common/*.cpp) $(wildcard src/common/mplayer/*.cpp)

YMFM_OBJECTS=$(filter %.o,$(YMFM_SOURCES:.cpp=.o))

EXECUTABLE=lol

all: $(SOURCES) $(YMFM_SOURCES) $(EXECUTABLE)

%.o: %.c
	$(CC) -c  $(CCFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) -c  $(CPPFLAGS) $< -o $@


$(EXECUTABLE): $(OBJECTS)  $(YMFM_OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(YMFM_OBJECTS) $(CCFLAGS) -o $(EXECUTABLE)

clean:
	rm -f $(OBJECTS)
	rm -f $(YMFM_OBJECTS)
	rm -f $(EXECUTABLE)
	rm -f src/*.d
	rm -f src/common/*.d
	rm -f src/dbg/*.d
	rm -f src/common/mplayer/*.d
	rm -f src/common/formats/*.d
	rm -f src/game/*.d

.PHONY: clean all

-include $(OBJECTS:.o=.d)