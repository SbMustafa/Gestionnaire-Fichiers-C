#include "interface.h"
#include "../fs_core.h"
#include "../red_black_tree.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

/**
 * Structure pour passer les données de l'application aux callbacks.
 */
typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkTreeStore *tree_store;
    GtkTextView *log_view;
    FSContext *fs_ctx;
} AppData;

/**
 * Enumération pour les colonnes du TreeView.
 */
enum {
    COLUMN_ICON,
    COLUMN_NAME,
    COLUMN_SIZE_ORIG,
    COLUMN_SIZE_COMP,
    COLUMN_TYPE,
    COLUMN_NODE_OFFSET, // Pour garder une référence vers le noeud du fichier
    N_COLUMNS
};

/* --- Fonctions Utilitaires --- */

/**
 * Affiche un message dans la zone de simulation de console.
 */
void log_message(AppData *app, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(app->log_view);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(text_buffer, &end);
    gtk_text_buffer_insert(text_buffer, &end, buffer, -1);
    gtk_text_buffer_insert(text_buffer, &end, "\n", 1);
    
    // Auto-scroll vers le bas
    GtkTextMark *mark = gtk_text_buffer_create_mark(text_buffer, NULL, &end, FALSE);
    gtk_text_view_scroll_to_mark(app->log_view, mark, 0.0, TRUE, 0.0, 1.0);
}

/**
 * Fonction récursive pour parcourir l'arbre binaire et remplir le GtkTreeStore.
 * Cette fonction lit les noeuds directement depuis le fichier.
 */
void traverser_et_remplir_tree(FILE *file, long current_offset, GtkTreeStore *store, GtkTreeIter *parent) {
    if (current_offset == -1) return;

    RBTNode node;
    // Lecture du noeud à l'offset donné
    read_rb_node(file, current_offset, &node);

    // On traite le sous-arbre gauche (ordre in-order pour l'affichage alphabétique si l'arbre est trié)
    traverser_et_remplir_tree(file, node.left_offset, store, parent);

    // Ajout du noeud courant dans l'interface
    GtkTreeIter iter;
    gtk_tree_store_append(store, &iter, parent);
    
    // Choix de l'icône selon le type
    const char *icon_name = (node.inode.type == DIRECTORY_NODE) ? "folder" : "text-x-generic";
    if (node.inode.type == DIRECTORY_NODE) {
        // Pour être sûr d'utiliser une icône dossier standard
        icon_name = "folder"; 
    }

    gtk_tree_store_set(store, &iter,
                       COLUMN_ICON, icon_name,
                       COLUMN_NAME, node.inode.name,
                       COLUMN_SIZE_ORIG, node.inode.original_size,
                       COLUMN_SIZE_COMP, node.inode.compressed_size,
                       COLUMN_TYPE, (node.inode.type == FILE_NODE) ? "Fichier" : "Dossier",
                       COLUMN_NODE_OFFSET, current_offset,
                       -1);

    // Si c'est un dossier et qu'il a des enfants, on devrait théoriquement appeler récursivement
    // sur le children_offset de l'inode.
    // MAIS : Dans l'implémentation actuelle, nous avons un arbre à plat ou basique.
    // Si la structure supportait les dossiers, on ferait :
    /*
    if (node.inode.type == DIRECTORY_NODE && node.inode.children_offset != -1) {
        traverser_et_remplir_tree(file, node.inode.children_offset, store, &iter);
    }
    */
   
    // On traite le sous-arbre droit
    traverser_et_remplir_tree(file, node.right_offset, store, parent);
}

/**
 * Actualise l'affichage de l'arbre.
 */
void actualiser_arborescence(AppData *app) {
    gtk_tree_store_clear(app->tree_store);
    
    if (app->fs_ctx->sb.root_inode_offset != -1) {
        traverser_et_remplir_tree(app->fs_ctx->file, app->fs_ctx->sb.root_inode_offset, app->tree_store, NULL);
    } else {
        log_message(app, "Système de fichiers vide.");
    }
}


/* --- Callbacks --- */

/**
 * Callback bouton "Ajouter".
 */
