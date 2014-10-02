#include <gpib/ib.h>

/* Standard buffer size */
#define MEAS_GPIB_BUF_SIZE 4096

/* controller parameters */
#define MEAS_GPIB_TIMEOUT 20
#define MEAS_GPIB_SENDEOI 1
#define MEAS_GPIB_EOS '\r'

/* In microsec */
#define MEAS_GPIB_DELAY 10

/* Prototypes */
int meas_gpib_open(int, int);
int meas_gpib_read(int, char *), meas_gpib_read_n(int, char *, int), meas_gpib_write(int, char *, int), meas_gpib_close(int, int);
int meas_gpib_cmd(int, char), meas_gpib_timeout(int, double), meas_gpib_clear(int), meas_gpib_old_read(int, int, char *, int), meas_gpib_old_write(int, int, char *, int), meas_gpib_set(int, int);
int meas_gpib_async_read_n(int, char *, int), meas_gpib_async_write(int, char *, int);

