#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>

#include "pbx.h"
#include "server.h"
#include "debug.h"
#include "csapp.h"

static void terminate(int status);

// SIGHUP handler
void sighup_handler(int sig) {
    debug("Received SIGHUP signal, terminating server...");
    terminate(EXIT_SUCCESS);
}

/*
 * "PBX" telephone exchange simulation.
 *
 * Usage: pbx <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    int port = 0;
    int opt;

    // Parse command-line arguments for the -p <port> option.
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                if (port <= 0) {
                    // Invalid port number
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (port == 0) {
        // Port number is required
        exit(EXIT_FAILURE);
    }

    // Perform required initialization of the PBX module.
    debug("Initializing PBX...");
    pbx = pbx_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function pbx_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    // Set up the SIGHUP handler using sigaction.
    struct sigaction sa;
    sa.sa_handler = sighup_handler;
    // sa.sa_flags = 0;
    // sigemptyset(&sa.sa_mask);

    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        // sigaction
        terminate(EXIT_FAILURE);
    }

    // Open a listening socket
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    listenfd = open_listenfd(argv[2]);

    while (1) {
        // Accept incoming client connections

        clientlen = sizeof(struct sockaddr_storage);
        connfdp = malloc(sizeof(int));

        if (connfdp == NULL) {
            // malloc
            continue; // Skip to the next iteration
        }

        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);

        // Create a thread to handle the client connection
        if (pthread_create(&tid, NULL, (void*) pbx_client_service, connfdp) != 0) {
            // pthread_create
            close(*connfdp);
            free(connfdp);
            continue; // Skip to the next iteration
        }

    }
    terminate(EXIT_SUCCESS);

    // fprintf(stderr, "You have to finish implementing main() "
	//     "before the PBX server will function.\n");

    // terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    debug("Shutting down PBX...");
    pbx_shutdown(pbx);
    debug("PBX server terminating");
    exit(status);
}
