03/02/2016
Ajout de la classe ArServerModeSupply qui hérite de ArServerMode.
Cette classe permet de déclencher un comportement de type Supply lors de la réception d'une commande "supply" 
envoyée par un client.

Il suffit de créer un objet depuis cette classe dans arnlServerForAmbiflux.cpp.
Il faut 
 - un constructeur
 - un appel à la méthode BaseActivate du parent
 - une méthode activate
 - une méthode supply() qui déclenche activate
 - une méthode netSupply() qui déclenche supply()

Comportement attendu :
- le robot recoit une demande de supply de son agent Jade
- Le robot fait un appel pour chargement en émettant un son (soubdQueue)
- Il attend le début de chargement avec un timeout de 60 sec
- Si le timeout est déclenché.... faire qque chose
- L'opérateur passe son badge ce qui déclenche le début de chargement
- Le robot annonce ce qui est demandé (Cepstral)
- L'opérateur charge le robot et valide avec son badge
- Le robot remercie
- Le status de l'opération change pour avertir son agent Jade