void on_add_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;

    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Ouvrir un fichier",
                                         GTK_WINDOW(app->window),
                                         action,
                                         "_Annuler",
                                         GTK_RESPONSE_CANCEL,
                                         "_Ouvrir",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // Lire le fichier réel
        FILE *f = fopen(filename, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            unsigned char *content = malloc(fsize);
            fread(content, 1, fsize, f);
            fclose(f);

            // Extraire le nom de fichier du chemin complet
            char *basename = g_path_get_basename(filename);

            // Ajouter au système
            log_message(app, "Importation de %s (%ld octets)...", basename, fsize);
            int ret = add_file(app->fs_ctx, basename, content, fsize);
            
            if (ret == 0) {
                log_message(app, "Succès : Fichier ajouté.");
                actualiser_arborescence(app);
            } else {
                log_message(app, "Erreur : Impossible d'ajouter le fichier (Code %d).", ret);
            }

            free(content);
            g_free(basename);
        } else {
            log_message(app, "Erreur : Impossible de lire le fichier source.");
        }
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

/**
 * Callback bouton "Supprimer".
 */
void on_delete_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        char *name;
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);

        log_message(app, "Suppression de %s...", name);
        
        // Appel à la logique de suppression (rb_delete)
        long *root_ptr = &app->fs_ctx->sb.root_inode_offset;
        int ret = rb_delete(app->fs_ctx->file, root_ptr, name);
        
        if (ret == 0) {
            // Mettre à jour le SuperBlock car la racine a pu changer
            fseek(app->fs_ctx->file, 0, SEEK_SET);
            fwrite(&app->fs_ctx->sb, sizeof(SuperBlock), 1, app->fs_ctx->file);
            
            log_message(app, "Fichier supprimé.");
            actualiser_arborescence(app);
        } else {
             log_message(app, "Erreur : Fichier non trouvé ou suppression échouée.");
        }

        g_free(name);
    } else {
        log_message(app, "Aucun fichier sélectionné.");
    }
}

/**
 * Callback bouton "Extraire".
 */
void on_extract_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        char *name;
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);

        log_message(app, "Extraction de %s...", name);

        size_t size;
        unsigned char *data_ptr = get_file_content(app->fs_ctx, name, &size);

        if (data_ptr) {
            // Sauvegarder sur le disque
            // Pour simplifier, on extrait dans le dossier courant avec le préfixe "extracted_"
            char out_name[256];
            snprintf(out_name, sizeof(out_name), "extracted_%s", name);
            
            FILE *f_out = fopen(out_name, "wb");
            if (f_out) {
                fwrite(data_ptr, 1, size, f_out);
                fclose(f_out);
                log_message(app, "Fichier extrait vers : %s", out_name);
            } else {
                log_message(app, "Erreur d'écriture sur le disque.");
            }
            free(data_ptr);
        } else {
            log_message(app, "Erreur : Impossible de lire les données du fichier.");
        }
        g_free(name);
    } else {
        log_message(app, "Aucun fichier sélectionné.");
    }
}

/**
 * Callback Entrée Commande (Simulation Console).
 */
void on_command_activate(GtkEntry *entry, gpointer data) {
    AppData *app = (AppData *)data;
    const char *text = gtk_entry_get_text(entry);

    log_message(app, "> %s", text);

    if (strcmp(text, "ls") == 0) {
        log_message(app, "--- Liste des fichiers ---");
        // On pourrait réutiliser list_files du core, mais on a déjà l'arbre. 
        // On va juste dire "Voir arbre ci-dessus".
        log_message(app, "(Voir l'arborescence graphique)");
    } else if (strncmp(text, "rm ", 3) == 0) {
        const char *name = text + 3;
        // Simuler clic suppression (sans sélection, donc appel direct logique)
        long *root_ptr = &app->fs_ctx->sb.root_inode_offset;
        int ret = rb_delete(app->fs_ctx->file, root_ptr, name);
         if (ret == 0) {
            fseek(app->fs_ctx->file, 0, SEEK_SET);
            fwrite(&app->fs_ctx->sb, sizeof(SuperBlock), 1, app->fs_ctx->file);
            log_message(app, "Fichier %s supprimé.", name);
            actualiser_arborescence(app);
        } else {
             log_message(app, "Erreur lors de la suppression de %s.", name);
        }
    } else if (strncmp(text, "add ", 4) == 0) {
        log_message(app, "Utilisez le bouton 'Ajouter' pour une meilleure expérience.");
    } else {
        log_message(app, "Commande inconnue : %s", text);
    }

    gtk_entry_set_text(entry, "");
}

