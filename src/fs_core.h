#ifndef FS_CORE_H
#define FS_CORE_H

#include <stdio.h>
#include "fs_structs.h"

typedef struct FSContext {
    FILE *file;
    SuperBlock sb;
} FSContext;

// Initialize a new filesystem in the given file.
// Returns 0 on success, < 0 on failure.
int init_filesystem(const char *filename);

// Load an existing filesystem.
// Populates context.
// Returns 0 on success.
int load_filesystem(const char *filename, FSContext *ctx);

// Close filesystem.
void close_filesystem(FSContext *ctx);

// Add a file to the filesystem.
// path: currently just filename (flat structure simplified or full path handling logic).
// data: original content.
// size: size of data.
int add_file(FSContext *ctx, const char *path, const unsigned char *data, size_t size);

// Retrieve file content.
// Returns buffer (caller must free) or NULL.
unsigned char* get_file_content(FSContext *ctx, const char *path, size_t *out_size);

// List files (debug).
void list_files(FSContext *ctx);

#endif // FS_CORE_H
