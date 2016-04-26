#ifndef ARASYNCSPEAK_H
#define ARASYNCSPEAK_H
#include "ArAsyncTask.h"
//#include "Globals.h"
#include "ArCepstral.h"


class ASyncSpeak : public ArASyncTask{
public:
	//ASyncSpeak();
	//ASyncSpeak(ArCepstral*,ArFunctor *functor);
	ASyncSpeak(ArFunctor *functor);
	virtual ~ASyncSpeak();
	virtual void * runThread(void *arg);
	void lockMutex();
	void unlockMutex();
	void stopRunning();
	void speak(char*);
	void speak(void);
	void setText(char*);

protected:
	//ArCepstral *myCepstral;
	ArCepstral myCepstral;
	char* myText;
	ArFunctor * myEndFunctor;
};

#endif