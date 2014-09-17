
#include "complxw.h"
#include <string.h>
#include <stdio.h>

int GreyVals[65536];
void findgrays(Display *xdisplay);

int DisplayDepth = 0;

void word22byte_8bit(  unsigned short *wbuf, unsigned char *bbuf, int count );
void word22byte_16bit( unsigned short *wbuf, unsigned char *bbuf, int count );
void word22byte_24bit( unsigned short *wbuf, unsigned char *bbuf, int count );

void createWindow(  char *windowname,
			int Width, 
			int Height, 
			int Pixeldepth, 
			struct ImageWindow *image_win)
{
    unsigned int display_width, display_height;
    char *app_name = windowname;
    XVisualInfo myVis;
    XSizeHints size_hints;
    int i;
    int loop ; 
    int loopcount ;
    unsigned long valuemask = 0; 
    XGCValues values;
    char *display_name = NULL;
    Region region;		
    u_short oXdim, oYdim, oZdim, oZedim;
    unsigned int x = 0, y = 0; 		
    int	fd;
    int retval;
    unsigned int border_width = 4;	/* four pixels */
    char *argv[1] = {"NONAME"};
    int	argc = 1;
	int *supdeps;
	int countd;
    int allcolors = 0;
    u_char timeout = 0;

    /* connect to X server */
    if ( (image_win->display=XOpenDisplay(display_name)) == NULL )
    {
	(void) fprintf( stderr, "%s: cannot connect to X server %s\n",
				    app_name, XDisplayName(display_name));
	exit( -1 );
    }
    image_win->screen 	= DefaultScreen(image_win->display);
    image_win->visual 	= DefaultVisual(image_win->display, image_win->screen);
    display_width 		= DisplayWidth(image_win->display, image_win->screen);
    display_height 		= DisplayHeight(image_win->display, image_win->screen);

	supdeps = XListDepths( image_win->display, image_win->screen, &countd);
	if ( countd == 0 ){
     	printf( "Can't Use Display Mode\n");
		exit( 0 );
	}
	else {		
		for (i=0;i<countd; i++) {
			if ( supdeps[i] == 16 )
			{	
				DisplayDepth = 16;
			}
		}
		if (DisplayDepth != 16 )
	    		DisplayDepth = supdeps[countd - 1];
	}

    image_win->width  	= Width;
    image_win->height 	= Height;
    image_win->depth 	= Pixeldepth;
    image_win->bytedepth= (Pixeldepth +7) /8;

   /* size window with enough room for image if possible */
    if (image_win->width > (display_width-32))
		image_win->width = display_width - 32;

    if (image_win->height > (display_height-32))
		image_win->height = display_height - 32;

 
    image_win->win = XCreateSimpleWindow(image_win->display, RootWindow(image_win->display,image_win->screen),
			      x, y, image_win->width, image_win->height,
			      border_width, BlackPixel(image_win->display, image_win->screen),
			      WhitePixel(image_win->display,image_win->screen));

    /* Set resize hints */
    size_hints.flags = PPosition | PSize | USSize | USPosition ;
    size_hints.x = x;
    size_hints.y = y;
    size_hints.width = size_hints.min_width = size_hints.max_width = image_win->width;
    size_hints.height =size_hints.min_height =size_hints.max_height = image_win->height;

    /* set Properties for window manager (always before mapping) */
    XSetStandardProperties(image_win->display, image_win->win, app_name, NULL, 
	0L, argv, argc, &size_hints);


    /* Select event types wanted */
    XSelectInput(image_win->display, image_win->win, ExposureMask | KeyPressMask | ButtonPressMask);

    /* create region for exposure event processing */
    region = XCreateRegion();

    /* Create default Graphics Context */
    image_win->gc = XCreateGC(image_win->display, image_win->win, valuemask, &values);

    /* Display window */
    XMapWindow(image_win->display, image_win->win);

    /* get window attrs for copy */
    XGetWindowAttributes(image_win->display, image_win->win, &image_win->attrs);

	/* Search System for its grays and sort em */
	if (DisplayDepth == 8)
		findgrays( image_win->display);
	
