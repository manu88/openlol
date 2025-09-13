CCFLAGS= -g `pkg-config --cflags sdl2` `pkg-config --cflags sdl2_image` -Wpedantic -Wall -MD -fsanitize=address -Isrc/common -Isrc/game

LDFLAGS=  `pkg-config --libs sdl2_image`

SOURCES=$(wildcard src/*.c) $(wildcard src/common/*.c) $(wildcard src/game/*.c)

OBJECTS=$(filter %.o,$(SOURCES:.c=.o))

ifeq ($(UNAME_S),Darwin)
OBJECTS+=$(filter %.o,$(SOURCES:.m=.o))
endif

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

.PHONY: clean all

-include $(OBJECTS:.o=.d)