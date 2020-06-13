// Nick Karlovich
// CSCI 4061 Spring 2020
// x500: karlo015

#include "blather.h"
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

// Global variables
int NOT_STOPPED = 1;
server_t the_serv_actual;
server_t * the_serv = &the_serv_actual;


// Signal handlers
void handle_SIGTERM(int sig_num) {
  NOT_STOPPED = 0;
  server_shutdown(the_serv);
}

void handle_SIGINT(int sig_num) {
  NOT_STOPPED = 0;
  server_shutdown(the_serv);
}


int main(int argc, char * argv[]) {

  // Signal handling from earlier lab
  struct sigaction my_sa = {};               // portable signal handling setup with sigaction()
  sigemptyset(&my_sa.sa_mask);               // don't block any other signals during handling
  my_sa.sa_flags = SA_RESTART;

  my_sa.sa_handler = handle_SIGTERM;         // run function handle_SIGTERM
  sigaction(SIGTERM, &my_sa, NULL);          // register SIGTERM with given action

  my_sa.sa_handler = handle_SIGINT;          // run function handle_SIGINT
  sigaction(SIGINT,  &my_sa, NULL);

  // initializing server.
  snprintf(the_serv->server_name, MAXPATH, "%s", argv[1]);
  the_serv->join_fd = -1;
  the_serv->join_ready = -1;
  the_serv->n_clients = 0;
  server_start(the_serv, the_serv->server_name, DEFAULT_PERMS);

  // constantly runs until signal is recieved which then shuts down server.
  while(NOT_STOPPED) {

    // uses poll to check all client fd's and join fd.
    server_check_sources(the_serv);

    // to get here, poll must have succeeded so now we need to determine
    // what triggered the poll to continue and stop blocking.
    // does a client want to join?
    if(the_serv->join_ready == 1) {
      server_handle_join(the_serv);
    }

    // check all clients if there is something in it's fifo.
    for(int i = 0; i < the_serv->n_clients; i++) {
      if(server_client_ready(the_serv, i) == 1) {

        // If the client has something to say, handle it.
        server_handle_client(the_serv, i);
      }
    }
  }
  return 0;
}
