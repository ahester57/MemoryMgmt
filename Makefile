
CC= gcc
CFLAGS= -Wall -g
LDLIBS= -pthread
COMMONSRCS= sighandler.c ipchelper.c memory.c
OSSSRCS = oss.c
USERSRCS = user.c

OBJECTS= $(patsubst %.c,%.o,$(COMMONSRCS))
# these are suffix rules ^

.PHONY: all clean

all: oss user 

oss: $(OBJECTS)

user: $(OBJECTS)

$(OBJECTS): sighandler.h ipchelper.h osstypes.h memory.h

clean:
	$(RM) $(OBJECTS) *.o *log oss user
