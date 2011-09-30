/* line terminator (1 = CR LF, 0 = CR) */
#define MEAS_SR245_TERM 1

/* Prototypes */
int meas_sr245_init(int, int, int, char *), meas_sr245_ports(int, int), meas_sr245_write(int, int, double), meas_sr245_scan_read(int, int *, int, double *, int), meas_sr245_reset(int);
int meas_sr245_ttl_mode(int, int, int), meas_sr245_ttl_write(int, int, int), meas_sr245_ttl_trigger(int, int, long), meas_sr245_disable_trigger(int), meas_sr245_enable_trigger(int);
int meas_sr245_ttl_read(int, int);
double meas_sr245_read(int, int);
