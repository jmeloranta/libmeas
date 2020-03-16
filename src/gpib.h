#ifdef GPIB

#include <gpib/ib.h>

/* Standard buffer size */
#define MEAS_GPIB_BUF_SIZE (3*65535)

/* controller parameters */
#define MEAS_GPIB_TIMEOUT 20
#define MEAS_GPIB_SENDEOI 1
#define MEAS_GPIB_EOS '\r'

/* In microsec */
#define MEAS_GPIB_DELAY 10

/* Maximum number of boards */
#define MEAS_GPIB_MAXBOARDS 5
#endif
