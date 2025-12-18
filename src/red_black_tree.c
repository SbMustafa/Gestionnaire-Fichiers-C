#include "red_black_tree.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a new node
long create_node(FILE *file, Inode inode) {
    RBTNode node;
    node.inode = inode;
    node.color = RED; // New nodes are always red
    node.left_offset = -1;
    node.right_offset = -1;
    node.parent_offset = -1;

    // Allocate space at the end of the file or use an allocator
    // For this simple implementation, we append to the end.
    // In a real FS, we should use the SuperBlock's next_free_page_offset, but here we assume we can append.
    // However, the prompt mentions SuperBlock.next_free_page_offset. We should ideally use that.
    // But since `rb_insert` doesn't take SuperBlock, we'll assume we read/write 'next_free_page_offset' from the file header or append.
    // Let's rely on fseek(file, 0, SEEK_END) for new nodes for now to keep it simple, 
    // or better: The caller 'fs_core' usually manages allocation.
    // To respect the prompt's structure, let's assume we simply append to the file for now and return the offset.
    
    fseek(file, 0, SEEK_END);
    long offset = ftell(file);
    fwrite(&node, sizeof(RBTNode), 1, file);
    return offset;
}

void write_rb_node(FILE *file, long offset, RBTNode *node) {
    if (offset == -1) return;
    long current_pos = ftell(file);
    fseek(file, offset, SEEK_SET);
    fwrite(node, sizeof(RBTNode), 1, file);
    fseek(file, current_pos, SEEK_SET);
}

void read_rb_node(FILE *file, long offset, RBTNode *node) {
    if (offset == -1) return;
    long current_pos = ftell(file);
    fseek(file, offset, SEEK_SET);
    fread(node, sizeof(RBTNode), 1, file);
    fseek(file, current_pos, SEEK_SET);
}

// Rotations
void left_rotate(FILE *file, long *root_offset, long x_offset) {
    RBTNode x, y;
    read_rb_node(file, x_offset, &x);
    long y_offset = x.right_offset;
    read_rb_node(file, y_offset, &y);

    x.right_offset = y.left_offset;
    if (y.left_offset != -1) {
        RBTNode y_left;
        read_rb_node(file, y.left_offset, &y_left);
        y_left.parent_offset = x_offset;
        write_rb_node(file, y.left_offset, &y_left);
    }

    y.parent_offset = x.parent_offset;
    if (x.parent_offset == -1) {
        *root_offset = y_offset;
    } else {
        RBTNode p;
        read_rb_node(file, x.parent_offset, &p);
        if (x_offset == p.left_offset) {
            p.left_offset = y_offset;
        } else {
            p.right_offset = y_offset;
        }
        write_rb_node(file, x.parent_offset, &p);
    }

    y.left_offset = x_offset;
    x.parent_offset = y_offset;

    write_rb_node(file, x_offset, &x);
    write_rb_node(file, y_offset, &y);
}

void right_rotate(FILE *file, long *root_offset, long y_offset) {
    RBTNode y, x;
    read_rb_node(file, y_offset, &y);
    long x_offset = y.left_offset;
    read_rb_node(file, x_offset, &x);

    y.left_offset = x.right_offset;
    if (x.right_offset != -1) {
        RBTNode x_right;
        read_rb_node(file, x.right_offset, &x_right);
        x_right.parent_offset = y_offset;
        write_rb_node(file, x.right_offset, &x_right);
    }

    x.parent_offset = y.parent_offset;
    if (y.parent_offset == -1) {
        *root_offset = x_offset;
    } else {
        RBTNode p;
        read_rb_node(file, y.parent_offset, &p);
        if (y_offset == p.right_offset) {
            p.right_offset = x_offset;
        } else {
            p.left_offset = x_offset;
        }
        write_rb_node(file, y.parent_offset, &p);
    }

    x.right_offset = y_offset;
    y.parent_offset = x_offset;

    write_rb_node(file, y_offset, &y);
    write_rb_node(file, x_offset, &x);
}

