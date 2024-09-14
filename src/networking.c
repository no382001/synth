#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/select.h>
#include <errno.h>
#include <stdlib.h>

#include "utils.h"

#define PORT 5000
#define BUFFER_SIZE 1024

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

void networking_thread(void) {
  int server_fd;
  int client_fd = -1;

  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len;
  char buffer[BUFFER_SIZE];

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    log_message(ERROR,"creating socket failed");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // bind socket to port
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    close(server_fd);
    log_message(ERROR,"bind failed");
    exit(1);
  }

  // incoming connections
  if (listen(server_fd, 3) < 0) {
    log_message(ERROR,"listen failed");
    exit(1);
  }

  log_message(INFO,"c server listening on port %d...\n", PORT);

  // socket to non-blocking mode
  set_nonblocking(server_fd);

  fd_set readfds;
  while (1) {
    // clear the set of read file descriptors
    FD_ZERO(&readfds);

    // add the server socket to the set
    FD_SET(server_fd, &readfds);
    int max_sd = server_fd;

    // add any existing client socket to the set
    if (client_fd > 0) {
      FD_SET(client_fd, &readfds);
      if (client_fd > max_sd) {
        max_sd = client_fd;
      }
    }

    // set timeout to 1 second
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // wait for an activity on one of the sockets
    int activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);

    if (activity < 0 && errno != EINTR) {
      log_message(ERROR,"select error");
      exit(1);
    }

    // incoming connection
    if (FD_ISSET(server_fd, &readfds)) {
      client_len = sizeof(client_addr);
      if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                              &client_len)) < 0) {
        log_message(ERROR,"accept failed");
      } else {
        log_message(INFO,"client connected!\n");
        set_nonblocking(
            client_fd); // make the client socket non-blocking as well
      }
    }

    // check if the client has sent data
    if (client_fd > 0 && FD_ISSET(client_fd, &readfds)) {
      memset(buffer, 0, BUFFER_SIZE);
      int valread = read(client_fd, buffer, BUFFER_SIZE);

      if (valread > 0) {
        log_message(DEBUG,"message from tcl: %s\n", buffer);

        // send a response back to Tcl
        char response[] = "Hello from C!\n";
        send(client_fd, response, strlen(response), 0);
        log_message(DEBUG,"sent response: %s\n", response);
      } else if (valread == 0) {
        // client has disconnected
        log_message(INFO,"client disconnected.\n");
        close(client_fd);
        client_fd = -1; // reset the client_fd
      }
    }
  }

  close(server_fd);
}