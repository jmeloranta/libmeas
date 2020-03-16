/*
 * Routine to find out endianess of the current system.
 * 
 */

/*
 * Return 1 for little endian (intel/amd; reversed order) and 0 for big endian (netowrk order; normal order).
 *
 */

EXPORT int meas_endian() {

  unsigned int i = 1;  

  char *c = (char*)&i;  
  if (*c) return 1;
  else return 0;
}

