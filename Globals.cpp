#include "Globals.h"

char * getRandomLostMessage(){
	return lostMessage[rand()%numLostMessage];
}
char* getRandomGreetingMessage(){
	return greetingMessage[rand()%numGreetingMessage];
}

char* getRandomSupplyingMessage(){
	return supplyingMessage[rand()%numSupplyingMessage];
}
char* getRandomDeliveryingMessage(){
	return deliveryMessage[rand()%numDeliveryingMessage];
}
char* getRandomByeMessage(){
	return byeMessage[rand()%numByeMessage];
}


