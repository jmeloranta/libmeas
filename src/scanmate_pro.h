/* Sometimes ack from the dye laser fails. Continue after 30s */
#define MEAS_SCANMATE_PRO_SKIP_HANDSHAKE 30

/* stepper motors */
#define MEAS_SCANMATE_PRO_GRATING 1
#define MEAS_SCANMATE_PRO_ETALON  2
#define MEAS_SCANMATE_PRO_SHG     3
#define MEAS_SCANMATE_PRO_SFG     4

/* Prototypes */
int meas_scanmate_pro_init(int, char *), meas_scanmate_pro_setwl(int, double), meas_scanmate_pro_grating(int, int), meas_scanmate_pro_getstp(int, int), meas_scanmate_pro_setstp(int, int, unsigned int);
double meas_scanmate_pro_getwl(int);