/* --- Fonction Principale UI --- */

void lancer_interface(int argc, char *argv[]) {
    AppData app;
    
    // Initialisation GTK
    gtk_init(&argc, &argv);

    // Initialisation Système de Fichiers
    // On suppose que main.c a déjà initialisé ou chargé, mais ici on va re-charger ou utiliser un contexte global.
    // Pour faire propre, on crée un contexte local ici car lancer_interface est bloquant.
    FSContext ctx;
    if (load_filesystem("fs_data.bin", &ctx) != 0) {
        printf("Création d'un nouveau système de fichiers...\n");
        init_filesystem("fs_data.bin");
        load_filesystem("fs_data.bin", &ctx);
    }
    app.fs_ctx = &ctx;

    // Création Fenêtre
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Gestionnaire de Fichiers C");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 800, 600);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Layout Principal (VBox)
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);

    // -- Partie Haute : Toolbar + Arbre --
    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_top, TRUE, TRUE, 0);

    // Panneau Latéral (TreeView)
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 600, -1);
    gtk_box_pack_start(GTK_BOX(hbox_top), scrolled_window, TRUE, TRUE, 0);

    app.tree_store = gtk_tree_store_new(N_COLUMNS, 
                                        G_TYPE_STRING, // Icon 
                                        G_TYPE_STRING, // Name
                                        G_TYPE_LONG,   // Size Orig
                                        G_TYPE_LONG,   // Size Comp
                                        G_TYPE_STRING, // Type Str
                                        G_TYPE_LONG);  // Node Offset

    app.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.tree_store));

    // Cell Renderers
    GtkCellRenderer *renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *col_icon = gtk_tree_view_column_new_with_attributes("Type", renderer_pixbuf, "icon-name", COLUMN_ICON, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_icon);

    GtkCellRenderer *renderer_text = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes("Nom", renderer_text, "text", COLUMN_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_name);

    GtkTreeViewColumn *col_size = gtk_tree_view_column_new_with_attributes("Taille Originale", renderer_text, "text", COLUMN_SIZE_ORIG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_size);

    gtk_container_add(GTK_CONTAINER(scrolled_window), app.tree_view);

    // Panneau Actions (Boutons)
    GtkWidget *vbox_buttons = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(hbox_top), vbox_buttons, FALSE, FALSE, 10);

    GtkWidget *btn_add = gtk_button_new_with_label("Ajouter Fichier");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_clicked), &app);
    gtk_box_pack_start(GTK_BOX(vbox_buttons), btn_add, FALSE, FALSE, 0);

    GtkWidget *btn_del = gtk_button_new_with_label("Supprimer");
    g_signal_connect(btn_del, "clicked", G_CALLBACK(on_delete_clicked), &app);
    gtk_box_pack_start(GTK_BOX(vbox_buttons), btn_del, FALSE, FALSE, 0);

    GtkWidget *btn_extract = gtk_button_new_with_label("Extraire");
    g_signal_connect(btn_extract, "clicked", G_CALLBACK(on_extract_clicked), &app);
    gtk_box_pack_start(GTK_BOX(vbox_buttons), btn_extract, FALSE, FALSE, 0);

    // -- Partie Basse : Console --
    GtkWidget *frame_console = gtk_frame_new("Console Virtuelle");
    gtk_box_pack_start(GTK_BOX(vbox), frame_console, FALSE, FALSE, 0);

    GtkWidget *vbox_console = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(frame_console), vbox_console);

    app.log_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(app.log_view, FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(app.log_view), -1, 150);
    
    GtkWidget *scroll_console = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll_console), GTK_WIDGET(app.log_view));
    gtk_box_pack_start(GTK_BOX(vbox_console), scroll_console, TRUE, TRUE, 0);

    GtkWidget *entry_cmd = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_cmd), "Entrez une commande (ex: ls, rm fichier.txt)...");
    g_signal_connect(entry_cmd, "activate", G_CALLBACK(on_command_activate), &app);
    gtk_box_pack_start(GTK_BOX(vbox_console), entry_cmd, FALSE, FALSE, 0);

    // Chargement initial
    actualiser_arborescence(&app);
    log_message(&app, "Système de fichiers chargé. Prêt.");

    gtk_widget_show_all(app.window);
    gtk_main();

    close_filesystem(&ctx);
}
