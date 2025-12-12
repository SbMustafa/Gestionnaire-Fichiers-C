#ifndef FS_STRUCTS_H
#define FS_STRUCTS_H

#include <stdint.h>

#define MAX_NAME_LEN 64
#define MAGIC_NUMBER 0xCAFEBABE

// Offsets in the binary file are represented as long (or int64_t usually, using long for C ANSI compatibility in 64-bit env, or int64_t if available. Using long per prompt implication, but distinct type is safer).
// Prompt struct says: "root_inode_offset (position...)"
// We will use long to match stdio fseek/ftell, but strictly explicit width is better. Let's use long as requested implicitly by "standard C" / "offsets".
// Actually, let's use int64_t for portability if we can, but prompt asked for "C ANSI" (C89/C90) which doesn't have int64_t.
// However, C99 is standard now. I'll use `long` as it's standard C89 and usually sufficient, or `size_t`? `long` is best for fseek.

typedef struct SuperBlock {
    long magic_number;
    long root_inode_offset;      // Offset to the root RBTNode of the entire FS (or root dir)
    long next_free_page_offset;  // Simple allocator implementation
    long fs_size;
} SuperBlock;

typedef enum NodeType {
    FILE_NODE,
    DIRECTORY_NODE
} NodeType;

typedef struct Inode {
    int type; // NodeType
    char name[MAX_NAME_LEN];
    long parent_offset;         // Offset of the parent Inode (or RBTNode?) - Prompt says "offset du parent"
                                // If Inodes are inside RBTNodes, this probably points to the parent Directory's RBTNode or Inode logic.
                                // Let's assume it points to the Parent Directory Inode's RBTNode offset.
    
    // For Directories:
    long children_offset;       // Offset to the Root RBTNode of the children tree. -1 if empty.

    // For Files:
    long data_offset;           // Offset to the first chunk of data.
    long original_size;
    long compressed_size;
} Inode;

typedef enum RBTColor {
    RED,
    BLACK
} RBTColor;

typedef struct RBTNode {
    Inode inode;
    int color; // RBTColor
    long left_offset;
    long right_offset;
    long parent_offset; // Parent in the RB Tree structure, distinct from Inode.parent_offset (which is logical FS parent)
} RBTNode;

#endif // FS_STRUCTS_H
