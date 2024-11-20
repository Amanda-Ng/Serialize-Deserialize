#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "debug.h"
#include "cookbook.h"
#include "cook.h"

#define DEFAULT_COOKBOOK "cookbook.ckb"
#define DEFAULT_MAX_COOKS 1

volatile int active_cooks = 0;
RECIPE_QUEUE *work_queue = NULL;
RECIPE_QUEUE *dependency_queue = NULL;

// Helper function to add a recipe to the queue
int enqueue(RECIPE_QUEUE **queue, RECIPE *recipe) {
    RECIPE_QUEUE *new_node = malloc(sizeof(RECIPE_QUEUE));
    if (!new_node) {
        // malloc
        return -1;
    }
    new_node->recipe = recipe;
    new_node->next = NULL;
    new_node->pid = -1;

    if (*queue == NULL) {
        *queue = new_node;
    } else {
        RECIPE_QUEUE *current = *queue;
        while (current->next) {
            current = current->next;
        }
        current->next = new_node;
    }
    return 0;
}

// Helper function to dequeue a recipe from the queue
RECIPE *dequeue(RECIPE_QUEUE **queue) {
    if (*queue == NULL) return NULL;

    RECIPE_QUEUE *front = *queue;
    RECIPE *recipe = front->recipe;
    *queue = front->next;
    free(front);
    return recipe;
}

RECIPE *find_recipe(COOKBOOK *cookbook, const char *name) {
    RECIPE *current_recipe = cookbook->recipes;
    while (current_recipe != NULL) {
        if (strcmp(current_recipe->name, name) == 0) {
            return current_recipe;
        }
        current_recipe = current_recipe->next;
    }
    return NULL; // Recipe not found
}

int find_leaf_recipes(RECIPE *recipe, RECIPE_QUEUE **queue) {
    // If the recipe has already been visited, return
    if (recipe->state != NULL) {
        return 0;
    }

    // Mark this recipe as visited
    recipe->state = (void *)1;

    // Check if the recipe has dependencies (sub-recipes)
    if (recipe->this_depends_on == NULL) {
        // No dependencies, it's a leaf recipe
        if (enqueue(queue, recipe) != 0) {
            return -1;  // Error in enqueueing
        }
    } else {
        // Recurse into each dependency
        RECIPE_LINK *link = recipe->this_depends_on;
        while (link != NULL) {
            if (link->recipe != NULL) {
                if (find_leaf_recipes(link->recipe, queue) != 0) {
                    return -1;  // Error in recursion
                }            }
            link = link->next;
        }
    }
    return 0;  // Success
}

int reset_subrecipe_states(RECIPE *recipe) {
    if (recipe == NULL || recipe->state == NULL) {
        // Recipe is NULL or already reset; no need to process
        return 0;
    }

    // Reset the state field of the current recipe
    recipe->state = NULL;

    // Recurse into sub-recipes
    RECIPE_LINK *link = recipe->this_depends_on;
    while (link != NULL) {
        if (link->recipe != NULL) {
            if (reset_subrecipe_states(link->recipe) != 0) {
                return -1;  // Error in recursion
            }        }
        link = link->next;
    }
    return 0;  // Success
}

int parse_arguments(int argc, char *argv[], char **cookbook_filename, int *max_cooks, char **main_recipe_name) {
    // *cookbook_filename = DEFAULT_COOKBOOK;
    // *max_cooks = DEFAULT_MAX_COOKS;
    // *main_recipe_name = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                *cookbook_filename = argv[i + 1];
                i++;  // Skip next argument since it's the filename
            } else {
                fprintf(stderr, "Error: -f flag requires a filename argument.\n");
                return -1;  // Error
            }
        } else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) {
                *max_cooks = atoi(argv[i + 1]);
                if (*max_cooks <= 0) {
                    fprintf(stderr, "Error: Invalid value for -c, must be a positive integer.\n");
                    return -1;  // Error
                }
                i++;  // Skip next argument since it's the max_cooks value
            } else {
                fprintf(stderr, "Error: -c flag requires a number argument.\n");
                return -1;  // Error
            }
        } else if (i == argc - 1) {
            *main_recipe_name = argv[i];
        } else {
            fprintf(stderr, "Error: Unexpected argument '%s'.\n", argv[i]);
            return -1;  // Error
        }
    }
    return 0;  // Success
}

