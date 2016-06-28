#include "Aria.h"
#include "ArExport.h"
#include "ArServerModeBidon.h"

AREXPORT ArServerModeBidonMere::ArServerModeBidonMere(ArServerBase *server, ArRobot *robot): 
ArServerMode(robot, server, "bidon"),
myServerFrontCB(this, &ArServerModeBidonMere::serverFront),
myServerBackCB(this, &ArServerModeBidonMere::serverBack),
myGroup(robot)
{
	myDistance = 200;
	myStatus = "Bidon starting";
	myMode = "Bidon";
	addModeData("front", "go straight forward", 
	      &myServerFrontCB, 
	      "none", "none", "Navigation", "RETURN_NONE");
	 addModeData("back", "go straight backward", &myServerBackCB,
		"none", "none", "Navigation", "RETURN_NONE");
	 myGoto.setRobot(robot);
	 myGoto.setDistance(myDistance);
	myGroup.addAction(&myGoto, 50);
	 myHandlerCommands = NULL;
	
	//myGroup.addAction(&myGoto, 50);
	//switchState(FRONT);
}

AREXPORT ArServerModeBidonMere::~ArServerModeBidonMere(){
}

AREXPORT void ArServerModeBidonMere::activate(){
	if (myIsActive){
		ArLog::log(ArLog::Normal, "Still activated");
		return;
	  }
	ArLog::log(ArLog::Normal, "Not activated");

	if (!baseActivate())
	  {
		ArLog::log(ArLog::Normal, "Mode Bidon : Couldn't activate, clearing interrupted");
		myModeInterrupted = NULL;
		//clearInterrupted();
		return;
	}

  lockMode(true);


	/*if(isActive())
		return;
	if(!baseActivate()){
		myModeInterrupted = NULL;
		return;
	}*/
	switchState(FRONT);
	ArLog::log(ArLog::Normal, "Bidon mode activated");

}

AREXPORT void ArServerModeBidonMere::requestUnlock()
{
  //if (!myState == FRONT)
    deactivate();
}

AREXPORT void ArServerModeBidonMere::forceUnlock(void)
{
  ArUtil::sleep(10);
  myState = BACK;
  ArServerMode::forceUnlock();
}

AREXPORT void ArServerModeBidonMere::clearInterrupted(void)
{
  myModeInterrupted = NULL;
  ArLog::log(ArLog::Normal, "DOCK: Clearing modeInterrupted");
}

AREXPORT void ArServerModeBidonMere::resumeInterrupted(bool assureDeactivation)
{
  ArLog::log(ArLog::Normal, "DOCK: Resume interrupted start %d", assureDeactivation);
  std::list<ArServerMode *> *requestedActivateModes = getRequestedActivateModes();

  // if something else wanted to activate we can just deactivate and 
  // that'll get activated
  if (isActive() && 
      (requestedActivateModes != NULL) &&
      requestedActivateModes->begin() != requestedActivateModes->end())
  {
    deactivate();
    if (getActiveMode() != NULL)
      ArLog::log(ArLog::Normal, 
		 "ARSERVERMODEBIDON: Resume interrupted deactivating and returning... %s got activated", 
		 getActiveMode()->getName());
    else
      ArLog::log(ArLog::Normal, 
		 "ARSERVERMODEBIDON: Resume interrupted deactivating and returning... nothing active...");
    return;
  }
  // if we're still active and we interrupted something when we
  // activated then start that up again, if it was stop we interrupted
  // just stay at the dock

  if (myModeInterrupted != NULL)
    ArLog::log(ArLog::Normal, "ARSERVERMODEBIDON: Resume interrupted another... isActive %d modeInterrupted %p %s", 
	       isActive(), myModeInterrupted, myModeInterrupted->getName());
  else
    ArLog::log(ArLog::Normal, "ARSERVERMODEBIDON: Resume interrupted another... isActive %d modeInterrupted NULL", isActive());


  if (isActive() && myModeInterrupted != NULL && 
      strcmp(myModeInterrupted->getName(), "stop") != 0)
  {
    ArLog::log(ArLog::Normal, "ARSERVERMODEBIDON: Trying to activate interrupted mode %s", 
	       myModeInterrupted->getName());
    myModeInterrupted->activate();
    myModeInterrupted = NULL;
    if (getActiveMode() != NULL)
      ArLog::log(ArLog::Normal, 
		 "ARSERVERMODEBIDON: Did activate mode %s", 
		 getActiveMode()->getName());
    return;
  }

  ArLog::log(ArLog::Normal, "ARSERVERMODEBIDON: Resume interrupted later");
  
  // if we're supposed to assure deactivation and we're still here
  // then deactivate
  if (isActive() && assureDeactivation)
  {
    ArLog::log(ArLog::Normal, "ARSERVERMODEBIDON: Deactivating");
    deactivate();
    return;
  }
  ArLog::log(ArLog::Normal, "ARSERVERMODEBIDON: Resume interrupted end");
}

AREXPORT void ArServerModeBidonMere::deactivate(){
	ArLog::log(ArLog::Normal, "Entering ArServerModeBidonMere::deactivate");
	if(myState == BACK){

		ArLog::log(ArLog::Normal, "Bidon mode deactivating");
		lockMode(false);
		baseDeactivate();
	}
	else{
		ArLog::log(ArLog::Normal, "Calling front() before deactivating");
		myStartedMovement = false;
		back();
	}
}