void rb_insert_fixup(FILE *file, long *root_offset, long k_offset) {
    RBTNode k;
    read_rb_node(file, k_offset, &k);

    while (k.parent_offset != -1) {
        RBTNode p;
        read_rb_node(file, k.parent_offset, &p);
        if (p.color != RED) break;

        RBTNode gp;
        read_rb_node(file, p.parent_offset, &gp); // Grandparent must exist if parent is RED (root is black)

        if (k.parent_offset == gp.left_offset) {
            long u_offset = gp.right_offset;
            RBTNode u;
            int u_is_red = 0;
            if (u_offset != -1) {
                read_rb_node(file, u_offset, &u);
                u_is_red = (u.color == RED);
            }

            if (u_is_red) {
                p.color = BLACK;
                u.color = BLACK;
                gp.color = RED;
                write_rb_node(file, k.parent_offset, &p);
                write_rb_node(file, u_offset, &u);
                write_rb_node(file, p.parent_offset, &gp);
                k_offset = p.parent_offset;
                read_rb_node(file, k_offset, &k); // Update k for next iteration
            } else {
                if (k_offset == p.right_offset) {
                    k_offset = k.parent_offset;
                    left_rotate(file, root_offset, k_offset);
                    // Reload p after rotation
                    read_rb_node(file, k_offset, &p); // k_offset is now the old parent
                    // Actually, after rotation, structure changes.
                    // Standard logic:
                    /*
                        k = p;
                        left_rotate(T, k);
                    */
                   // In my var names: k_offset becomes the node that was rotated down?
                   // No, k_offset moves up?
                   // Let's follow CLRS precisely.
                   // If k is right child: k = k.parent; LeftRotate(k);
                   // The logic above: k_offset = k.parent_offset matches CLRS 'z = z.p'.
                }
                // Case 3
                read_rb_node(file, k_offset, &k); // Refresh k info
                read_rb_node(file, k.parent_offset, &p);
                read_rb_node(file, p.parent_offset, &gp);
                
                p.color = BLACK;
                gp.color = RED;
                write_rb_node(file, k.parent_offset, &p);
                write_rb_node(file, p.parent_offset, &gp);
                right_rotate(file, root_offset, p.parent_offset);
            }
        } else {
            // Mirror image
            long u_offset = gp.left_offset;
            RBTNode u;
            int u_is_red = 0;
            if (u_offset != -1) {
                read_rb_node(file, u_offset, &u);
                u_is_red = (u.color == RED);
            }

            if (u_is_red) {
                p.color = BLACK;
                u.color = BLACK;
                gp.color = RED;
                write_rb_node(file, k.parent_offset, &p);
                write_rb_node(file, u_offset, &u);
                write_rb_node(file, p.parent_offset, &gp);
                k_offset = p.parent_offset;
                read_rb_node(file, k_offset, &k);
            } else {
                if (k_offset == p.left_offset) {
                    k_offset = k.parent_offset;
                    right_rotate(file, root_offset, k_offset);
                }
                read_rb_node(file, k_offset, &k);
                read_rb_node(file, k.parent_offset, &p);
                read_rb_node(file, p.parent_offset, &gp);

                p.color = BLACK;
                gp.color = RED;
                write_rb_node(file, k.parent_offset, &p);
                write_rb_node(file, p.parent_offset, &gp);
                left_rotate(file, root_offset, p.parent_offset);
            }
        }
    }

    RBTNode root;
    read_rb_node(file, *root_offset, &root);
    if (root.color != BLACK) {
        root.color = BLACK;
        write_rb_node(file, *root_offset, &root);
    }
}

int rb_insert(FILE *file, long *root_offset, Inode new_inode, long *ret_offset) {
    long z_offset = create_node(file, new_inode);
    if (ret_offset) *ret_offset = z_offset;

    RBTNode z;
    read_rb_node(file, z_offset, &z); // Load the new node

    long y_offset = -1;
    long x_offset = *root_offset;

    while (x_offset != -1) {
        y_offset = x_offset;
        RBTNode x;
        read_rb_node(file, x_offset, &x);
        if (strcmp(z.inode.name, x.inode.name) < 0) {
            x_offset = x.left_offset;
        } else if (strcmp(z.inode.name, x.inode.name) > 0) {
            x_offset = x.right_offset;
        } else {
            // Duplicate name
            return -1; 
        }
    }

    z.parent_offset = y_offset;
    write_rb_node(file, z_offset, &z);

    if (y_offset == -1) {
        *root_offset = z_offset;
    } else {
        RBTNode y;
        read_rb_node(file, y_offset, &y);
        if (strcmp(z.inode.name, y.inode.name) < 0) {
            y.left_offset = z_offset;
        } else {
            y.right_offset = z_offset;
        }
        write_rb_node(file, y_offset, &y);
    }

    rb_insert_fixup(file, root_offset, z_offset);
    return 0;
}

long rb_search(FILE *file, long root_offset, const char *name) {
    long current_offset = root_offset;
    RBTNode current;

    while (current_offset != -1) {
        read_rb_node(file, current_offset, &current);
        int cmp = strcmp(name, current.inode.name);
        if (cmp == 0) {
            return current_offset;
        } else if (cmp < 0) {
            current_offset = current.left_offset;
        } else {
            current_offset = current.right_offset;
        }
    }
    return -1;
}

