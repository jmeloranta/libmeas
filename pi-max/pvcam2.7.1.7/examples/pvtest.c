#include <stdio.h>
#include <stdlib.h>

#include "master.h"
#include "pvcam.h"
#include "complxw.h"




void OnAcquireContinuous(	short hcam,
				struct ImageWindow *window,
				unsigned short *buffer,
				long size,
				int numbershots);
				 
void OnFocusContinuous( 	short hcam,
				struct ImageWindow *window,
				unsigned short *buffer, 
				long size, 
				int numbershots);
				
void OnSnapSingleShot(  	short hcam,
				struct ImageWindow *window,
			 	unsigned short *data,   
			 	long size, 
			 	int number );

			
void OnFocusAll();

#define NUM_CONTROLLERS 1
int NumCams = NUM_CONTROLLERS;
short hCam[NUM_CONTROLLERS];
struct ImageWindow window[NUM_CONTROLLERS];
unsigned short *data[NUM_CONTROLLERS];
unsigned char *names[NUM_CONTROLLERS] = 
{
	"rspipci0"
};

int x1,x2,xb,y1,y2,yb,exposure;

/************************************************************************
*
*	Parse number out of string in the format "Value=32"
*
************************************************************************/
int GetNumber( char * string)
{
	char val[32];
	int i = 0;
	int j = 0;

	for ( i=0; i<32;i++)
		val[i] = 0;

	i = 0;
	while( string[i] != '=' )
		i++;
	/* one more to push past = */
	i++;

	while ( string[i] != 0 ) {
		val[j] = string[i];
         	j++; i++;
	}
	
	return atoi( val );
}
/************************************************************************
*
*    GetValues From Ini File and set them into global variables...
*
************************************************************************/
void GetValues( void )
{
	FILE *fp;
	char Line[32];
        int i;

	for ( i=0; i<32;i++)
		Line[i] = 0;
	
	/* Open the file */
	fp = fopen("pvtest.ini", "r");

	if ( fp )
	{
		/* While not end of file find out whats in it and set values up */
        while ( fscanf( fp, "%s\n", Line) != EOF )
		{
            if ( strstr( Line, "XStart")  )
				x1 = GetNumber( Line );
			if ( strstr( Line, "XEnd")  )
				x2 = GetNumber( Line );
			if ( strstr( Line, "XBin") )
				xb = GetNumber( Line );
			if ( strstr( Line, "YStart")   )
				y1 = GetNumber( Line );
			if ( strstr( Line, "YEnd")  )
				y2 = GetNumber( Line );
			if ( strstr( Line, "YBin")  )
				yb = GetNumber( Line );
			if ( strstr( Line, "Exposure")   )
				exposure = GetNumber( Line );

			/* Make sure buffer is clear for next round */
			for ( i=0; i<32;i++)
				Line[i] = 0;

		}
		fclose(fp);
	}
	else
	{
		x1 = 0; x2 = 255; xb = 1;
		y1 = 0; y2 = 255; yb = 1;
		exposure = 100;
	}
}


