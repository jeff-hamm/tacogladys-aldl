#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/* local objects */
#include "../error.h"
#include "../aldl-io.h"
#include "../loadconfig.h"
#include "../useful.h"

typedef struct _datastreamer_conf {
  dfile_t *dconf; /* raw config data */
  int autostart;
  char *log_filename;
  int log_all;
  int sync;
  int rate;
  int skip;
  int marker;
  FILE *fdesc;
} datastreamer_conf_t;

int streamer_be_quiet(aldl_conf_t *aldl);

void datastreamer_make_file(datastreamer_conf_t *conf,aldl_conf_t *aldl);

datastreamer_conf_t *datastreamer_load_config(aldl_conf_t *aldl);
const char * SOF =  "STREAMDEFINITIONS:";
void *datastreamer_init(void *aldl_in) {
  unsigned int n_records = 0; /* number of record counter */
  unsigned long last_timestamp = 0;
	printf("Starting\n");
	fflush(stdout);
//  int x = 0; /* tmp */
  float pps; /* packet per second rate */
  aldl_conf_t *aldl = (aldl_conf_t *)aldl_in;

  /* grab config data */
  datastreamer_conf_t *conf = datastreamer_load_config(aldl);
	printf("Config\n");
	fflush(stdout);
  /* wait for buffered connection */
  pause_until_buffered(aldl);
	printf("Connection\n");
	fflush(stdout);

  /* create logfile */
  datastreamer_make_file(conf,aldl);
  	printf("Writing\n");fflush(stdout);
  fprintf(stdout,SOF,"%s");
  fwrite(&aldl->n_defs,sizeof(long),1,conf->fdesc);
  fwrite(aldl->def, sizeof(aldl_define_t), aldl->n_defs, conf->fdesc);
  	printf("Wrote\n");fflush(stdout);
  
  aldl_record_t *rec = aldl->r;

  /* event loop */
  while(1) {
    if(conf->skip == 1) {
      rec = newest_record_wait(aldl,rec);
    } else {
      rec = next_record_wait(aldl,rec);
    }
    if(rec == NULL) {
      if(streamer_be_quiet(aldl) == 0) {
        fprintf(stderr,"datastreamer: Connection state: %s.  Waiting for connection...\n",
                get_state_string(get_connstate(aldl)));
      }
      pause_until_connected(aldl);
      if(streamer_be_quiet(aldl) == 0) {
        fprintf(stderr,"datastreamer: Reconnected.  Resuming logging...\n");
      } 
      continue;
    }
    if(last_timestamp + conf->rate >= rec->t) continue; /* skip record */
    fwrite(&aldl->n_defs,sizeof(long),1,conf->fdesc);
    fwrite(&rec->t, sizeof(long), 1, conf->fdesc);
    fwrite(rec->data,sizeof(aldl_data_t), aldl->n_defs,conf->fdesc);
    if(conf->sync == 1) fflush(conf->fdesc);
    if(streamer_be_quiet(aldl) == 0) {
      n_records++;
      if(n_records % 300 == 0) {
        lock_stats();
        pps = aldl->stats->packetspersecond;
        unlock_stats();
//        fprintf(stderr,"datastreamer: Logged %u pkts @ %.2f/sec\n",n_records,pps);
      }
    }
    last_timestamp = rec->t; /* update timestamp */
  }

  free(conf);
  /* end ... */
  return NULL;
}

void datastreamer_make_file(datastreamer_conf_t *conf,aldl_conf_t *aldl) {
    conf->fdesc = stdout;
  if(conf->fdesc == NULL) error(1,ERROR_PLUGIN,"cannot append to log");

  /* print hello string if consoleif is disabled */
  if(streamer_be_quiet(aldl) == 0) {
    fprintf(stderr,"datastreamer: Logging data to file: %s\n","stdout");
  }
}

datastreamer_conf_t *datastreamer_load_config(aldl_conf_t *aldl) {
  datastreamer_conf_t *conf = smalloc(sizeof(datastreamer_conf_t));
  if(aldl->datastreamer_config == NULL) error(1,ERROR_CONFIG,
                               "no datastreamer config file specified");
  conf->dconf = dfile_load(aldl->datastreamer_config);
  if(conf->dconf == NULL) error(1,ERROR_CONFIG,
                                  "datastreamer config file missing");
  dfile_t *config = conf->dconf;
  conf->autostart = configopt_int(config,"AUTOSTART",0,1,1);
  conf->log_all = configopt_int(config,"LOG_ALL",0,1,0);
  conf->log_filename = configopt_fatal(config,"LOG_FILENAME");
  conf->sync = configopt_int(config,"SYNC",0,1,1);
  conf->skip = configopt_int(config,"SKIP",0,1,1);
  conf->marker = configopt_int(config,"MARKER",0,10000,100);
  conf->rate = configopt_int(config,"RATE",1,10000,1);
  return conf;
}

int streamer_be_quiet(aldl_conf_t *aldl) {
  if(aldl->consoleif_enable == 1) return 1;
  return 0;
}

