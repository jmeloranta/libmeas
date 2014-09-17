#include <stdio.h>
#include <stdlib.h>
#include "master.h"
#include "pvcam.h"

/* Prototype functions */
static rs_bool SetParamExample (int16 hcam, uns32 param_id, int16 value);

int main(int argc, char **argv)
{
    char cam_name[CAM_NAME_LEN];    /* camera name   */
    int16 hCam;                     /* camera handle */

    /* Initialize the PVCam Library and Open the First Camera */
    pl_pvcam_init();    
    pl_cam_get_name( 0, cam_name );
    pl_cam_open(cam_name, &hCam, OPEN_EXCLUSIVE );

    /* Change the min skip block and number of min blocks to 2 and 100 */
    SetParamExample(hCam, PARAM_MIN_BLOCK, 2);
    SetParamExample(hCam, PARAM_NUM_MIN_BLOCK, 100);

    pl_cam_close( hCam );

    pl_pvcam_uninit();    

    return 0;
}


rs_bool SetParamExample (int16 hcam, uns32 param_id, int16 value)
{
    rs_bool status;     /* status of pvcam functions                   */
    rs_bool avail_flag; /* ATTR_AVAIL, param is available              */
    uns16 access;       /* ATTR_ACCESS, param is read, write or exists */
    uns16 type;         /* ATTR_TYPE, param data type                  */

    status = pl_get_param(hcam, param_id, ATTR_AVAIL, (void *)&avail_flag);
    /* check for errors */    
    if (status) {
        /* check to see if parameter id is supported by hardware or software */
        if (avail_flag) {
            /* we got a valid parameter, now get access rights and data type */
            status = pl_get_param(hcam, param_id, ATTR_ACCESS, (void *)&access);
            if (status) {
                if (access == ACC_EXIST_CHECK_ONLY) {
                    printf(" error param id %x is an exists check, "
                           "and not writable\n", param_id);
                }
                else if (access == ACC_READ_ONLY) {
                    printf(" error param id %x is a readonly variable, "
                           "and not writeable\n", param_id);
                }
                else if (access == ACC_READ_WRITE) {
                    /* we can set it, let's be safe and check to make sure
                       it is the right data type */
                    status = pl_get_param(hcam, param_id, ATTR_TYPE, 
                                          (void *) &type);
                    if (status) {
                        if (type == TYPE_INT16) {
                            /* OK lets write to it */
                            pl_set_param(hcam, param_id, (void *)&value);
                            printf( "param %x set to %i\n", param_id, value );
                        }
                        else {
                            printf( "data type mismatch for param_id "
                                    "%x\n", param_id );
                            status = FALSE;
                        }
                    }
                    else {
                        printf( "functions failed pl_get_param, with "
                                "error code %ld\n", pl_error_code());
                    }
                }
                else {
                    printf(" error in access check for param_id "
                           "%x\n", param_id);
                }
            }
            else { /* error occurred calling function */
                printf( "functions failed pl_get_param, with error code %ld\n",
                        pl_error_code());
            }

        }
        else {  /* parameter id is not available with current setup */
            printf(" parameter %x is not available with current hardware or "
                   "software setup\n", param_id);
        }
    }
    else { /* error occurred calling function; print out error code */
        printf( "functions failed pl_get_param, with error code %ld\n",
                pl_error_code());
    }

    return(status);
}
