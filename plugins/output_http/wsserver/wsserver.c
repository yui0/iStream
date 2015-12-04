//---------------------------------------------------------
//	Motion JPEG
//
//		©2013 Yuichiro Nakada
//---------------------------------------------------------
// gcc -o wsserver wsserver.c -Os ./lib/*.c -I ./lib/
// gcc -o wsserver wsserver.c jpeg.c -Os ./lib/*.c -I ./lib/ -lrt -lX11 -lXext

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "websocket.h"

//#define PORT 8088
//#define PORT 17743
#define PORT 443
#define BUF_LEN 512*1024
//#define PACKET_DUMP

void error(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

int safeSend(int clientSocket, const uint8_t *buffer, size_t bufferSize)
{
#ifdef PACKET_DUMP
	printf("out packet: [%d]\n", bufferSize);
	fwrite(buffer, 1, bufferSize, stdout);
	printf("\n");
#endif
	ssize_t written = send(clientSocket, buffer, bufferSize, 0);
	if (written == -1) {
		close(clientSocket);
		perror("send failed");
		return EXIT_FAILURE;
	}
	if (written != bufferSize) {
		close(clientSocket);
		perror("written not all bytes");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

#include <time.h>
#include <X11/Xlib.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#define TIME_DIF_TO_NS(s,f) \
	((f.tv_sec-s.tv_sec)*1000000000.0 + (f.tv_nsec-s.tv_nsec))
//http://blog.frankvh.com/2009/06/09/blackfin-fast-jpeg-encoding/
#include "jpeg.h"
Display* dpy;
int scr;
Window root;
	XImage *image;
	XShmSegmentInfo shminfo;
unsigned char img[512*1024];
int size;
void worker_init()
{
	XWindowAttributes attributes;

	dpy = XOpenDisplay("");
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);

	XGetWindowAttributes(dpy, root, &attributes);
	attributes.width = 800;
	attributes.height = 600;

//	XShmSegmentInfo shminfo;
	image = XShmCreateImage(dpy,
		DefaultVisual(dpy, scr), DefaultDepth(dpy, scr),
		ZPixmap, NULL, &shminfo, attributes.width, attributes.height);
	shminfo.shmid = shmget(IPC_PRIVATE,
		image->bytes_per_line * image->height, IPC_CREAT|0777);
	//if (shminfo.shmid == -1) err
	shminfo.shmaddr = image->data = shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;
}
void worker_cleanup()
{
	XDestroyImage(image);
	shmdt(shminfo.shmaddr);

	XCloseDisplay(dpy);
}
void *worker_thread(void *arg)
{
	/* set cleanup handler to cleanup allocated ressources */
//	pthread_cleanup_push(worker_cleanup, NULL);

/*	XWindowAttributes attributes;
//	XImage *image;

	dpy = XOpenDisplay("");
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);

	XGetWindowAttributes(dpy, root, &attributes);
	attributes.width = 800;
	attributes.height = 600;

//	XShmSegmentInfo shminfo;
	image = XShmCreateImage(dpy,
		DefaultVisual(dpy, scr), DefaultDepth(dpy, scr),
		ZPixmap, NULL, &shminfo, attributes.width, attributes.height);
	shminfo.shmid = shmget(IPC_PRIVATE,
		image->bytes_per_line * image->height, IPC_CREAT|0777);
	//if (shminfo.shmid == -1) err
	shminfo.shmaddr = image->data = shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;

	while (!pglobal->stop) {*/
		struct timespec t1, t2;
		clock_gettime(CLOCK_MONOTONIC, &t1);

		if (!XShmAttach(dpy, &shminfo)) {}
		if (!XShmGetImage(dpy, root, image, 0, 0, AllPlanes)) {}

		/* copy JPG picture to global buffer */
//		pthread_mutex_lock(&pglobal->in[plugin_no].db);
//		pglobal->in[plugin_no].size = 0;
		size = 0;
		if (image != NULL) {
			unsigned char *p = encode_image(image->data, img/*pglobal->in[plugin_no].buf*/, 2/*quality*/, RGB32, /*attributes.width*/800, /*attributes.height*/600);
//			pglobal->in[plugin_no].size = p - pglobal->in[plugin_no].buf;
			size = p - img;
		}

		XShmDetach(dpy, &shminfo);

		/* signal fresh_frame */
//		pthread_cond_broadcast(&pglobal->in[plugin_no].db_update);
//		pthread_mutex_unlock(&pglobal->in[plugin_no].db);

		clock_gettime(CLOCK_MONOTONIC, &t2);
		/*double*/int d = ((1000*1000*1000/20) - TIME_DIF_TO_NS(t1, t2))/1000.0;
//		fprintf(stderr, "time:%d size:%dkB\n", d, pglobal->in[plugin_no].size/1024);
		fprintf(stderr, "time:%d size:%dkB\n", d, size/1024);
		if (d>0) usleep(d);
/*	}

	XDestroyImage(image);
	shmdt(shminfo.shmaddr);

	XCloseDisplay(dpy);*/

//	IPRINT("leaving input thread, calling cleanup function now\n");
//	pthread_cleanup_pop(1);

	return NULL;
}

Display* dpy;
int scr;
Window root;
// マウスの移動
inline void move_mouse(int x, int y)
{
	XTestFakeMotionEvent(dpy, scr, x, y, CurrentTime);
}
// マウスイベントの発生
inline void mouse_button(int n, int type)
{
	if (type & 1) XTestFakeButtonEvent(dpy, n, True, CurrentTime);
	if (type & 2) XTestFakeButtonEvent(dpy, n, False, CurrentTime);
}
// キーイベントの発生
void keyboard(int n)
{
	// http://homepage3.nifty.com/tsato/xvkbd/events-j.html
	unsigned int key = XKeysymToKeycode(dpy, n);
	XTestFakeKeyEvent(dpy, key, True, CurrentTime);
	XTestFakeKeyEvent(dpy, key, False, CurrentTime);
}
void ws_command(char *s)
{
	dpy = XOpenDisplay("");
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);

	int v, x, y;

	sscanf(&s[2], "%d", &v);
	x = (v>>16);
	y = v&0xffff;
	printf("cmd: [%s](%d:%d,%d)\n", s, v, x, y);

	switch (s[0]) {
	case '0':	// 左クリック
		move_mouse(x, y);
		mouse_button(1, s[1]);
		break;
	case '1':	// 中クリック
		move_mouse(x, y);
		mouse_button(2, s[1]);
		break;
	case '2':	// 右クリック
		move_mouse(x, y);
		mouse_button(3, s[1]);
		break;
	case '3':	// wheel
		break;
	case '4':	// move
		move_mouse(x, y);
		break;

	case '5':	// キーボード
		keyboard(v);
		break;

	/*case '6':	// 解像度
		resolution(x, y);
		break;
	case '7':	// RDP開始
		input_run(plugin_no);
		break;
	case '8':	// RDP停止
		input_stop(plugin_no);
		break;
	case '9':	// Motion JPEG quality
		quality = value;
		break;*/
	}

	XCloseDisplay(dpy);
}

void wsserver(int clientSocket, char *p, int n)
{
	uint8_t buffer[BUF_LEN];
	//memset(buffer, 0, BUF_LEN);
	if (p) memcpy(buffer, p, n);
	size_t readedLength = n; //0;
	size_t frameSize = BUF_LEN;
	enum wsState state = WS_STATE_OPENING;
	uint8_t *data = NULL;
	size_t dataSize = 0;
	enum wsFrameType frameType = WS_INCOMPLETE_FRAME;
	struct handshake hs;
	nullHandshake(&hs);

	#define prepareBuffer frameSize = BUF_LEN; memset(buffer, 0, BUF_LEN);
	#define initNewFrame frameType = WS_INCOMPLETE_FRAME; readedLength = 0; /*memset(buffer, 0, BUF_LEN);*/

	while (frameType == WS_INCOMPLETE_FRAME) {
		/*ssize_t readed = recv(clientSocket, buffer+readedLength, BUF_LEN-readedLength, MSG_DONTWAIT);
		if (readed<0) {
			// for stream
			worker_thread(NULL);

			char buff[512*1024];
			int s = base64enc(&buffer[10], img, size);
			uint8_t *p = wsMakeFrame(NULL, s, buffer, &frameSize, WS_TEXT_FRAME);
//			printf("%d %d %d\n",size,s,frameSize);
			if (safeSend(clientSocket, p, frameSize) == EXIT_FAILURE) break;
			frameType = WS_INCOMPLETE_FRAME; readedLength = 0;
			continue;
		} else*/

		ssize_t readed = recv(clientSocket, buffer+readedLength, BUF_LEN-readedLength, 0);
		if (!readed) {
			close(clientSocket);
			perror("recv failed");
			return;
		}
#ifdef PACKET_DUMP
		printf("in packet: [%d]\n", readed);
		fwrite(buffer, 1, readed, stdout);
		printf("\n");
#endif
		readedLength += readed;
		assert(readedLength <= BUF_LEN);
		buffer[readedLength] = 0;

		if (state == WS_STATE_OPENING) {
			frameType = wsParseHandshake(buffer, readedLength, &hs);
		} else {
			frameType = wsParseInputFrame(buffer, readedLength, &data, &dataSize);
		}

		if ((frameType == WS_INCOMPLETE_FRAME && readedLength == BUF_LEN) || frameType == WS_ERROR_FRAME) {
			// 受信失敗
			if (frameType == WS_INCOMPLETE_FRAME)
				printf("buffer too small\n");
			else
				printf("error in incoming frame\n");

			if (state == WS_STATE_OPENING) {
				prepareBuffer;
				frameSize = sprintf((char *)buffer,
					"HTTP/1.1 400 Bad Request\r\n"
					"%s%s\r\n\r\n",
					versionField,
					version);
				safeSend(clientSocket, buffer, frameSize);
				break;
			} else {
				prepareBuffer;
				wsMakeFrame(NULL, 0, buffer, &frameSize, WS_CLOSING_FRAME);
				if (safeSend(clientSocket, buffer, frameSize) == EXIT_FAILURE) break;
				state = WS_STATE_CLOSING;
				initNewFrame;
			}
		}

		if (state == WS_STATE_OPENING) {
			assert(frameType == WS_OPENING_FRAME);
			if (frameType == WS_OPENING_FRAME) {
				// if resource is right, generate answer handshake and send it
//				if (strcmp(hs.resource, "/echo") != 0) {
				if (strcmp(hs.resource, "/socket") != 0) {
					frameSize = sprintf((char *)buffer, "HTTP/1.1 404 Not Found\r\n\r\n");
					if (safeSend(clientSocket, buffer, frameSize) == EXIT_FAILURE) break;
				}

				// for echo
				prepareBuffer;
				wsGetHandshakeAnswer(&hs, buffer, &frameSize);
				if (safeSend(clientSocket, buffer, frameSize) == EXIT_FAILURE) break;
				state = WS_STATE_NORMAL;
				initNewFrame;
			}
		} else {
			if (frameType == WS_CLOSING_FRAME) {
				if (state == WS_STATE_CLOSING) {
					break;
				} else {
					prepareBuffer;
					wsMakeFrame(NULL, 0, buffer, &frameSize, WS_CLOSING_FRAME);
					safeSend(clientSocket, buffer, frameSize);
					break;
				}
			} else if (frameType == WS_TEXT_FRAME) {
#ifdef ECHO
				// for echo
				uint8_t *recievedString = NULL;
				recievedString = malloc(dataSize+1);
				assert(recievedString);
				memcpy(recievedString, data, dataSize);
				recievedString[ dataSize ] = 0;

				prepareBuffer;
				wsMakeFrame(recievedString, dataSize, buffer, &frameSize, WS_TEXT_FRAME);
//				char buff[1024*1024];
//				base64enc(buff, PIC_160x120_1, sizeof(PIC_160x120_1));
//				wsMakeFrame(buff, sizeof(PIC_160x120_1), buffer, &frameSize, WS_TEXT_FRAME);
				if (safeSend(clientSocket, buffer, frameSize) == EXIT_FAILURE) break;
#else
				ws_command(data);
#endif
				initNewFrame
			} else if (frameType == WS_BINARY_FRAME) {
				// for echo xxxx
				printf("WS_BINARY_FRAME\n");
				uint8_t *recievedString = NULL;
				recievedString = malloc(dataSize+1);
				assert(recievedString);
				memcpy(recievedString, data, dataSize);
				recievedString[ dataSize ] = 0;

				printf("r:%x[%d]\n",recievedString[0],dataSize);
				//recievedString[0] = "1";

				prepareBuffer;
				wsMakeFrame(recievedString, dataSize, buffer, &frameSize, WS_BINARY_FRAME);
				if (safeSend(clientSocket, buffer, frameSize) == EXIT_FAILURE) break;
				initNewFrame;
			}
		}
	} // read/write cycle

	close(clientSocket);
}

#ifndef PLUGIN_WS
int server_init()
{
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == -1) {
		error("create socket failed");
	}

	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(PORT);
	if (bind(listenSocket, (struct sockaddr *) &local, sizeof(local)) == -1) {
		error("bind failed");
	}

	if (listen(listenSocket, 1) == -1) {
		error("listen failed");
	}
	printf("opened %s:%d\n", inet_ntoa(local.sin_addr), ntohs(local.sin_port));

	worker_init();
	while (TRUE) {
		struct sockaddr_in remote;
		socklen_t sockaddrLen = sizeof(remote);
		int clientSocket = accept(listenSocket, (struct sockaddr*)&remote, &sockaddrLen);
		if (clientSocket == -1) {
			error("accept failed");
		}

		printf("connected %s:%d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
		wsserver(clientSocket, 0, 0);
		printf("disconnected\n");
	}
	worker_cleanup();

	close(listenSocket);
	return EXIT_SUCCESS;
}

/*void server_cleanup(void *arg)
{
	context *pcontext = arg;
	int i;

	OPRINT("cleaning up ressources allocated by server thread #%02d\n", pcontext->id);

	for(i = 0; i < MAX_SD_LEN; i++)
		close(pcontext->sd[i]);
}
void *server_thread(void *arg)
{
	context *pcontext = arg;
	pglobal = pcontext->pglobal;

	// set cleanup handler to cleanup ressources
	pthread_cleanup_push(server_cleanup, pcontext);*/

	// create a child for every client that connects
	/*while (!pglobal->stop) {
		do {
			err = select(max_fds + 1, &selectfds, NULL, NULL, NULL);
		} while(err <= 0);

		for(i = 0; i < max_fds + 1; i++) {
			if(pcontext->sd[i] != -1 && FD_ISSET(pcontext->sd[i], &selectfds)) {
			pcfd->fd = accept(pcontext->sd[i], (struct sockaddr *)&client_addr, &addr_len);
			pcfd->pc = pcontext;
			if(pthread_create(&client, NULL, &client_thread, pcfd) != 0) {
				DBG("could not launch another client thread\n");
				close(pcfd->fd);
				free(pcfd);
				continue;
			}
			pthread_detach(client);
			}
		}
	}

	DBG("leaving server thread, calling cleanup function now\n");
	pthread_cleanup_pop(1);

	return NULL;
}*/

int main(int argc, char** argv)
{
	server_init();
	return EXIT_SUCCESS;
}
#endif
