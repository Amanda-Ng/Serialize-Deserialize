/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

int MINIMUM_BLOCK_SIZE = 32;

int calculate_block_size(int size);
int get_free_list_index(int size);
void copy_block_header_to_footer(sf_block *block);

int calculate_block_size(int size) {
    // Calculates the total block size (in bytes) required for the given size (in bytes) of payload.
    // The total size includes the block header and footer (16 bytes).
    int total_size = size + 16;  // Header and footer add 16 bytes.

    // Round up to the nearest multiple of 32
    if (total_size < 32) {
        return 32;  // Minimum block size.
    }

    return (total_size + 31) & ~31;  // Round up to the nearest multiple of 32.
}

int get_free_list_index(int size) {
    // Fibonacci sequence-based size classes
    int free_list_index = 0;
    if (size == MINIMUM_BLOCK_SIZE) {
        free_list_index = 0;  // First list: blocks of size M (32 bytes)
    } else if (size == MINIMUM_BLOCK_SIZE * 2) {
        free_list_index = 1;  // Second list: blocks of size 2M
    } else if (size == MINIMUM_BLOCK_SIZE * 3) {
        free_list_index = 2;  // Third list: blocks of size 3M
    } else if (size <= MINIMUM_BLOCK_SIZE * 5) {
        free_list_index = 3;  // Fourth list: blocks in the interval (3M, 5M]
    } else if (size <= MINIMUM_BLOCK_SIZE * 8) {
        free_list_index = 4;  // Fifth list: blocks in the interval (5M, 8M]
    } else if (size <= MINIMUM_BLOCK_SIZE * 13) {
        free_list_index = 5;  // Sixth list: blocks in the interval (8M, 13M]
    } else if (size <= MINIMUM_BLOCK_SIZE * 21) {
        free_list_index = 6;  // Seventh list: blocks in the interval (13M, 21M]
    } else {
        free_list_index = 7;  // Eighth list: blocks larger than 21M
    }
    return free_list_index;
}

void copy_block_header_to_footer(sf_block *block) {
    // Copies the block's header to its footer to maintain consistency between them.
    sf_block *temp_block = block;
    size_t *temp_block_position = (size_t *) temp_block;
    size_t size = (block->header & ~0x1F) / 8; // Calculate block size in 8-byte chunks.

    temp_block_position += size - 1; // Move to the footer position.
    temp_block = (sf_block *) temp_block_position;

    temp_block->header = block->header; // Copy header to footer.
    return;
}

// void set_prev_alloc(sf_block *pointer, int prev_alloc) {
//     // Sets the prev_alloc status of the block, indicating whether the previous block is allocated.
//     pointer->header = pointer->header & ~0b00001000; // clear the prev_alloc bit
//     pointer->header = pointer->header | (prev_alloc << 3); // set the prev_alloc bit
// }

// int get_prev_alloc(sf_block *pointer) {
//     // Returns 1 if the previous block is allocated, otherwise returns 0.
//     size_t block_header = pointer->header;
//     return (block_header & 8) == 8;  // Check if prev_alloc bit (bit 3) is set (1 if previous block is allocated)
// }

// sf_block *get_prev_block(sf_block *current_block) {
//     // To get the previous block, subtract the size of the previous block from the current block pointer
//     size_t prev_block_size = (current_block->header) & ~0x1F; // Mask out lower 5 bits to get block size
//     return (sf_block *)((char *)current_block - prev_block_size);
// }