int find_all_related_recipes(RECIPE *recipe, RECIPE_QUEUE **queue) {
    if (recipe == NULL) {
        return 0;
    }

    // Check if the recipe is already in the queue to avoid duplicates
    RECIPE_QUEUE *current = *queue;
    while (current != NULL) {
        if (current->recipe == recipe) {
            return 0; // Recipe already added
        }
        current = current->next;
    }

    // Add the recipe to the queue
    RECIPE_QUEUE *new_node = malloc(sizeof(RECIPE_QUEUE));
    if (!new_node) {
        // malloc
        return -1;
    }
    new_node->recipe = recipe;
    new_node->next = *queue;
    new_node->pid = -1;
    *queue = new_node;

    // Recursively add all dependencies of this recipe
    RECIPE_LINK *link = recipe->this_depends_on;
    while (link != NULL) {
        if (find_all_related_recipes(link->recipe, queue) != 0) {
            return -1;  // Error in recursion
        }
        link = link->next;
    }
    return 0;  // Success
}

void free_dependency_queue(RECIPE_QUEUE **queue) {
    RECIPE_QUEUE *current = *queue;
    while (current != NULL) {
        RECIPE_QUEUE *temp = current;
        current = current->next;
        free(temp);
    }
    *queue = NULL; // Ensure the queue is fully cleared
}

// Function to free a single step
void free_step(STEP *step) {
    while (step != NULL) {
        STEP *next_step = step->next;

        // Free the array of words
        if (step->words != NULL) {
            for (char **word = step->words; *word != NULL; word++) {
                free(*word);
            }
            free(step->words);
        }

        free(step);
        step = next_step;
    }
}

// Function to free tasks in a recipe
void free_task(TASK *task) {
    while (task != NULL) {
        TASK *next_task = task->next;

        // Free input and output file names
        free(task->input_file);
        free(task->output_file);

        // Free steps in the task
        free_step(task->steps);

        free(task);
        task = next_task;
    }
}

// Function to free recipe links
void free_recipe_links(RECIPE_LINK *link) {
    while (link != NULL) {
        RECIPE_LINK *next_link = link->next;

        // Free the link name
        free(link->name);

        free(link);
        link = next_link;
    }
}

// Function to free all recipes in a cookbook
void free_recipes(RECIPE *recipe) {
    while (recipe != NULL) {
        RECIPE *next_recipe = recipe->next;

        // Free the recipe name
        free(recipe->name);

        // Free the dependency lists
        free_recipe_links(recipe->this_depends_on);
        free_recipe_links(recipe->depend_on_this);

        // Free tasks in the recipe
        free_task(recipe->tasks);

        // // Free the recipe state
        // free(recipe->state);
        // Only free state if it is not NULL

        free(recipe);
        recipe = next_recipe;
    }
}

// Function to free the entire cookbook
void free_cookbook(COOKBOOK *cookbook) {
    if (cookbook == NULL) {
        return;
    }

    // Free the list of recipes
    free_recipes(cookbook->recipes);

    // Free the cookbook state
    free(cookbook->state);

    // Free the cookbook itself
    free(cookbook);
}

int initialize_work_queue(COOKBOOK *cookbook, char *main_recipe_name, RECIPE_QUEUE **work_queue) {
    // Find the main recipe
    RECIPE *main_recipe = find_recipe(cookbook, main_recipe_name);
    if (main_recipe == NULL) {
        return -1;
    }

    // Reset states of all dependent recipes starting from the main recipe
    if (reset_subrecipe_states(main_recipe) != 0) {
        return -1;  // Error in resetting sub-recipes
    }

    // Initialize the dependency queue
    if (find_all_related_recipes(main_recipe, &dependency_queue) != 0) {
        return -1;  // Error in finding related recipes
    }

    // Find all leaf recipes and enqueue them into the work queue
    if (find_leaf_recipes(main_recipe, work_queue) != 0) {
        return -1;  // Error in finding leaf recipes
    }

    if (reset_subrecipe_states(main_recipe) != 0) {
        return -1;  // Error in resetting sub-recipes after processing
    }
    return 0;  // Success
}

