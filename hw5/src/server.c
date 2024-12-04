/*
 * "PBX" server module.
 * Manages interaction with a client telephone unit (TU).
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>


#include "debug.h"
#include "pbx.h"
#include "server.h"
#include "csapp.h"

int next_cmd(FILE *f, char **str) {
    int size = 8, index = 0;
    char *command = malloc(sizeof(char) * size);
    if (!command) {
        return -2; // Memory allocation failed.
    }

    char c;
    while ((c = fgetc(f)) != EOF) {
        if (index == size - 1) {
            size *= 2;
            command = realloc(command, sizeof(char) * size);
            if (!command) {
                return -3; // Memory reallocation failed.
            }
        }

        command[index++] = c;
        if (c == '\r' || c == '\n') {
            command[index - 1] = '\0';
        }

        if (c == '\n') {
            break;
        }
    }

    // Command is empty.
    if (index == 0) {
        free(command);
        return -1;
    }

    command[index--] = '\0';
    *str = command;
    return 0;
}

/*
 * Thread function for the thread that handles interaction with a client TU.
 * This is called after a network connection has been made via the main server
 * thread and a new thread has been created to handle the connection.
 */
#if 1
void *pbx_client_service(void *arg) {
    // TO BE IMPLEMENTED
    // abort();

    // Detach the thread.
    pthread_detach(pthread_self());

    // Free the file descriptor storage.
    if (!arg)
        return NULL;
    int fd = *(int *)arg;
    free(arg);

    // Initialize a TU for this file descriptor.
    TU *tu = tu_init(fd);
    if (!tu)
        return NULL;

    // Open a FILE* for easier handling of the socket input.
    FILE *f = fdopen(fd, "r");
    if (!f) {
        tu_unref(tu, "failed to open FILE*");
        return NULL;
    }

    // Register the TU with the PBX.
    if (pbx_register(pbx, tu, fd)) {
        // Registration failed.
        fclose(f);
        tu_unref(tu, "failed to register TU");
        return NULL;
    }

    char *command = NULL;

    // Service loop for handling client commands.
    while (1) {
        // Get the next command from the client.
        if (next_cmd(f, &command)) {
            break; // End of input or error.
        }

        // Direct inline logic from pbx_run_command.
        if (!strcmp(tu_command_names[TU_PICKUP_CMD], command)) {
            // PICKUP command.
            if (tu_pickup(tu) < 0) {
                debug("ERROR: Failed to process PICKUP command");
            }
        } else if (!strcmp(tu_command_names[TU_HANGUP_CMD], command)) {
            // HANGUP command.
            if (tu_hangup(tu) < 0) {
                debug("ERROR: Failed to process HANGUP command");
            }
        } else if (strstr(command, tu_command_names[TU_DIAL_CMD]) == command) {
            // DIAL command.
            char *ptr = command + strlen(tu_command_names[TU_DIAL_CMD]);
            while (*ptr == ' ') {
                ptr++;
            }
            if (*ptr) {
                int ext = atoi(ptr);
                if (pbx_dial(pbx, tu, ext) < 0) {
                    debug("ERROR: Failed to process DIAL command with argument: %s", ptr);
                }
            } else {
                debug("ERROR: DIAL command missing argument");
            }
        } else if (strstr(command, tu_command_names[TU_CHAT_CMD]) == command) {
            // CHAT command.
            char *ptr = command + strlen(tu_command_names[TU_CHAT_CMD]);
            while (*ptr == ' ') {
                ptr++;
            }
            if (*ptr) {
                if (tu_chat(tu, ptr) < 0) {
                    debug("ERROR: Failed to process CHAT command with message: %s", ptr);
                }
            } else {
                debug("ERROR: CHAT command missing message");
            }
        } else {
            debug("ERROR: Unknown command: %s", command);
        }

        // Free the command buffer after processing.
        free(command);
        command = NULL;
    }

    // Cleanup when the client disconnects or input ends.
    fclose(f);
    pbx_unregister(pbx, tu);
    tu_unref(tu, "unregister TU");
    return NULL;
}
#endif
