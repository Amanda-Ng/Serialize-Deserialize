#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cookbook.h"
#include "cook.h"
#include "debug.h"

int main(int argc, char *argv[]) {
    COOKBOOK *cbp;
    int err = 0;
    char *cookbook = "rsrc/cookbook.ckb";
    FILE *in;
    char *main_recipe_name = NULL;
    int max_cooks = 1;
    // RECIPE_QUEUE *work_queue = NULL;

    // Parse the command-line arguments
    int parse_status = parse_arguments(argc, argv, &cookbook, &max_cooks, &main_recipe_name);
    if (parse_status == -1) {
        exit(EXIT_FAILURE);  // Exit with failure status
    }

    if((in = fopen(cookbook, "r")) == NULL) {
	fprintf(stderr, "Can't open cookbook '%s': %s\n", cookbook, strerror(errno));
	exit(1);
    }
    cbp = parse_cookbook(in, &err);
    fclose(in);  // Close the file after parsing
    if(err) {
	fprintf(stderr, "Error parsing cookbook '%s'\n", cookbook);
	exit(1);
    }

    // If main_recipe_name is NULL, set it to the first recipe's name
    if (main_recipe_name == NULL && cbp->recipes != NULL) {
        main_recipe_name = cbp->recipes->name;
    }

    // Initialize the work queue
    int init_status = initialize_work_queue(cbp, main_recipe_name, &work_queue);
    if (init_status == -1) {
        free_cookbook(cbp);
        exit(EXIT_FAILURE);  // Exit if initialization fails
    }

    if(process_work_queue(&work_queue, max_cooks) == 0){
        free_dependency_queue(&dependency_queue);
        free_cookbook(cbp);
        exit(EXIT_SUCCESS);
    } else {
        free_dependency_queue(&dependency_queue);
        free_cookbook(cbp);
        exit(EXIT_FAILURE);
    }

    // unparse_cookbook(cbp, stdout);
    exit(0);
}
