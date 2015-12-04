//---------------------------------------------------------
//	Motion JPEG
//
//		©2013 Yuichiro Nakada
//---------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include <syslog.h>

#include "../../mjpg_streamer.h"
#include "../../utils.h"

#define INPUT_PLUGIN_NAME "SCREEN input plugin"

/* private functions and variables to this plugin */
static pthread_t   worker;
static globals     *pglobal;
static pthread_mutex_t controls_mutex;
static int plugin_no;

static int delay = 1000;
static int quality = 2;		// Motion JPEGのクオリティ

/*** plugin interface functions ***/

/******************************************************************************
Description.: this functions cleans up allocated ressources
Input Value.: arg is unused
Return Value: -
******************************************************************************/
void worker_cleanup(void *arg)
{
	static unsigned char first_run = 1;

	if (!first_run) {
		DBG("already cleaned up ressources\n");
		return;
	}

	first_run = 0;
	DBG("cleaning up ressources allocated by input thread\n");

	if (pglobal->in[plugin_no].buf != NULL) free(pglobal->in[plugin_no].buf);
}

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
void *worker_thread(void *arg)
{
	/* set cleanup handler to cleanup allocated ressources */
	pthread_cleanup_push(worker_cleanup, NULL);

//	Display* dpy;
//	int scr;
//	Window root;
	XWindowAttributes attributes;
//	int width, height;
	XImage *image;

	dpy = XOpenDisplay("");
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);

	XGetWindowAttributes(dpy, root, &attributes);
//	attributes.width = 1024;
//	attributes.height = 768;
	attributes.width = 800;
	attributes.height = 600;

	XShmSegmentInfo shminfo;
	image = XShmCreateImage(dpy,
		DefaultVisual(dpy, scr), DefaultDepth(dpy, scr),
		ZPixmap, NULL, &shminfo, attributes.width, attributes.height);
	shminfo.shmid = shmget(IPC_PRIVATE,
		image->bytes_per_line * image->height, IPC_CREAT|0777);
	//if (shminfo.shmid == -1) err
	shminfo.shmaddr = image->data = shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;

	while (!pglobal->stop) {
//		struct timeval t1, t2;
		//double elapsedTime;
//		gettimeofday(&t1, NULL); // start timer
		struct timespec t1, t2;
		clock_gettime(CLOCK_MONOTONIC, &t1);

		/*image = XGetImage(dpy, root,
			0, 0, attributes.width, attributes.height,
			AllPlanes, ZPixmap);*/
		if (!XShmAttach(dpy, &shminfo)) {}
		if (!XShmGetImage(dpy, root, image, 0, 0, AllPlanes)) {}

		/* copy JPG picture to global buffer */
		pthread_mutex_lock(&pglobal->in[plugin_no].db);
		pglobal->in[plugin_no].size = 0;
		if (image != NULL) {
#if 0
			if (image->bits_per_pixel == 32)
				//writeXImageToP3File(image, IMAGE_FILE_PATH);
				memcpy(pglobal->in[plugin_no].buf, image->data, /*image->bytes_per_line * image->height*/640*480*4);
			else
				fprintf(stderr, "Not Supported format : bits_per_pixel = %d\n", image->bits_per_pixel);
#endif
			//fprintf(stderr, "color %d, line %d/%d\n", image->bits_per_pixel, image->width, image->bytes_per_line/4);

			unsigned char *p = encode_image(image->data, pglobal->in[plugin_no].buf, /*2*/quality/*1-8*/, RGB32, attributes.width/*image->bytes_per_line*/, attributes.height);
			pglobal->in[plugin_no].size = p - pglobal->in[plugin_no].buf;

			/*FILE *fp = fopen("myimage.jpg", "w");
			fwrite (pglobal->in[plugin_no].buf, 1, p - pglobal->in[plugin_no].buf, fp);
			fclose (fp);*/

//			XFree(image);
		}

		XShmDetach(dpy, &shminfo);

		/* signal fresh_frame */
		pthread_cond_broadcast(&pglobal->in[plugin_no].db_update);
		pthread_mutex_unlock(&pglobal->in[plugin_no].db);

/*		gettimeofday(&t2, NULL); // stop timer after one full loop
		//int e = (t2.tv_usec - t1.tv_usec);
		int d = 1000*1000/20 - (t2.tv_usec - t1.tv_usec);
		fprintf(stderr, "%d %d\n", d, (t2.tv_usec - t1.tv_usec));
		if (d>20000 || d<0) d = 1000;
		//usleep(delay);*/

		clock_gettime(CLOCK_MONOTONIC, &t2);
		/*double*/int d = ((1000*1000*1000/20) - TIME_DIF_TO_NS(t1, t2))/1000.0;
		fprintf(stderr, "time:%d size:%dkB\n", d, pglobal->in[plugin_no].size/1024);
		if (d>0) usleep(d);
	}

	XDestroyImage(image);
	shmdt(shminfo.shmaddr);

	XCloseDisplay(dpy);

	IPRINT("leaving input thread, calling cleanup function now\n");
	pthread_cleanup_pop(1);

	return NULL;
}

