#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    // int ret;
    // if(validargs(argc, argv))
    //     USAGE(*argv, EXIT_FAILURE);
    // if(global_options & 0x1)
    //     USAGE(*argv, EXIT_SUCCESS);

    // return EXIT_SUCCESS;

    int ret = validargs(argc, argv);

    if (ret == -1) {
        USAGE(*argv, EXIT_FAILURE);  // Print usage message and exit with failure status
        return EXIT_FAILURE;         // This line will not be reached but ensures proper exit status
    } else if (global_options & 0x1) {
        USAGE(*argv, EXIT_SUCCESS);  // Print usage message and exit with success status for -h flag
        return EXIT_SUCCESS;         // This line will not be reached but ensures proper exit status
    } else {
        // Perform serialization or deserialization
        if (global_options & 0x2) {
            // Perform serialization
            if (serialize()) {
                return EXIT_FAILURE;  // Return failure status if serialization fails
            }
        } else if (global_options & 0x4) {
            // Perform deserialization
            if (deserialize()) {
                return EXIT_FAILURE;  // Return failure status if deserialization fails
            }
        }
        // If you reach this point, the operation was successful
        return EXIT_SUCCESS;
    }
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
