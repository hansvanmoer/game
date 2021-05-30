/*
 * This file is part of game.
 *
 * game is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *    game is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with game.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "settings.h"
#include "status.h"

#include <assert.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

static const char * verbosity_args[] = {"debug", "info", "warning", "error"};

void log_program_settings(const struct program_settings * settings){
  assert(settings != NULL);
  LOG_INFO("program settings:");
  LOG_INFO("server %s", settings->server ? "enabled" : "disabled");
  LOG_INFO("client %s", settings->client ? "enabled" : "disabled");
  LOG_INFO("interrupt %s", !settings->daemon ? "enabled" : "disabled");  
  LOG_INFO("verbosity: %s", verbosity_args[(int)settings->log_priority]);
}

static int parse_verbosity(struct program_settings * settings, const char * verbosity){
  for(int i = 0; i <= (int)LOG_PRIORITY_ERROR; ++i){
    if(strcmp(verbosity, verbosity_args[i]) == 0){
      settings->log_priority = (enum log_priority)i;
      return 0;
    }
  }
  return -1;
}

static int parse_args(struct program_settings * settings, int arg_count, char * const args[]){
  assert(settings != NULL);
  assert(arg_count > 0);
  assert(args != NULL);

  struct option options[] = {
			     {"client", no_argument, NULL, 'c'},
			     {"daemon", no_argument, NULL, 'd'},
			     {"language", required_argument, NULL, 'l'},
			     {"resource_path", required_argument, NULL, 'r'},
			     {"server", no_argument, NULL, 's'},
			     {"verbosity", required_argument, NULL, 'v'},
			     {NULL, 0, NULL, 0}
  };

  int index = 0;
  while(true){
    int c = getopt_long(arg_count, args, "cdl:r:sv:", options, &index);
    if(c == -1){
      break;
    }else if(c == '?'){
      fputs("invalid program argument\n", stderr);
      set_status(STATUS_INVALID_PROGRAM_ARGUMENT);
      return -1;
    }else if(c == 'c'){
      settings->client = true;
    }else if(c == 'd'){
      settings->daemon = true;
    }else if(c == 'l'){
      settings->language = optarg;
    }else if(c == 'r'){
      settings->resource_path = optarg;
    }else if(c == 's'){
      settings->server = true;
    }else if(c == 'v'){
      if(parse_verbosity(settings, optarg)){
	fputs("invalid program argument: invalid verbosity\n", stderr);
	set_status(STATUS_INVALID_PROGRAM_ARGUMENT);
	return -1;
      }
    }
  }
  return 0;
}

int load_program_settings(struct program_settings * settings, int arg_count, char * const args[]){
  assert(settings != NULL);
  
  // set default settings
  settings->server = false;
  settings->client = false;
  settings->log_priority = LOG_PRIORITY_ERROR;
  settings->language = NULL;
  settings->resource_path = NULL;
  
  return parse_args(settings, arg_count, args);
}