void *sf_malloc(size_t size) {
    // To be implemented
    // abort();
    // Check if the requested size is zero; if so, return NULL
    if(size == 0) {
        return NULL; // No memory to allocate
    }
    // Calculate the total bytes needed for the requested size, including metadata
    int total_bytes_needed = calculate_block_size(size);
    // Get the index of the free list to use based on the total bytes needed
    int free_list_index = get_free_list_index(total_bytes_needed);
    // Check if the heap has been initialized
    int heap_initialized = (sf_mem_end() != sf_mem_start());
    // If the heap is not initialized, set it up
    if(!heap_initialized) {
        sf_mem_grow(); // Grow the heap memory
        // Initialize free list heads with sentinel nodes
        for (int i = 0; i < NUM_FREE_LISTS; i++) {
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }
        // Set a pointer to the start of the heap
        sf_block *heap_block_pointer = (sf_block *) sf_mem_start();

        size_t *heap_block_pointer_temp = ((size_t*)heap_block_pointer) + 3; // Skip the prologue (24 bytes)

        // Set the prologue block's metadata
        heap_block_pointer->header = 32; // Set block size

        // Clear bit 4 (allocation status) before setting it to the new value.
        heap_block_pointer->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (1) {
            heap_block_pointer->header |= 0x10;  // Set bit 4 (allocated).
        }

        copy_block_header_to_footer(heap_block_pointer); // Copy header to footer
        // Move to the first free block
        heap_block_pointer_temp += 4; // Move to first free block
        heap_block_pointer = (sf_block *)heap_block_pointer_temp;
        // Set up the first free block
        heap_block_pointer->header = 1984; // Set size for the first free block

        // Inserts the block into the free list at the specified index.
        heap_block_pointer->body.links.next = sf_free_list_heads[8].body.links.next;
        heap_block_pointer->body.links.prev = &sf_free_list_heads[8];
        // Adjust the links of the neighboring blocks.
        sf_free_list_heads[8].body.links.next->body.links.prev = heap_block_pointer;
        sf_free_list_heads[8].body.links.next = heap_block_pointer;

        copy_block_header_to_footer(heap_block_pointer); // Copy header to footer for free block
        // Set up the epilogue block
        size_t *epilogue_pointer = ((size_t *)sf_mem_end()) - 1;
        sf_block *epilogue_block = (sf_block *)epilogue_pointer;
        epilogue_block->header = 0; // Set header for epilogue

        // Clear bit 4 (allocation status) before setting it to the new value.
        epilogue_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (1) {
            epilogue_block->header |= 0x10;  // Set bit 4 (allocated).
        }

    }
    int block_found = 0; // Flag to indicate if a suitable block is found
    sf_block *allocated_payload_pointer = NULL; // Pointer to the allocated memory block
    int current_free_index = free_list_index; // Start searching from the calculated free list index
    // Loop to find a suitable free block or extend the heap if necessary
    while(block_found == 0) {
        current_free_index = free_list_index; // Reset to the starting free list index
        // Search through the free lists for a suitable block
        while(current_free_index != 9) { // Loop through available free lists
            sf_block *current_free_block = sf_free_list_heads[current_free_index].body.links.next;
            // Loop through blocks in the current free list
            while(current_free_block->header != 0) {
                // Check if the current free block has enough space
                if(total_bytes_needed <= (current_free_block->header & ~0x1F)) {
                    size_t existing_size = (current_free_block->header & ~0x1F); // Store existing block size
                    size_t remaining_size = existing_size - total_bytes_needed; // Calculate remaining size
                    // Removes the block from the free list by updating the links of the neighboring blocks.
                    current_free_block->body.links.prev->body.links.next = current_free_block->body.links.next;
                    current_free_block->body.links.next->body.links.prev = current_free_block->body.links.prev;
                    // Clear the block's next and prev pointers.
                    current_free_block->body.links.next = NULL;
                    current_free_block->body.links.prev = NULL;
                    // Allocate the required memory
                    current_free_block->header = total_bytes_needed;

                    // Clear bit 4 (allocation status) before setting it to the new value.
                    current_free_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
                    if (1) {
                        current_free_block->header |= 0x10;  // Set bit 4 (allocated).
                    }

                    // // Set prev_alloc based on the previous block
                    // // Calculate the address of the previous block
                    // sf_block *prev_block = (sf_block *)((char *)curr_node_free - (curr_node_free->header & ~0x1F) + sizeof(sf_header)); // Use the size of the header for alignment
                    // set_prev_alloc(curr_node_free, ((prev_block->header & 16) == 16)); // Set based on previous block

                    copy_block_header_to_footer(current_free_block); // Copy header to footer
                    // Set pointer to the payload area of the allocated block
                    size_t *payload_temp = ((size_t *) current_free_block) + 1;
                    allocated_payload_pointer = (sf_block *) payload_temp; // Assign to the payload pointer
                    // Check if the remaining size is less than the minimum block size
                    if(remaining_size >= 32) {
                        // If over-allocated, no need to add the remaining block to the free list
                        // If there is leftover space, prepare it for reuse
                        size_t *next_free_block_temp = ((size_t *) current_free_block) + ((current_free_block->header & ~0x1F) / 8);

                        current_free_block = (sf_block *) next_free_block_temp; // Update to point to the new free block
                        // Set the size and mark the new block as free
                        current_free_block->header = remaining_size;

                        // Clear bit 4 (allocation status) before setting it to the new value.
                        current_free_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
                        if (0) {
                            current_free_block->header |= 0x10;  // Set bit 4 (allocated).
                        }

                        // Clear bit 4 (allocation status) before setting it to the new value.
                        current_free_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
                        if (0) {
                            current_free_block->header |= 0x10;  // Set bit 4 (allocated).
                        }

                        // Insert the new free block into the appropriate free list
                        if(current_free_index == 8) {
                            // Inserts the block into the free list at the specified index.
                            current_free_block->body.links.next = sf_free_list_heads[8].body.links.next;
                            current_free_block->body.links.prev = &sf_free_list_heads[8];
                            // Adjust the links of the neighboring blocks.
                            sf_free_list_heads[8].body.links.next->body.links.prev = current_free_block;
                            sf_free_list_heads[8].body.links.next = current_free_block;
                        } else {
                            int index = get_free_list_index(current_free_block->header & ~0x1F);
                            // Inserts the block into the free list at the specified index.
                            current_free_block->body.links.next = sf_free_list_heads[index].body.links.next;
                            current_free_block->body.links.prev = &sf_free_list_heads[index];
                            // Adjust the links of the neighboring blocks.
                            sf_free_list_heads[index].body.links.next->body.links.prev = current_free_block;
                            sf_free_list_heads[index].body.links.next = current_free_block;
                        }
                        copy_block_header_to_footer(current_free_block); // Copy header to footer for the new free block
                    }

                    block_found = 1; // Set the block found flag
                    break; // Exit the loop
                } else {
                    // Move to the next block in the current free list
                    current_free_block = current_free_block->body.links.next;
                }
            }
            if(block_found == 1) {
                break; // Exit if block has been found
            }
            current_free_index++; // Move to the next free list index
        }
        if(block_found == 1) {
            break; // Exit the outer loop if a block was found
        }
        // If no suitable block was found, try to extend the heap
        size_t current_space = 0;
        sf_block *wilderness_block = sf_free_list_heads[8].body.links.next; // Get current wilderness block.
        current_space += wilderness_block->header;

        while (current_space < total_bytes_needed) {
            wilderness_block = sf_free_list_heads[8].body.links.next; // Update to current wilderness block.
            sf_block *new_wilderness_footer = (sf_block *)sf_mem_end(); // Set footer at the end of the heap.
            if (sf_mem_grow() == NULL) { // Try to grow the heap. If it fails, return an error.
                sf_errno = ENOMEM; // Set error to indicate memory allocation failure
                return NULL; // Return NULL to indicate failure
            }
            current_space += 2048; // Add the size of the newly allocated page (2048 bytes).

            // Set the new epilogue block.
            size_t *epilogue_ptr = ((size_t *)sf_mem_end()) - 1;
            sf_block *epilogue_block = (sf_block *)epilogue_ptr;
            epilogue_block->header = 0;

            // Clear bit 4 (allocation status) before setting it to the new value.
            epilogue_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
            if (1) {
                epilogue_block->header |= 0x10;  // Set bit 4 (allocated).
            }

            // Coalesce the wilderness with the newly added page.
            size_t *new_wilderness_footer_ptr = ((size_t *)new_wilderness_footer) - 1;
            new_wilderness_footer = (sf_block *)new_wilderness_footer_ptr;
            new_wilderness_footer->header = 2048; // Set the size of the new wilderness.

            // Clear bit 4 (allocation status) before setting it to the new value.
            new_wilderness_footer->header &= ~0x10;  // Clear bit 4 (reset allocation status).
            if (0) {
                new_wilderness_footer->header |= 0x10;  // Set bit 4 (allocated).
            }

            // If wilderness is already present, remove it from the free list and insert it back.
            if (wilderness_block->body.links.next != wilderness_block) {

                // Removes the block from the free list by updating the links of the neighboring blocks.
                wilderness_block->body.links.prev->body.links.next = wilderness_block->body.links.next;
                wilderness_block->body.links.next->body.links.prev = wilderness_block->body.links.prev;
                // Clear the block's next and prev pointers.
                wilderness_block->body.links.next = NULL;
                wilderness_block->body.links.prev = NULL;

                int index = get_free_list_index(wilderness_block->header & ~0x1F);
                // Inserts the block into the free list at the specified index.
                wilderness_block->body.links.next = sf_free_list_heads[index].body.links.next;
                wilderness_block->body.links.prev = &sf_free_list_heads[index];
                // Adjust the links of the neighboring blocks.
                sf_free_list_heads[index].body.links.next->body.links.prev = wilderness_block;
                sf_free_list_heads[index].body.links.next = wilderness_block;
            }

            // Update the footer of the new wilderness block.
            copy_block_header_to_footer(new_wilderness_footer);

            size_t *left_neighbor_ptr = new_wilderness_footer_ptr - 1;
            sf_block *left_neighbor = (sf_block *)left_neighbor_ptr; // Get left neighbor block (if free).

            // If the left block is free, coalesce with it.
            while ((((left_neighbor->header & 16) == 16) == 0) && ((left_neighbor->header & ~0x1F) >= 32)) {
                left_neighbor_ptr -= ((left_neighbor->header & ~0x1F) / 8) - 1; // Move to the header of the left block.
                left_neighbor = (sf_block *)left_neighbor_ptr;

                size_t combined_size = left_neighbor->header + new_wilderness_footer->header;

                // Removes the block from the free list by updating the links of the neighboring blocks.
                left_neighbor->body.links.prev->body.links.next = left_neighbor->body.links.next;
                left_neighbor->body.links.next->body.links.prev = left_neighbor->body.links.prev;
                // Clear the block's next and prev pointers.
                left_neighbor->body.links.next = NULL;
                left_neighbor->body.links.prev = NULL;

                // Coalesce left block and wilderness.
                left_neighbor->header = combined_size;

                // Clear bit 4 (allocation status) before setting it to the new value.
                left_neighbor->header &= ~0x10;  // Clear bit 4 (reset allocation status).
                if (0) {
                    left_neighbor->header |= 0x10;  // Set bit 4 (allocated).
                }

                copy_block_header_to_footer(left_neighbor);

                new_wilderness_footer = left_neighbor;
                new_wilderness_footer_ptr = (size_t *)new_wilderness_footer;
                left_neighbor_ptr = new_wilderness_footer_ptr - 1;
                left_neighbor = (sf_block *)left_neighbor_ptr;
            }

            // Reinsert coalesced wilderness into free list.
            // Inserts the block into the free list at the specified index.
            new_wilderness_footer->body.links.next = sf_free_list_heads[8].body.links.next;
            new_wilderness_footer->body.links.prev = &sf_free_list_heads[8];
            // Adjust the links of the neighboring blocks.
            sf_free_list_heads[8].body.links.next->body.links.prev = new_wilderness_footer;
            sf_free_list_heads[8].body.links.next = new_wilderness_footer;
        }
    }
    // Return pointer to the allocated payload area
    return allocated_payload_pointer;
}

