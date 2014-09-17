#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

struct ImageWindow
{
	Display *display;
	int screen;
	Visual *visual;
	XImage *ximage;
	unsigned int width;
	unsigned int height;
	unsigned int extdepth;
	unsigned int depth;
	unsigned int bytedepth;
	Window win;
	GC gc;
	XWindowAttributes attrs;
	XEvent report;
	int shift;
	char *displaybuf;
};

void	createWindow(	char *name, 
			 	int Width, 
			 	int Height, 
			 	int Pixeldepth, 
			 	struct ImageWindow *window);
			 	
int	paintWindow( struct ImageWindow *window,
			 char *imagedata);
			 
void	destroywindow(struct ImageWindow *window);

int GreyLut[65536];

int NumGreys;
