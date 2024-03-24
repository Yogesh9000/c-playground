#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &((struct sockaddr_in *)sa)->sin_addr;
  }
  return &((struct sockaddr_in6 *)sa)->sin6_addr;
}

int mgetline(char buff[], int capacity) {
  char c;
  int i = 0;
  for (; i < capacity - 1 && (c = getchar()) != EOF && c != '\n'; ++i) {
    buff[i] = c;
  }
  buff[i] = '\0';
  return i;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s host port", argv[0]);
  }
  int sockfd;
  int rv;
  struct addrinfo hints, *res, *p;
  char buff[255] = "";
  char serverIp[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(EXIT_FAILURE);
  }

  for (p = res; p != NULL; p = p->ai_next) {
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) {
      perror("socket");
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "failed to connect to server!!!\n");
    exit(EXIT_FAILURE);
  }
  inet_ntop(p->ai_family, get_in_addr(p->ai_addr), serverIp, sizeof(serverIp));
  printf("client connected successfully to %s\n", serverIp);
  freeaddrinfo(res);

  struct pollfd pfds[2];
  pfds[0].fd = 0;  // standard Input
  pfds[0].events = POLLIN;
  pfds[1].fd = sockfd;  // server socket
  pfds[1].events = POLLIN;

  while (strcmp(buff, "quit") != 0) {
    int pollcount = poll(pfds, 2, -1);
    if (pollcount == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    }
    if (pfds[0].revents & POLLIN) {  // user entered some input
      int nbytes = mgetline(buff, 255);
      if (strcmp(buff, "quit") == 0) {
        send(sockfd, "bye\0", 4, 0);
        continue;
      }
      if (send(sockfd, buff, nbytes, 0) == -1) {
        perror("send");
        continue;
      }
    } else {  // server sent some message
      int nbytes;
      if ((nbytes = recv(sockfd, buff, sizeof(buff), 0)) == -1) {
        perror("recv");
        continue;
      }
      buff[nbytes] = '\0';
      printf("%s\n", buff);
      fflush(stdout);
    }
  }
  return 0;
}
