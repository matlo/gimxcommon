ifeq ($(OS),Windows_NT)
OBJECTS += $(patsubst %.c,%.o,$(wildcard src/windows/*.c))
else
OBJECTS += $(patsubst %.c,%.o,$(wildcard src/posix/*.c))
endif

CPPFLAGS += -Iinclude -I../
CFLAGS += -fPIC

all: $(OBJECTS)

clean:
	$(RM) $(OBJECTS)

install:

uninstall:
