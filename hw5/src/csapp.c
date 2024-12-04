// Functions from the CS:APP3e textbook

#include "csapp.h"
#include <string.h>     // For memset
#include <stdlib.h>     // For NULL
#include <unistd.h>     // For close
#include <sys/types.h>  // For socket
#include <sys/socket.h> // For socket functions
#include <netdb.h>      // For getaddrinfo
#include <errno.h>      // For errno

#define LISTENQ 1024 /* Maximum number of pending connections */

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

int open_listenfd(char *port) {
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;      /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV;     /* ... using port number */
    if (getaddrinfo(NULL, port, &hints, &listp) != 0) {
        return -1; /* Failed to get address info */
    }

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Socket creation failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                       (const void *)&optval, sizeof(int)) < 0) {
            close(listenfd);
            continue; /* Failed to set socket options, try next */
        }

        /* Bind the descriptor to the address */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* Success */

        close(listenfd); /* Bind failed, try the next */
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
        return -1;
    }

    return listenfd;
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
	unix_error("Accept error");
    return rc;
}

void rio_readinitb(rio_t *rp, int fd){
	rp->rio_fd = fd;
	rp->rio_cnt = 0;
	rp->rio_bufptr = rp->rio_buf;
}

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
	int cnt;

	while (rp->rio_cnt <= 0) { /* Refill if buf is empty */
		rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
		sizeof(rp->rio_buf));
		if (rp->rio_cnt < 0) {
		if (errno != EINTR) /* Interrupted by sig handler return */
			return -1;
		}
		else if (rp->rio_cnt == 0) /* EOF */
			return 0;
		else
			rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */
	}

	/* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
	cnt = n;
	if (rp->rio_cnt < n)
		cnt = rp->rio_cnt;
	memcpy(usrbuf, rp->rio_bufptr, cnt);
	rp->rio_bufptr += cnt;
	rp->rio_cnt -= cnt;
	return cnt;
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;
	    if (c == '\n') {
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, no data read */
	    else
		break;    /* EOF, some data was read */
	} else
	    return -1;	  /* Error */
    }
    *bufp = 0;
    return n-1;
}

void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
	unix_error("Close error");
}
