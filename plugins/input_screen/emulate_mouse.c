/*=======================================================================
   �}�E�X�{�^���̃G�~�����[�^
               by So Kitsunezaki  1997 4/11
    		   * fj.sources �� xhacker(by shinya okada) ���Q�l�ɂ��܂����B
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


/************** �}�E�X�̈ړ� ********************/

void move_mouse(int dx,int dy)
{
Window       root,child;
int          x,y,wx,wy;
unsigned int key;

    /* �}�E�X�ʒu(x,y)�̎擾 */
    root=DefaultRootWindow( dpy );        /* ���[�g�E�B���h�E�̎擾 */
    XQueryPointer(dpy,root,&root,&child,&x,&y,&wx,&wy,&key);

    XWarpPointer(dpy,None,root,0,0,0,0,x+dx,y+dy );
    XFlush( dpy );
}


/************** �}�E�X�C�x���g�̔��� ********************/

void mouse_button(int n, char *type, char *mod)
{
Window       root,win,child;
unsigned int key;            /* ���݂̏C���L�[�ƃ}�E�X�{�^���̏�� */
int          x,y,wx,wy;      /* �}�E�X�̈ʒu */
int          revert_to;
XEvent       event1,event2;


    /*�}�E�X��Ԃ̎擾 */
    child=root=DefaultRootWindow( dpy );        /* ���[�g�E�B���h�E�̎擾 */
    do{
       win=child;
       XQueryPointer(dpy,win,&root,&child,&x,&y,&wx,&wy,&key);
    }while(child); /* �}�E�X�ʒu�Ɏq�E�B���h�E�������Ȃ��E�B���h�E��T���B*/

    /* �f�B�X�v���C�A�E�B���h�E�A�}�E�X�̈ʒu�Ȃǂ̎w�� */
    event1.xbutton.display=dpy;
    event1.xbutton.window=win;
    event1.xbutton.root=root;
    event1.xbutton.subwindow=None;
    event1.xbutton.x=wx;
    event1.xbutton.y=wy;
    event1.xbutton.x_root=x;
    event1.xbutton.y_root=y;
    event1.xbutton.same_screen=True;

    /* ���݉�����Ă���C���L�[�̂����Ŗ���������̂��w�� */
    if     (!strcmp(mod,"Shift")  ){ key&=~ShiftMask;	}
    else if(!strcmp(mod,"Lock")   ){ key&=~LockMask;    }
    else if(!strcmp(mod,"Control")){ key&=~ControlMask; }
    else if(!strcmp(mod,"Mod1")   ){ key&=~Mod1Mask;    }
    else if(!strcmp(mod,"Mod2")   ){ key&=~Mod2Mask;    }
    else if(!strcmp(mod,"Mod3")   ){ key&=~Mod3Mask;    }
    else if(!strcmp(mod,"Mod4")   ){ key&=~Mod4Mask;    }
    else if(!strcmp(mod,"Mod5")   ){ key&=~Mod5Mask;    }

    /* �}�E�X�{�^���̔ԍ��̎w�� */
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

    /* �C�x���g�̑��M */
    event2=event1;
    event1.type=ButtonPress;
    event2.type=ButtonRelease;

    if(strcmp(type,"Release")) /* type �� Release �łȂ���΃{�^�������� */
    	XSendEvent( dpy,win,True,ButtonPressMask,&event1 );

    if(strcmp(type,"Press"))   /* type �� Press �łȂ�������{�^���𗣂� */
        XSendEvent( dpy,win,True,ButtonReleaseMask,&event2 );

    XFlush(dpy);
}


/*************** �G���[���b�Z�[�W ***********************/

void error_exit(){
  fprintf(stderr,
    	 "<�g�����P> emulate_mouse Move [x] [y] \n"
		 " ���݂̍��W+(x,y) �Ƀ}�E�X���ړ�����B(�f�t�H���g x=y=0)\n\n"
		 "<�g�����Q> emulate_mouse type [n] [mod]\n"
         " n    = 1,2,3 : �}�E�X�{�^���̔ԍ�(�f�t�H���g n=1)\n"
         " type = Press,Release,Both: �����^�����^�����ė���(�f�t�H���g)\n"
         " mod  = Shift,Lock,Control,Mod1�`Mod5: \n"
   	     "          ������Ă��Ă���������C���L�[���w�肷��\n\n"
    	 "<��> fvwm �Ȃǂ� Meta(Mod1)�L�[ �{ Control �{ a ��\n"
    	 "   emulate_mouse 1 Press Mod1 \n"
    	 "   �����s����悤�ɂ��Ă����� Control + mouse 1 �̑���ɂȂ�B\n\n"
    	 " ! xterm,kterm �ȂǂɎg������ .Xdefaults �Ȃǂ� allowSendEvents ��\n"
         "   true �ɂ���K�v������i�Z�L�����e�B�[�z�[���ɂȂ�̂Œ��Ӂj�B\n\n"
		 );
  exit(1);
}