  	switch ( DisplayDepth )
	{
		case 8:
			image_win->displaybuf = (char *)calloc((int)image_win->width, (int)image_win->height);
			image_win->ximage = XCreateImage(image_win->display,
									 image_win->visual,
					 				 8,
									 ZPixmap,
									 0,
									 image_win->displaybuf,
					    			 image_win->width,
									 image_win->height,
									 8,
									 0);
		break;
     	case 16:
			image_win->displaybuf = (char *)calloc((int)image_win->width*2, (int)image_win->height);
			image_win->ximage = XCreateImage(image_win->display,
									 image_win->visual,
					 				 16,
									 ZPixmap,
									 0,
									 image_win->displaybuf,
					    			 image_win->width,
									 image_win->height,
									 16,
									 image_win->width*2);
		break;

		case 24:
			image_win->displaybuf = (char *)calloc((int)image_win->width*4, (int)image_win->height);
			image_win->ximage = XCreateImage(image_win->display,
									 image_win->visual,
					 				 24,
									 ZPixmap,
									 0,
									 image_win->displaybuf,
					    			 image_win->width,
									 image_win->height,
									 32,
									 image_win->width*4);

		break;		
	}

    /* wait for expose event */
    XNextEvent(image_win->display, &image_win->report);
    XFlush(image_win->display) ;

}

int paintWindow(struct ImageWindow *image_win, char *imagedata)
{
	static int do_update = 1;

	if (!do_update)
	{
		printf ("image display paused\n");
            	XPeekEvent(image_win->display, &image_win->report);
		printf ("continuing image display \n");
	}

    if (XPending(image_win->display))
   	{
    	XNextEvent(image_win->display, &image_win->report);

		if (image_win->report.type == KeyPress)
	    {
			switch (image_win->report.xkey.keycode)
			{
				case 37:
				case 38:
				case 39:
				case 40:
				case 41:
				case 42:
				case 43:
				case 44:
				default:
					printf ("keycode id %d %c (exiting)\n",
										image_win->report.xkey.keycode,
										(char)image_win->report.xkey.keycode);
				return 1;
				break;
			}
	    }
	}

	switch ( DisplayDepth )
	{
		case 8:
		/*note hard code # bytes = 2 below */
			word22byte_8bit(	(ushort *)imagedata,
								(unsigned char *)image_win->displaybuf,
								image_win->width * image_win->height * image_win->bytedepth);
		break;
		case 16:
		/*note hard code # bytes = 2 below */
			word22byte_16bit(	(ushort *)imagedata,
								(unsigned char *)image_win->displaybuf,
								image_win->width * image_win->height * image_win->bytedepth);
		break;
		case 24:
		/*note hard code # bytes = 2 below */
			word22byte_24bit(	(ushort *)imagedata,
								(unsigned char *)image_win->displaybuf,
								image_win->width * image_win->height * image_win->bytedepth);
		break;
	}

	image_win->ximage->data = image_win->displaybuf;

	XPutImage(	image_win->display,
				image_win->win,
				image_win->gc,
				image_win->ximage,
				0,0, 0,0,
				image_win->attrs.width,
				image_win->attrs.height);
			
	XFlush(image_win->display);

    return 0;
}

void destroywindow(struct ImageWindow *image_win)
{
    XFreeGC(image_win->display, image_win->gc);
    XCloseDisplay(image_win->display);
}



/*****************************************************************************************
*
*
*
*****************************************************************************************/
void word22byte_24bit( unsigned short *wbuf, unsigned char *bbuf, int count )
{
	register unsigned short *from, *end;
	register unsigned char *to;
	register unsigned short val;
	int xval;
	long min, max;
	long div;
	int i;

	min = 65536;
	max = 0;

	from = wbuf;
	for ( i = 0; i < count/2; i++ )
	{
    	if ( *from < min )
			min = *from;
		if ( *from > max )
			max = *from;
		from++;
	}
	from = wbuf;

	div = (max - min)/256;
	if ( div == 0 )
		div = 1;
	
	end = (unsigned short *)((char *)from + count);
	to = bbuf;
	
	while (from < end )
	{
		xval = (*from-min)/div;
		*to++ = xval;
		*to++ = xval;
		*to++ = xval;
		to++; from++;

	}
}


