/*
 * Data structures for the pulse generator module.
 *
 */

#define MEAS_PULSE_CH1	7
#define MEAS_PULSE_CH2	29
#define MEAS_PULSE_CH3	31
#define MEAS_PULSE_CH4	32
#define MEAS_PULSE_CH5	33
#define MEAS_PULSE_CH6	36
#define MEAS_PULSE_CH7	11
#define MEAS_PULSE_CH8	12
#define MEAS_PULSE_CH9	35
#define MEAS_PULSE_CH10	38
#define MEAS_PULSE_CH11	40
#define MEAS_PULSE_CH12	41
#define MEAS_PULSE_CH13	16
#define MEAS_PULSE_CH14	18
#define MEAS_PULSE_CH15	22
#define MEAS_PULSE_CH16	37
#define MEAS_PULSE_CH17	13

struct meas_pulse {
  char channel;        /* channel number */
  unsigned int length; /* Pulse length in microseconds */
  unsigned int delay;  /* Delay after the pulse in microseconds */
  char polarity;       /* 0 = active high (normal), 1 = active low (inverted) */
};

