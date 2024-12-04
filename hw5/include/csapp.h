#ifndef CSAPP_H
#define CSAPP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define	MAXLINE	 8192  /* Max text line length */

typedef struct sockaddr SA;

#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;                /* Descriptor for this internal buf */
    int rio_cnt;               /* Unread bytes in internal buf */
    char *rio_bufptr;          /* Next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE]; /* Internal buffer */
} rio_t;

void unix_error(char *msg);
// Function prototype for open_listenfd
int open_listenfd(char *port);
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
void rio_readinitb(rio_t *rp, int fd);
void Close(int fd);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

#endif // CSAPP_H
