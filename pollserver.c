#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "9034"  // port client will connect to
#define BACKLOG 10   // maximum number of connection that can be queued

/*
 * fetches the ip address info from a sockaddr struct
 * return sin_addr if sa_family is AF_INET else return sin6_addr
 */
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &((struct sockaddr_in *)&sa)->sin_addr;
  } else {
    return &((struct sockaddr_in6 *)&sa)->sin6_addr;
  }
}

/*
 * fetches a socket listening on the port PORT,
 * return -1 on error
 */
int get_listener() {
  int sockfd;
  int yes = 1;
  int rv;
  struct addrinfo hints, *res, *p;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return -1;
  }
  for (p = res; p != NULL; p = p->ai_next) {
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd < 0) continue;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
      close(sockfd);
      continue;
    }
    break;
  }
  freeaddrinfo(res);
  if (p == NULL) {
    return -1;
  }
  if (listen(sockfd, BACKLOG) < 0) {
    return -1;
  }
  return sockfd;
}

/*
 * add new socket descriptor to the pfds.
 * it relallocs pfds if fd_count == fd_size
 */
void add_to_pfds(struct pollfd *pfds[], int new_fd, int *fd_count,
                 int *fd_size) {
  if (*fd_count == *fd_size) {
    *fd_size *= 2;
    *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
  }
  (*pfds)[*fd_count].fd = new_fd;
  (*pfds)[*fd_count].events = POLLIN;
  (*fd_count)++;
}

/*
 * delete a entry from pfds at index i
 */
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count) {
  pfds[i] = pfds[*fd_count - 1];
  (*fd_count)--;
}

int main() {
  int listener;
  int new_fd;
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen;

  char buf[255];
  char remoteIp[INET6_ADDRSTRLEN];
  int fd_count = 0;
  int fd_size = 5;
  struct pollfd *pfds = malloc(sizeof(*pfds) * fd_size);

  listener = get_listener();
  if (listener == -1) {
    fprintf(stderr, "error getting listening socket\n");
    exit(1);
  }
  pfds[0].fd = listener;
  pfds[0].events = POLLIN;
  fd_count = 1;
  for (;;) {
    int poll_count = poll(pfds, fd_count, -1);
    if (poll_count == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < fd_count; ++i) {
      if (pfds[i].revents & POLLIN) {
        if (pfds[i].fd == listener) {  // we got a new connection
          addrlen = sizeof(remoteaddr);
          new_fd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
          if (new_fd == -1) {
            perror("accept");
          } else {
            add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);
            printf("pollserver got a connection from %s on socket %d\n",
                   inet_ntop(remoteaddr.ss_family,
                             get_in_addr((struct sockaddr *)&remoteaddr),
                             remoteIp, sizeof(remoteIp)),
                   new_fd);
          }
        } else {
          int nbytes = recv(pfds[i].fd, buf, sizeof(buf), 0);
          int sender_fd = pfds[i].fd;
          if (nbytes <= 0) {
            if (nbytes == 0) {  // connection closed by a client
              printf("pollserver: socket %d hung up\n", sender_fd);
            } else {
              perror("recv");
            }
            close(pfds[i].fd);
            del_from_pfds(pfds, i, &fd_count);
          } else {
            for (int j = 0; j < fd_count; ++j) {
              int dest_fd = pfds[j].fd;
              if (dest_fd != listener && dest_fd != sender_fd) {
                if (send(dest_fd, buf, nbytes, 0) == -1) {
                  perror("send");
                }
              }
            }
          }
        }
      }
    }
  }
  return 0;
}
