ROOT=..
include $(ROOT)/Make.defines.macros

LIBMISC = libq.a
OBJS = error.o ascii.o graph_lib.o graph_global.o \
	   char_lib.o

all: $(LIBMISC)

$(LIBMISC): $(OBJS)
	$(AR) rv $(LIBMISC) $?

%:	%.c $(LIBQ)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(TEMPFILES)

