# Manuel Utilisateur - Gestionnaire de Fichiers C

## 1. Installation des Prérequis
Avant de commencer, assurez-vous d'avoir les outils nécessaires.
Sur Linux (Debian/Ubuntu) :
```bash
sudo apt-get update
sudo apt-get install build-essential libgtk-3-dev pkg-config
```
*Si vous êtes sur Windows via SSH, assurez-vous d'avoir un serveur X (Xming) configuré.*

## 2. Compilation
Pour construire le programme, ouvrez un terminal dans le dossier du projet et lancez :

```bash
gcc -Wall -g src/main.c src/fs_core.c src/red_black_tree.c src/huffman.c src/ui/interface.c -o fs_manager $(pkg-config --cflags --libs gtk+-3.0)
```
Cela va créer un exécutable nommé `fs_manager`.

## 3. Lancement de l'Application

### Mode Graphique (Recommandé)
Pour ouvrir l'interface visuelle :
```bash
./fs_manager gui
```
*Si vous avez une erreur "cannot open display", vérifiez votre serveur X11.*

### Mode Terminal (Avancé)
Vous pouvez aussi utiliser les commandes directes :
- **Initialiser** un nouveau disque virtuel : `./fs_manager init fs_data.bin`
- **Lister** les fichiers : `./fs_manager list fs_data.bin`

## 4. Utilisation de l'Interface Graphique

Une fois la fenêtre ouverte, vous disposez de plusieurs outils :

### A. La Liste des Fichiers (Zone Gauche)
L'arborescence vous montre tous les fichiers stockés dans votre disque virtuel.
- **Icônes** : Indiquent si c'est un Fichier ou un Dossier.
- **Colonnes** : Nom, Taille réelle, Taille compressée (pour voir le gain Huffman).

### B. Les Boutons d'Action (Zone Droite)
1.  **Ajouter Fichier** :
    - Ouvre une fenêtre pour choisir un fichier sur votre VRAI ordinateur.
    - Il sera automatiquement compressé et ajouté au disque virtuel.
2.  **Supprimer** :
    - Sélectionnez un fichier dans la liste.
    - Cliquez pour le supprimer définitivement du disque virtuel.
3.  **Extraire** :
    - Sélectionnez un fichier dans la liste.
    - Il sera décompressé et sauvegardé dans le dossier courant (nommé `extracted_...`).

### C. La Console (Zone Basse)
Vous pouvez taper des commandes manuelles :
- `ls` : Affiche la liste des fichiers dans le journal.
- `rm nom_du_fichier` : Supprime le fichier spécifié.