AREXPORT void ArServerModeBidonMere::serverBack(ArServerClient* client, ArNetPacket *packet){
	 ArLog::log(ArLog::Normal, "Received Back");
	/* if (!myIsActive)
		 activate();*/
	 myRobot->lock();
	 back();
	  myRobot->unlock();

}

void ArServerModeBidonMere::switchState(State state){
	State oldState = myState;
	myState = state;
	myNewState = true;
	ArLog::log(ArLog::Normal, "State = %s", toString(myState));
	myStartedState.setToNow();
	
	//stateChanged();
}

AREXPORT void ArServerModeBidonMere::serverFront(ArServerClient* client, ArNetPacket *packet){
	 ArLog::log(ArLog::Normal, "Received Front");
	 /*  if (!myIsActive)
		 activate();*/
	  myRobot->lock();
	 front();
	  myRobot->unlock();
}


AREXPORT const char *ArServerModeBidonMere::toString(State s){
	switch (s) {
		case GOFRONT:
			return "GOFRONT";
		case FRONT:
			return "FRONT";
		case GOBACK:
			return "GOBACK";
		case BACK:
			return "BACK";

	}
}

AREXPORT void ArServerModeBidonMere::addControlCommands(
	ArServerHandlerCommands *handlerCommands)
{
  myHandlerCommands = handlerCommands;
  myHandlerCommands->addCommand(
          "back",
          "Go back",
          &myServerBackCB);
  myHandlerCommands->addCommand(
          "front",
          "Go front",
          &myServerFrontCB);
}





AREXPORT ArServerModeBidon::ArServerModeBidon(ArServerBase *server, ArRobot *robot):
ArServerModeBidonMere(server, robot)
{
}

AREXPORT ArServerModeBidon::~ArServerModeBidon(){
}


AREXPORT void ArServerModeBidon::front(){
	 if (myIsActive && myState == FRONT)
    return;
	if(!myIsActive)
		activate();
 
  if (!myIsActive)
    return;
	switchState(GOFRONT);

}

AREXPORT void ArServerModeBidon::back(){
	/*if(!myIsActive)
		activate();*/

	if(myState == BACK)
		resumeInterrupted(true);
	else if(myState == FRONT)
		switchState(GOBACK);
	else{
	}

	/* if (myIsActive && myState == BACK)
    return;

  activate();
 
  if (!myIsActive)
    return;
  if(myState == BACK)
	  resumeInterrupted(true);
	switchState(GOBACK);*/

}

//AREXPORT void ArServerModeBidon::activate(){
//	ArServerModeBidonMere::activate();
//
//}

AREXPORT void ArServerModeBidon::deactivate(){

	ArLog::log(ArLog::Normal, "Entering ArServerModeBidon::deactivate()");
 
  ArLog::log(ArLog::Normal, "Calling ArServerModeBidonMere::deactivate() from ArServerModeBidon::deactivate()");
  ArServerModeBidonMere::deactivate();
  ArLog::log(ArLog::Normal, "End of ArServerModeBidon::deactivate()");
}

AREXPORT void ArServerModeBidon::userTask(){
	bool printing = false;
	switch(myState){
		case GOFRONT:
			//myRobot->forceTryingToMove();
			if (printing)
				printf("Going front %ld\n", myStartedState.secSince());
			if(!myStartedMovement){
				myGoto.setDistance(myDistance);
				myRobot->clearDirectMotion();
				myGroup.activateExclusive();
				myStartedMovement = true;
				myStatus = "Starting front";
			}
			if(myStartedMovement && myGoto.haveAchievedDistance()){
				switchState(FRONT);
				myStartedMovement = false;
				break;
			}
			if(myStartedMovement && !myGoto.haveAchievedDistance() && myStartedState.secSince() > 4){
				myStatus = "Failed";
				myRobot->stop();
				switchState(FRONT);
				myStartedMovement = false;
			}

	
			break;
		case FRONT:
			if (printing)
				printf("At Front %ld\n", myStartedState.secSince());
			if(myNewState){
				myNewState = false;
				myStatus = "Front";
			}
			break;
			

		case GOBACK:
			if (printing)
				printf("Going back %ld\n", myStartedState.secSince());
			if(!myStartedMovement){
				myGoto.setDistance(-myDistance);
				myRobot->clearDirectMotion();
				myGroup.activateExclusive();
				myStartedMovement = true;
				myStatus = "Starting back";
			}
			if(myStartedMovement && myGoto.haveAchievedDistance()){
				switchState(BACK);
				resumeInterrupted(true);
				myStartedMovement = false;
				break;
			}
			if(myStartedMovement && !myGoto.haveAchievedDistance() && myStartedState.secSince() > 4){
				myRobot->stop();
				myStatus = "Failed";
				switchState(BACK);
				//resumeInterrupted(true);
				myStartedMovement = false;
			}
			break;
		case BACK:
			if (printing)
				printf("At Back %ld\n", myStartedState.secSince());
			if(myNewState){
				myNewState = false;
				myStatus = "Back";
			}
			
			break;
	}
}





