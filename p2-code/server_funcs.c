// Nick Karlovich
// CSCI 4061 Spring 2020
// x500: karlo015

#include "blather.h"

// helper function to get a client_t from a server via index.
client_t *server_get_client(server_t *server, int index) {
  return &(server->client[index]);
}

// initializes server by removing existing fifo and creating new one.
void server_start(server_t *server, char *server_name, int perms) {

  // NOTE modifies the server name so it has the .fifo extension (because it's a pointer not a copy)
  char * fifo_name = strncat(server_name,".fifo",5);
  remove(fifo_name);
  mkfifo(fifo_name, perms);
  int fd = open(fifo_name, perms);
  server->join_fd = fd;
}

// shuts down server by closing and removing fifo's and sending shutdown messages to connected clients.
void server_shutdown(server_t *server) {
  printf("Shutting down: %s\n",server->server_name);

  //close and remove server's fifo so that nothing else can join.
  close(server->join_fd);
  int ret = remove(server->server_name);
  if(ret < 0) {perror("Server fifo was failed to be removed\n");}

  printf("server fifo: %s removed\n", server->server_name);
  mesg_t shutdown;

  //set struct to null values.
  memset(&shutdown, 0, sizeof(mesg_t));
  shutdown.kind = BL_SHUTDOWN;
  snprintf(shutdown.name,16,"SERVER SHUTDOWN");

  // broadcasting shutdown to clients.
  server_broadcast(server, &shutdown);
  for(int i = 0; i < server->n_clients; i++) {
    server_remove_client(server, i);
  }
}

// Adds a client to the server from the join fifo.
int server_add_client(server_t *server, join_t *join) {
  printf("client: %s is being added\n", join->name);

  // If the server is full, ignore request.
  if (server->n_clients >= MAXCLIENTS) {
    printf("server is full, ignoring join request\n");
    return -1;
  }

  // creating client_t object from join_t object.
  client_t temp;
  strncpy(temp.name,join->name, MAXPATH);
  strncpy(temp.to_client_fname,join->to_client_fname, MAXPATH);
  strncpy(temp.to_server_fname,join->to_server_fname, MAXPATH);
  temp.to_client_fd = open(join->to_client_fname, DEFAULT_PERMS);
  temp.to_server_fd = open(join->to_server_fname, DEFAULT_PERMS);
  temp.data_ready = 0;

  server->client[server->n_clients] = temp;
  (server->n_clients)++;
  printf("client: %s has been added\n", join->name);
  return 0;
}

int server_remove_client(server_t *server, int idx) {
  client_t * client = server_get_client(server,idx);

  //cleaning up fifo's
  close(client->to_client_fd);
  close(client->to_server_fd);
  remove(strncat(client->to_client_fname,".fifo",5));
  remove(strncat(client->to_server_fname,".fifo",5));

  //shifting remaining clients down client_arr.
  for(int i = idx; i < server->n_clients; i++) {
    client_t * new = server_get_client(server,i + 1);  // shifting down.
    server->client[i] = *new;
  }
  server->n_clients--;
  return 0;
}

int server_broadcast(server_t *server, mesg_t *mesg) {

  printf("printing message type: %d\n", mesg->kind);

  // iterate through all clients and print out message to them.
  for(int i = 0; i < server->n_clients; i++) {
    client_t * client = server_get_client(server,i);
    write(client->to_client_fd, mesg, sizeof(mesg_t));
  }
  return 0;
}

void server_check_sources(server_t *server) {
  struct pollfd clients[MAXCLIENTS + 1];
  int i = 0;
  for( ; i < server->n_clients; i++) {
    clients[i].fd = server_get_client(server,i)->to_server_fd;
    clients[i].events = POLLIN;
    clients[i].revents = 0;
  }
  // setting default values to please VALGRIND
  for(; i < MAXCLIENTS; i++) {
    clients[i].fd = 0;
    clients[i].events = POLLIN;
    clients[i].revents = 0;
  }
  clients[MAXCLIENTS].fd = server->join_fd;
  clients[MAXCLIENTS].events = POLLIN;
  clients[MAXCLIENTS].revents = 0;

  int ret = poll(clients, MAXCLIENTS + 1, 1000);
  if(ret < 0) {perror("poll() failed\n");}

  // poll is blocking so when we get here we know some data must be ready.
  for(int i = 0; i < server->n_clients; i++) {
    if(clients[i].revents & POLLIN) {
      // tag the client that's ready for later processing.
      server_get_client(server,i)->data_ready = 1;
    }
  }

  // if it wasn't a client that's ready but instead was a join request.
  if(clients[MAXCLIENTS].revents & POLLIN) {
    server->join_ready = 1;
  }
}

// helper function.
int server_join_ready(server_t *server) {
  return server->join_ready;
}

// handle when a client wants to join.
int server_handle_join(server_t *server) {
  join_t real_join;
  join_t * join_req = &real_join;

  //read in join_t struct from join fifo.
  read(server->join_fd, join_req, sizeof(join_t));
  server_add_client(server, join_req);
  mesg_t join;

  //init to null values.
  memset(&join, 0, sizeof(mesg_t));
  join.kind = BL_JOINED;
  snprintf(join.name, MAXPATH, "%s", join_req->name);

  // broadcast join request to server.
  server_broadcast(server, &join);
  server->join_ready = 0;
  return 0;
}

// helper function.
int server_client_ready(server_t *server, int idx) {
  return server_get_client(server, idx)->data_ready;
}

// If the server detected data from a client, handle it.
int server_handle_client(server_t *server, int idx) {
  if(server_client_ready(server, idx) == 1) {
    mesg_t message;
    message.kind = 0;

    //read message from that clients fifo.
    int ret = read(server_get_client(server, idx)->to_server_fd, &message, sizeof(mesg_t));
    if(ret < 0) {perror("Failed to read message from client\n");}
    if(message.kind == 30) {
      server_remove_client(server,idx);
    }
    server_broadcast(server, &message);
    server_get_client(server, idx)->data_ready = 0;
  } else {
    //redundant but for clarity
    server_get_client(server, idx)->data_ready = 0;
    return 0;
  }
  return 0;
}