void sf_free(void *pp) {
    // To be implemented
    // abort();
    // If the pointer is NULL, abort
    if(pp == NULL) {
        abort();
    }
    // Check if the pointer is aligned to a 32-byte boundary, if not, abort
    if( (size_t)pp % 32 != 0) {
        abort();
    }
    // Move pointer back to block footer
    sf_block *block_to_free = (sf_block *)pp;
    size_t *block_header_ptr = ((size_t *) block_to_free) - 1;
    block_to_free = (sf_block *) block_header_ptr;
    // Validate block size and allocation status (size must be at least 32 and a multiple of 32)
    if(((block_to_free->header & ~0x1F) < 32) ||
    ((block_to_free->header & ~0x1F) % 32 != 0 ) ||
    (((block_to_free->header & 16) == 16) == 0)) {
        abort();
    }

    // // Check prev_alloc field
    // if(get_prev_alloc(pp_block) == 0) {  // If prev_alloc is 0, previous block is free
    //     sf_block *prev_block = get_prev_block(pp_block);
    //     if(((prev_block->header & 16) == 16) != 0) {  // Check if previous block's alloc field is 1 (allocated)
    //         abort();  // Invalid pointer if prev_alloc is 0 but previous block is marked as allocated
    //     }
    // }

    // Check memory bounds
    size_t *heap_start = ((size_t *)sf_mem_start()) + 3; // Skip prologue
    size_t *heap_end = ((size_t *) sf_mem_end()) - 1;
    // Move to the footer of the block to check boundaries
    sf_block *block_footer = (sf_block *)pp;
    size_t *block_footer_ptr = ((size_t *)block_footer) + (((block_footer->header & ~0x1F) / 8) - 1); // Move to footer
    block_footer = (sf_block *)block_footer_ptr;
    // Abort if block is outside heap boundaries
    if(((size_t)pp < (size_t)heap_start) ||
        ((size_t)block_footer > (size_t)heap_end)) {
        abort();
    }

    // Mark the block as free and update the footer to match the header
    // Clear bit 4 (allocation status) before setting it to the new value.
    block_to_free->header &= ~0x10;  // Clear bit 4 (reset allocation status).
    if (0) {
        block_to_free->header |= 0x10;  // Set bit 4 (allocated).
    }

    copy_block_header_to_footer(block_to_free);
    // Coalesce with left adjacent block if free
    size_t *left_footer_ptr = block_header_ptr - 1; // Move to left footer
    sf_block *left_block = (sf_block *) left_footer_ptr;
    while((((left_block->header & 16) == 16) == 0) && ((left_block->header & ~0x1F) >= 32)) {
        // Move to left block header
        left_footer_ptr -= ((left_block->header & ~0x1F) / 8) - 1;

        left_block = (sf_block *) left_footer_ptr;
        // Merge the left block with current block
        size_t new_block_size = left_block->header + block_to_free->header;

        // Removes the block from the free list by updating the links of the neighboring blocks.
        left_block->body.links.prev->body.links.next = left_block->body.links.next;
        left_block->body.links.next->body.links.prev = left_block->body.links.prev;
        // Clear the block's next and prev pointers.
        left_block->body.links.next = NULL;
        left_block->body.links.prev = NULL;

        left_block->header = new_block_size;

        // Clear bit 4 (allocation status) before setting it to the new value.
        left_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (0) {
            left_block->header |= 0x10;  // Set bit 4 (allocated).
        }

        copy_block_header_to_footer(left_block);
        // Update current block to the coalesced left block
        block_to_free = left_block;
        block_header_ptr = (size_t *) block_to_free;
        // Move left for next possible coalescing
        left_footer_ptr = block_header_ptr - 1;
        left_block = (sf_block *) left_footer_ptr;
    }

    // Coalesce with right adjacent block if free
    size_t *right_header_ptr = block_header_ptr + ((block_to_free->header & ~0x1F) / 8);
    sf_block *block_right = (sf_block *) right_header_ptr;
    while((((block_right->header & 16) == 16) == 0) && ((block_right->header & ~0x1F) >= 32)) {
        // Merge the right block with the current block
        size_t new_block_size = block_right->header + block_to_free->header;

        // Removes the block from the free list by updating the links of the neighboring blocks.
        block_right->body.links.prev->body.links.next = block_right->body.links.next;
        block_right->body.links.next->body.links.prev = block_right->body.links.prev;
        // Clear the block's next and prev pointers.
        block_right->body.links.next = NULL;
        block_right->body.links.prev = NULL;

        block_to_free->header = new_block_size;

        // Clear bit 4 (allocation status) before setting it to the new value.
        block_to_free->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (0) {
            block_to_free->header |= 0x10;  // Set bit 4 (allocated).
        }

        copy_block_header_to_footer(block_to_free);
        // Move to next possible right block for coalescing
        right_header_ptr = block_header_ptr + ((block_to_free->header & ~0x1F) / 8);
        block_right = (sf_block *) right_header_ptr;
    }
    // Check if the block should be inserted into wilderness or other free lists
    block_header_ptr += (block_to_free->header & ~0x1F) / 8;
    // Check if the wilderness free list is empty
    sf_block *wilderness_block = sf_free_list_heads[8].body.links.next;
    size_t * wilderness_block_ptr = (size_t *) wilderness_block;
    size_t wilderness_block_address = (size_t) wilderness_block_ptr;
    if(wilderness_block->body.links.next == wilderness_block) { // Wilderness list is empty
        // Inserts the block into the free list at the specified index.
        block_to_free->body.links.next = sf_free_list_heads[8].body.links.next;
        block_to_free->body.links.prev = &sf_free_list_heads[8];
        // Adjust the links of the neighboring blocks.
        sf_free_list_heads[8].body.links.next->body.links.prev = block_to_free;
        sf_free_list_heads[8].body.links.next = block_to_free;
    }
    else if( (wilderness_block->body.links.next != wilderness_block) && ((size_t) block_header_ptr > wilderness_block_address)) {
        // If next block is in wilderness, coalesce and insert

        // Removes the block from the free list by updating the links of the neighboring blocks.
        wilderness_block->body.links.prev->body.links.next = wilderness_block->body.links.next;
        wilderness_block->body.links.next->body.links.prev = wilderness_block->body.links.prev;
        // Clear the block's next and prev pointers.
        wilderness_block->body.links.next = NULL;
        wilderness_block->body.links.prev = NULL;

        int index = get_free_list_index(wilderness_block->header & ~0x1F);

        // Inserts the block into the free list at the specified index.
        wilderness_block->body.links.next = sf_free_list_heads[index].body.links.next;
        wilderness_block->body.links.prev = &sf_free_list_heads[index];
        // Adjust the links of the neighboring blocks.
        sf_free_list_heads[index].body.links.next->body.links.prev = wilderness_block;
        sf_free_list_heads[index].body.links.next = wilderness_block;

        // Inserts the block into the free list at the specified index.
        block_to_free->body.links.next = sf_free_list_heads[8].body.links.next;
        block_to_free->body.links.prev = &sf_free_list_heads[8];
        // Adjust the links of the neighboring blocks.
        sf_free_list_heads[8].body.links.next->body.links.prev = block_to_free;
        sf_free_list_heads[8].body.links.next = block_to_free;    }
    else if((size_t)heap_end == (size_t)block_header_ptr) {
        // Check if block is adjacent to epilogue
        // Inserts the block into the free list at the specified index.
        block_to_free->body.links.next = sf_free_list_heads[8].body.links.next;
        block_to_free->body.links.prev = &sf_free_list_heads[8];
        // Adjust the links of the neighboring blocks.
        sf_free_list_heads[8].body.links.next->body.links.prev = block_to_free;
        sf_free_list_heads[8].body.links.next = block_to_free;    }
    else {
        // Insert into appropriate free list based on size
        int index = get_free_list_index(block_to_free->header & ~0x1F);

        // Inserts the block into the free list at the specified index.
        block_to_free->body.links.next = sf_free_list_heads[index].body.links.next;
        block_to_free->body.links.prev = &sf_free_list_heads[index];
        // Adjust the links of the neighboring blocks.
        sf_free_list_heads[index].body.links.next->body.links.prev = block_to_free;
        sf_free_list_heads[index].body.links.next = block_to_free;
    }
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    // To be implemented
    // abort();
    // Abort if the pointer is NULL or block size is not aligned properly
    if (pp == NULL) {
        sf_errno = EINVAL;
        abort();
    }
    // Check if pointer is aligned to 32 bytes
    if ((size_t)pp % 32 != 0) {
        sf_errno = EINVAL;
        abort();
    }
    // If requested size is 0, free the block and return NULL
    if (rsize == 0) {
        sf_free(pp);
        return NULL;
    }
    // Move the pointer back to access the block's header
    sf_block *block_ptr = (sf_block *)pp;
    size_t *block_header_ptr = ((size_t *)block_ptr) - 1;
    block_ptr = (sf_block *)block_header_ptr;
    // Validate block size, alignment, and allocation status
    if (((block_ptr->header & ~0x1F) < 32) ||
        ((block_ptr->header & ~0x1F) % 32 != 0) ||
        (((block_ptr->header & 16) == 16) == 0)) {
        abort();
    }

    // // Check prev_alloc field
    // if(get_prev_alloc(pp_block) == 0) {  // If prev_alloc is 0, previous block is free
    //     sf_block *prev_block = get_prev_block(pp_block);
    //     if(((prev_block->header & 16) == 16) != 0) {  // Check if previous block's alloc field is 1 (allocated)
    //         abort();  // Invalid pointer if prev_alloc is 0 but previous block is marked as allocated
    //     }
    // }

    // Check if the pointer is within the heap boundaries
    size_t *heap_start = ((size_t *)sf_mem_start()) + 3;
    size_t *heap_end = ((size_t *)sf_mem_end()) - 1;
    // Calculate footer location for the block
    sf_block *block_footer_ptr = (sf_block *)pp;
    size_t *block_footer_address = ((size_t *)block_footer_ptr) + (((block_footer_ptr->header & ~0x1F) / 8) - 1);

    block_footer_ptr = (sf_block *)block_footer_address;
    // Abort if the block is outside of heap boundaries
    if (((size_t)pp < (size_t)heap_start) ||
        ((size_t)block_footer_ptr > (size_t)heap_end)) {
        abort();
    }
    // If requested size is greater than the current block, allocate a new block
    if (rsize > (block_ptr->header & ~0x1F)) {
        sf_block *new_block_ptr = sf_malloc(rsize);
        if (new_block_ptr == NULL) {
            return NULL;
        }
        // Copy old data into the newly allocated block and free the old one
        memcpy(new_block_ptr, pp, rsize);
        sf_free(pp);
        return new_block_ptr; // Return the new block
    }
    // If the requested size is smaller, check if we need to split the block
    else if (rsize < (block_ptr->header & ~0x1F)) {
        // Calculate the required size with alignment
        size_t new_size = calculate_block_size(rsize);
        size_t space_to_free = (block_ptr->header & ~0x1F) - new_size;
        // If the leftover space is too small for a new block, avoid splitting (splinter)
        if (space_to_free < 32) {
            return pp; // Return the original block if it's not worth splitting
        }
        // Split the block and free the extra space
        size_t *split_block_ptr = (block_header_ptr + ((block_ptr->header & ~0x1F) / 8)) - (space_to_free / 8);

        sf_block *free_block_ptr = (sf_block *)split_block_ptr;
        // Set the size and mark the block as free
        free_block_ptr->header = space_to_free;

        // Clear bit 4 (allocation status) before setting it to the new value.
        free_block_ptr->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (0) {
            free_block_ptr->header |= 0x10;  // Set bit 4 (allocated).
        }

        copy_block_header_to_footer(free_block_ptr);
        // Coalesce if adjacent blocks are free
        size_t *right_block_header_ptr = split_block_ptr + ((free_block_ptr->header & ~0x1F) / 8);
        sf_block *right_block_ptr = (sf_block *)right_block_header_ptr;

        size_t *heap_end = ((size_t *) sf_mem_end()) - 1;
        // Traverse and coalesce free blocks to the right
        while ((((right_block_ptr->header & 16) == 16) == 0) && ((right_block_ptr->header & ~0x1F) >= 32)) {
            size_t new_block_size = right_block_ptr->header + free_block_ptr->header;

            // Removes the block from the free list by updating the links of the neighboring blocks.
            right_block_ptr->body.links.prev->body.links.next = right_block_ptr->body.links.next;
            right_block_ptr->body.links.next->body.links.prev = right_block_ptr->body.links.prev;
            // Clear the block's next and prev pointers.
            right_block_ptr->body.links.next = NULL;
            right_block_ptr->body.links.prev = NULL;

            // Update size and mark the coalesced block as free
            free_block_ptr->header = new_block_size;

            // Clear bit 4 (allocation status) before setting it to the new value.
            free_block_ptr->header &= ~0x10;  // Clear bit 4 (reset allocation status).
            if (0) {
                free_block_ptr->header |= 0x10;  // Set bit 4 (allocated).
            }

            copy_block_header_to_footer(free_block_ptr);

            right_block_header_ptr = split_block_ptr + ((free_block_ptr->header & ~0x1F) / 8);
            right_block_ptr = (sf_block *)right_block_header_ptr;
        }
        // Insert the coalesced free block into the appropriate free list
        split_block_ptr += (free_block_ptr->header & ~0x1F) / 8;
        if ((size_t)heap_end == (size_t)split_block_ptr) {
            // If the block reaches the end of the heap, insert into wilderness
            // Inserts the block into the free list at the specified index.
            free_block_ptr->body.links.next = sf_free_list_heads[8].body.links.next;
            free_block_ptr->body.links.prev = &sf_free_list_heads[8];
            // Adjust the links of the neighboring blocks.
            sf_free_list_heads[8].body.links.next->body.links.prev = free_block_ptr;
            sf_free_list_heads[8].body.links.next = free_block_ptr;
        } else {
            int index = get_free_list_index(free_block_ptr->header & ~0x1F);

            // Inserts the block into the free list at the specified index.
            free_block_ptr->body.links.next = sf_free_list_heads[index].body.links.next;
            free_block_ptr->body.links.prev = &sf_free_list_heads[index];
            // Adjust the links of the neighboring blocks.
            sf_free_list_heads[index].body.links.next->body.links.prev = free_block_ptr;
            sf_free_list_heads[index].body.links.next = free_block_ptr;
        }
        // Update the current block's size and mark it as allocated
        block_ptr->header = new_size;

        // Clear bit 4 (allocation status) before setting it to the new value.
        block_ptr->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (1) {
            block_ptr->header |= 0x10;  // Set bit 4 (allocated).
        }

        copy_block_header_to_footer(block_ptr);
        block_header_ptr += 1;  // Move past the header to return the payload
        block_ptr = (sf_block *)block_header_ptr;
        return block_ptr; // Return the reallocated block
    }
    return NULL; // If no condition matched, return NULL
}

