/*
 * PBX: simulates a Private Branch Exchange.
 */
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>

#include "pbx.h"
#include "debug.h"

/*
 * PBX structure definition.
 * This structure will hold the TUs and synchronization primitives.
 */
typedef struct pbx {
    pthread_mutex_t mutex;                // Mutex for thread synchronization
    TU *tus[PBX_MAX_EXTENSIONS];          // Array to store registered TUs
    volatile int active_threads;          // Count of active threads
    pthread_cond_t shutdown_cond;         // Condition variable for shutdown
} PBX;


/*
 * Initialize a new PBX.
 *
 * @return the newly initialized PBX, or NULL if initialization fails.
 */
#if 1
PBX *pbx_init() {
    // TO BE IMPLEMENTED
    // abort();
    // Allocate memory for PBX structure
    PBX *pbx = malloc(sizeof(PBX));
    if (!pbx) {
        debug("ERROR: Failed to allocate memory for PBX.");
        return NULL;
    }

    // Initialize the TUs array to NULL
    for (int i = 0; i < PBX_MAX_EXTENSIONS; i++) {
        pbx->tus[i] = NULL;
    }

    // Initialize the mutex for synchronization
    if (pthread_mutex_init(&pbx->mutex, NULL) != 0) {
        debug("ERROR: Failed to initialize PBX mutex.");
        free(pbx);
        return NULL;
    }

    // PBX initialized successfully
    return pbx;
}
#endif

/*
 * Shut down a pbx, shutting down all network connections, waiting for all server
 * threads to terminate, and freeing all associated resources.
 * If there are any registered extensions, the associated network connections are
 * shut down, which will cause the server threads to terminate.
 * Once all the server threads have terminated, any remaining resources associated
 * with the PBX are freed.  The PBX object itself is freed, and should not be used again.
 *
 * @param pbx  The PBX to be shut down.
 */
#if 1
void pbx_shutdown(PBX *pbx) {
    // TO BE IMPLEMENTED
    // abort();
    if (!pbx) {
        debug("ERROR: Attempt to shutdown a NULL PBX.");
        return;
    }

    pthread_mutex_lock(&pbx->mutex);

    // Step 1: Shut down all registered TUs
    for (int i = 0; i < PBX_MAX_EXTENSIONS; i++) {
        if (pbx->tus[i]) {
            // Hang up the TU and release it
            tu_hangup(pbx->tus[i]);
            // Shut down the network connection for the TU
            shutdown(tu_fileno(pbx->tus[i]), SHUT_RD);
            tu_unref(pbx->tus[i], "Shutting down TU as part of PBX shutdown");

            // Remove it from the PBX registry
            pbx->tus[i] = NULL;
        }
    }

    pthread_mutex_unlock(&pbx->mutex);

    // Step 3: Clean up synchronization primitives
    pthread_mutex_destroy(&pbx->mutex);

    // Step 4: Free the PBX structure itself
    free(pbx);
}
#endif

/*
 * Register a telephone unit with a PBX at a specified extension number.
 * This amounts to "plugging a telephone unit into the PBX".
 * The TU is initialized to the TU_ON_HOOK state.
 * The reference count of the TU is increased and the PBX retains this reference
 *for as long as the TU remains registered.
 * A notification of the assigned extension number is sent to the underlying network
 * client.
 *
 * @param pbx  The PBX registry.
 * @param tu  The TU to be registered.
 * @param ext  The extension number on which the TU is to be registered.
 * @return 0 if registration succeeds, otherwise -1.
 */
#if 1
int pbx_register(PBX *pbx, TU *tu, int ext) {
    // TO BE IMPLEMENTED
    // abort();
    if (!pbx || !tu) {
        debug("ERROR: pbx or tu is NULL");
        return -1;
    }
    if (ext < 1 || ext > PBX_MAX_EXTENSIONS) {
        debug("ERROR: Extension out of range");
        return -1;
    }

    pthread_mutex_lock(&pbx->mutex);

    if (pbx->tus[ext]) {
        debug("ERROR: Extension %d is already occupied", ext);
        pthread_mutex_unlock(&pbx->mutex);
        return -1;
    }

    // Register the TU
    tu_ref(tu, "Registering TU to PBX");
    tu_set_extension(tu, ext);
    // pbx->tus[ext - 1] = tu;
    pbx->tus[ext] = tu;

    pthread_mutex_unlock(&pbx->mutex);
    return 0;
}
#endif

/*
 * Unregister a TU from a PBX.
 * This amounts to "unplugging a telephone unit from the PBX".
 * The TU is disassociated from its extension number.
 * Then a hangup operation is performed on the TU to cancel any
 * call that might be in progress.
 * Finally, the reference held by the PBX to the TU is released.
 *
 * @param pbx  The PBX.
 * @param tu  The TU to be unregistered.
 * @return 0 if unregistration succeeds, otherwise -1.
 */
#if 1
int pbx_unregister(PBX *pbx, TU *tu) {
    // TO BE IMPLEMENTED
    // abort();
    if (!pbx || !tu) {
        debug("ERROR: pbx or tu is NULL");
        return -1;
    }

    pthread_mutex_lock(&pbx->mutex);

    int ext = tu_extension(tu);
    if (ext < 1 || ext > PBX_MAX_EXTENSIONS +1 || !pbx->tus[ext]) {
        debug("ERROR: TU not found at extension %d", ext);
        pthread_mutex_unlock(&pbx->mutex);
        return -1;
    }

    // Hang up the TU to cancel any active calls
    tu_hangup(tu);

    // Shut down the network connection associated with the TU
    shutdown(tu_fileno(tu), SHUT_RD);

    // Release the reference held by the PBX
    tu_unref(tu, "Unregistering TU from PBX");

    // Remove the TU from the registry
    pbx->tus[ext] = NULL;

    pthread_mutex_unlock(&pbx->mutex);
    return 0;
}
#endif

/*
 * Use the PBX to initiate a call from a specified TU to a specified extension.
 *
 * @param pbx  The PBX registry.
 * @param tu  The TU that is initiating the call.
 * @param ext  The extension number to be called.
 * @return 0 if dialing succeeds, otherwise -1.
 */
#if 1
int pbx_dial(PBX *pbx, TU *tu, int ext) {
    // TO BE IMPLEMENTED
    // abort();
    if (!pbx || !tu) {
        debug("ERROR: pbx or tu is NULL");
        return -1;
    }
    if (ext < 0 || ext > PBX_MAX_EXTENSIONS) {
        debug("ERROR: Extension out of range");
        return -1;
    }

    pthread_mutex_lock(&pbx->mutex);

    // Attempt to initiate a call
    int status = tu_dial(tu, pbx->tus[ext]);

    pthread_mutex_unlock(&pbx->mutex);
    return status;
}
#endif
