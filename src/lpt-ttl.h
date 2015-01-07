/* Interface (direct I/O port access or ppdev interface) */
/* Note that the direct I/O might not work for PCI based parallel port cards. */
/* MEAS_LPT_DIRECTIO may require you to unload the kernel parallel port device drivers. */
/* #define MEAS_LPT_DIRECTIO 1 /**/

#define MEAS_LPT_LPT1 0
#define MEAS_LPT_LPT2 1

#define MEAS_LPT_BUF_SIZE 4096
