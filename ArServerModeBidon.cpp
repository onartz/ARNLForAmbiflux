#include "Aria.h"
#include "ArExport.h"
#include "ArServerModeBidon.h"

AREXPORT ArServerModeBidon::ArServerModeBidon(ArServerBase *server, ArRobot *robot): 
ArServerMode(robot, server, "bidon"),
myServerFrontCB(this, &ArServerModeBidon::serverFront),
myServerBackCB(this, &ArServerModeBidon::serverBack),
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

AREXPORT ArServerModeBidon::~ArServerModeBidon(){
}

AREXPORT void ArServerModeBidon::activate(){
	if(isActive())
		return;
	if(!baseActivate()){
		myModeInterrupted = NULL;
		return;
	}
	switchState(FRONT);
	ArLog::log(ArLog::Normal, "Bidon mode activated");

}

AREXPORT void ArServerModeBidon::deactivate(){
	if(myState == FRONT){

		ArLog::log(ArLog::Normal, "Bidon mode deactivating");
		baseDeactivate();
	}
	else{
		myStartedMovement = false;
		front();
	}
}

AREXPORT void ArServerModeBidon::serverBack(ArServerClient* client, ArNetPacket *packet){
	 ArLog::log(ArLog::Normal, "Received Back");
	 activate();
	 back();

}

AREXPORT void ArServerModeBidon::serverFront(ArServerClient* client, ArNetPacket *packet){
	 ArLog::log(ArLog::Normal, "Received Front");
	  activate();
	 front();
}

AREXPORT void ArServerModeBidon::front(){
	switchState(GOFRONT);

}

AREXPORT void ArServerModeBidon::back(){
	switchState(GOBACK);

}

AREXPORT void ArServerModeBidon::userTask(){
	switch(myState){
		case GOFRONT:
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
			if(myNewState){
				myNewState = false;
				myStatus = "Front";
			}
			break;
		case GOBACK:
			if(!myStartedMovement){
				myGoto.setDistance(-myDistance);
				myRobot->clearDirectMotion();
				myGroup.activateExclusive();
				myStartedMovement = true;
				myStatus = "Starting back";
			}
			if(myStartedMovement && myGoto.haveAchievedDistance()){
				switchState(BACK);
				myStartedMovement = false;
				break;
			}
			if(myStartedMovement && !myGoto.haveAchievedDistance() && myStartedState.secSince() > 4){
				myRobot->stop();
				myStatus = "Failed";
				switchState(BACK);
				myStartedMovement = false;
			}
			break;
		case BACK:
			if(myNewState){
				myNewState = false;
				myStatus = "Back";
			}
			
			break;
	}
}

void ArServerModeBidon::switchState(State state){
	State oldState = myState;
	myState = state;
	myNewState = true;
	ArLog::log(ArLog::Normal, "State = %s", toString(myState));
	myStartedState.setToNow();
	
	//stateChanged();
}

AREXPORT const char *ArServerModeBidon::toString(State s){
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

AREXPORT void ArServerModeBidon::addControlCommands(
	ArServerHandlerCommands *handlerCommands)
{
  myHandlerCommands = handlerCommands;
  myHandlerCommands->addCommand(
          "back",
          "This disables the auto docking and undocking",
          &myServerBackCB);
  myHandlerCommands->addCommand(
          "front",
          "This enables the auto docking and undocking",
          &myServerFrontCB);
}