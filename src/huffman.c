#include "huffman.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Priority Queue for Huffman Tree construction
typedef struct PQNode {
    HuffmanNode *tree_node;
    struct PQNode *next;
} PQNode;

PQNode* pq_new(HuffmanNode *tree_node) {
    PQNode *node = malloc(sizeof(PQNode));
    node->tree_node = tree_node;
    node->next = NULL;
    return node;
}

void pq_insert(PQNode **head, HuffmanNode *tree_node) {
    PQNode *node = pq_new(tree_node);
    if (!*head || (*head)->tree_node->frequency >= tree_node->frequency) {
        node->next = *head;
        *head = node;
    } else {
        PQNode *current = *head;
        while (current->next && current->next->tree_node->frequency < tree_node->frequency) {
            current = current->next;
        }
        node->next = current->next;
        current->next = node;
    }
}

HuffmanNode* pq_pop(PQNode **head) {
    if (!*head) return NULL;
    PQNode *temp = *head;
    *head = (*head)->next;
    HuffmanNode *tree_node = temp->tree_node;
    free(temp);
    return tree_node;
}

HuffmanNode* build_huffman_tree_from_freq(unsigned int freq[MAX_SYMBOLS]) {
    PQNode *head = NULL;
    for (int i = 0; i < MAX_SYMBOLS; i++) {
        if (freq[i] > 0) {
            HuffmanNode *node = malloc(sizeof(HuffmanNode));
            node->symbol = (unsigned char)i;
            node->frequency = freq[i];
            node->left = NULL;
            node->right = NULL;
            pq_insert(&head, node);
        }
    }

    if (!head) return NULL;

    while (head->next) {
        HuffmanNode *left = pq_pop(&head);
        HuffmanNode *right = pq_pop(&head);

        HuffmanNode *parent = malloc(sizeof(HuffmanNode));
        parent->symbol = 0; // Internal node
        parent->frequency = left->frequency + right->frequency;
        parent->left = left;
        parent->right = right;

        pq_insert(&head, parent);
    }

    return pq_pop(&head);
}

void free_huffman_tree(HuffmanNode *root) {
    if (!root) return;
    free_huffman_tree(root->left);
    free_huffman_tree(root->right);
    free(root);
}

void generate_codes_recursive(HuffmanNode *node, char *current_code, int depth, char *codes[MAX_SYMBOLS]) {
    if (!node) return;
    if (!node->left && !node->right) {
        current_code[depth] = '\0';
        codes[node->symbol] = strdup(current_code);
        return;
    }
    if (node->left) {
        current_code[depth] = '0';
        generate_codes_recursive(node->left, current_code, depth + 1, codes);
    }
    if (node->right) {
        current_code[depth] = '1';
        generate_codes_recursive(node->right, current_code, depth + 1, codes);
    }
}

void generate_huffman_codes(HuffmanNode *root, char *codes[MAX_SYMBOLS]) {
    char buffer[256];
    generate_codes_recursive(root, buffer, 0, codes);
}

// Compress data: | Freq Table (256*4 bytes) | Data content |
unsigned char* compress_data(const unsigned char *data, size_t size, size_t *out_size) {
    unsigned int freq[MAX_SYMBOLS] = {0};
    for (size_t i = 0; i < size; i++) freq[data[i]]++;

    HuffmanNode *root = build_huffman_tree_from_freq(freq);
    char *codes[MAX_SYMBOLS] = {0};
    if (root) generate_huffman_codes(root, codes);

    // Calculate size
    // Header size: 256 * 4 bytes for frequencies
    size_t header_size = MAX_SYMBOLS * sizeof(unsigned int);
    size_t buffer_size = header_size + size; // Approximation, usually smaller compressed
    unsigned char *output = malloc(buffer_size); // Initial guess, might realloc if strictly needed, but let's be safe
    // Since Huffman can expand slightly in worst case or overhead... let's allocate 2*size + header
    unsigned char *final_output = malloc(header_size + size * 2 + 1024);
    
    // Write header
    memcpy(final_output, freq, header_size);

    size_t bit_pos = 0;
    size_t byte_pos = header_size;
    memset(final_output + byte_pos, 0, size * 2); 

    for (size_t i = 0; i < size; i++) {
        char *code = codes[data[i]];
        if (!code) continue; 
        for (int j = 0; code[j]; j++) {
            if (code[j] == '1') {
                final_output[byte_pos] |= (1 << (7 - bit_pos));
            }
            bit_pos++;
            if (bit_pos == 8) {
                bit_pos = 0;
                byte_pos++;
            }
        }
    }
    if (bit_pos > 0) byte_pos++; // Finish partial byte

    *out_size = byte_pos;

    // Cleanup
    free_huffman_tree(root);
    for (int i = 0; i < MAX_SYMBOLS; i++) {
        if (codes[i]) free(codes[i]);
    }
    free(output); // Oops, I used final_output. Free unused `output`.
    
    return final_output;
}

unsigned char* decompress_data(const unsigned char *compressed_data, size_t compressed_size, size_t original_size) {
    if (compressed_size < MAX_SYMBOLS * sizeof(unsigned int)) return NULL;

    unsigned int freq[MAX_SYMBOLS];
    memcpy(freq, compressed_data, MAX_SYMBOLS * sizeof(unsigned int));

    HuffmanNode *root = build_huffman_tree_from_freq(freq);
    unsigned char *output = malloc(original_size + 1); // +1 safety

    size_t bit_pos = 0;
    size_t byte_pos = MAX_SYMBOLS * sizeof(unsigned int);
    size_t out_pos = 0;

    HuffmanNode *current = root;
    while (out_pos < original_size && byte_pos < compressed_size) {
        int bit = (compressed_data[byte_pos] >> (7 - bit_pos)) & 1;
        bit_pos++;
        if (bit_pos == 8) {
            bit_pos = 0;
            byte_pos++;
        }

        if (bit == 0) current = current->left;
        else current = current->right;

        if (!current->left && !current->right) {
            output[out_pos++] = current->symbol;
            current = root;
        }
    }

    free_huffman_tree(root);
    return output;
}
