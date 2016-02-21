#ifndef DEF_DELIVERTASK_H // Si la constante n'a pas été définie le fichier n'a jamais été inclus
#define DEF_DELIVERTASK_H // On définit la constante pour que la prochaine fois le fichier ne soit plus inclus

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

#define TIMEOUT_ATTENTE_HUMAIN 10
#define MAX_ATTEMPTS_FAILED 3

class DeliverTask : public ArASyncTask{
public :
	enum State {
		FSM_START,
		FSM_WAITING_FOR_HUMAN_TO_START,
		FSM_GIVING_INFORMATIONS,
		FSM_WAITING_FOR_HUMAN_TO_END,
		FSM_END,
		FSM_FAILED
  };
	DeliverTask();
	DeliverTask(const char *);
	DeliverTask(const char * content, ArFunctor1<char *> *functor);
	void queueNowEmpty();
    void queueNowNonempty();
    bool no();

	virtual ~DeliverTask();
	virtual void * runThread(void *arg);
	void lockMutex();
	void unlockMutex();
	void stopRunning();
	/// Gets the  state we're in
   virtual State getState(void) const { return myState; }
   void init(const char *);
   void handleCardRead(char *);
   void handleHttpResponse(char*);
   void handleHttpFailed(void);

   void setDeliverDoneCB(ArFunctor1<char*>*);
   void setDeliverFailedCB(ArFunctor1<char*>*);

   
   
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

	bool myHttpNewResponse;
	char * myHttpResponse;

	bool myHttpRequestFailed;

	LecteurCarteTask myCardReader;
    ArFunctor1C<DeliverTask, char *> myCardReadCB;
	ArFunctor1C<DeliverTask, char *> myHttpResponseCB;
	ArFunctorC<DeliverTask> myHttpFailedCB;

	ArFunctor1<char*> *myDeliverDoneCB;
	ArFunctor1<char*> *myDeliverFailedCB;
	//ArFunctor1C<DeliverTask, char*>* myDeliverFailedCB;
	DALRest myHttpRequest;
	
	ArSoundsQueue soundQueue;

	std::string myOperatorsName;
	//Number of Attempt  to initiate communication with human
	int attemptFailed;
	//ArCepstral myCepstral;
	char errorMessage[64];

	

};
#endif