/******************************************************************************
Description.: print help message
Input Value.: -
Return Value: -
******************************************************************************/
void help(void)
{
	fprintf(stderr, " ---------------------------------------------------------------\n" \
	" Help for input plugin..: "INPUT_PLUGIN_NAME"\n" \
	" ---------------------------------------------------------------\n" \
	" The following parameters can be passed to this plugin:\n\n" \
	" [-d | --delay ]........: delay to pause between frames\n" \
	" [-r | --resolution]....: can be 960x720, 640x480, 320x240, 160x120\n"
	" ---------------------------------------------------------------\n");
}

/******************************************************************************
Description.: parse input parameters
Input Value.: param contains the command line string and a pointer to globals
Return Value: 0 if everything is ok
******************************************************************************/
int input_init(input_parameter *param, int no)
{
	int i;
	plugin_no = no;

	if (pthread_mutex_init(&controls_mutex, NULL) != 0) {
		IPRINT("could not initialize mutex variable\n");
		exit(EXIT_FAILURE);
	}

	param->argv[0] = INPUT_PLUGIN_NAME;

	/* show all parameters for DBG purposes */
	for (i = 0; i < param->argc; i++) {
		DBG("argv[%d]=%s\n", i, param->argv[i]);
	}

    reset_getopt();
    while(1) {
        int option_index = 0, c = 0;
        static struct option long_options[] = {
            {"h", no_argument, 0, 0},
            {"help", no_argument, 0, 0},
            {"d", required_argument, 0, 0},
            {"delay", required_argument, 0, 0},
            {"r", required_argument, 0, 0},
            {"resolution", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        c = getopt_long_only(param->argc, param->argv, "", long_options, &option_index);

        /* no more options to parse */
        if(c == -1) break;

        /* unrecognized option */
        if(c == '?') {
            help();
            return 1;
        }

        switch(option_index) {
            /* h, help */
        case 0:
        case 1:
            DBG("case 0,1\n");
            help();
            return 1;
            break;

            /* d, delay */
        case 2:
        case 3:
            DBG("case 2,3\n");
            delay = atoi(optarg);
            break;

            /* r, resolution */
        /*case 4:
        case 5:
            DBG("case 4,5\n");
            for(i = 0; i < LENGTH_OF(picture_lookup); i++) {
                if(strcmp(picture_lookup[i].resolution, optarg) == 0) {
                    pics = &picture_lookup[i];
                    break;
                }
            }
            break;*/

        default:
            DBG("default case\n");
            help();
            return 1;
        }
    }

	pglobal = param->global;

	IPRINT("delay.............: %i\n", delay);
	//IPRINT("resolution........: %s\n", pics->resolution);

	// http://d.hatena.ne.jp/chiharunpo/20110809/1312901637
	// 他の X11 API 呼び出しよりも前に
	XInitThreads();

	return 0;
}

/******************************************************************************
Description.: stops the execution of the worker thread
Input Value.: -
Return Value: 0
******************************************************************************/
int input_stop(int id)
{
	DBG("will cancel input thread\n");
	pthread_cancel(worker);

	return 0;
}

/******************************************************************************
Description.: starts the worker thread and allocates memory
Input Value.: -
Return Value: 0
******************************************************************************/
int input_run(int id)
{
	pglobal->in[id].buf = malloc(/*256 * 1024*/640*480*4);
	if (pglobal->in[id].buf == NULL) {
		fprintf(stderr, "could not allocate memory\n");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&worker, 0, worker_thread, NULL) != 0) {
		free(pglobal->in[id].buf);
		fprintf(stderr, "could not start worker thread\n");
		exit(EXIT_FAILURE);
	}
	pthread_detach(worker);

	return 0;
}

// http://offog.org/darcs/misccode/sendxkey.c
/*void sendfocus(Window w, int in_out)
{
	XFocusChangeEvent focev;

	focev.display = dpy;
	focev.type = in_out;
	focev.window = w;
	focev.mode = NotifyNormal;
	focev.detail = NotifyPointer;
	XSendEvent(dpy, w, True, FocusChangeMask, (XEvent *) &focev);
	//XSync(dpy, True);

	return;
}*/

// マウスの移動
void move_mouse(int x, int y)
{
	//Window root;
	//root = DefaultRootWindow(dpy);	// ルートウィンドウの取得
//	XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
//	XFlush(dpy);
	XTestFakeMotionEvent(dpy, scr, x, y, CurrentTime);
}

// マウスイベントの発生
void mouse_button(int n, int type, char *mod)
{
#if 0
	Window       /*root,*/ win, child;
	unsigned int key;		// 現在の修飾キーとマウスボタンの状態
	int          x, y, wx, wy;      // マウスの位置
//	int          revert_to;
	XEvent       event1, event2;

	// マウス状態の取得
	child = root;// = DefaultRootWindow(dpy);	// ルートウィンドウの取得
	do {
		win = child;
		XQueryPointer(dpy, win, &root, &child, &x, &y, &wx, &wy, &key);
	} while (child);	// マウス位置に子ウィンドウを持たないウィンドウを探す

	// ディスプレイ、ウィンドウ、マウスの位置などの指定
	event1.xbutton.display = dpy;
	event1.xbutton.window = win;
	event1.xbutton.root = root;
	event1.xbutton.subwindow = None;
	event1.xbutton.x = wx;
	event1.xbutton.y = wy;
	event1.xbutton.x_root = x;
	event1.xbutton.y_root = y;
	event1.xbutton.same_screen = True;

	// 現在押されている修飾キーのうちで無視するものを指定
	if      (!strcmp(mod, "Shift")  ) { key&=~ShiftMask;   }
	else if (!strcmp(mod, "Lock")   ) { key&=~LockMask;    }
	else if (!strcmp(mod, "Control")) { key&=~ControlMask; }
	else if (!strcmp(mod, "Mod1")   ) { key&=~Mod1Mask;    }
	else if (!strcmp(mod, "Mod2")   ) { key&=~Mod2Mask;    }
	else if (!strcmp(mod, "Mod3")   ) { key&=~Mod3Mask;    }
	else if (!strcmp(mod, "Mod4")   ) { key&=~Mod4Mask;    }
	else if (!strcmp(mod, "Mod5")   ) { key&=~Mod5Mask;    }

	// マウスボタンの番号の指定
	if (n<1) n=1; else if(n>3) n=3;
	switch (n) {
	case 1:
		event1.xbutton.state = key|Button1Mask;
		event1.xbutton.button = Button1;
		break;
	case 2:
		event1.xbutton.state = key|Button2Mask;
		event1.xbutton.button = Button2;
		break;
	case 3:
		event1.xbutton.state = key|Button3Mask;
		event1.xbutton.button = Button3;
		break;
	}

	// イベントの送信
	event2 = event1;
	event1.type = ButtonPress;
	event2.type = ButtonRelease;

	// フォーカスを移す
	//XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
	//if (type & 1) sendfocus(win, FocusIn);

	if (type & 1) /* type が Release でなければボタンを押す */
		XSendEvent(dpy, win, True, ButtonPressMask, &event1);

	if (type & 2)   /* type が Press でなかったらボタンを離す */
		XSendEvent(dpy, win, True, ButtonReleaseMask, &event2);

	XFlush(dpy);
#endif
	if (type & 1) XTestFakeButtonEvent(dpy, n, True, CurrentTime);
	if (type & 2) XTestFakeButtonEvent(dpy, n, False, CurrentTime);
}

#include <X11/keysym.h>
// キーイベントの発生
void keyboard(int n)
{
#if 0
	Window       /*root,*/ win, child;
	unsigned int key;		// 現在の修飾キーとマウスボタンの状態
	int          x, y, wx, wy;      // マウスの位置
	XEvent       event1, event2;

	// マウス状態の取得
	child = root;// = DefaultRootWindow(dpy);	// ルートウィンドウの取得
	do {
		win = child;
		XQueryPointer(dpy, win, &root, &child, &x, &y, &wx, &wy, &key);
	} while (child);	// マウス位置に子ウィンドウを持たないウィンドウを探す

	// ディスプレイ、ウィンドウ、マウスの位置などの指定
	event1.xkey.display = dpy;
	event1.xkey.window = win;
	event1.xkey.root = root;
	event1.xkey.subwindow = None;
	event1.xkey.x = wx;
	event1.xkey.y = wy;
	event1.xkey.x_root = x;
	event1.xkey.y_root = y;
	event1.xkey.same_screen = True;

	event1.xkey.type = KeyPress;
	event1.xkey.keycode = XKeysymToKeycode(dpy, n);
	event1.xkey.state = key;
	XSendEvent(dpy, win, True, KeyPressMask, &event1);
	event2 = event1;
	event2.xkey.type = KeyRelease;
	XSendEvent(dpy, win, True, KeyPressMask, &event2);
	XFlush(dpy);
#endif
	// 大文字の時の処理
	if (n>=XK_A && n<=XK_Z) XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Shift_L), True, CurrentTime);

	// http://homepage3.nifty.com/tsato/xvkbd/events-j.html
	unsigned int key = XKeysymToKeycode(dpy, n);
	XTestFakeKeyEvent(dpy, key, True, CurrentTime);
	XTestFakeKeyEvent(dpy, key, False, CurrentTime);

	// 大文字の時の処理
	if (n>=XK_A && n<=XK_Z) XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Shift_L), False, CurrentTime);
}

