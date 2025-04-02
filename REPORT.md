# Rapport de l'optimisation de l'arbre quadtree

@TheKhalamar
Comme vous l'avez dit, il existe plusieurs façons d'implémenter les quadrats et vous en avez choisi une. Je pense que le principal défaut de cette approche est que si un objet chevauche les enfants 1 et 2, et que je ne regarde que l'enfant 4, les objets qui se chevauchent seront rendus comme faisant partie du parent, même s'ils ne sont pas visibles.

L'autre solution consiste à ajouter des objets à tous les enfants qu'ils chevauchent, mais dans ce cas, ils seront rendus plusieurs fois, ce qui peut également avoir des effets secondaires néfastes (en particulier en cas d'utilisation de la transparence). Dans ce cas, votre conteneur peut vous aider si vous stockez chaque élément comme une paire (objet, rendu). Vous définissez ce booléen à true si la recherche a déterminé que l'objet doit être rendu. Cependant, cela vous oblige à nettoyer ces booléens à chaque image. Vous pouvez également stocker le compteur d'images à la place.

Cette approche casse également l'implémentation de la taille de votre quadtree. Celle-ci est fixée par votre conteneur, mais on pourrait aussi garder une trace du compteur sur chaque noeud et incrémenter ce compteur lors de l'insertion d'un objet.

Enfin, votre approche construit automatiquement un arbre de profondeur 8 si un objet ne traverse aucune arête. Ce que j'aime faire, c'est diviser un noeud seulement s'il atteint un nombre critique d'objets (par exemple, s'il y a plus de 20 objets, alors je commence à les diviser). C'est préférable pour les arbres statiques, car la gestion de ce type d'arbre devient assez délicate lorsqu'on travaille avec des arbres dynamiques.

@nerdrage562
La vidéo est bien faite, elle explique de façon simple le fonctionnement des arbres quadruples ! J'ai quand même quelques idées d'optimisation :) La première chose, c'est cette liste, je comprends que le fait que les itérateurs ne soient pas invalidés est un gros avantage, mais je ne suis pas sûr que cela compense la vitesse que donne le vecteur lorsqu'on itère des millions d'objets. Dans ce cas, la mémoire contiguë devient énorme. L'autre chose, mais c'est purement un choix de conception, j'aurais directement stocké les pointeurs (je pense que cela pourrait être un bon endroit pour les pointeurs bruts) dans l'arbre, et utilisé un conteneur externe pour garder les objets. L'idée est que l'arbre est purement une structure de recherche et qu'il ne « possède » pas les objets.


@SylphDS
Je vois que vous stockez les sous-arbres en utilisant shared_ptr. Est-ce juste une question d'habitude ou y a-t-il des cas d'utilisation où vous voudriez partager la propriété de ces sous-arbres avec d'autres entités ?
@jonatanlind5408
Un cas d'utilisation intéressant serait celui des « structures de données persistantes », bien que dans cette vidéo, il s'agisse probablement d'une question d'habitude. Ces structures sont typiquement caractérisées par le fait que chaque nœud est en lecture seule après construction. Cela permet aux grandes structures de données de partager des données internes avec d'autres permutations d'elles-mêmes avec une duplication minimale des données. Par exemple, une simulation peut utiliser un quadtree pour garder une trace des objets et ce quadtree est une permutation de lui-même à partir de l'itération précédente. Ce qui est peut-être plus intéressant, c'est de savoir comment un tel quadtree peut être copié pour, disons, la sérialisation sur disque sans duplication de données et sans introduire de course aux données.
@quentinquadrat9389
Si les shared_ptr sont purement internes (non exposés par des méthodes publiques), il est en effet préférable d'utiliser unique_ptr (plus léger mais vous avez besoin de C++14 puisque make_unique n'est pas fourni avec C++11).Si votre classe a des getters sur les smart pointers, je préfère retourner la référence de leur contenu puisque le problème du shared pointer est de savoir qui est le vrai propriétaire ? A 21:58, la suppression des pointeurs intelligents (le noeud racine) supprimera implicitement les noeuds enfants, mais il faut veiller à ce que cette récursion implicite ne fasse pas déborder la pile comme n'importe quelle fonction récursive. Dans tous les cas, j'aurais essayé une implémentation quadtree sans aucune allocation si possible : array<StaticQuadTree<T>, 4u) m_child ; puisque l'ajout/la suppression d'un élément entraîne une réorganisation de l'élément.Mais cela nécessiterait probablement une variable membre supplémentaire, un tableau de nœuds vides.


@Ash_18037
Bien vu.  Cependant, pour l'accélération spatiale 2d, je n'ai jamais eu besoin d'utiliser un arbre quadruple.  Il n'y a pas d'autre solution que d'utiliser une carte spatiale plus simple pour faire le travail et c'est plus rapide qu'un arbre quadruple à implémenter et à exécuter.  Il aurait donc été bon de savoir ce qu'un arbre spatial apporte de plus qu'une simple carte spatiale, si tant est qu'il y ait quelque chose.  Par carte spatiale, j'entends simplement l'ajout d'une référence à tous les objets dans un grand tableau où les coordonnées X et Y des objets sont divisées par une certaine « taille de cellule » et qui produit un index dans le tableau (chaque emplacement du tableau est un tableau ou une liste d'objets à l'emplacement).  Si votre monde est absolument énorme avec de grandes zones d'espace vide, je pense que les cartes spatiales sont plus efficaces en termes de mémoire (pas besoin de cellules couvrant ces espaces vides), mais à part cela, y a-t-il d'autres avantages ?

@xeridea
Vous pouvez utiliser un vecteur, mais il suffit de vider le quadtree si le redimensionnement du vecteur entraîne une réaffectation. Si vous initialisez un vecteur plus grand que nécessaire, vous n'aurez besoin d'effacer le quadtree que si l'insertion provoque une réallocation. Si vous vérifiez la taille du vecteur par rapport à la quantité allouée avant l'insertion, vous éviterez les problèmes. Si le code est multithreadé, il est nécessaire de mutexer les insertions. C'est un prix peu élevé à payer pour les insertions, comparé à l'allocation systématique lors d'une nouvelle insertion avec une liste.

@Jkauppa
Si vous stockez les résultats de recherche précédents, image par image, vous pouvez obtenir des résultats « en cache » encore plus rapides, parce que le point de vue ne changera pas beaucoup.
Si vous voulez un ordre de dessin statique, stockez l'ordre de dessin dans la liste de pointeurs, c'est-à-dire la profondeur de l'ordre z, puis peignez dans le tampon z.

@boggo3848
Il est probable que ce soit plus tard dans cette vidéo ou dans une prochaine (je n'ai pas encore fini), mais j'ai dû en écrire une pour quelque chose récemment et j'ai fini par utiliser l'encodage de la courbe de Morton qui permet d'obtenir presque le même résultat implicitement avec une fraction du coût de la mémoire et du CPU.
@javidx9
il y a 3 ans
Les courbes de Morton fonctionnent bien lorsqu'elles sont combinées avec un hashmap et donnent d'excellentes performances, si vous pouvez réduire votre problème au domaine des nombres entiers assez facilement. L'ajout d'une zone en virgule flottante dans le mélange rend les choses un peu plus compliquées. Les gens devraient quand même se renseigner sur Morton pour leurs implémentations.
