03/02/2016
Ajout de la classe ArServerModeSupply qui h�rite de ArServerMode.
Cette classe permet de d�clencher un comportement de type Supply lors de la r�ception d'une commande "supply" 
envoy�e par un client.

Il suffit de cr�er un objet depuis cette classe dans arnlServerForAmbiflux.cpp.
Il faut 
 - un constructeur
 - un appel � la m�thode BaseActivate du parent
 - une m�thode activate
 - une m�thode supply() qui d�clenche activate
 - une m�thode netSupply() qui d�clenche supply()

Comportement attendu :
- le robot recoit une demande de supply de son agent Jade
- Le robot fait un appel pour chargement en �mettant un son (soubdQueue)
- Il attend le d�but de chargement avec un timeout de 60 sec
- Si le timeout est d�clench�.... faire qque chose
- L'op�rateur passe son badge ce qui d�clenche le d�but de chargement
- Le robot annonce ce qui est demand� (Cepstral)
- L'op�rateur charge le robot et valide avec son badge
- Le robot remercie
- Le status de l'op�ration change pour avertir son agent Jade