#ifndef GLOBALS_H
#define GLOBALS_H
//#include "ArCepstral.h"
//#include "ArSoundsQueue.h"
//#include "ArSoundPlayer.h"

//extern ArCepstral g_Cepstral;
//extern ArSoundsQueue g_SoundsQueue;

#include <stdlib.h>
#ifndef TIMEOUT_ATTENTE_HUMAIN
	#define TIMEOUT_ATTENTE_HUMAIN 30
#endif
#ifndef MAX_ATTEMPTS_FAILED
	#define MAX_ATTEMPTS_FAILED 3
#endif

char* const greetingMessage[]={"Hello %s",
"Hi %s"};

char* const deliveryMessage[]={"I have %s for you.",
"You can take %s from my back.",
"This is for you : %s"};

char* const lostMessage[]={"Are you here?",
"Where are you?",
 "Is there anybody here?"};

char* const supplyingMessage[]={"Can you give me %s",
"Can you put %s in my case",
 "I need %s"};

char* const byeMessage[]={"It was a pleasure. Bye",
"ByeBye",
 "See you soon"};

int const numGreetingMessage = sizeof(greetingMessage)/sizeof(greetingMessage[0]);
int const numSupplyingMessage = sizeof(supplyingMessage)/sizeof(supplyingMessage[0]);
int const numDeliveryingMessage = sizeof(deliveryMessage)/sizeof(deliveryMessage[0]);
int const numLostMessage = sizeof(lostMessage)/sizeof(lostMessage[0]);
int const numByeMessage = sizeof(byeMessage)/sizeof(byeMessage[0]);

char * getRandomGreetingMessage();
char * getRandomSupplyingMessage();
char * getRandomDeliveryingMessage();
char * getRandomLostMessage();
char * getRandomByeMessage();

//static const char * getRandomLostMessage(){
//	return lostMessage[rand()%numLostMessage];
//}

//const char* ArServerModeDeliver::getRandomGreetingMessage(){
//	return greetingMessage[rand()%numGreetingMessage];
//}
//
//const char* ArServerModeDeliver::getRandomDeliveryMessage(){
//	return deliveryMessage[rand()%numDeliveryingMessage];
//}
//
//const char* ArServerModeDeliver::getRandomLostMessage(){
//	return lostMessage[rand()%numLostMessage];
//}
#endif