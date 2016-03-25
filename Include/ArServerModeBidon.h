#ifndef ARSERVERMODEBIDON_H
#define ARSERVERMODEBIDON_H

#include "ariaTypedefs.h"
#include "Aria.h"
#include "ArServerBase.h"
#include "ArServerMode.h"
#include "ArServerHandlerCommands.h"

class ArServerModeBidon : public ArServerMode{
public:
	enum State{
		GOFRONT,
		FRONT,
		GOBACK,
		BACK	
	};
	AREXPORT ArServerModeBidon(ArServerBase *server, ArRobot *robot);
	AREXPORT virtual ~ArServerModeBidon();
	AREXPORT virtual void activate(void);
	AREXPORT virtual void deactivate(void);
	AREXPORT virtual void userTask(void);
	AREXPORT void front(void);
	AREXPORT void back(void);
	AREXPORT void serverFront(ArServerClient * /*client*/, 
					       ArNetPacket *packet);
	AREXPORT void serverBack(ArServerClient * /*client*/, 
					       ArNetPacket *packet);
	AREXPORT const char *toString(State s);
	AREXPORT void addControlCommands(ArServerHandlerCommands *);

protected:
	ArServerMode *myModeInterrupted;
	//ArFunctor2C<ArServerModeBidon, ArServerClient *, ArNetPacket *> myNetBidonCB;
	ArActionDriveDistance myGoto;
	ArFunctor2C<ArServerModeBidon, ArServerClient*, ArNetPacket*> myServerFrontCB;
	ArFunctor2C<ArServerModeBidon, ArServerClient*, ArNetPacket*> myServerBackCB;
	int myDistance;
	ArActionGroup myGroup;

	void switchState(State);
	State myState;
	bool myNewState;
	ArTime myStartedState;
	bool myStartedMovement;
	ArServerHandlerCommands *myHandlerCommands;
};


#endif