/*****************************************************************************************
*
*
*
*****************************************************************************************/
void word22byte_16bit( unsigned short *wbuf, unsigned char *bbuf, int count )
{
	register unsigned short *from, *end;
	register unsigned char *to;
	register unsigned short val;
	int xval;
	unsigned short magic;
	unsigned short min, max;
	int i;
	float factor;
	int counter = 0;
    static int x = 0;
	from = wbuf;
	
	end = (unsigned short *)((char *)from + count);
	to = bbuf;
	
	min = 4096;
	max = 0;
	
	from = wbuf;
	for ( i = 0; i < count/2; i++ )
	{
    	if ( *from < min )
			min = *from;
		if ( *from > max )
			max = *from;
		from++;
	}
	from = wbuf;
	
	while (from < end )
	{
		xval = 4096/16;
		*from = *from/(xval+1);
		xval = *from;
		
		magic = xval;
		magic |= xval << 5;
		magic |= xval << 10;
		*to++ = (magic >> 8) & 0x00FF;
		*to++ = magic & 0x00FF;
		from++;

	}

}



/*****************************************************************************************
*
*
*
*****************************************************************************************/
void word22byte_8bit( unsigned short *wbuf, unsigned char *bbuf, int count )
{
	register unsigned short *from, *end;
	register unsigned char *to;
	register unsigned short val;
	unsigned int min, max;
	int i;
	

	float factor;
	int counter = 0;
	
	min = 65536;
	max = 0;
	
	from = wbuf;
	for ( i = 0; i < count/2; i++ )
	{
    	if ( *from < min )
			min = *from;
		if ( *from > max )
			max = *from;
		from++;
	}
	from = wbuf;
	
	end = (unsigned short *)((char *)from + count);
	to = bbuf;
	
	factor = ( (float)(NumGreys - 1)/((float)max- (float)min) );
	while (from < end )
	{
		val = (unsigned short)((float)(*from)*factor);
		*to++ = (unsigned char)GreyLut[val];
		from++;

		counter++;
	}
}

/********************************************************************************
*
*	Sort Out the Grays From the System
*
*******************************************************************************/
void sortgrays()
{
	int i,j,k;
    long lowest = 65537;
	int Replicate[65536];
	int index;

	for ( i=0; i<65536; i++ )
		Replicate[i] = GreyLut[i];

	for ( j=0; j < NumGreys; j++)
	{
		for ( i=0; i < NumGreys; i++)
		{
     		if ( GreyVals[i] < lowest ) {
				lowest = GreyVals[i];
				index  = i;
			}
		}
		GreyVals[index] = 65537;
		lowest = 65537;
		GreyLut[j] = Replicate[index];

    }
	
}

#define MAXCOLORS 256
/********************************************************************************
*
*	Interrogate the system for its grey scale values...
*
*******************************************************************************/
void findgrays(Display *xdisplay)
{	
    int scr ;
    unsigned long pixels[MAXCOLORS], pmask[1], pixel[1];
    int         i,j;
    int         base;
	unsigned long cells[32];
    Colormap cmap;
	int status;
	Visual *vis;
    int numbertoscan;
	XColor color;

    scr  = DefaultScreen(xdisplay);
	cmap = DefaultColormap(xdisplay, scr);
	vis  = DefaultVisual(xdisplay, scr);
    j 	 = 0;

	switch ( DisplayDepth)
	{
     	case 8:
			numbertoscan = 256;
			break;
		case 16:
			numbertoscan = 65536;
			break;
		default:
			numbertoscan = 256;
			break;
	}
	
	for ( i = 0; i < numbertoscan; i++ )
	{
		color.pixel = i;
		XQueryColor( xdisplay, cmap, &color );
   		if ( ( color.red == color.blue) &&
			 ( color.green == color.red))
		{
			   GreyVals[j] = color.red;
			   GreyLut[j] = i;
			   j++;
				NumGreys = j;
				
		}			
	}	
	sortgrays();	
}