/************************************************************************
*
*    main - Initialize the library, read the settings from file, create
*   		displays, allocate memory and do some running tests.
*	
*
************************************************************************/
main( int argc, char **argv)
{
	
	rgn_type region;
	unsigned short x, y;
	int i;
	long imagebytes;
	boolean circbuffer_flag, status;

	/* Initialize the PVCam Library */
	pl_pvcam_init();	

	GetValues();

	for ( i=0;i<NUM_CONTROLLERS;i++)
		pl_cam_open( names[i], &hCam[i], OPEN_EXCLUSIVE );

	if (pl_error_code() != 0 ) {
		printf( "Camera Not Found, Or Communication Error\n");
		exit(0);
	}
	
	for ( i=0;i<NUM_CONTROLLERS;i++)
	{	
		
		/* Find The Sensor Dimensions */
		pl_ccd_get_par_size(hCam[i], &y);
		pl_ccd_get_ser_size(hCam[i], &x);

		/* Make sure its valid region */
		if ( x2 > x ) x2 = x;
		if ( y2 > y ) y2 = y;
		
		data[i] = (unsigned short *)malloc( ((x2-x1+1)/xb)*((y2-y1+1)/yb)*2 );

		/* Force Open Shutter All ways */
//		pl_shtr_set_close_dly(hCam[i], 24);
//		pl_shtr_set_open_mode(hCam[i], OPEN_PRE_SEQUENCE);
	}	


	for ( i=0;i<NUM_CONTROLLERS;i++)
	{
		/* Can it do Circular Buffer Acquistions */
    	status = pl_get_param( hCam[i], PARAM_CIRC_BUFFER, ATTR_AVAIL, (void *)&circbuffer_flag );
	
		if ( !status || !circbuffer_flag )
		{			
			printf( "Sorry, your controller does not support circular buffers\n");
			exit(0);
		}
	}

	
	/* Create Image Windows */	
	for ( i=0;i<NUM_CONTROLLERS;i++)
		createWindow( names[i], ((x2-x1+1)/xb), ((y2-y1+1)/yb), 12, &window[i]);


	/* Calculate buffer size */
	imagebytes = ((x2-x1+1)/xb)*((y2-y1+1)/yb)*2;

	for ( i=0;i<NUM_CONTROLLERS;i++) {
		/* Acquire x Frames asynchronously */
		printf( "Testing Snap Mode\n" );
		OnSnapSingleShot( hCam[i], &window[i], data[i], imagebytes, 10 );
	}



	for ( i=0;i<NUM_CONTROLLERS;i++) {
		/* Fast Focus For x Frames */
		printf( "Testing High Speed Focus Mode\n");
		if ( strstr( names[i], "rspipci" ) )
			OnFocusContinuous( hCam[i], &window[i], data[i], imagebytes, 10 );
	}
	
	for ( i=0;i<NUM_CONTROLLERS;i++) {
		/* High Speed Acquire x frames synch no skipping */
		printf( "Testing High Speed Acquire Mode\n");
		if ( strstr( names[i], "rspipci" ) )
			OnAcquireContinuous( hCam[i], &window[i], data[i], imagebytes, 10 );
	}

	
	printf( "Continuing In Free Run Focus Mode\n");
	OnFocusAll();


	for ( i=0;i<NUM_CONTROLLERS;i++)
		destroywindow(&window[i]);



}


/************************************************************************
*
*
*
************************************************************************/
void OnAcquireContinuous(	short hcam,
				struct ImageWindow *windowx,
				 unsigned short *buffer, 
				 long size, 
				 int numbershots)
{
	rgn_type region;
	long bytes;
	int code;
	short status = 0;
	void *address;
		
	region.s1 	= x1 -1;	region.p1 	= y1 - 1;
	region.s2 	= x2 -1;	region.p2 	= y2 - 1;
	region.sbin 	= xb;		region.pbin 	= yb;
	
	pl_exp_init_seq();
	pl_exp_setup_seq( hcam, 1, 1, &region, TIMED_MODE, exposure, &size );

	pl_exp_set_cont_mode(hcam, CIRC_NO_OVERWRITE );
	pl_exp_start_cont(hcam, buffer, size );

	/* ACQUISITION LOOP */
	while( numbershots ){
	
		if ( pl_exp_get_oldest_frame( hcam, &address )) {
			numbershots--;
			paintWindow( windowx, (char *)address);
			printf( "Remaining Frames %i\n", numbershots );
			pl_exp_unlock_oldest_frame(hcam);
		}
		
		code = pl_error_code();
		if (( code == 3033 ) || (code == 3034)) {
			printf("Error\n");
			break;
		}
	}
	
	pl_exp_finish_seq( hcam, buffer, 0);
	pl_exp_uninit_seq();
	pl_exp_stop_cont(hcam,0);

}

