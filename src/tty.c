/*
 * TTY handling functions (single character input etc.).
 *
 */

#include "tty.h"

static struct termios orig_termios;
static int ttyfd = STDIN_FILENO, been_here = 0;

/*
 * Switch tty back to normal mode.
 *
 */

EXPORT void meas_tty_normal() {

  if(!been_here) {
    fprintf(stderr, "libmeas: meas_tty_raw() not called - ignoring switch to normal.\n");
    return;
  }
  tcsetattr(ttyfd,TCSAFLUSH,&orig_termios);
}

/*
 * Switch tty into raw mode.
 *
 */

EXPORT void meas_tty_raw() {

  struct termios raw;

  if (!isatty(ttyfd)) {
    fprintf(stderr, "libmeas: stdin is not a tty.\n");
    exit(1);
  }

  if (tcgetattr(ttyfd,&orig_termios) < 0) {
    fprintf(stderr, "libmeas: Cannot get original tty settings.\n");
    exit(1);
  }

  if (atexit(meas_tty_normal) != 0) {
    fprintf(stderr, "libmeas: Cannot register tty reset.\n");
    exit(1);
  }

  raw = orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 5; raw.c_cc[VTIME] = 8;
  raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0;
  raw.c_cc[VMIN] = 2; raw.c_cc[VTIME] = 0;
  raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 8;
  if (tcsetattr(ttyfd,TCSAFLUSH,&raw) < 0) fatal("can't set raw mode");

  been_here = 1;
}
