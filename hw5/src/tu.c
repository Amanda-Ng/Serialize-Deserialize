/*
 * TU: simulates a "telephone unit", which interfaces a client with the PBX.
 */
#include <stdlib.h>
#include <pthread.h>

#include "pbx.h"
#include "debug.h"

// TU structure definition
typedef struct tu {
    int fd;                    // File descriptor for network connection
    FILE *f;                   // Stream for writing to the file descriptor
    int ext;                   // Extension number
    int ref;                   // Reference count
    TU_STATE state;            // Current state of the TU
    pthread_mutex_t mutex;     // Mutex for thread-safe access
    struct tu *peer;           // Peer TU if connected
    int locked;                // Locked state
} TU;

int notify(TU *tu) {
    if (!tu) {
        debug("ERROR: Attempted to notify a NULL TU");
        return -1;
    }

    int status = 0;

    // Send the current state name to the TU client
    if (fputs(tu_state_names[tu->state], tu->f) < 0) {
        status = -1;
    }

    // Add additional information based on the TU state
    if (tu->state == TU_ON_HOOK) {
        // Print the TU's file descriptor when on hook
        if (fprintf(tu->f, " %d", tu->fd) < 0) {
            status = -1;
        }
    } else if (tu->state == TU_CONNECTED) {
        // Print the peer's file descriptor when connected
        if (fprintf(tu->f, " %d", tu->peer->fd) < 0) {
            status = -1;
        }
    }

    // Add line termination and flush the output buffer
    if (fprintf(tu->f, "\r\n") < 0 || fflush(tu->f) < 0) {
        status = -1;
    }

    // Log an error message if any I/O operation failed
    if (status == -1) {
        debug("ERROR: Failed to send notification to client");
    }

    return status;
}

/*
 * Initialize a TU
 *
 * @param fd  The file descriptor of the underlying network connection.
 * @return  The TU, newly initialized and in the TU_ON_HOOK state, if initialization
 * was successful, otherwise NULL.
 */
#if 1
TU *tu_init(int fd) {
    // TO BE IMPLEMENTED
    // abort();
    TU *tu = malloc(sizeof(TU));
    if (tu == NULL) {
        debug("ERROR: Failed to allocate memory for TU");
        return NULL;
    }

    // Initialize TU fields
    tu->fd = fd;
    tu->f = fdopen(fd, "w");
    if (tu->f == NULL) {
        debug("ERROR: Failed to create file stream for TU");
        free(tu);
        return NULL;
    }
    tu->ext = 0;               // Default extension is unassigned
    tu->ref = 1;               // Initial reference count
    tu->state = TU_ON_HOOK;    // Initial state
    tu->peer = NULL;           // No peer connected initially
    tu->locked = 0;
    pthread_mutex_init(&tu->mutex, NULL);

    return tu;
}
#endif

/*
 * Increment the reference count on a TU.
 *
 * @param tu  The TU whose reference count is to be incremented
 * @param reason  A string describing the reason why the count is being incremented
 * (for debugging purposes).
 */
#if 1
void tu_ref(TU *tu, char *reason) {
    // TO BE IMPLEMENTED
    // abort();
    if (tu == NULL) {
        debug("ERROR: Attempted to increment reference count on NULL TU");
        return;
    }

    pthread_mutex_lock(&tu->mutex);
    tu->ref++;
    pthread_mutex_unlock(&tu->mutex);
}
#endif

/*
 * Decrement the reference count on a TU, freeing it if the count becomes 0.
 *
 * @param tu  The TU whose reference count is to be decremented
 * @param reason  A string describing the reason why the count is being decremented
 * (for debugging purposes).
 */
#if 1
void tu_unref(TU *tu, char *reason) {
    // TO BE IMPLEMENTED
    // abort();
    if (tu == NULL) {
        debug("ERROR: Attempted to decrement reference count on NULL TU");
        return;
    }

    pthread_mutex_lock(&tu->mutex);
    tu->ref--;

    if (tu->ref == 0) {
        pthread_mutex_unlock(&tu->mutex);
        pthread_mutex_destroy(&tu->mutex);
        free(tu);
    } else {
        pthread_mutex_unlock(&tu->mutex);
    }
}
#endif

