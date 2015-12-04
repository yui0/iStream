###############################################################
#
# Purpose: Makefile for "M-JPEG Streamer"
# Author.: Tom Stoeveken (TST)
# Version: 0.3
# License: GPL
#
###############################################################

CC = gcc

OTHER_HEADERS = ../../mjpg_streamer.h ../../utils.h ../output.h ../input.h

CFLAGS += -Os -DLINUX -D_GNU_SOURCE -Wall -shared -fPIC
#CFLAGS +=  -g
#CFLAGS += -DDEBUG
LFLAGS += -lpthread -ldl

ifeq ($(WXP_COMPAT),true)
CFLAGS += -DWXP_COMPAT
endif

all: output_http.so

clean:
	rm -f *.a *.o core *~ *.so *.lo

output_http.so: $(OTHER_HEADERS) output_http.c httpd.lo
	$(CC) $(CFLAGS) -o $@ output_http.c httpd.lo wsserver/wsserver.c wsserver/lib/base64_enc.c wsserver/lib/sha1.c wsserver/lib/websocket.c -I wsserver/lib/ -DPLUGIN_WS -lX11 -lXtst

httpd.lo: $(OTHER_HEADERS) httpd.h httpd.c
	$(CC) -c $(CFLAGS) -o $@ httpd.c

wsserver.a: $(OTHER_HEADERS) wsserver/wsserver.c wsserver/lib/base64_enc.h wsserver/lib/sha1.h wsserver/lib/websocket.h
	$(CC) $(CFLAGS) -o $@ wsserver/wsserver.c wsserver/lib/base64_enc.c wsserver/lib/sha1.c wsserver/lib/websocket.c -I wsserver/lib/
