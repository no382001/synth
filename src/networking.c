#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

#include "hash.h"
#include "utils.h"
#include "commands.h"

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
    log_message(ERROR, "creating socket failed");
    exit(1);
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    log_message(ERROR, "setsockopt(SO_REUSEADDR) failed");
    close(server_fd);
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // bind socket to port
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    close(server_fd);
    log_message(ERROR, "bind failed");
    exit(1);
  }

  // incoming connections
  if (listen(server_fd, 3) < 0) {
    log_message(ERROR, "listen failed");
    exit(1);
  }

  log_message(INFO, "c server listening on port %d...", PORT);

  // socket to non-blocking mode
  set_nonblocking(server_fd);

  fd_set readfds;
  while (1) {
    // -------------------------
    // accept data from socket
    // -------------------------

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
      log_message(ERROR, "select error");
      exit(1);
    }

    // incoming connection
    if (FD_ISSET(server_fd, &readfds)) {
      client_len = sizeof(client_addr);
      if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                              &client_len)) < 0) {
        log_message(ERROR, "accept failed");
      } else {
        log_message(INFO, "client connected!");
        set_nonblocking(
            client_fd); // make the client socket non-blocking as well
      }
    }

    // check if the client has sent data
    if (client_fd > 0 && FD_ISSET(client_fd, &readfds)) {
      memset(buffer, 0, BUFFER_SIZE);
      int valread = read(client_fd, buffer, BUFFER_SIZE);

      if (valread > 0) {
        size_t len = strlen(buffer);
        if (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
          buffer[len - 1] = '\0';
        }
        log_message(DEBUG, "message from tcl: ->%s<-", buffer);

        // -------------------------
        // do something w/ that
        // -------------------------

        char head[50], tail[50] = {NULL};
        if (sscanf(buffer, "%s %s", head, tail) != 2) {
          log_message(ERROR, "invalid command format: ->%s<-", buffer);
          continue;
        }

        command_fn f = find_function_by_command(head);
        if (f != NULL) {
          f(tail);
        } else {
          log_message(ERROR, "invalid command: ->%s<-", head);
        }

      } else if (valread == 0) {
        // client has disconnected
        log_message(INFO, "client disconnected.");
        close(client_fd);
        client_fd = -1; // reset the client_fd
      }
    }
  }

  close(server_fd);
}