/*
 * Get the file descriptor for the network connection underlying a TU.
 * This file descriptor should only be used by a server to read input from
 * the connection.  Output to the connection must only be performed within
 * the PBX functions.
 *
 * @param tu
 * @return the underlying file descriptor, if any, otherwise -1.
 */
#if 1
int tu_fileno(TU *tu) {
    // TO BE IMPLEMENTED
    // abort();
    if (tu == NULL) {
        debug("ERROR: Attempted to access file descriptor of a NULL TU");
        return -1;
    }
    pthread_mutex_lock(&tu->mutex);
    int fd = tu->fd;
    pthread_mutex_unlock(&tu->mutex);
    return fd;
}
#endif

/*
 * Get the extension number for a TU.
 * This extension number is assigned by the PBX when a TU is registered
 * and it is used to identify a particular TU in calls to tu_dial().
 * The value returned might be the same as the value returned by tu_fileno(),
 * but is not necessarily so.
 *
 * @param tu
 * @return the extension number, if any, otherwise -1.
 */
#if 1
int tu_extension(TU *tu) {
    // TO BE IMPLEMENTED
    // abort();
    if (tu == NULL) {
        debug("ERROR: Attempted to access extension of a NULL TU");
        return -1;
    }
    pthread_mutex_lock(&tu->mutex);
    int ext = tu->ext;
    pthread_mutex_unlock(&tu->mutex);
    return ext;
}
#endif

/*
 * Set the extension number for a TU.
 * A notification is set to the client of the TU.
 * This function should be called at most once one any particular TU.
 *
 * @param tu  The TU whose extension is being set.
 */
#if 1
int tu_set_extension(TU *tu, int ext) {
    // TO BE IMPLEMENTED
    // abort();
    if (tu == NULL) {
        debug("ERROR: Attempted to set extension on a NULL TU");
        return -1;
    }
    pthread_mutex_lock(&tu->mutex);
    if (tu->ext != 0) {
        debug("ERROR: Extension has already been set for this TU");
        pthread_mutex_unlock(&tu->mutex);
        return -1;
    }

    tu->ext = ext;

    // pthread_mutex_unlock(&tu->mutex);
    // return 0;
    int status = notify(tu);
    pthread_mutex_unlock(&tu->mutex);
    return status;
}
#endif