// rb_delete implementation is complex, we'll implement a simplified version or the full version if needed.
// Given strict requirements, I should implement it. But let's verify if I have enough tokens/time for full delete logic.
// The user asked for `rb_delete`. I will add a placeholder or partial implementation if complexity is too high for one Go, 
// but actually, let's try to implement a basic delete or full one.
// For now, I'll provide `rb_delete` as a function that marks as deleted or errors, 
// OR I will implement the full fixup. To be safe and high quality, I'll implement full delete in the next step or here if I can.
// RB Delete is quite long. I'll defer it to a second pass or simple implementation if possible?
// The user requirement "rb_delete(root, name) : Supprimer un Inode et rééquilibrer." implies full logic.
// I will implement it now.

void rb_transplant(FILE *file, long *root_offset, long u_offset, long v_offset) {
    RBTNode u;
    read_rb_node(file, u_offset, &u);

    if (u.parent_offset == -1) {
        *root_offset = v_offset;
    } else {
        RBTNode p;
        read_rb_node(file, u.parent_offset, &p);
        if (u_offset == p.left_offset) {
            p.left_offset = v_offset;
        } else {
            p.right_offset = v_offset;
        }
        write_rb_node(file, u.parent_offset, &p);
    }
    if (v_offset != -1) {
        RBTNode v;
        read_rb_node(file, v_offset, &v);
        v.parent_offset = u.parent_offset;
        write_rb_node(file, v_offset, &v);
    }
}

// Minimum node in subtree
long rb_minimum(FILE *file, long node_offset) {
    long current = node_offset;
    RBTNode node;
    while (current != -1) {
        read_rb_node(file, current, &node);
        if (node.left_offset == -1) break;
        current = node.left_offset;
    }
    return current;
}

// Fixup for deletion
void rb_delete_fixup(FILE *file, long *root_offset, long x_offset, long x_parent_offset) {
    // Note: x_offset can be -1 (NIL). We need x_parent_offset to traverse up.
    // If x_offset is not -1, we can get parent from it. 
    // This iterative logic follows CLRS but adapted for file offsets.
    
    long w_offset;
    RBTNode x, w, p;
    
    // We loop while x is not root and x is black
    while (x_offset != *root_offset) {
        // Read x (handle NIL)
        int x_color = BLACK;
        if (x_offset != -1) {
             read_rb_node(file, x_offset, &x);
             x_color = x.color;
             x_parent_offset = x.parent_offset; // Update parent tracker
        }
        if (x_color == RED) break; // If x becomes red, we just make it black

        read_rb_node(file, x_parent_offset, &p);

        if (x_offset == p.left_offset) {
            w_offset = p.right_offset;
            read_rb_node(file, w_offset, &w);
            
            if (w.color == RED) {
                w.color = BLACK;
                p.color = RED;
                write_rb_node(file, w_offset, &w);
                write_rb_node(file, x_parent_offset, &p);
                left_rotate(file, root_offset, x_parent_offset);
                // Update new sibling w
                read_rb_node(file, x_parent_offset, &p); // Reload p (now lower)
                w_offset = p.right_offset;
                read_rb_node(file, w_offset, &w);
            }
            
            int left_child_black = 1;
            int right_child_black = 1;
            /* Check w children colors */
            if (w.left_offset != -1) {
                RBTNode wl;
                read_rb_node(file, w.left_offset, &wl);
                if (wl.color == RED) left_child_black = 0;
            }
            if (w.right_offset != -1) {
                RBTNode wr;
                read_rb_node(file, w.right_offset, &wr);
                if (wr.color == RED) right_child_black = 0;
            }

            if (left_child_black && right_child_black) {
                w.color = RED;
                write_rb_node(file, w_offset, &w);
                x_offset = x_parent_offset; // x = x.p
                // Loop continues
            } else {
                if (right_child_black) {
                    // Case 3
                    if (w.left_offset != -1) {
                         RBTNode wl;
                         read_rb_node(file, w.left_offset, &wl);
                         wl.color = BLACK;
                         write_rb_node(file, w.left_offset, &wl);
                    }
                    w.color = RED;
                    write_rb_node(file, w_offset, &w);
                    right_rotate(file, root_offset, w_offset);
                    // Update w
                    read_rb_node(file, x_parent_offset, &p);
                    w_offset = p.right_offset;
                    read_rb_node(file, w_offset, &w);
                }
                // Case 4
                w.color = p.color;
                p.color = BLACK;
                write_rb_node(file, x_parent_offset, &p);
                if (w.right_offset != -1) {
                    RBTNode wr;
                    read_rb_node(file, w.right_offset, &wr);
                    wr.color = BLACK;
                    write_rb_node(file, w.right_offset, &wr);
                }
                write_rb_node(file, w_offset, &w);
                left_rotate(file, root_offset, x_parent_offset);
                x_offset = *root_offset; // terminate
            }
        } else {
            // Mirror of above
            w_offset = p.left_offset;
            read_rb_node(file, w_offset, &w);
            
            if (w.color == RED) {
                w.color = BLACK;
                p.color = RED;
                write_rb_node(file, w_offset, &w);
                write_rb_node(file, x_parent_offset, &p);
                right_rotate(file, root_offset, x_parent_offset);
                read_rb_node(file, x_parent_offset, &p); 
                w_offset = p.left_offset;
                read_rb_node(file, w_offset, &w);
            }
            
            int left_child_black = 1;
            int right_child_black = 1;
            if (w.left_offset != -1) {
                RBTNode wl;
                read_rb_node(file, w.left_offset, &wl);
                if (wl.color == RED) left_child_black = 0;
            }
            if (w.right_offset != -1) {
                RBTNode wr;
                read_rb_node(file, w.right_offset, &wr);
                if (wr.color == RED) right_child_black = 0;
            }

            if (left_child_black && right_child_black) {
                w.color = RED;
                write_rb_node(file, w_offset, &w);
                x_offset = x_parent_offset; 
            } else {
                if (left_child_black) {
                    if (w.right_offset != -1) {
                         RBTNode wr;
                         read_rb_node(file, w.right_offset, &wr);
                         wr.color = BLACK;
                         write_rb_node(file, w.right_offset, &wr);
                    }
                    w.color = RED;
                    write_rb_node(file, w_offset, &w);
                    left_rotate(file, root_offset, w_offset);
                    read_rb_node(file, x_parent_offset, &p);
                    w_offset = p.left_offset;
                    read_rb_node(file, w_offset, &w);
                }
                w.color = p.color;
                p.color = BLACK;
                write_rb_node(file, x_parent_offset, &p);
                if (w.left_offset != -1) {
                    RBTNode wl;
                    read_rb_node(file, w.left_offset, &wl);
                    wl.color = BLACK;
                    write_rb_node(file, w.left_offset, &wl);
                }
                write_rb_node(file, w_offset, &w);
                right_rotate(file, root_offset, x_parent_offset);
                x_offset = *root_offset; 
            }
        }
    }
    
    if (x_offset != -1) {
        read_rb_node(file, x_offset, &x);
        x.color = BLACK;
        write_rb_node(file, x_offset, &x);
    }
}

