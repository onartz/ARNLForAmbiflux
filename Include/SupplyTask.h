#ifndef DEF_SUPPLYTASK_H // Si la constante n'a pas été définie le fichier n'a jamais été inclus
#define DEF_SUPPLYTASK_H // On définit la constante pour que la prochaine fois le fichier ne soit plus inclus

//Obligatoire pour ne pas inclure winsock.h
//#include <WinSock2.h> 
#include "ArAsyncTask.h"
#include "AriaUtil.h"
#include "LecteurCarteTask.h"
#include "ArSoundsQueue.h"
#include "ArSoundPlayer.h"
#include "JSONParser.h"
#include "DALRest.h"
//#include "ArCepstral.h"

class SupplyTask : public ArASyncTask{
public :
	enum State {
		CALLING,
		WAITING_FOR_CARD,
		WAITING_FOR_RESPONSE,
		TELLING_WHAT_TO_DO,
		WAITING_FOR_CARD_TO_CLOSE,
		WAITINGFOREND,
		END
  };
	SupplyTask();
	SupplyTask(const char *);
	void queueNowEmpty();
    void queueNowNonempty();
    bool no();

	virtual ~SupplyTask();
	virtual void * runThread(void *arg);
	void lockMutex();
	void unlockMutex();
	/// Gets the docking state we're in
   virtual State getState(void) const { return myState; }
   void setContent(const char *);
   void handleCardRead(char *);
   void handleHttpResponse(char*);
   
   
protected:
	void switchState(State state);
    void stateChanged(void);
	State myState;
	State myLastState;
	bool myNewState;
	ArTime myStartedState;
	bool myRunning;
	const char * myContent;
	//A new card has been read
	bool myNewCardRead;
	//Card ID
	char * myCardRead;

	bool myHttpResponseReceived;
	char * myHttpResponse;

	LecteurCarteTask myCardReader;
    ArFunctor1C<SupplyTask, char *> myCardReadCB;
	ArFunctor1C<SupplyTask, char *> myHttpResponseHandler;
	
	ArSoundsQueue soundQueue;

	std::string myOperatorsName;
	//ArCepstral myCepstral;

	

};
#endif