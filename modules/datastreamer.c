#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

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
long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/100; // calculate 10s milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}


int write_def(aldl_conf_t *aldl,datastreamer_conf_t *conf) {
    int zero = 0;
    int size = sizeof(aldl_define_t);
    int len;
  fwrite(&aldl->n_defs,sizeof(int),1,conf->fdesc);
  fwrite(&size,sizeof(int),1,conf->fdesc);
  for(int i = 0; i < aldl->n_defs; i++) {
      aldl_define_t def = aldl->def[i];
      //fputc()
      if(def.name != NULL)  {
          len = strlen(def.name);
          fwrite(&len,sizeof(int),1,conf->fdesc);
          fputs(def.name,conf->fdesc);
      }
      else
          fwrite(&zero,sizeof(int),1,conf->fdesc);
      if(def.uom != NULL)  {
          len = strlen(def.uom);
          fwrite(&len,sizeof(int),1,conf->fdesc);
        fputs(def.uom,conf->fdesc);
      }
      else
          fwrite(&zero,sizeof(int),1,conf->fdesc);
      //fputs('\0',conf->fdesc);
      if(def.description != NULL) {
          len = strlen(def.description);
          fwrite(&len,sizeof(int),1,conf->fdesc);
        fputs(def.description,conf->fdesc);
      }
      else
          fwrite(&zero,sizeof(int),1,conf->fdesc);
    fwrite(&def, sizeof(def), 1, conf->fdesc);
    fputs("|\n",conf->fdesc);
  }
}

datastreamer_conf_t *datastreamer_load_config(aldl_conf_t *aldl);
const char * SOF =  "STREAMDEFINITIONS:";
void *datastreamer_init(void *aldl_in) {
  unsigned int n_records = 0; /* number of record counter */
  unsigned long last_timestamp = 0;
//  int x = 0; /* tmp */
  float pps; /* packet per second rate */
  aldl_conf_t *aldl = (aldl_conf_t *)aldl_in;

  /* grab config data */
  datastreamer_conf_t *conf = datastreamer_load_config(aldl);
  /* wait for buffered connection */
  pause_until_buffered(aldl);

  /* create logfile */
  datastreamer_make_file(conf,aldl);
  
  int len = 0;
  if(aldl->datastreamer_definitions == 1) {
    len = strlen(SOF);
    fwrite(&len,sizeof(int),1,conf->fdesc);
    fputs(SOF,conf->fdesc);
    write_def(aldl,conf);
	fflush(conf->fdesc);
  }
  if(aldl->datastreamer_messages){
	  aldl_record_t *rec = aldl->r;
	  int dataSize = sizeof(aldl_data_t);
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
		fwrite(&rec->t, sizeof(unsigned long), 1, conf->fdesc);
		fwrite(&aldl->n_defs,sizeof(int),1,conf->fdesc);
		fwrite(&dataSize,sizeof(int),1,conf->fdesc);
		fwrite(rec->data,sizeof(aldl_data_t), aldl->n_defs,conf->fdesc);
		fputs("|\n",conf->fdesc);
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