/************************************************************************
*
*
*
************************************************************************/
void OnFocusContinuous( 	short hcam,
				struct ImageWindow *windowx,
				unsigned short *buffer, 
				long size, 
				int numbershots)
{
	rgn_type region;
	long bytes;
	int code;
	short status = 0;
	void *address;
	
	region.s1 	= x1 -1;	region.p1 	= y1 - 1;
	region.s2 	= x2 -1;	region.p2 	= y2 - 1;
	region.sbin 	= xb;		region.pbin 	= yb;

	pl_exp_init_seq();
	pl_exp_setup_seq( hcam, 1, 1, &region, TIMED_MODE, exposure, &size );

	pl_exp_set_cont_mode(hcam, CIRC_OVERWRITE );

	pl_exp_start_cont(hcam, buffer, size );
	
	/* ACQUISITION LOOP */
	while( numbershots ){
	
		if ( pl_exp_get_latest_frame( hcam, &address )) {
		    paintWindow( windowx, (char *)address);
			numbershots--;
			printf( "Remaining Frames %i\n", numbershots );
		}
		
		code = pl_error_code();
		if (( code == 3033 ) || (code == 3034)) {
			printf("Error\n");
			break;
		}
	}
	
	pl_exp_finish_seq( hcam, buffer, 0);
	pl_exp_uninit_seq();
	pl_exp_stop_cont(hcam,0);
	
}

/************************************************************************
*
*
*
************************************************************************/
void OnSnapSingleShot(		short hcam,
				struct ImageWindow *windowx,
				unsigned short *buffer, 
				long size, 
				int numbershots)
{
	rgn_type region;
	long bytes;
	short status = 0;
	int i;

	region.s1 	= x1 -1;	region.p1 	= y1 - 1;
	region.s2 	= x2 -1;	region.p2 	= y2 - 1;
	region.sbin 	= xb;		region.pbin 	= yb;

	pl_exp_init_seq();
	pl_exp_setup_seq( hcam, 1, 1, &region, TIMED_MODE, exposure, &size );
	
	/* ACQUISITION LOOP */
	while ( numbershots ){
		
		pl_exp_start_seq( hcam, buffer);
	
		while ( status != READOUT_COMPLETE ){
			
			pl_exp_check_status( hcam, &status, &bytes );
		
		}
		
		paintWindow( windowx, (char *)buffer);
		printf( "Remaining Frames %i\n",numbershots );		
		numbershots--;
		status = 0;
	}
	
	pl_exp_finish_seq( hcam, buffer, 0);
	pl_exp_uninit_seq();
}


/************************************************************************
*
*
*
************************************************************************/
void OnFocusAll( )
{
	rgn_type region;
	long bytes;
	int code;
	short status = 0;
	void *address;
	int done_flag = FALSE;
	int i;
	long size;
	
	region.s1 	= x1 -1;	region.p1 	= y1 - 1;
	region.s2 	= x2 -1;	region.p2 	= y2 - 1;
	region.sbin = xb;		region.pbin 	= yb;


	pl_exp_init_seq();
	
	
	for (i=0; i<NUM_CONTROLLERS; i++)
	{	
		pl_exp_setup_seq( hCam[i], 1, 1, &region, TIMED_MODE, exposure, &size );
		pl_exp_set_cont_mode(hCam[i], CIRC_OVERWRITE );
		pl_exp_start_cont(hCam[i], data[i], size );
	}
		
	/* ACQUISITION LOOP */
	while( !done_flag )
	{
	
		i = (i+1)%NUM_CONTROLLERS;
				
	
		if ( pl_exp_get_latest_frame( hCam[i], &address )) {

			done_flag = paintWindow( &window[i], (char *)address);	

		}
		
		code = pl_error_code();
		if (( code == 3033 ) || (code == 3034)) {
			printf("Error\n");
			break;
		}
	
	}
	

	for (i=0; i<NUM_CONTROLLERS; i++)
	{	
		pl_exp_finish_seq( hCam[i], data[i], 0);		
		pl_exp_stop_cont(hCam[i],0);		
	}
	pl_exp_uninit_seq();
}





