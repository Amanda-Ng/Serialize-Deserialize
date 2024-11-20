#ifndef COOK_H    // 1. Check if COOK_H is not defined
#define COOK_H    // 2. Define COOK_H to indicate the file has been included

#include "cookbook.h"

// Define a simple queue structure for recipes
typedef struct recipe_queue {
    RECIPE *recipe;
    struct recipe_queue *next;
    pid_t pid;               // Process ID of the cook processing this recipe
} RECIPE_QUEUE;

extern RECIPE_QUEUE *work_queue;
extern RECIPE_QUEUE *dependency_queue;    // Queue for all recipes related to the main recipe

// Function prototypes or other declarations
int parse_arguments(int argc, char *argv[], char **cookbook_filename, int *max_cooks, char **main_recipe_name);
int initialize_work_queue(COOKBOOK *cbp, char *main_recipe_name, RECIPE_QUEUE **queue);
// void process_work_queue(RECIPE_QUEUE **work_queue);
int process_work_queue(RECIPE_QUEUE **work_queue, int max_cooks);
void free_dependency_queue(RECIPE_QUEUE **queue);
void free_cookbook(COOKBOOK *cookbook);

#endif  // 3. End the conditional inclusion