/*
 * Initiate a call from a specified originating TU to a specified target TU.
 *   If the originating TU is not in the TU_DIAL_TONE state, then there is no effect.
 *   If the target TU is the same as the originating TU, then the TU transitions
 *     to the TU_BUSY_SIGNAL state.
 *   If the target TU already has a peer, or the target TU is not in the TU_ON_HOOK
 *     state, then the originating TU transitions to the TU_BUSY_SIGNAL state.
 *   Otherwise, the originating TU and the target TU are recorded as peers of each other
 *     (this causes the reference count of each of them to be incremented),
 *     the target TU transitions to the TU_RINGING state, and the originating TU
 *     transitions to the TU_RING_BACK state.
 *
 * In all cases, a notification of the resulting state of the originating TU is sent to
 * to the associated network client.  If the target TU has changed state, then its client
 * is also notified of its new state.
 *
 * If the caller of this function was unable to determine a target TU to be called,
 * it will pass NULL as the target TU.  In this case, the originating TU will transition
 * to the TU_ERROR state if it was in the TU_DIAL_TONE state, and there will be no
 * effect otherwise.  This situation is handled here, rather than in the caller,
 * because here we have knowledge of the current TU state and we do not want to introduce
 * the possibility of transitions to a TU_ERROR state from arbitrary other states,
 * especially in states where there could be a peer TU that would have to be dealt with.
 *
 * @param tu  The originating TU.
 * @param target  The target TU, or NULL if the caller of this function was unable to
 * identify a TU to be dialed.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_dial(TU *tu, TU *target) {
    // TO BE IMPLEMENTED
    // abort();
    if (tu == NULL) {
        debug("ERROR: Originating TU is NULL");
        return -1;
    }

    pthread_mutex_lock(&tu->mutex);

    if (tu->peer)
    {
        debug("ERROR: Client is already in chat");
        notify(tu);
        pthread_mutex_unlock(&tu->mutex);
        return -1;
    }

    // Check if the originating TU is in the correct state
    if (tu->state != TU_DIAL_TONE) {
        debug("ERROR: Originating TU is not in TU_DIAL_TONE state");
        notify(tu);  // Notify client of the current state
        pthread_mutex_unlock(&tu->mutex);
        // return 0;  // No effect
        return -1;
    }


    if (target)
    {
        if (tu == target)
        {
            debug("ERROR: calling it self");
            tu->state = TU_BUSY_SIGNAL;
            int status = notify(tu);
            pthread_mutex_unlock(&tu->mutex);
            return status;
        }
        if (tu->ext < target->ext)
        {
            pthread_mutex_unlock(&tu->mutex);
            pthread_mutex_lock(&tu->mutex);
            pthread_mutex_lock(&target->mutex);
        }
        else {
            pthread_mutex_unlock(&tu->mutex);
            pthread_mutex_lock(&target->mutex);
            pthread_mutex_lock(&tu->mutex);
        }
        if (target->peer)
        {
            debug("ERROR: target is already in chat");
            tu->state = TU_BUSY_SIGNAL;
            int status = notify(tu);
            pthread_mutex_unlock(&tu->mutex);
            pthread_mutex_unlock(&target->mutex);
            return status;
        }
        if (target->state != TU_ON_HOOK)
        {
            debug("ERROR: target not in on hook state");
            tu->state = TU_BUSY_SIGNAL;
            int status = notify(tu);
            pthread_mutex_unlock(&tu->mutex);
            pthread_mutex_unlock(&target->mutex);
            return status;
        }

        tu->ref++;
        target->ref++;
        tu->peer = target;
        target->peer = tu;
        tu->state = TU_RING_BACK;
        target->state = TU_RINGING;
        int status = 0;
        if (notify(tu))
            status = -1;
        if (notify(target))
            status = -1;
        pthread_mutex_unlock(&tu->mutex);
        pthread_mutex_unlock(&target->mutex);
        return status;
    }
    else {
        debug("ERROR: cannot find target");
        tu->state = TU_ERROR;
        int status = notify(tu);
        pthread_mutex_unlock(&tu->mutex);
        return status;
    }
}
#endif

/*
 * Take a TU receiver off-hook (i.e. pick up the handset).
 *   If the TU is in neither the TU_ON_HOOK state nor the TU_RINGING state,
 *     then there is no effect.
 *   If the TU is in the TU_ON_HOOK state, it goes to the TU_DIAL_TONE state.
 *   If the TU was in the TU_RINGING state, it goes to the TU_CONNECTED state,
 *     reflecting an answered call.  In this case, the calling TU simultaneously
 *     also transitions to the TU_CONNECTED state.
 *
 * In all cases, a notification of the resulting state of the specified TU is sent to
 * to the associated network client.  If a peer TU has changed state, then its client
 * is also notified of its new state.
 *
 * @param tu  The TU that is to be picked up.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_pickup(TU *tu) {
    // TO BE IMPLEMENTED
    // abort();
    if (!tu) {
        debug("ERROR: TU is NULL");
        return -1;
    }

    // Lock the TU for thread safety.
    while (1) {
        // Lock the TU mutex to start thread-safe operations.
        pthread_mutex_lock(&tu->mutex);

        // Temporarily store the peer pointer.
        TU *tmp = tu->peer;

        // If no peer exists, we're done; return successfully.
        if (!tmp)
            break;

        // Determine locking order based on extension number to avoid deadlocks.
        if (tu->ext < tmp->ext) {
            // Unlock the current mutex and lock both in the correct order.
            pthread_mutex_unlock(&tu->mutex);
            pthread_mutex_lock(&tu->mutex);
            pthread_mutex_lock(&tmp->mutex);
            tu->locked = 1;
        } else {
            // Reverse the order for consistency based on extension comparison.
            pthread_mutex_unlock(&tu->mutex);
            pthread_mutex_lock(&tmp->mutex);
            pthread_mutex_lock(&tu->mutex);
            tu->locked = 1;
        }

        // Verify the peer hasn't changed during the locking process.
        if (tu->peer == tmp)
            break;

        // If the peer changed, unlock both mutexes and retry.
        pthread_mutex_unlock(&tmp->mutex);
        pthread_mutex_unlock(&tu->mutex);
    }

    if (tu->state == TU_ON_HOOK) {
        // Transition from ON_HOOK to DIAL_TONE.
        tu->state = TU_DIAL_TONE;
        int status = notify(tu);

        // If the TU has a peer, unlock its mutex first to maintain consistency.
        if (tu->locked) {
            pthread_mutex_unlock(&tu->peer->mutex);
            tu->locked = 0;
        }
        // Unlock the TU mutex.
        pthread_mutex_unlock(&tu->mutex);

        return status;
    }

    if (tu->state == TU_RINGING) {
        // Handle the case where TU is RINGING.
        if (tu->peer) {
            if (tu->peer->state == TU_RING_BACK && tu == tu->peer->peer) {
                // Transition both TUs to CONNECTED.
                tu->state = TU_CONNECTED;
                tu->peer->state = TU_CONNECTED;

                // Notify both TUs of the state change.
                int status = 0;
                if (notify(tu)) status = -1;
                if (notify(tu->peer)) status = -1;

                // If the TU has a peer, unlock its mutex first to maintain consistency.
                if (tu->locked) {
                    pthread_mutex_unlock(&tu->peer->mutex);
                    tu->locked = 0;
                }
                // Unlock the TU mutex.
                pthread_mutex_unlock(&tu->mutex);

                return status;
            }

            // TU has a peer, but the peer is not in a valid state.
            debug("ERROR: TU_RINGING with invalid peer state");
            notify(tu);
            // If the TU has a peer, unlock its mutex first to maintain consistency.
            if (tu->locked) {
                pthread_mutex_unlock(&tu->peer->mutex);
                tu->locked = 0;
            }
            // Unlock the TU mutex.
            pthread_mutex_unlock(&tu->mutex);

            return -1;
        } else {
            // TU is RINGING but has no peer.
            debug("ERROR: TU_RINGING but no peer");
            // If the TU has a peer, unlock its mutex first to maintain consistency.
            if (tu->locked) {
                pthread_mutex_unlock(&tu->peer->mutex);
                tu->locked = 0;
            }
            // Unlock the TU mutex.
            pthread_mutex_unlock(&tu->mutex);

            return -1;
        }
    }

    // No state transition occurred.
    debug("ERROR: TU_PICKUP called in invalid state");
    notify(tu);
    // If the TU has a peer, unlock its mutex first to maintain consistency.
    if (tu->locked) {
        pthread_mutex_unlock(&tu->peer->mutex);
        tu->locked = 0;
    }
    // Unlock the TU mutex.
    pthread_mutex_unlock(&tu->mutex);

    return -1;
}
#endif

/*
 * Hang up a TU (i.e. replace the handset on the switchhook).
 *
 *   If the TU is in the TU_CONNECTED or TU_RINGING state, then it goes to the
 *     TU_ON_HOOK state.  In addition, in this case the peer TU (the one to which
 *     the call is currently connected) simultaneously transitions to the TU_DIAL_TONE
 *     state.
 *   If the TU was in the TU_RING_BACK state, then it goes to the TU_ON_HOOK state.
 *     In addition, in this case the calling TU (which is in the TU_RINGING state)
 *     simultaneously transitions to the TU_ON_HOOK state.
 *   If the TU was in the TU_DIAL_TONE, TU_BUSY_SIGNAL, or TU_ERROR state,
 *     then it goes to the TU_ON_HOOK state.
 *
 * In all cases, a notification of the resulting state of the specified TU is sent to
 * to the associated network client.  If a peer TU has changed state, then its client
 * is also notified of its new state.
 *
 * @param tu  The tu that is to be hung up.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_hangup(TU *tu) {
    // TO BE IMPLEMENTED
    // abort();
    if (!tu) {
        debug("ERROR: tu does not exist");
        return -1;
    }

    while (1) {
        // Lock the TU mutex to start thread-safe operations.
        pthread_mutex_lock(&tu->mutex);

        // Temporarily store the peer pointer.
        TU *tmp = tu->peer;

        // If no peer exists, we're done; return successfully.
        if (!tmp)
            break;

        // Determine locking order based on extension number to avoid deadlocks.
        if (tu->ext < tmp->ext) {
            // Unlock the current mutex and lock both in the correct order.
            pthread_mutex_unlock(&tu->mutex);
            pthread_mutex_lock(&tu->mutex);
            pthread_mutex_lock(&tmp->mutex);
            tu->locked = 1;
        } else {
            // Reverse the order for consistency based on extension comparison.
            pthread_mutex_unlock(&tu->mutex);
            pthread_mutex_lock(&tmp->mutex);
            pthread_mutex_lock(&tu->mutex);
            tu->locked = 1;
        }

        // Verify the peer hasn't changed during the locking process.
        if (tu->peer == tmp)
            break;

        // If the peer changed, unlock both mutexes and retry.
        pthread_mutex_unlock(&tmp->mutex);
        pthread_mutex_unlock(&tu->mutex);
    }

    // If the TU is already on hook, no action needed.
    if (tu->state == TU_ON_HOOK) {
        int status = notify(tu);
        // If the TU has a peer, unlock its mutex first to maintain consistency.
        if (tu->locked) {
            pthread_mutex_unlock(&tu->peer->mutex);
            tu->locked = 0;
        }
        // Unlock the TU mutex.
        pthread_mutex_unlock(&tu->mutex);

        return status;
    }

    // If the TU is in the TU_CONNECTED or TU_RINGING state, disconnect the call.
    if (tu->state == TU_CONNECTED || tu->state == TU_RINGING) {
        if (tu->peer) {
            // Transition the TU and peer to the appropriate states.
            tu->state = TU_ON_HOOK;
            tu->peer->state = TU_DIAL_TONE;

            // Notify the TU and peer of the state change.
            int status = 0;
            if (notify(tu)) {
                status = -1;
            }
            if (notify(tu->peer)) {
                status = -1;
            }

            // Clean up references and destroy the peer if necessary.
            tu->peer->peer = NULL;
            tu->peer->ref--;
            if (tu->peer->ref <= 0) {
                pthread_mutex_unlock(&(tu->peer)->mutex);
                pthread_mutex_destroy(&(tu->peer)->mutex);
                free(tu->peer);
            } else {
                pthread_mutex_unlock(&tu->peer->mutex);
            }

            tu->peer = NULL;
            tu->ref--;
            tu->locked = 0;

            // Destroy the TU if it's no longer referenced, otherwise unlock.
            if (tu->ref == 0) {
                pthread_mutex_unlock(&tu->mutex);
                pthread_mutex_destroy(&tu->mutex);
                free(tu);
            } else {
                pthread_mutex_unlock(&tu->mutex);
            }

            return status;
        } else {
            debug("ERROR: TU is connected or ringing, but no peer found");
            tu->state = TU_ON_HOOK;
            notify(tu);
            // If the TU has a peer, unlock its mutex first to maintain consistency.
            if (tu->locked) {
                pthread_mutex_unlock(&tu->peer->mutex);
                tu->locked = 0;
            }
            // Unlock the TU mutex.
            pthread_mutex_unlock(&tu->mutex);

            return -1;
        }
    }

    // If the TU is in the TU_RING_BACK state, handle the calling TU and its peer.
    if (tu->state == TU_RING_BACK) {
        if (tu->peer) {
            // Transition both TU and peer to the TU_ON_HOOK state.
            tu->state = TU_ON_HOOK;
            tu->peer->state = TU_ON_HOOK;

            // Notify both TU and peer.
            int status = 0;
            if (notify(tu)) {
                status = -1;
            }
            if (notify(tu->peer)) {
                status = -1;
            }

            // Clean up references and destroy the peer if necessary.
            tu->peer->peer = NULL;
            tu->peer->ref--;
            if (tu->peer->ref <= 0) {
                pthread_mutex_unlock(&(tu->peer)->mutex);
                pthread_mutex_destroy(&(tu->peer)->mutex);
                free(tu->peer);
            } else {
                pthread_mutex_unlock(&tu->peer->mutex);
            }

            tu->peer = NULL;
            tu->ref--;
            tu->locked = 0;

            if (tu->ref == 0) {
                pthread_mutex_unlock(&tu->mutex);
                pthread_mutex_destroy(&tu->mutex);
                free(tu);
            } else {
                pthread_mutex_unlock(&tu->mutex);
            }

            return status;
        } else {
            debug("ERROR: TU is ringing back, but no peer found");
            tu->state = TU_ON_HOOK;
            notify(tu);
            // If the TU has a peer, unlock its mutex first to maintain consistency.
            if (tu->locked) {
                pthread_mutex_unlock(&tu->peer->mutex);
                tu->locked = 0;
            }
            // Unlock the TU mutex.
            pthread_mutex_unlock(&tu->mutex);

            return -1;
        }
    }

    if (tu->peer) {
        tu->state = TU_ON_HOOK;
        notify(tu);
        // If the TU has a peer, unlock its mutex first to maintain consistency.
        if (tu->locked) {
            pthread_mutex_unlock(&tu->peer->mutex);
            tu->locked = 0;
        }
        // Unlock the TU mutex.
        pthread_mutex_unlock(&tu->mutex);

        return -1;
    }

    tu->state = TU_ON_HOOK;
    int status = notify(tu);
    // If the TU has a peer, unlock its mutex first to maintain consistency.
    if (tu->locked) {
        pthread_mutex_unlock(&tu->peer->mutex);
        tu->locked = 0;
    }
    // Unlock the TU mutex.
    pthread_mutex_unlock(&tu->mutex);

    return status;
}
#endif

/*
 * "Chat" over a connection.
 *
 * If the state of the TU is not TU_CONNECTED, then nothing is sent and -1 is returned.
 * Otherwise, the specified message is sent via the network connection to the peer TU.
 * In all cases, the states of the TUs are left unchanged and a notification containing
 * the current state is sent to the TU sending the chat.
 *
 * @param tu  The tu sending the chat.
 * @param msg  The message to be sent.
 * @return 0  If the chat was successfully sent, -1 if there is no call in progress
 * or some other error occurs.
 */
