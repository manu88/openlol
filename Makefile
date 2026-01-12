CCFLAGS= -g `pkg-config --cflags sdl2` `pkg-config --cflags SDL2_image` `pkg-config --cflags SDL2_ttf` `pkg-config --cflags sndfile` -Wpedantic -Wall -MD -fsanitize=address -Isrc/common -Isrc/game -Isrc/dbg
CCFLAGS+=-Wno-unknown-pragmas

LDFLAGS=  `pkg-config --libs SDL2_image` `pkg-config --libs SDL2_ttf` `pkg-config --libs sndfile`

SOURCES=$(wildcard src/*.c) $(wildcard src/common/*.c) $(wildcard src/common/formats/*.c) $(wildcard src/game/*.c) $(wildcard src/dbg/*.c)

OBJECTS=$(filter %.o,$(SOURCES:.c=.o))

EXECUTABLE= lol

all: $(SOURCES) $(EXECUTABLE)

%.o: %.c
	$(CC) -c  $(CCFLAGS) $< -o $@


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(CCFLAGS) $(LDFLAGS) -o $(EXECUTABLE)

clean:
	rm -f $(OBJECTS)
	rm -f $(EXECUTABLE)
	rm -f src/*.d
	rm -f src/common/*.d
	rm -f src/common/formats/*.d
	rm -f src/game/*.d

.PHONY: clean all

-include $(OBJECTS:.o=.d)