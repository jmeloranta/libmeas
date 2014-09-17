    #include "stdio.h"
	#include "stdlib.h"
	#include "master.h"
	#include "pvcam.h"


    /* **** pl_get_param & pl_get_enum_param example. ***** */

    static void DisplayIntsFltsInfo (int16 hcam, uns32 param_id);
    static void DisplayEnumInfo (int16 hcam, uns32 param_id);
	void DisplayParamIdInfo (int16 hcam, uns32 param_id);
	boolean SetParamExample (int16 hcam, uns32 param_id, unsigned long value);

	main(int argc, char **argv)
	{
		short hCam;
		/* Initialize the PVCam Library */
		pl_pvcam_init();	
		pl_cam_open("rspipci0", &hCam, OPEN_EXCLUSIVE );

		
		printf( "\nAnti_Blooming\n");
       	DisplayParamIdInfo (hCam, PARAM_ANTI_BLOOMING);
		printf( "\nLogic Output\n");
		DisplayParamIdInfo (hCam, PARAM_LOGIC_OUTPUT);
		printf( "\nEdge Trigger\n");
      	DisplayParamIdInfo (hCam, PARAM_EDGE_TRIGGER);
		printf( "\nIntensifier Gain\n");
       	DisplayParamIdInfo (hCam, PARAM_INTENSIFIER_GAIN);
		printf( "\nGate Mode\n");
       	DisplayParamIdInfo (hCam, PARAM_SHTR_GATE_MODE);
		printf( "\nMin Block\n");
       	DisplayParamIdInfo (hCam, PARAM_MIN_BLOCK);
		printf( "\nNum Min Block\n");
       	DisplayParamIdInfo (hCam, PARAM_NUM_MIN_BLOCK);
		printf( "\nStrips Per Clean\n");
       	DisplayParamIdInfo (hCam, PARAM_NUM_OF_STRIPS_PER_CLR);
		printf( "\nReadout Port\n");
       	DisplayParamIdInfo (hCam, PARAM_READOUT_PORT);
		printf( "\nController Alive\n");
       	DisplayParamIdInfo (hCam, PARAM_CONTROLLER_ALIVE);
		printf( "\nReadout Time\n");
       	DisplayParamIdInfo (hCam, PARAM_READOUT_TIME);
		printf( "\nCircular Buffer Support\n");
       	DisplayParamIdInfo (hCam, PARAM_CIRC_BUFFER);


	pl_pvcam_uninit();	

	}

    /* This is an example that will display information we can get from parameter id.      */
    void DisplayParamIdInfo (int16 hcam, uns32 param_id)
    {
        boolean status, status2;    /* status of pvcam functions                           */
        boolean avail_flag;         /* ATTR_AVAIL, is param id available                   */
        uns16 access;               /* ATTR_ACCESS, get if param is read, write or exists. */
        uns16 type;                 /* ATTR_TYPE, get data type.                           */

        status = pl_get_param(hcam, param_id, ATTR_AVAIL, (void *)&avail_flag);
        /* check for errors */    
        if (status) {
            /* check to see if paramater id is supported by hardware, software or system.  */
            if (avail_flag) {
                /* we got a valid parameter, now get access writes and data type */
                status = pl_get_param(hcam, param_id, ATTR_ACCESS, (void *)&access);
                status2 = pl_get_param(hcam, param_id, ATTR_TYPE, (void *) &type);
                if (status && status2) {
                    if (access == ACC_EXIST_CHECK_ONLY) {
                        printf(" param id %ld exists\n", param_id);
                    }
                    else if ((access == ACC_READ_ONLY) || (access == ACC_READ_WRITE)) {
                        /* now we can start displaying information. */
                        /* handle enumerated types separate from other data */
                        if (type == TYPE_ENUM) {
                            DisplayEnumInfo(hcam, param_id);
                        }
                        else {  /* take care of he rest of the data types currently used */
                            DisplayIntsFltsInfo(hcam, param_id);
                        }
                    }
                    else {
                        printf(" error in access check for param_id %ld\n", param_id);
                    }
                }
                else { /* error occured calling function. */
                    printf( "functions failed pl_get_param, with error code %ld\n",
                            pl_error_code());
                }

            }
            else {  /* parameter id is not available with current setup */
                printf(" parameter %ld is not available with current hardware or software setup\n",
                        param_id);
            }
        }
        else { /* error occured calling function print out error and error code */
            printf( "functions failed pl_get_param, with error code %ld\n",
                    pl_error_code());
        }
    }               /* end of function DisplayParamIdInfo */

    /* this routine assumes the param id is an enumerated type, it will print out all the */
    /* enumerated values that are allowed with the param id and display the associated    */
    /* ASCII text.                                                                        */
    static void DisplayEnumInfo (int16 hcam, uns32 param_id)
    {
        boolean status;     /* status of pvcam functions.                */
        uns32 count, index; /* counters for enumerated types.            */
        char enumStr[100];  /* string for enum text.                     */
        int32 enumValue;    /* enum value returned for index & param id. */

        /* get number of enumerated values. */
        status = pl_get_param(hcam, param_id, ATTR_COUNT, (void *)&count);
        if (status) {
            printf(" enum values for param id %ld\n", param_id);
            for (index=0; index < count; index++) {
                /* get enum value and enum string */
                status = pl_get_enum_param(hcam, param_id, index, &enumValue,
                                           enumStr, 100);
                /* if everything alright print out the results. */
                if (status) {
                    printf(" index =%ld enum value = %ld, text = %s\n", 
                            index, enumValue, enumStr);
                }
                else {
                    printf( "functions failed pl_get_enum_param, with error code %ld\n",
                            pl_error_code());
                }
            }
        }
        else {
            printf( "functions failed pl_get_param, with error code %ld\n",
                     pl_error_code());
        }
    }               /* end of function DisplayEnumInfo */


    /* This routine displays all the information associated with the paramater id give.   */
    /* this routines assumes that the data is either uns8, uns16, uns32, int8, int16,     */
    /* int32, or flt64.                                                                   */
    static void DisplayIntsFltsInfo (int16 hcam, uns32 param_id)
    {
        /* current, min&max, & default values of parameter id */
        union {
            double  dval;
            unsigned long ulval;
            long lval;
            unsigned short usval;
            short sval;
            unsigned char ubval;
            signed char bval;
        } currentVal, minVal, maxVal, defaultVal, incrementVal; 
        uns16 type;                 /* data type of paramater id */
        boolean status, status2, status3, status4, status5; /* status of pvcam functions */

        /* get the data type of parameter id */
        status = pl_get_param(hcam, param_id, ATTR_TYPE, (void *)&type);
        /* get the default, current, min and max values for parameter id. */
        /* Note : since the data type for these depends on the parameter  */
        /* id you have to call pl_get_param with the correct data type    */
        /* passed for param_value.                                        */
        if (status) {
            switch (type) {
                case TYPE_INT8:
                    status = pl_get_param(hcam, param_id, ATTR_CURRENT, 
                                          (void *)&currentVal.bval);
                    status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT, 
                                          (void *)&defaultVal.bval);
                    status3 = pl_get_param(hcam, param_id, ATTR_MAX, 
                                          (void *)&maxVal.bval);
                    status4 = pl_get_param(hcam, param_id, ATTR_MIN, 
                                          (void *)&minVal.bval);
                    status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT, 
                                          (void *)&incrementVal.bval);
                    printf(" param id %ld\n", param_id);
                    printf(" current value = %c\n", currentVal.bval);
                    printf(" default value = %c\n", defaultVal.bval);
                    printf(" min = %c, max = %c\n", minVal.bval, maxVal.bval);
                    printf(" increment = %c\n", incrementVal.bval);
                    break;
                case TYPE_UNS8:
                    status = pl_get_param(hcam, param_id, ATTR_CURRENT, 
                                          (void *)&currentVal.ubval);
                    status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT, 
                                          (void *)&defaultVal.ubval);
                    status3 = pl_get_param(hcam, param_id, ATTR_MAX, 
                                          (void *)&maxVal.ubval);
                    status4 = pl_get_param(hcam, param_id, ATTR_MIN, 
                                          (void *)&minVal.ubval);
                    status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT, 
                                          (void *)&incrementVal.ubval);
                    printf(" param id %ld\n", param_id);
                    printf(" current value = %uc\n", currentVal.ubval);
                    printf(" default value = %uc\n", defaultVal.ubval);
                    printf(" min = %uc, max = %uc\n", minVal.ubval, maxVal.ubval);
                    printf(" increment = %uc\n", incrementVal.ubval);
                    break;
                case TYPE_INT16:
                    status = pl_get_param(hcam, param_id, ATTR_CURRENT, 
                                          (void *)&currentVal.sval);
                    status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT, 
                                          (void *)&defaultVal.sval);
                    status3 = pl_get_param(hcam, param_id, ATTR_MAX, 
                                          (void *)&maxVal.sval);
                    status4 = pl_get_param(hcam, param_id, ATTR_MIN, 
                                          (void *)&minVal.sval);
                    status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT, 
                                          (void *)&incrementVal.sval);
                    printf(" param id %ld\n", param_id);
                    printf(" current value = %i\n", currentVal.sval);
                    printf(" default value = %i\n", defaultVal.sval);
                    printf(" min = %i, max = %i\n", minVal.sval, maxVal.sval);
                    printf(" increment = %i\n", incrementVal.sval);
                    break;
                case TYPE_UNS16:
                    status = pl_get_param(hcam, param_id, ATTR_CURRENT, 
                                          (void *)&currentVal.usval);
                    status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT, 
                                          (void *)&defaultVal.usval);
                    status3 = pl_get_param(hcam, param_id, ATTR_MAX, 
                                          (void *)&maxVal.usval);
                    status4 = pl_get_param(hcam, param_id, ATTR_MIN, 
                                          (void *)&minVal.usval);
                    status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT, 
                                          (void *)&incrementVal.usval);
                    printf(" param id %ld\n", param_id);
                    printf(" current value = %u\n", currentVal.usval);
                    printf(" default value = %u\n", defaultVal.usval);
                    printf(" min = %uh, max = %u\n", minVal.usval, maxVal.usval);
                    printf(" increment = %u\n", incrementVal.usval);
                    break;
                case TYPE_INT32:
                    status = pl_get_param(hcam, param_id, ATTR_CURRENT, 
                                          (void *)&currentVal.lval);
                    status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT, 
                                          (void *)&defaultVal.lval);
                    status3 = pl_get_param(hcam, param_id, ATTR_MAX, 
                                          (void *)&maxVal.lval);
                    status4 = pl_get_param(hcam, param_id, ATTR_MIN, 
                                          (void *)&minVal.lval);
                    status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT, 
                                          (void *)&incrementVal.lval);
                    printf(" param id %ld\n", param_id);
                    printf(" current value = %ld\n", currentVal.lval);
                    printf(" default value = %ld\n", defaultVal.lval);
                    printf(" min = %ld, max = %ld\n", minVal.lval, maxVal.lval);
                    printf(" increment = %ld\n", incrementVal.lval);
                    break;
                case TYPE_UNS32:
                    status = pl_get_param(hcam, param_id, ATTR_CURRENT, 
                                          (void *)&currentVal.ulval);
                    status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT, 
                                          (void *)&defaultVal.ulval);
                    status3 = pl_get_param(hcam, param_id, ATTR_MAX, 
                                          (void *)&maxVal.ulval);
                    status4 = pl_get_param(hcam, param_id, ATTR_MIN, 
                                          (void *)&minVal.ulval);
                    status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT, 
                                          (void *)&incrementVal.ulval);
                    printf(" param id %ld\n", param_id);
                    printf(" current value = %ld\n", currentVal.ulval);
                    printf(" default value = %ld\n", defaultVal.ulval);
                    printf(" min = %ld, max = %ld\n", minVal.ulval, maxVal.ulval);
                    printf(" increment = %ld\n", incrementVal.ulval);
                    break;
                case TYPE_FLT64:
                    status = pl_get_param(hcam, param_id, ATTR_CURRENT, 
                                          (void *)&currentVal.dval);
                    status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT, 
                                          (void *)&defaultVal.dval);
                    status3 = pl_get_param(hcam, param_id, ATTR_MAX, 
                                          (void *)&maxVal.dval);
                    status4 = pl_get_param(hcam, param_id, ATTR_MIN, 
                                          (void *)&minVal.dval);
                    status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT, 
                                          (void *)&incrementVal.dval);
                    printf(" param id %ld\n", param_id);
                    printf(" current value = %g\n", currentVal.dval);
                    printf(" default value = %g\n", defaultVal.dval);
                    printf(" min = %g, max = %g\n", minVal.dval, maxVal.dval);
                    printf(" increment = %g\n", incrementVal.dval);
                    break;
                default:
                    printf(" data type not supported in this functions\n");
                    break;
            }
            if (!status || !status2 || !status3 || !status4 || !status5) {
                 printf( "functions failed pl_get_param, with error code %ld\n",
                         pl_error_code());
            }
        }
        else {
             printf( "functions failed pl_get_param, with error code %ld\n",
                      pl_error_code());
        }
        

    }               /* end of function DisplayIntsFltsInfo */


    /* **** pl_set_param example ***** */
    /* this example assumes data type to set is unsigned long. This routine */
    /* does do the error checks to make sure you can write to the param and */
    /* that it param id is a unsigned long */
    boolean SetParamExample (int16 hcam, uns32 param_id, unsigned long value)
    {
        boolean status;         /* status of pvcam functions */
        boolean avail_flag;         /* ATTR_AVAIL, is param id available                   */
        uns16 access;               /* ATTR_ACCESS, get if param is read, write or exists. */
        uns16 type;                 /* ATTR_TYPE, get data type.                           */

        status = pl_get_param(hcam, param_id, ATTR_AVAIL, (void *)&avail_flag);
        /* check for errors */    
        if (status) {
            /* check to see if paramater id is supported by hardware, software or system.  */
            if (avail_flag) {
                /* we got a valid parameter, now get access writes and data type */
                status = pl_get_param(hcam, param_id, ATTR_ACCESS, (void *)&access);
                if (status) {
                    if (access == ACC_EXIST_CHECK_ONLY) {
                        printf(" error param id %ld is an exists check, and not writable\n", 
                                param_id);
                    }
                    else if (access == ACC_READ_ONLY) {
                        printf(" error param id %ld is a read only variable, and not writable\n", 
                                param_id);
                    }
                    else if (access == ACC_READ_WRITE) {
                        /* we can set variable, lets be safe and check to make sure it is the */
                        /* right data type.                                                   */
                        status = pl_get_param(hcam, param_id, ATTR_TYPE, (void *) &type);
                        if (status) {
                            if (type == TYPE_UNS32) {
                                /* OK lets write to it. */
                                pl_set_param(hcam, param_id, (void *)&value);
                            }
                            else {
                                status = FALSE;
                            }
                        }
                        else {
                            printf( "functions failed pl_get_param, with error code %ld\n",
                                    pl_error_code());
                        }
                    }
                    else {
                        printf(" error in access check for param_id %ld\n", param_id);
                    }
                }
                else { /* error occured calling function. */
                    printf( "functions failed pl_get_param, with error code %ld\n",
                            pl_error_code());
                }

            }
            else {  /* parameter id is not available with current setup */
                printf(" parameter %ld is not available with current hardware or software setup\n",
                        param_id);
            }
        }
        else { /* error occured calling function print out error and error code */
            printf( "functions failed pl_get_param, with error code %ld\n",
                    pl_error_code());
        }

    return(status);
    }