#if 1
int tu_chat(TU *tu, char *msg) {
    // TO BE IMPLEMENTED
    // abort();
    // Lock the TU to ensure thread-safety.
    pthread_mutex_lock(&tu->mutex);

    // Check if the TU is in the CONNECTED state.
    if (tu->state == TU_CONNECTED) {
        // Get the peer TU (the peer).
        TU *target = tu->peer;

        if (target) {
            // Ensure mutual locking order based on extension numbers to avoid deadlocks.
            if (tu->ext < target->ext) {
                pthread_mutex_unlock(&tu->mutex);
                pthread_mutex_lock(&tu->mutex);
                pthread_mutex_lock(&target->mutex);
            } else {
                pthread_mutex_unlock(&tu->mutex);
                pthread_mutex_lock(&target->mutex);
                pthread_mutex_lock(&tu->mutex);
            }

            // Check if both TUs are still connected and that the connection is mutual.
            if (tu->peer == target && target->peer == tu &&
                tu->state == TU_CONNECTED && target->state == TU_CONNECTED) {
                // Send the message to the peer TU.
                int status = 0;
                if (fprintf(target->f, "CHAT %s\r\n", msg) < 0 || fflush(target->f) < 0) {
                    status = -1;
                }

                // Notify both TUs of the current state.
                if (notify(tu) < 0) {
                    status = -1;
                }

                // Unlock the mutexes for both TUs after the operation.
                pthread_mutex_unlock(&target->mutex);
                pthread_mutex_unlock(&tu->mutex);

                return status;

            } else {
                // Handle the case where the target has changed or the TUs do not match.
                debug("ERROR: target changed or locked do not match");
                notify(tu);

                pthread_mutex_unlock(&target->mutex);
                pthread_mutex_unlock(&tu->mutex);
                return -1;
            }
        } else {
            // Handle the case where there is no peer (peer is NULL).
            debug("ERROR: connected but no peer");
            notify(tu);
            pthread_mutex_unlock(&tu->mutex);
            return -1;
        }
    } else {
        // Handle the case where the TU is not in the CONNECTED state.
        debug("ERROR: not in connected state");
        notify(tu);
        pthread_mutex_unlock(&tu->mutex);
        return -1;
    }
}
#endif
