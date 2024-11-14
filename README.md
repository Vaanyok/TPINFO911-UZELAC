# INFO911/INFO001 Segmentation temps-réel semi-supervisée par distance d'histogramme de couleurs

### Members : 
Uzelac Yvann

## Intro

Le processus comprend plusieurs étapes, dont la capture du fond et des objets, l'extraction de leurs histogrammes de couleurs, et la mise en place d'un mode de reconnaissance qui permet de détecter et étiqueter les objets en temps réel. Une fois les objets reconnus, ils sont segmentés et colorés selon leur catégorie (fond ou objet).

## Questions traités

* 1) Set-up initial avec mini-application de capture vidéo  -> OK 
* 2) Distribution de couleurs ou histogramme de couleurs    -> OK 
* 3) Comparaison fond et objet                              -> OK 
* 4) Mode reconnaissance                                    -> OK 
* 5) Développements possibles                               -> Traités
    - Optimiser la vitesse d'exécution                   -> OK 
    - Améliorer la robustesse de l'analyse des couleurs  -> OK 
    - Mieux capturer la géométrie des objets             -> OK 

Optionnel -> Ajout des couleurs associés en fonction des objets

## Commande 


Les principales commandes à utiliser pour interagir avec le système sont les suivantes :

    'b' : Enregistrer les histogrammes de couleur du fond.
    'a' : Enregistrer l'histogramme de couleur d'un objet.
    'n' : Ajouter un nouvel objet à la base de données des objets.
    'r' : Activer le mode de reconnaissance et commencer la segmentation en temps réel.
    'f' : Geler ou reprendre l'affichage du flux vidéo.
    'v' : Vérifier les distributions de couleur du fond et d'un objet, puis calculer leur distance.
    'q' : Quitter l'application.




## Commandes à faire 

1) b
2) a (Placer objet au milieu)
3) n (Ajout nouvel objet)
3-2) Faire sur nouvel objet
4) r


## Des images

Des images sont associés en fonction des commandes