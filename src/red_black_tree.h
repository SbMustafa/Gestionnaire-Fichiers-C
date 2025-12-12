#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include <stdio.h>
#include "fs_structs.h"

// In-memory representation might be useful, or we manipulate RBTNode directly.
// Given strict persistence, we might need helper functions to read/write nodes during rotation.
// For simplicity and speed in this "simulation", we can define functions that work on FILE*.

// Inserts an inode into the tree rooted at *root_offset.
// Returns 0 on success.
// Updates *root_offset if the root changes (rebalancing).
int rb_insert(FILE *file, long *root_offset, Inode new_inode, long *new_node_offset);

// Search for an inode by name in the tree.
// Returns offset of the RBTNode found, or -1 if not found.
long rb_search(FILE *file, long root_offset, const char *name);

// Delete an inode by name.
// Updates *root_offset.
int rb_delete(FILE *file, long *root_offset, const char *name);

// Low-level persistence helpers
void write_rb_node(FILE *file, long offset, RBTNode *node);
void read_rb_node(FILE *file, long offset, RBTNode *node);

// Debug / Traversal
void rb_inorder_print(FILE *file, long node_offset);

#endif // RED_BLACK_TREE_H
