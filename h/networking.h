#pragma once
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

#define PORT 5000
#define MSG_BUFFER_SIZE 1024

typedef struct network_cfg {
  int server_fd;
  int client_fd;

  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len;
  char buffer[MSG_BUFFER_SIZE];

  int opt;

  fd_set readfds;
  int max_sd;

} network_cfg_t;