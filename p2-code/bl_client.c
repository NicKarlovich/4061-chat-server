// Nick Karlovich
// CSCI 4061 Spring 2020
// x500: karlo015

#include "blather.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

client_t client_actual;
client_t *client = &client_actual;

pthread_t input_thread;
pthread_t server_thread;

int SHUTDOWN_MSG_LENGTH = 33;
int MAX_PID_LENGTH = 7;

void *input_func(void *arg) {

  while(!simpio->end_of_input) {
    simpio_reset(simpio);
    iprintf(simpio, "");

    //reading input
    while(!simpio->line_ready && !simpio->end_of_input) {
      simpio_get_char(simpio);
    }

    // guessing happens when \n is entered
    if(simpio->line_ready) {

      mesg_t temp;
      memset(&temp, 0, sizeof(temp));
      temp.kind = BL_MESG;
      snprintf(temp.name, MAXPATH, "%s", client->name);
      snprintf(temp.body, MAXLINE, "%s", simpio->buf);
      write(client->to_server_fd, &temp, sizeof(mesg_t));
    }
  }

  mesg_t depart;

  // init struct to default values.
  memset(&depart, 0, sizeof(mesg_t));
  depart.kind = BL_DEPARTED;
  snprintf(depart.name, MAXPATH, "%s", client->name);
  write(client->to_server_fd, &depart, sizeof(mesg_t));
  pthread_cancel(server_thread);
  return NULL;
}

void *server_func(void *arg) {
  while(1) {
    mesg_t temp;

    // init struct to default values.
    memset(&temp, 0, sizeof(mesg_t));

    // read message from client_fd and place in temp struct.
    read(client->to_client_fd, &temp, sizeof(mesg_t));

    //if the client recieves shutdown from server then shutdown.
    if(temp.kind == BL_SHUTDOWN) {
      char message[SHUTDOWN_MSG_LENGTH];

      //init struct to null values.
      memset(message, 0, sizeof(message));
      snprintf(message, SHUTDOWN_MSG_LENGTH, "!!! server is shutting down !!!\n");
      iprintf(simpio, message);
      pthread_cancel(input_thread);
      return NULL;
    }

    // Message formatting for message, join and departure.
    if(temp.kind == BL_MESG) {
      iprintf(simpio, "[%s] : %s\n", temp.name, temp.body);
    }
    if(temp.kind == BL_JOINED) {
      iprintf(simpio, "-- %s JOINED --\n", temp.name);
    }
    if(temp.kind == BL_DEPARTED) {
      iprintf(simpio, "-- %s DEPARTED --\n", temp.name);
    }
  }
  pthread_cancel(input_thread);
}

int main(int argc, char * argv[]) {
  char pid_actual[64];
  char * pid = &pid_actual[64];

  // this line of code for turing pid into string taken from
  // https://stackoverflow.com/questions/15262315/how-to-convert-pid-t-to-string 

  // Based on this doc, the max length of a pid on a 64 bit system is 7.
  // So I'm going to assume that 7 is a safe length.
  snprintf(pid, MAX_PID_LENGTH, "%ld", (long) getpid());

  // creating the fifo filenames the client will be using.
  char client_act[MAXNAME];
  char server_act[MAXNAME];
  char * to_client_fifo_name = &client_act[MAXNAME];
  char * to_server_fifo_name = &server_act[MAXNAME];
  snprintf(to_client_fifo_name, MAX_PID_LENGTH + 12, "%s.client.fifo", pid);
  snprintf(to_server_fifo_name, MAX_PID_LENGTH + 12, "%s.server.fifo", pid);

  // remove any existing fifo's
  remove(to_client_fifo_name);
  remove(to_server_fifo_name);

  // creating and opening server and client fifos
  snprintf(client->name,MAXPATH, "%s", argv[2]);
  mkfifo(to_client_fifo_name, DEFAULT_PERMS);
  int c_fd = open(to_client_fifo_name, DEFAULT_PERMS);
  mkfifo(to_server_fifo_name, DEFAULT_PERMS);
  int s_fd = open(to_server_fifo_name, DEFAULT_PERMS);
  client->to_client_fd = c_fd;
  client->to_server_fd = s_fd;
  snprintf(client->to_client_fname, MAXPATH, "%s", to_client_fifo_name);
  snprintf(client->to_server_fname, MAXPATH, "%s", to_server_fifo_name);

  // sending a join request to the server via the server's join_fd
  join_t join;

  //init to null values.
  memset(&join, 0, sizeof(join));
  snprintf(join.name, MAXPATH, "%s", client->name);
  snprintf(join.to_client_fname, MAXPATH, "%s", client->to_client_fname);
  snprintf(join.to_server_fname, MAXPATH, "%s", client->to_server_fname);

  char join_fifo_actual[MAXNAME];
  char * join_fifo = &join_fifo_actual[MAXNAME];
  snprintf(join_fifo, MAXPATH, "%s.fifo", argv[1]);
  int join_fd = open(join_fifo, O_RDWR);
  write(join_fd, &join, sizeof(join_t));

  // setting up the simpio prompt for the client to view.
  char prompt[MAXNAME];
  snprintf(prompt, MAXPATH + 3, "%s>> ",client->name); // create a prompt string
  simpio_set_prompt(simpio, prompt);
  simpio_reset(simpio);
  simpio_noncanonical_terminal_mode();

  pthread_create(&input_thread, NULL, input_func, NULL);
  pthread_create(&server_thread, NULL, server_func, NULL);

  // once the threads finish (ie they leave the server or are disconnected)
  // do cleanup (remove fifo's reset terminal)
  pthread_join(input_thread, NULL);
  pthread_join(server_thread, NULL);
  simpio_reset_terminal_mode();
  //cleaning up fifos
  remove(to_client_fifo_name);
  remove(to_server_fifo_name);
  printf("\n");                 // newline just to make returning to the terminal prettier
  return 0;
}
