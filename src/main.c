#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs_core.h"

// Simple usage:
// ./fs_prog init fs.bin
// ./fs_prog add fs.bin filename "content string"
// ./fs_prog get fs.bin filename
// ./fs_prog list fs.bin

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage:\n");
        printf("  %s init <fs_file>\n", argv[0]);
        printf("  %s add <fs_file> <dest_filename> <content>\n", argv[0]);
        printf("  %s addfile <fs_file> <dest_filename> <src_file_path>\n", argv[0]);
        printf("  %s get <fs_file> <filename>\n", argv[0]);
        printf("  %s list <fs_file>\n", argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    const char *fs_file = argv[2];

    if (strcmp(cmd, "init") == 0) {
        if (init_filesystem(fs_file) == 0) {
            printf("Filesystem initialized in %s\n", fs_file);
        } else {
            fprintf(stderr, "Failed to initialize filesystem.\n");
            return 1;
        }
    } else if (strcmp(cmd, "add") == 0) {
        if (argc < 5) return 1;
        const char *filename = argv[3];
        const char *content = argv[4];

        FSContext ctx;
        if (load_filesystem(fs_file, &ctx) != 0) {
            fprintf(stderr, "Failed to load FS.\n");
            return 1;
        }

        if (add_file(&ctx, filename, (unsigned char*)content, strlen(content)) == 0) {
            printf("File '%s' added.\n", filename);
        } else {
            fprintf(stderr, "Failed to add file.\n");
        }
        close_filesystem(&ctx);

    } else if (strcmp(cmd, "addfile") == 0) {
        if (argc < 5) return 1;
        const char *dest_filename = argv[3];
        const char *src_path = argv[4];

        FILE *f = fopen(src_path, "rb");
        if (!f) {
            fprintf(stderr, "Cannot open source file %s\n", src_path);
            return 1;
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char *buf = malloc(size);
        fread(buf, 1, size, f);
        fclose(f);

        FSContext ctx;
        if (load_filesystem(fs_file, &ctx) != 0) {
            free(buf);
            fprintf(stderr, "Failed to load FS.\n");
            return 1;
        }

        if (add_file(&ctx, dest_filename, buf, size) == 0) {
            printf("File '%s' added from '%s'.\n", dest_filename, src_path);
        } else {
            fprintf(stderr, "Failed to add file.\n");
        }
        
        free(buf);
        close_filesystem(&ctx);

    } else if (strcmp(cmd, "get") == 0) {
        const char *filename = argv[3];
        FSContext ctx;
        if (load_filesystem(fs_file, &ctx) != 0) {
            fprintf(stderr, "Failed to load FS.\n");
            return 1;
        }

        size_t size = 0;
        unsigned char *content = get_file_content(&ctx, filename, &size);
        if (content) {
            // Write to stdout or save? Let's print string if it looks like one, or binary.
            // For safety, let's just print as string with length constraint
            printf("Content of %s (%ld bytes):\n", filename, size);
            fwrite(content, 1, size, stdout);
            printf("\n");
            free(content);
        } else {
            fprintf(stderr, "File not found or error.\n");
        }
        close_filesystem(&ctx);

    } else if (strcmp(cmd, "list") == 0) {
        FSContext ctx;
        if (load_filesystem(fs_file, &ctx) != 0) {
            fprintf(stderr, "Failed to load FS.\n");
            return 1;
        }
        list_files(&ctx);
        close_filesystem(&ctx);
    } else {
        printf("Unknown command.\n");
        return 1;
    }

    return 0;
}
