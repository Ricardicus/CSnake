#if 0
Snake, an old time classic game. 
Consult readme if you can't compile this. 
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/* Method declarations */
void init_x(void);
void close_x(void);
void draw(void);
void move(void);
void remove_tail(void);
void place_food(void);
void *timer_event_handle(void*);
void *event_looping(void*);
void *timer_loop(void*);

/* Type declarations */
typedef void (*timer_func)(void);

// Shared GUI data
Display *dis;
int screen;
Window win;
GC gc;

// Shared game data
static int width, height, nbr_boxes,snake_x,snake_y;
static XColor red,green,white;
int board[16][16];
int up = 1;
int down = 2;
int left = 3;
int right = 4;
int food = -1;
int empty = 0;
int dir;

/* Initialisation of the program */
void init_x() {
	/* get the colors black and white (see section for details) */
	unsigned long black,whiteP;

	/* the nbr of boxes are set */
	nbr_boxes = 16;

	/* setting initial height and width */
	width = 16*30;
	height = 16*30;

	/* Status checking varaible declared */
	Status rc;

	for(int x = 0; x<nbr_boxes;x++)
	{
		for(int y = 0; y<nbr_boxes;y++)
		{
			board[x][y] = 0;
		}
	}

	/* starting position */
	snake_x=nbr_boxes/2;
	snake_y=nbr_boxes/2;
	board[snake_y][snake_x] = 1;

	/* staring direction */
	dir = 2;

	/*q placing some food */
	place_food();

	/* use the information from the environment variable DISPLAY 
	   to create the X connection:
	*/	
	dis=XOpenDisplay((char *)0);
   	screen=DefaultScreen(dis);
	black=BlackPixel(dis,screen),	/* get color black */
	whiteP=WhitePixel(dis, screen);  /* get color white */

	/* once the display is initialized, create the window.
	   This window will be have be 200 pixels across and 300 down.
	   It will have the foreground white and background black
	*/
   	win=XCreateSimpleWindow(dis,DefaultRootWindow(dis),0,0,	
		width, height, 5, whiteP, whiteP);

	/* here is where some properties of the window can be set.
	   The third and fourth items indicate the name which appears
	   at the top of the window and the name of the minimized window
	   respectively.
	*/
	XSetStandardProperties(dis,win,"My Window","HI!",None,NULL,0,NULL);

	/* this routine determines which types of input are allowed in
	   the input.  see the appropriate section for details...
	*/
	XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask|StructureNotifyMask);

	/* create the Graphics Context */
    gc=XCreateGC(dis, win, 0,0);        

	/* here is another routine to set the foreground and background
	   colors _currently_ in use in the window.
	*/
	XSetBackground(dis,gc,whiteP);
	XSetForeground(dis,gc,black);

	/* sets the fillstyle for the GC to be 'solid filling'. */
	XSetFillStyle(dis,gc,FillSolid);

	/* clear the window and bring it on top of the other windows */
	XClearWindow(dis, win);
	XMapRaised(dis, win);

	/* Allocating space for colors */
	Colormap screen_colormap = DefaultColormap(dis, DefaultScreen(dis));

	rc = XAllocNamedColor(dis, screen_colormap, "red", &red, &red);
	if (rc == 0) {
    fprintf(stderr, "XAllocNamedColor - failed to allocated 'red' color.\n");
    exit(1);
  	}

	rc = XAllocNamedColor(dis, screen_colormap, "green", &green, &green);
	if (rc == 0) {
    fprintf(stderr, "XAllocNamedColor - failed to allocated 'red' color.\n");
    exit(1);
  	}

	rc = XAllocNamedColor(dis, screen_colormap, "white", &white, &white);
	if (rc == 0) {
    fprintf(stderr, "XAllocNamedColor - failed to allocated 'red' color.\n");
    exit(1);
  	}

}

/* returns system resources to the system */
void close_x() {
	XFreeGC(dis, gc);
	XDestroyWindow(dis,win);
	XCloseDisplay(dis);	
	exit(1);				
}

/* draws the game board on screen */

void draw()
{
	XSetForeground(dis,gc,white.pixel);
	XFillRectangle(dis, win, gc, 0,0,width,height);
	int blockwidth = width/nbr_boxes;
	int blockheight = height/nbr_boxes;
	for(int x = 0; x<nbr_boxes;x++)
	{
		for(int y = 0; y<nbr_boxes;y++)
		{
			if(board[x][y] == empty)
			{
				XSetForeground(dis,gc,white.pixel);
				XFillRectangle(dis, win, gc, x*blockwidth,y*blockheight,blockwidth,blockheight);
				XFlush(dis);
			}
			else if(board[x][y] == food)
			{
				XSetForeground(dis,gc,green.pixel);
				XFillRectangle(dis, win, gc, x*blockwidth,y*blockheight,blockwidth,blockheight);
				XFlush(dis);
			}
			else if(board[x][y] > 0)
			{
				XSetForeground(dis,gc,red.pixel);
				XFillRectangle(dis,win,gc,x*blockwidth,y*blockheight,blockwidth,blockheight);
				XFlush(dis);
			}
		}
	}
}

