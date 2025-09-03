CCFLAGS= -g `pkg-config --cflags sdl2` `pkg-config --cflags sdl2_ttf` -Wpedantic -Wall -MD -fsanitize=address -Isrc/common

LDFLAGS=  `pkg-config --libs sdl2_ttf`

SOURCES=$(wildcard src/*.c) $(wildcard src/common/*.c)

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