// Process a single step (e.g., running a command in a pipeline)
int process_step(STEP *step) {
    if (step == NULL || step->words == NULL || step->words[0] == NULL) {
        return 0; // No step or no words
    }

    // The command (first word)
    char *cmd = step->words[0];
    char util_cmd[256]; // Buffer to hold the path to the command in util/

    // Create the path to the command in util/
    snprintf(util_cmd, sizeof(util_cmd), "./util/%s", cmd);

    // Try executing the command from the util/ subdirectory
    if (execvp(util_cmd, step->words) == -1) {
        // If it fails, try executing the command normally using the system path
        if (execvp(cmd, step->words) == -1) {
            return -1; // Execvp failed
        }
    }

    return 0; // Command executed successfully (if we get here, execvp was successful)
}

// Process a single task, which involves steps
int process_task(TASK *task, RECIPE_QUEUE *recipe_queue) {
    if (task == NULL) {
        return -1; // Invalid task
    }

    STEP *step = task->steps;
    int pipe_fds[2];
    int sync_fds[2];   // Synchronization pipe
    int prev_fd = -1;  // To keep track of the previous step's output
    pid_t pids[256];   // To store PIDs of child processes (assuming max 256 steps)
    int step_index = 0;

    // Process each step in the pipeline
    while (step != NULL) {
        // Create a pipe for this step
        if (pipe(pipe_fds) == -1) {
            // pipe failed
            return -1;
        }

        // Create a synchronization pipe for this step
        if (pipe(sync_fds) == -1) {
            // sync pipe failed
            return -1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            // fork failed
            return -1;
        }

        if (pid == 0) {
            // Child process
            // Close unused synchronization pipe ends
            close(sync_fds[0]); // Close the read end
            close(sync_fds[1]); // Close the write end

            // Handle input redirection if this is the first step and an input file is specified
            if (step == task->steps && task->input_file != NULL) {
                int in_fd = open(task->input_file, O_RDONLY);
                if (in_fd == -1) {
                    // input file open failed
                    exit(EXIT_FAILURE);
                }
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // Handle output redirection if this is the last step and an output file is specified
            if (step->next == NULL && task->output_file != NULL) {
                int out_fd = open(task->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out_fd == -1) {
                    // output file open failed
                    exit(EXIT_FAILURE);
                }
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }

            // If it's not the first step, pipe the input from the previous step
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            // Set up the output pipe for this step
            if (step->next != NULL) {
                dup2(pipe_fds[1], STDOUT_FILENO);
            }

            close(pipe_fds[0]); // Close the read end of the pipe
            close(pipe_fds[1]); // Close the write end of the pipe

            // Execute the step's command
            if (process_step(step) != 0) {
                exit(EXIT_FAILURE); // If execvp fails, exit with an error
            }

            exit(EXIT_SUCCESS); // Child process completes successfully
        } else {
            // Parent process
            pids[step_index++] = pid; // Store the child PID

            // Close the write end of the synchronization pipe after signaling
            close(sync_fds[1]);

            // Wait for the child process to initialize
            char sync_buf[1];
            read(sync_fds[0], sync_buf, 1); // Read a byte to ensure initialization

            close(sync_fds[0]); // Close the read end of the synchronization pipe

            if (prev_fd != -1) {
                close(prev_fd); // Close the previous step's pipe read end
            }

            // Close the write end of the current pipe (parent doesn't need it)
            close(pipe_fds[1]);

            // Update prev_fd to the read end of the current pipe
            prev_fd = pipe_fds[0];
        }

        step = step->next; // Move to the next step in the pipeline
    }

    // Wait for all child processes to finish in the order they were forked
    for (int i = 0; i < step_index; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Child process %d failed\n", pids[i]);
            return -1; // One of the steps failed
        }
    }

    return 0; // Task processed successfully
}

