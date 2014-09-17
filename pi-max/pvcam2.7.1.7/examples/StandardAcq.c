#include <stdio.h>
#include <stdlib.h>
#include "master.h"
#include "pvcam.h"

static void AcquireStandard( int16 hCam );

int main(int argc, char **argv)
{
    char cam_name[CAM_NAME_LEN];    /* camera name                    */
    int16 hCam;                     /* camera handle                  */

    /* Initialize the PVCam Library and Open the First Camera */
    pl_pvcam_init();
    pl_cam_get_name( 0, cam_name );
    pl_cam_open(cam_name, &hCam, OPEN_EXCLUSIVE );

    AcquireStandard( hCam );

    pl_cam_close( hCam );

    pl_pvcam_uninit();

    return 0;
}

void AcquireStandard( int16 hCam )
{
    rgn_type region = { 0, 511, 1, 0, 511, 1 };
    uns32 size;
    uns16 *frame;
    int16 status;
    uns32 not_needed;
    uns16 numberframes = 5;
    
    /* Init a sequence set the region, exposure mode and exposure time */
    pl_exp_init_seq();
    pl_exp_setup_seq( hCam, 1, 1, &region, TIMED_MODE, 100, &size );

    frame = (uns16*)malloc( size );

    /* Start the acquisition */
    printf( "Collecting %i Frames\n", numberframes );
    
    /* ACQUISITION LOOP */
    while( numberframes ) {
        pl_exp_start_seq(hCam, frame );

        /* wait for data or error */
        while( pl_exp_check_status( hCam, &status, &not_needed ) && 
               (status != READOUT_COMPLETE && status != READOUT_FAILED) );

        /* Check Error Codes */
        if( status == READOUT_FAILED ) {
            printf( "Data collection error: %i\n", pl_error_code() );
            break;
        }

        /* frame now contains valid data */
        printf( "Center Three Points: %i, %i, %i\n", 
                frame[size/sizeof(uns16)/2 - 1],
                frame[size/sizeof(uns16)/2],
                frame[size/sizeof(uns16)/2 + 1] );
        numberframes--;
        printf( "Remaining Frames %i\n", numberframes );
    } /* End while */

    /* Finish the sequence */
    pl_exp_finish_seq( hCam, frame, 0);
    
    /*Uninit the sequence */
    pl_exp_uninit_seq();

    free( frame );
}
