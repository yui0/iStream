CC = gcc

OTHER_HEADERS = ../../mjpg_streamer.h ../../utils.h ../output.h ../input.h

CFLAGS += -Os -DLINUX -D_GNU_SOURCE -Wall -shared -fPIC
#CFLAGS += -DDEBUG
LFLAGS += -lpthread -ldl

all: input_screen.so

clean:
	rm -f *.a *.o core *~ *.so *.lo

input_screen.so: $(OTHER_HEADERS) input_screen.c jpeg.c
	$(CC) $(CFLAGS) -o $@ input_screen.c jpeg.c -lrt -lX11 -lXext -lXtst
