#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "aldl-types.h"
#include "useful.h"
#include "error.h"

/************ SCOPE *********************************
  Useful but generic functions.
****************************************************/

timespec_t get_time() {
  timespec_t currenttime;
  #ifdef USEFUL_BETTERCLOCK
  clock_gettime(CLOCK_MONOTONIC,&currenttime);
  #else
  gettimeofday(&currenttime,NULL);
  #endif
  return currenttime;
}

unsigned long get_elapsed_ms(timespec_t timestamp) {
  timespec_t currenttime = get_time();
  unsigned long seconds = currenttime.tv_sec - timestamp.tv_sec;
  #ifdef USEFUL_BETTERCLOCK
  unsigned long milliseconds =(currenttime.tv_nsec-timestamp.tv_nsec) / 1000000;
  #else
  unsigned long milliseconds =(currenttime.tv_usec-timestamp.tv_usec) / 1000;
  #endif
  return ( seconds * 1000 ) + milliseconds;
}

byte checksum_generate(byte *buf, int len) {
  #ifdef RETARDED
  retardptr(buf,"checksum buf");
  #endif
  int x = 0;
  unsigned int sum = 0;
  for(x=0;x<len;x++) sum += buf[x];
  return ( 256 - ( sum % 256 ) );
}

int checksum_test(byte *buf, int len) {
  int x = 0;
  unsigned int sum = 0;
  for(x=0;x<len;x++) sum += buf[x];
  if(( sum & 0xFF ) == 0) return 1;
  return 0;
}

int cmp_bytestring(byte *h, int hsize, byte *n, int nsize) {
  if(nsize > hsize) return 0; /* needle is larger than haystack */
  if(hsize < 1 || nsize < 1) return 0;
  int cursor = 0; /* haystack compare cursor */
  int matched = 0; /* needle compare cursor */
  while(cursor <= hsize) {
    if(nsize == matched) return 1;
    if(h[cursor] != n[matched]) { /* reset match */
      matched = 0;
    } else {
      matched++;
    }
    cursor++;
  }
  return 0;
}

void printhexstring(byte *str, int length) {
  int x;
  for(x=0;x<length;x++) printf("%X ",(unsigned int)str[x]);
  printf("\n");
}

#ifdef MALLOC_ERRCHECK
void *smalloc(size_t size) {
  void *m = malloc(size); 
  if(m == NULL) {
   fprintf(stderr, "Out of memory trying to alloc %u bytes",(unsigned int)size);
   exit(1);
  }
  return m;
}
#endif

