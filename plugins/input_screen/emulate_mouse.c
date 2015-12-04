/*=======================================================================
   マウスボタンのエミュレータ
               by So Kitsunezaki  1997 4/11
    		   * fj.sources の xhacker(by shinya okada) を参考にしました。
========================================================================*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>

Display *dpy;
void     error_exit(void);
void     move_mouse(int,int);
void     mouse_button(int, char *, char *);


main(int argc, char **argv)
{
    int n,dx,dy;
    char mod[64]="",type[64]="";

    if( argc > 1 ) strcpy(type,argv[1]); else error_exit();

    if( (dpy=XOpenDisplay( NULL ))==NULL )
    {  fprintf(stderr,"push_mouse: Display can't open.");  exit(1); }

	if(!strcmp(type,"Move")){
	   if( argc > 2 ) dx=atoi(argv[2]); else dx=0;
	   if( argc > 3 ) dy=atoi(argv[3]); else dy=0;
	   move_mouse(dx,dy);
	}else{
       if( argc > 2 ) n=atoi(argv[2]);      else n=1;
       if( argc > 3 ) strcpy(mod ,argv[3]); else strcpy(mod ,"None");
       mouse_button(n,type,mod);
    }
}


/************** マウスの移動 ********************/

void move_mouse(int dx,int dy)
{
Window       root,child;
int          x,y,wx,wy;
unsigned int key;

    /* マウス位置(x,y)の取得 */
    root=DefaultRootWindow( dpy );        /* ルートウィンドウの取得 */
    XQueryPointer(dpy,root,&root,&child,&x,&y,&wx,&wy,&key);

    XWarpPointer(dpy,None,root,0,0,0,0,x+dx,y+dy );
    XFlush( dpy );
}


/************** マウスイベントの発生 ********************/

void mouse_button(int n, char *type, char *mod)
{
Window       root,win,child;
unsigned int key;            /* 現在の修飾キーとマウスボタンの状態 */
int          x,y,wx,wy;      /* マウスの位置 */
int          revert_to;
XEvent       event1,event2;


    /*マウス状態の取得 */
    child=root=DefaultRootWindow( dpy );        /* ルートウィンドウの取得 */
    do{
       win=child;
       XQueryPointer(dpy,win,&root,&child,&x,&y,&wx,&wy,&key);
    }while(child); /* マウス位置に子ウィンドウを持たないウィンドウを探す。*/

    /* ディスプレイ、ウィンドウ、マウスの位置などの指定 */
    event1.xbutton.display=dpy;
    event1.xbutton.window=win;
    event1.xbutton.root=root;
    event1.xbutton.subwindow=None;
    event1.xbutton.x=wx;
    event1.xbutton.y=wy;
    event1.xbutton.x_root=x;
    event1.xbutton.y_root=y;
    event1.xbutton.same_screen=True;

    /* 現在押されている修飾キーのうちで無視するものを指定 */
    if     (!strcmp(mod,"Shift")  ){ key&=~ShiftMask;	}
    else if(!strcmp(mod,"Lock")   ){ key&=~LockMask;    }
    else if(!strcmp(mod,"Control")){ key&=~ControlMask; }
    else if(!strcmp(mod,"Mod1")   ){ key&=~Mod1Mask;    }
    else if(!strcmp(mod,"Mod2")   ){ key&=~Mod2Mask;    }
    else if(!strcmp(mod,"Mod3")   ){ key&=~Mod3Mask;    }
    else if(!strcmp(mod,"Mod4")   ){ key&=~Mod4Mask;    }
    else if(!strcmp(mod,"Mod5")   ){ key&=~Mod5Mask;    }

    /* マウスボタンの番号の指定 */
    if(n<1) n=1; else if(n>3) n=3;
     switch(n){
      case 1:
        event1.xbutton.state=key|Button1Mask;
    	event1.xbutton.button=Button1; break;
      case 2:
        event1.xbutton.state=key|Button2Mask;
    	event1.xbutton.button=Button2; break;
        case 3:
        event1.xbutton.state=key|Button3Mask;
    	event1.xbutton.button=Button3; break;
    }

    /* イベントの送信 */
    event2=event1;
    event1.type=ButtonPress;
    event2.type=ButtonRelease;

    if(strcmp(type,"Release")) /* type が Release でなければボタンを押す */
    	XSendEvent( dpy,win,True,ButtonPressMask,&event1 );

    if(strcmp(type,"Press"))   /* type が Press でなかったらボタンを離す */
        XSendEvent( dpy,win,True,ButtonReleaseMask,&event2 );

    XFlush(dpy);
}


/*************** エラーメッセージ ***********************/

void error_exit(){
  fprintf(stderr,
    	 "<使い方１> emulate_mouse Move [x] [y] \n"
		 " 現在の座標+(x,y) にマウスを移動する。(デフォルト x=y=0)\n\n"
		 "<使い方２> emulate_mouse type [n] [mod]\n"
         " n    = 1,2,3 : マウスボタンの番号(デフォルト n=1)\n"
         " type = Press,Release,Both: 押す／離す／押して離す(デフォルト)\n"
         " mod  = Shift,Lock,Control,Mod1～Mod5: \n"
   	     "          押されていても無視する修飾キーを指定する\n\n"
    	 "<例> fvwm などで Meta(Mod1)キー ＋ Control ＋ a で\n"
    	 "   emulate_mouse 1 Press Mod1 \n"
    	 "   が実行するようにしておけば Control + mouse 1 の代わりになる。\n\n"
    	 " ! xterm,kterm などに使う時は .Xdefaults などで allowSendEvents を\n"
         "   true にする必要がある（セキュリティーホールになるので注意）。\n\n"
		 );
  exit(1);
}
