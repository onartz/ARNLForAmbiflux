#ifndef ARSERVERMODEBIDON_H
#define ARSERVERMODEBIDON_H

#include "ariaTypedefs.h"
#include "Aria.h"
#include "ArServerBase.h"
#include "ArServerMode.h"
#include "ArServerHandlerCommands.h"
class ArServerModeBidonMere : public ArServerMode{
	public:
	enum State{
		GOFRONT,
		FRONT,
		GOBACK,
		BACK	
	};
	AREXPORT ArServerModeBidonMere(ArServerBase *server, ArRobot *robot);
	AREXPORT virtual ~ArServerModeBidonMere();
	AREXPORT virtual void activate(void);
	AREXPORT virtual void deactivate(void);
	//AREXPORT virtual void userTask(void);
	AREXPORT virtual void front(void) = 0;
	AREXPORT virtual void back(void) = 0;
	AREXPORT void serverFront(ArServerClient * /*client*/, 
					       ArNetPacket *packet);
	AREXPORT void serverBack(ArServerClient * /*client*/, 
					       ArNetPacket *packet);
	AREXPORT const char *toString(State s);

	AREXPORT void addControlCommands(ArServerHandlerCommands *);
	AREXPORT void requestUnlock(void);
	AREXPORT void forceUnlock(void);
	AREXPORT void clearInterrupted(void);
	AREXPORT void resumeInterrupted(bool);

protected:
	ArServerMode *myModeInterrupted;
	//ArFunctor2C<ArServerModeBidon, ArServerClient *, ArNetPacket *> myNetBidonCB;
	ArActionDriveDistance myGoto;
	ArFunctor2C<ArServerModeBidonMere, ArServerClient*, ArNetPacket*> myServerFrontCB;
	ArFunctor2C<ArServerModeBidonMere, ArServerClient*, ArNetPacket*> myServerBackCB;
	int myDistance;
	ArActionGroup myGroup;

	void switchState(State);
	State myState;
	bool myNewState;
	ArTime myStartedState;
	bool myStartedMovement;
	ArServerHandlerCommands *myHandlerCommands;
};

class ArServerModeBidon : public ArServerModeBidonMere{
public:
	AREXPORT ArServerModeBidon(ArServerBase *server, ArRobot *robot);
	AREXPORT virtual ~ArServerModeBidon();
	//AREXPORT virtual void activate(void);
	AREXPORT virtual void deactivate(void);
	AREXPORT virtual void userTask(void);
	AREXPORT void front(void);
	AREXPORT void back(void);
};



#endif