/* used in the algorithm of the snake game to not increase the length of the snake while its moving */
void remove_tail()
{
	for(int x = 0; x<nbr_boxes;x++)
	{
		for(int y = 0; y<nbr_boxes;y++)
		{
			if(board[x][y] > 0)
			{
				board[x][y] = board[x][y] - 1;
			}
		}
	}

}

/* an event listener function */
void *event_listener()
{
	XEvent event;
	KeySym key;
	char text[255];

	Status rc;

	/* look for events */
	while(1) {		
		/* get the next event and stuff it into the event variable. */
		XNextEvent(dis, &event);
	
		if (event.type==Expose && event.xexpose.count==0) {
		/* the window was exposed and will be redrawn */
			draw();
		}
		if (event.type==KeyPress&&
		    XLookupString(&event.xkey,text,255,&key,0)==1) {
		/* the XLookupString routine converts the invent
		   KeyPress data into regular text.  Weird but necessary.
		*/
			if (text[0]=='q') {
				close_x();
				pthread_exit(NULL);
			}
			else if(text[0] == 'w' && dir != down)
			{
				dir = up;
			}
			else if(text[0] == 'a' && dir != right)
			{
				dir = left;
			}
			else if(text[0] == 'd' && dir != left)
			{
				dir = right;
			}
			else if(text[0] == 's' && dir != up)
			{
				dir = down;
			}
		}		
		/* if (event.type==ButtonPress) {
			printf("Button pressed at (%i,%i)\n",
				event.xbutton.x,event.xbutton.y);
			XSetForeground(dis,gc,white.pixel);
			XFillRectangle(dis, win, gc, 0,0,width,height);
			XSetForeground(dis,gc,red.pixel);
			XFillRectangle(dis, win, gc, event.xbutton.x,event.xbutton.y, event.xbutton.x + 10,event.xbutton.y + 10);
		}
		*/		
        else if (event.type == ConfigureNotify) {
            XConfigureEvent xce = event.xconfigure;
            /* This event type is generated for a variety of
               happenings, so check whether the window has been
               resized. */
            if (xce.width != width ||
                xce.height != height) {
                width = xce.width;
               	height = xce.height;
               	printf("new width: %d\nnew height: %d\n",width,height);
            }
        }
	}
	return NULL;
}

/* moves the snake and checks for collisions */
void move()
{
	int old_snake_x = snake_x;
	int old_snake_y = snake_y;
	if(dir == up)
	{
		snake_y = (nbr_boxes+snake_y-1)%nbr_boxes;
	} 
	else if( dir == down)
	{
		snake_y = (snake_y+1)%nbr_boxes;
	}
	else if( dir == left)
	{
		snake_x = (nbr_boxes+snake_x-1)%nbr_boxes;
	}
	else if( dir == right)
	{
		snake_x = (snake_x+1)%nbr_boxes;
	}
	if(board[snake_x][snake_y] == food) /* Found food! */
	{
		board[snake_x][snake_y] = board[old_snake_x][old_snake_y] + 1;
		place_food();
	}
	else if(board[snake_x][snake_y]>0)
	{
		close_x();
		exit(-1);
	}
	else 
	{
		board[snake_x][snake_y] = board[old_snake_x][old_snake_y] + 1;
		remove_tail();
	}
}

/* Placing some on the board */
void place_food()
{
	int food_x = rand()%nbr_boxes;
	int food_y = rand()%nbr_boxes;

	while(board[food_x][food_y]!=empty)
	{
		food_x = rand()%nbr_boxes;
		food_y = rand()%nbr_boxes;
	}

	board[food_x][food_y] = food;
}

/* function invoked by the timer loop */
void* timer_event_handle(void * todo)
{
	static int action = 0;
	int move_draw_ratio = 4;

	if(action==0)
	{
		move();
		draw();
	} else {
		draw();
	}
	action=(action+1)%move_draw_ratio;
	return NULL;
}

/* the timer loop! Constitutes along with the event listener loop pretty much the heart of the game loop */
void * timer_loop(void* tf)
{
	unsigned long event_trigger = 10000000;
  	unsigned long event_counter = 0;

  	timer_func tf_f = (timer_func) tf;

  	while(1)
  	{
  		event_counter++;
  		if(event_counter==event_trigger)
  		{
  			event_counter=0;
  			tf_f();
  		}
  	}
  	pthread_exit(NULL);
}

/* main method of the game */
int main(void)
{
	int rc;

/* Initialises the GUI */
	init_x();

/* prepating the X window client for multi-threading */
  	XInitThreads();

/* Threads declaration */
	pthread_t event_loop;
	pthread_t timer;

    rc = pthread_create(&event_loop, NULL, event_listener,NULL);
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    rc = pthread_create(&timer, NULL, timer_loop,timer_event_handle);
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    pthread_exit(NULL);
    close_x();
	return 0;
}