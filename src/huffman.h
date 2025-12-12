#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stddef.h>

#define MAX_SYMBOLS 256

typedef struct HuffmanNode {
    unsigned char symbol;
    unsigned int frequency;
    struct HuffmanNode *left, *right;
} HuffmanNode;

typedef struct {
    unsigned char *data;
    size_t size;         // Size in bits usually, or bytes with a bit count
    size_t bit_count;    // Total bits used
} CompressedData;

// Function prototypes
HuffmanNode* build_huffman_tree(const unsigned char *data, size_t size);
void free_huffman_tree(HuffmanNode *root);
void generate_huffman_codes(HuffmanNode *root, char *codes[MAX_SYMBOLS]);

// Compresses data. 
// Returns a buffer that must be freed by caller. 
// out_size is set to the size of the returned buffer in bytes.
unsigned char* compress_data(const unsigned char *data, size_t size, size_t *out_size);

// Decompresses data.
// Returns a buffer with original data.
// original_size must be known (or we can store it in the compressed stream, but Inode handles it).
// The tree must be reconstructed or stored. 
// Requirement: "decompress_data(compressed_data, size, tree, output_buffer)"
// We need to support serializing the tree or rebuilding it? 
// Usually we store the frequency table or the tree structure with the file.
// For this assignment, we might assume the tree is stored or frequencies are stored.
// Let's implement a way to serialize/deserialize the tree or frequency table to be embedded in the file.
// For simplicity, we'll prefix the compressed data with the frequency table (256 * 4 bytes = 1KB overhead, acceptable?).
// Or optimized tree serialization.
unsigned char* decompress_data(const unsigned char *compressed_data, size_t compressed_size, size_t original_size);

#endif // HUFFMAN_H
