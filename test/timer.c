/*
 Copyright (c) 2016 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

static int timer_close(void * user __attribute__((unused))) {
  done = 1;
  return 1;
}

static int timer_read(void * user __attribute__((unused))) {
  /*
   * Returning a non-zero value makes gpoll return, allowing to check the 'done' variable.
   */
  return 1;
}
