/*
 * File: misc.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "misc.h"

static uid_t real_uid, effective_uid;
static int been_here_root = 0;
static struct timeval reference = {0, 0};

/* Enable root privs - requires setuid root */
void meas_misc_root_on() {

  if(!been_here_root) {
    real_uid = getuid();
    effective_uid = geteuid();
    been_here_root = 1;
  }
  seteuid(effective_uid);
}

/* Disable root privs */
void meas_misc_root_off() {

  if(!been_here_root) {
    real_uid = getuid();
    effective_uid = geteuid();
    been_here_root = 1;
  }
  seteuid(real_uid);
}

/* nanosecond resolution sleep function */
void meas_misc_nsleep(time_t sec, long nsec) {

  struct timespec sl;

  sl.tv_sec = sec;
  sl.tv_nsec = nsec;
  nanosleep(&sl, NULL); /* we are ignoring possible early wake ups from signals */
}

static struct timespec tp;

/* set timer */
void meas_misc_set_time() {

  clock_gettime(CLOCK_MONOTONIC, &tp);
}

/* sleep for the time = sleep time - (current time - set timer) */
void meas_misc_sleep_rest(time_t sec, long nsec) {

  struct timespec cur, sl;

  clock_gettime(CLOCK_MONOTONIC, &cur);
  sl.tv_sec = sec - (cur.tv_sec - tp.tv_sec);
  sl.tv_nsec = nsec - (cur.tv_nsec - tp.tv_nsec);
  if(sl.tv_sec < 0) sl.tv_sec = 0; /* went over... */
  if(sl.tv_nsec < 0) sl.tv_nsec = 0; /* went over... */
  if(sl.tv_sec == 0 && sl.tv_nsec == 0) {
    fprintf(stderr, "Warning: time resolution not accurate!\n");
    return;
  }
  nanosleep(&sl, NULL); /* we are ignoring possible early wake ups from signals */  
}

/* disable signals */
void meas_misc_disable_signals() {

  signal(SIGINT, SIG_IGN);
}

/* enable signals */
void meas_misc_enable_signals() {

  signal(SIGINT, SIG_DFL);
}

/* set time reference */
void meas_misc_set_reftime() {

  (void) gettimeofday(&reference, NULL);
}

/* return time in seconds vs. reference */
double meas_misc_get_reftime() {

  struct timeval cur;
  double ti;

  if(!reference.tv_sec && !reference.tv_usec)
    meas_err("misc: meas_misc_get_reftime() called without meas_misc_set_reftime().");
  (void) gettimeofday(&cur, NULL);
  ti = ((double) (cur.tv_sec - reference.tv_sec)) + 1E-6 * ((double) (cur.tv_usec - reference.tv_usec));
  return ti;
}