// Process the recipe by iterating through tasks
int process_recipe(RECIPE *recipe) {
    TASK *task = recipe->tasks;

    // Find the corresponding RECIPE_QUEUE based on the recipe
    RECIPE_QUEUE *recipe_queue = NULL;
    RECIPE_QUEUE *current = work_queue;
    while (current != NULL) {
        if (current->recipe == recipe) {
            recipe_queue = current;
            break;
        }
        current = current->next;
    }

    while (task != NULL) {
        if (process_task(task, recipe_queue) != 0) {
            return -1; // Task processing failed
        }
        task = task->next; // Move to the next task
    }

    return 0; // Recipe processed successfully
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;

    // Reap all terminated children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {

        // Find the completed recipe in the queue
        RECIPE_QUEUE *current = work_queue;
        RECIPE_QUEUE *previous = NULL;
        while (current != NULL) {
            if (current->pid == pid) {
                break;
            }
            previous = current;
            current = current->next;
        }

        if (current == NULL) {
            continue; // No matching recipe found for this pid
        }

        RECIPE *completed_recipe = current->recipe;

        // Update recipe state
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            completed_recipe->state = (void *)1; // Recipe completed successfully
        } else {
            completed_recipe->state = (void *)-1; // Recipe failed
        }

        // Enqueue dependent recipes that can now be started
        RECIPE_LINK *link = completed_recipe->depend_on_this;
        while (link != NULL) {
            RECIPE *dependent_recipe = link->recipe;

            // Check if all dependencies are completed
            int can_start = 1;
            RECIPE_LINK *dep_link = dependent_recipe->this_depends_on;
            while (dep_link != NULL) {

                if (dep_link->recipe->state != (void *)1){
                // if (dep_link->recipe->state == NULL) {
                    can_start = 0;
                    break;
                }
                dep_link = dep_link->next;
            }

            if (can_start) {
                // Check if the dependent recipe is part of the dependency queue
                RECIPE_QUEUE *dep_node = dependency_queue;
                while (dep_node != NULL) {
                    if (dep_node->recipe == dependent_recipe) {
                        enqueue(&work_queue, dependent_recipe);
                        break;
                    }
                    dep_node = dep_node->next;
                }

            }

            link = link->next;
        }

        // Remove the completed recipe from the queue
        if (previous == NULL) {
            work_queue = current->next; // Update the head of the queue
        } else {
            previous->next = current->next; // Skip the completed recipe
        }
        free(current);

        // Decrement the active cooks counter
        active_cooks--;
    }
}

// Main function to start processing recipes and tasks
int process_work_queue(RECIPE_QUEUE **work_queue, int max_cooks) {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        // sigaction failed
        return -1;
    }

    // Create a signal set to use with sigsuspend
    sigset_t mask, oldmask;
    sigemptyset(&mask);                // Initialize an empty signal set
    sigaddset(&mask, SIGCHLD);         // Add SIGCHLD to the signal set
    sigprocmask(SIG_BLOCK, &mask, &oldmask); // Block SIGCHLD temporarily

    while (*work_queue != NULL || active_cooks > 0) {
        if (*work_queue == NULL || active_cooks >= max_cooks) {
            sigsuspend(&oldmask);  // Wait for SIGCHLD to arrive
        } else {
            RECIPE_QUEUE *node = *work_queue; // Do not dequeue yet

            // Find the first recipe in the queue that is not already in-process
            while (node != NULL && node->recipe->state == (void *)2) {
                node = node->next;
            }
            if (node == NULL) {
                sigsuspend(&oldmask); // Wait if no tasks are available
                continue;
            }

            RECIPE *recipe = node->recipe;

            if (recipe != NULL) {
                pid_t pid = fork();
                if (pid < 0) {
                    // fork failed
                    return -1;
                }

                if (pid == 0) {
                    if (process_recipe(recipe) != 0) {
                        // return -1;  // If recipe fails, exit with error
                        exit(EXIT_FAILURE);
                    }
                    exit(EXIT_SUCCESS);  // Recipe completed successfully
                } else {
                    // Parent process: Track the PID in the work queue
                    node->pid = pid;
                    recipe->state = (void *)2; // Mark recipe as "in-process"
                    active_cooks++;
                }
            }
        }
    }
    // Restore the original signal mask
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    return 0;
}