void resolution(int sx, int sy)
{
	char s[256];
	snprintf(s, 256, "xrandr -s %dx%d", sx, sy);
	system(s);
}

// http://localhost:17743/?action=command&dest=0&plugin=0&id=0&group=1&value=0
// http://localhost:17743/?action=command&dest=0&plugin=0&id=2&group=1&value=16711935
int input_cmd(int plugin, unsigned int control_id, unsigned int group, int value)
{
	int res = 0;

	//DBG("Requested cmd (id: %d) for the %d plugin. Group: %d value: %d\n", control_id, plugin, group, value);
	fprintf(stderr, "Requested cmd (id: %d) for the %d plugin. Group: %d value: %d\n", control_id, plugin, group, value);
	switch (control_id) {
	case 0:	// 左クリック
		//fprintf(stderr, "Hello from input plugin\n");
		move_mouse((value>>16), value&0xffff);
		mouse_button(1, group, "");
		break;
	case 1:	// 中クリック
		move_mouse((value>>16), value&0xffff);
		mouse_button(2, group, "");
		break;
	case 2:	// 右クリック
		move_mouse((value>>16), value&0xffff);
		mouse_button(3, group, "");
		break;
	case 3:	// wheel
		break;
	case 4:	// move
		move_mouse((value>>16), value&0xffff);
		break;

	case 5:	// キーボード
		keyboard(value);
		break;

	case 6:	// 解像度
		resolution((value>>16), value&0xffff);
		break;
	case 7:	// RDP開始
		input_run(plugin_no);
		break;
	case 8:	// RDP停止
		input_stop(plugin_no);
		break;
	case 9:	// Motion JPEG quality
		quality = value;
		break;

	default:
		DBG("nothing matched\n");
		res = -1;
	}

	return res;
}