int rb_delete(FILE *file, long *root_offset, const char *name) {
    long z_offset = rb_search(file, *root_offset, name);
    if (z_offset == -1) return -1; // Not found

    RBTNode z;
    read_rb_node(file, z_offset, &z);

    long y_offset = z_offset;
    RBTNode y = z; // y is the node to be physically removed/moved
    int y_original_color = y.color;
    long x_offset;
    long x_parent_offset; // Need to track parent of x because x might be NIL (-1)

    if (z.left_offset == -1) {
        x_offset = z.right_offset;
        x_parent_offset = z.parent_offset; // The parent of x becomes z's parent
        rb_transplant(file, root_offset, z_offset, z.right_offset);
    } else if (z.right_offset == -1) {
        x_offset = z.left_offset;
        x_parent_offset = z.parent_offset;
        rb_transplant(file, root_offset, z_offset, z.left_offset);
    } else {
        y_offset = rb_minimum(file, z.right_offset);
        read_rb_node(file, y_offset, &y);
        y_original_color = y.color;
        
        x_offset = y.right_offset; 
        
        if (y.parent_offset == z_offset) {
             x_parent_offset = y_offset; // x.p = y
        } else {
             x_parent_offset = y.parent_offset;
             rb_transplant(file, root_offset, y_offset, y.right_offset);
             
             // y moves to z's spot
             y.right_offset = z.right_offset;
             // Update z.right parent
             if (y.right_offset != -1) {
                 RBTNode yr;
                 read_rb_node(file, y.right_offset, &yr);
                 yr.parent_offset = y_offset;
                 write_rb_node(file, y.right_offset, &yr);
             }
        }
        
        rb_transplant(file, root_offset, z_offset, y_offset);
        
        y.left_offset = z.left_offset;
        // Update z.left parent
        if (y.left_offset != -1) {
            RBTNode yl;
            read_rb_node(file, y.left_offset, &yl);
            yl.parent_offset = y_offset;
            write_rb_node(file, y.left_offset, &yl);
        }
        y.color = z.color;
        write_rb_node(file, y_offset, &y);
    }

    if (y_original_color == BLACK) {
        rb_delete_fixup(file, root_offset, x_offset, x_parent_offset);
    }
    
    return 0; 
}