void *sf_memalign(size_t size, size_t align) {
    // To be implemented
    // abort();
    // Check if alignment is less than the minimum block size (32 bytes)
    if (align < 32) {
        sf_errno = EINVAL;  // Invalid alignment error
        return NULL;
    }
    // If requested size is 0, no allocation is needed
    if (size == 0) {
        return NULL;
    }

    // Ensure alignment is a power of two, starting from 32
    if (align < 32 || (align & (align - 1)) != 0) {
        sf_errno = EINVAL;  // Alignment must be a power of two
        return NULL;
    }

    // Initialize free lists if the heap is not yet initialized
    int is_heap_initialized = (sf_mem_end() != sf_mem_start());
    if(!is_heap_initialized) {
        sf_mem_grow(); // Grow the heap
        // Initialize free list sentinels
        for (int i = 0; i < NUM_FREE_LISTS; i++) {
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }
        // Setup prologue block (24 bytes), which is 32 bytes in total
        sf_block *prologue_block = (sf_block *) sf_mem_start();
        size_t *prologue_ptr = ((size_t*)prologue_block) + 3;
        prologue_block->header = 32; // Prologue block is 32 bytes

        // Clear bit 4 (allocation status) before setting it to the new value.
        prologue_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (1) {
            prologue_block->header |= 0x10;  // Set bit 4 (allocated).
        }

        copy_block_header_to_footer(prologue_block);
        // Move pointer to first free block after prologue
        prologue_ptr += 4; // Move past the prologue
        // First free block (wilderness block)
        prologue_block = (sf_block *)prologue_ptr ;
        prologue_block->header = 1984; // Set wilderness size

        // Inserts the block into the free list at the specified index.
        prologue_block->body.links.next = sf_free_list_heads[8].body.links.next;
        prologue_block->body.links.prev = &sf_free_list_heads[8];
        // Adjust the links of the neighboring blocks.
        sf_free_list_heads[8].body.links.next->body.links.prev = prologue_block;
        sf_free_list_heads[8].body.links.next = prologue_block;

        copy_block_header_to_footer(prologue_block);
        // Set up the epilogue block at the end of the heap
        size_t *epilogue_ptr = ((size_t *)sf_mem_end()) - 1;
        sf_block *epilogue = (sf_block *)epilogue_ptr;
        epilogue->header = 0; // Set epilogue header to 0

        // Clear bit 4 (allocation status) before setting it to the new value.
        epilogue->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (1) {
            epilogue->header |= 0x10;  // Set bit 4 (allocated).
        }

    }
    // Calculate required block size (size + alignment + extra space)
    size_t total_block_size = size + align + 32 ;
    // Allocate a new block with the required size
    sf_block *allocated_block = sf_malloc(total_block_size);
    size_t *allocated_block_ptr = (size_t *) allocated_block;
    // Calculate payload address within the block
    sf_block *payload_block = allocated_block;
    size_t *payload_ptr = (size_t *) payload_block;
    size_t payload_address = (size_t) payload_ptr;
    // Check if the payload is already aligned
    if ((payload_address % align) == 0) {
        return payload_block;  // Return the aligned payload
    }
    // Not aligned, adjust the block to find an aligned address
    allocated_block_ptr -= 1; // Move to footer
    allocated_block = (sf_block *) allocated_block_ptr;
    size_t original_block_size = (allocated_block->header & ~0x1F);

    sf_block *unaligned_block = allocated_block; 
    size_t unaligned_block_size = 0; // bytes
    // Adjust until the payload is aligned
    while(1) {
        if ((payload_address % align) == 0 && (unaligned_block_size % 32 == 0)) {
            break;
        }

        allocated_block_ptr += 1;
        allocated_block = (sf_block *) allocated_block_ptr;
        unaligned_block_size += 8; // Increment block size
        original_block_size -= 8;

        payload_ptr += 1;
        payload_address = (size_t)payload_ptr;
        payload_block = (sf_block *) payload_ptr;
    }
    // Split and create the unaligned block
    unaligned_block->header = unaligned_block_size;

    // Clear bit 4 (allocation status) before setting it to the new value.
    unaligned_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
    if (0) {
        unaligned_block->header |= 0x10;  // Set bit 4 (allocated).
    }

    int index = get_free_list_index(unaligned_block->header & ~0x1F);

    // Inserts the block into the free list at the specified index.
    unaligned_block->body.links.next = sf_free_list_heads[index].body.links.next;
    unaligned_block->body.links.prev = &sf_free_list_heads[index];
    // Adjust the links of the neighboring blocks.
    sf_free_list_heads[index].body.links.next->body.links.prev = unaligned_block;
    sf_free_list_heads[index].body.links.next = unaligned_block;

    copy_block_header_to_footer(unaligned_block);
    // Update the original block with the new aligned size
    allocated_block->header = original_block_size;

    // Clear bit 4 (allocation status) before setting it to the new value.
    allocated_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
    if (1) {
        allocated_block->header |= 0x10;  // Set bit 4 (allocated).
    }

    copy_block_header_to_footer(allocated_block);
    // Check if splitting the right block is possible
    if((calculate_block_size(size + 16)) < original_block_size) {
        // Split and create a new block
        size_t remaining_size = original_block_size - (calculate_block_size(size + 16));
        allocated_block->header = (calculate_block_size(size + 16));

        // Clear bit 4 (allocation status) before setting it to the new value.
        allocated_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (1) {
            allocated_block->header |= 0x10;  // Set bit 4 (allocated).
        }

        copy_block_header_to_footer(allocated_block);

        allocated_block_ptr += (allocated_block->header & ~0x1F) / 8;
        sf_block *split_block = (sf_block *) allocated_block_ptr; 
        size_t *split_block_ptr = (size_t *) split_block; 
        // Set up the split block
        split_block->header = remaining_size;

        // Clear bit 4 (allocation status) before setting it to the new value.
        split_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
        if (0) {
            split_block->header |= 0x10;  // Set bit 4 (allocated).
        }

        copy_block_header_to_footer(split_block);
        // Attempt to coalesce with the right block if free
        size_t *right_block_ptr = split_block_ptr + ((split_block->header & ~0x1F) / 8);
        sf_block *block_right = (sf_block *) right_block_ptr;
        // Coalesce adjacent free blocks if possible
        while((((block_right->header & 16) == 16) == 0) && ((block_right->header & ~0x1F) >= 32)) {
            size_t new_coalesced_size = block_right->header + split_block->header;

            // Removes the block from the free list by updating the links of the neighboring blocks.
            block_right->body.links.prev->body.links.next = block_right->body.links.next;
            block_right->body.links.next->body.links.prev = block_right->body.links.prev;
            // Clear the block's next and prev pointers.
            block_right->body.links.next = NULL;
            block_right->body.links.prev = NULL;

            split_block->header = new_coalesced_size;

            // Clear bit 4 (allocation status) before setting it to the new value.
            split_block->header &= ~0x10;  // Clear bit 4 (reset allocation status).
            if (0) {
                split_block->header |= 0x10;  // Set bit 4 (allocated).
            }

            copy_block_header_to_footer(split_block);

            right_block_ptr = split_block_ptr + ((split_block->header & ~0x1F) / 8);
            block_right = (sf_block *) right_block_ptr;
        }

        split_block_ptr += (split_block->header & ~0x1F) / 8;
        // Insert the final split block into the free list or wilderness
        size_t *heap_end = ((size_t *) sf_mem_end()) - 1;
        if((size_t)heap_end == (size_t)split_block_ptr) {
            // Inserts the block into the free list at the specified index.
            split_block->body.links.next = sf_free_list_heads[8].body.links.next;
            split_block->body.links.prev = &sf_free_list_heads[8];
            // Adjust the links of the neighboring blocks.
            sf_free_list_heads[8].body.links.next->body.links.prev = split_block;
            sf_free_list_heads[8].body.links.next = split_block;
        } else {
            int i = get_free_list_index(split_block->header & ~0x1F);
            // Inserts the block into the free list at the specified index.
            split_block->body.links.next = sf_free_list_heads[i].body.links.next;
            split_block->body.links.prev = &sf_free_list_heads[i];
            // Adjust the links of the neighboring blocks.
            sf_free_list_heads[i].body.links.next->body.links.prev = split_block;
            sf_free_list_heads[i].body.links.next = split_block;
        }
    }
    // If allocation failed
    if(payload_block == NULL) {
        sf_errno = ENOMEM; // Not enough memory error
        return NULL;
    }
    return payload_block; // Return the aligned payload
}
