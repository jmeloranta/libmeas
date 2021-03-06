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
  raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0;
  if (tcsetattr(ttyfd,TCSAFLUSH,&raw) < 0) {
    fprintf(stderr, "libmeas: cannot set tty to raw mode.\n");
    exit(1);
  }
  been_here = 1;
}

/*
 * Read one character from keyboard.
 * For async behavior, the tty needs to be in raw mode (see above).
 *
 * If no character is available returns 0 (NULL).
 *
 */

EXPORT char meas_tty_read() {

  char chr;
  
  if(read(ttyfd, &chr, 1) < 1) return 0;
  else return chr;
}
