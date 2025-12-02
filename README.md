#  Gestionnaire de Fichiers Virtuel (Projet C)

Ce projet consiste à développer un système de gestion de fichiers autonome, où l'intégralité de l'arborescence et des données est stockée dans un fichier binaire unique.

##  Fonctionnalités Clés

* **Structure Optimale :** Gestion de l'arborescence via un **Arbre Binaire de Recherche auto-équilibré** .
* **Compression :** Stockage des fichiers compressés en utilisant l'algorithme de **Huffman**.
* **Interface Graphique :** Application développée en **GTK** permettant l'exploration, l'ajout et la suppression de fichiers/dossiers.
* **Modularité :** Support des extensions dynamiques (`.so`/`.dll`) pour ajouter des fonctionnalités comme la cryptographie et l'authentification.

##  Architecture du Projet

Le travail est organisé autour de trois modules principaux :

| Module | Description | Responsable |
| :--- | :--- | :--- |
| **Core** | Gestion de l'arbre (index) et I/O binaire de la structure. | [Nom du Membre 1] |
| **Storage** | Implémentation de Huffman et gestion des pages de données compressées. | [Nom du Membre 2] |
| **UI & Extensions** | Interface graphique GTK et mécanisme de chargement des librairies dynamiques. | [Nom du Membre 3] |

##  Démarrage

### Prérequis

* Compilateur C (GCC)
* Bibliothèque de développement GTK
