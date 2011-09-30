#define MEAS_B9600   0
#define MEAS_B19200  1
#define MEAS_B57600  2
#define MEAS_B115200 3

#define MEAS_SERIAL_EOS '\r'

/* Prototypes */
int meas_rs232_open(char *, int);
int meas_rs232_close(int);
int meas_rs232_read(int, char *, int), meas_rs232_write(int, char *, int), meas_rs232_writenl(int, char *), meas_rs232_writeb(int, unsigned char);
