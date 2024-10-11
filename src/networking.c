#include <signal.h>
#include <unistd.h>

#include "commands.h"
#include "hash.h"
#include "networking.h"
#include "utils.h"

static network_cfg_t *global_network_cfg;
extern struct lfq_ctx *g_lfq_ctx;

static void signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    log_message(INFO, "received termination signal, shutting down...");
    if (global_network_cfg && global_network_cfg->server_fd > 0) {
      close(global_network_cfg->server_fd);
    }
    if (global_network_cfg && global_network_cfg->client_fd > 0) {
      close(global_network_cfg->client_fd);
    }
    exit(0);
  }
}

static void setup_signal_handling(network_cfg_t *n) {
  global_network_cfg = n;

  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) < 0) {
    log_message(ERROR, "sigaction for SIGINT failed");
    exit(1);
  }

  if (sigaction(SIGTERM, &sa, NULL) < 0) {
    log_message(ERROR, "sigaction for SIGTERM failed");
    exit(1);
  }

  log_message(INFO, "signal handlers set up successfully");
}

static void set_nonblocking(int sockfd) {
  int flags = fcntl(sockfd, F_GETFL, 0);
  fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

static void init_networking(network_cfg_t *n) {
  n->client_fd = -1;

  if ((n->server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    log_message(ERROR, "creating socket failed");
    exit(1);
  }

  if (setsockopt(n->server_fd, SOL_SOCKET, SO_REUSEADDR, &n->opt,
                 sizeof(n->opt)) < 0) {
    log_message(ERROR, "setsockopt(SO_REUSEADDR) failed");
    close(n->server_fd);
    exit(1);
  }

  n->server_addr.sin_family = AF_INET;
  n->server_addr.sin_addr.s_addr = INADDR_ANY;
  n->server_addr.sin_port = htons(PORT);

  // bind socket to port
  if (bind(n->server_fd, (struct sockaddr *)&n->server_addr,
           sizeof(n->server_addr)) < 0) {
    close(n->server_fd);
    log_message(ERROR, "bind failed");
    exit(1);
  }

  // incoming connections
  if (listen(n->server_fd, 3) < 0) {
    log_message(ERROR, "listen failed");
    exit(1);
  }

  log_message(INFO, "c server listening on port %d...", PORT);

  // socket to non-blocking mode
  set_nonblocking(n->server_fd);
}

static void accept_data(network_cfg_t *n) {

  // clear the set of read file descriptors
  FD_ZERO(&n->readfds);

  // add the server socket to the set
  FD_SET(n->server_fd, &n->readfds);
  n->max_sd = n->server_fd;

  // add any existing client socket to the set
  if (n->client_fd > 0) {
    FD_SET(n->client_fd, &n->readfds);
    if (n->client_fd > n->max_sd) {
      n->max_sd = n->client_fd;
    }
  }

  // set timeout to 1 second
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 10000; // 10 milliseconds

  // wait for an activity on one of the sockets
  int activity = select(n->max_sd + 1, &n->readfds, NULL, NULL, &timeout);

  if (activity < 0 && errno != EINTR) {
    log_message(ERROR, "select error");
    exit(1);
  }

  // incoming connection
  if (FD_ISSET(n->server_fd, &n->readfds)) {
    n->client_len = sizeof(n->client_addr);
    if ((n->client_fd = accept(n->server_fd, (struct sockaddr *)&n->client_addr,
                               &n->client_len)) < 0) {
      log_message(ERROR, "accept failed");
    } else {
      log_message(INFO, "client connected!");
      set_nonblocking(
          n->client_fd); // make the client socket non-blocking as well
    }
  }
}

static void handle_data(network_cfg_t *n) {
  // check if the client has sent data
  if (n->client_fd > 0 && FD_ISSET(n->client_fd, &n->readfds)) {
    memset(n->buffer, 0, MSG_BUFFER_SIZE);
    int valread = read(n->client_fd, n->buffer, MSG_BUFFER_SIZE);

    if (valread > 0) {
      size_t len = strlen(n->buffer);
      if (len > 0 &&
          (n->buffer[len - 1] == '\n' || n->buffer[len - 1] == '\r')) {
        n->buffer[len - 1] = '\0';
      }
      // log_message(DEBUG, "message from tcl: ->%s<-", buffer);

      char head[50], tail[50] = {NULL};
      if (sscanf(n->buffer, "%s %s", head, tail) != 2) {
        log_message(ERROR, "invalid command format: ->%s<-", n->buffer);
        // continue;
      }

      command_fn f = find_function_by_command(head);
      if (f != NULL) {
        if (!strcmp(tail, "semicolon")) {
          f(";");
        } else if (!strcmp(tail, "apostrophe")) {
          f("'");
        } else {
          f(tail);
        }
      } else {
        log_message(ERROR, "invalid command: ->%s<-", head);
      }

    } else if (valread == 0) {
      // client has disconnected
      log_message(INFO, "client disconnected.");
      close(n->client_fd);
      n->client_fd = -1; // reset the client_fd
    }
  }
}

extern Synth *g_synth;

static void send_data(network_cfg_t *n) {
  if (n->client_fd > 0) {

    static int8_t out_buffer[DOWNSAMPLE_SIZE];
    static size_t downsampled_size_in_bytes = DOWNSAMPLE_SIZE * sizeof(int8_t);

    for (size_t i = 0, j = 0; i < DOWNSAMPLE_SIZE; ++i, j += DOWNSAMPLE_FACTOR) {
      float sample = g_synth->signal[j];

      int8_t resampled_value = (int8_t)(sample * 127);
      out_buffer[i] = resampled_value;
    }

    ssize_t bytes_sent =
        send(n->client_fd, (void *)out_buffer, downsampled_size_in_bytes, 0);

    if (bytes_sent < 0) {
      log_message(ERROR, "error sending");
      close(n->client_fd);
      n->client_fd = -1;
    } else {
      log_message(DEBUG, "sent %ld bytes", bytes_sent);
    }
  }
}

void networking_thread(void) {
  network_cfg_t n = {NULL};
  setup_signal_handling(&n);
  init_networking(&n);

  while (1) {
    accept_data(&n);
    handle_data(&n);
    send_data(&n);
  }

  close(n.server_fd);
}