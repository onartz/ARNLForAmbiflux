#include "Aria.h"
#include "ArExport.h"
#include "ArServerMyMode.h"

AREXPORT ArServerMyMode::ArServerMyMode(ArServerBase *server,
										ArRobot *robot,
										ArLocalizationTask *locTask,
										ArPathPlanningTask *pathTask,
										ArMapInterface *arMap,
										double approachDist,
										double backOutDist):
ArServerMode(robot, server, "myMode"),
myServerGotoTriangleCB(this,&ArServerMyMode::serverGotoTriangle),
myServerUndockFromCB(this,&ArServerMyMode::serverUndockFrom),
myGoalDoneCB(this, &ArServerMyMode::goalDone),
myGoalFailedCB(this, &ArServerMyMode::goalFailed),
myGroup(robot),
myBackGroup(robot),
myDriveTo("triangleDriveTo",0,500,100,100,30),
myApproachDist(approachDist),
myBackOutDist(backOutDist),
myNeedToPathPlan(false)
{
	myPathTask = pathTask;
	myLocTask = locTask;
	myMap = arMap;
	myMode = "Dock to goal";
	//myStatus = "Starting";

	const ArRobotConfigPacketReader *origConfig;
	  if ((origConfig = robot->getOrigRobotConfig()) != NULL)
	  {
		if (strcmp(robot->getRobotName(), "simler") == 0 || 
			strcmp(robot->getRobotName(), "MobileSim") == 0)
		{
		  ArLog::log(ArLog::Normal, "Simulation mode");
		  setStallsAsBumps(true);
		}
	  }

	myPathTask->addGoalDoneCB(&myGoalDoneCB);
	myPathTask->addGoalFailedCB(&myGoalFailedCB);
	addModeData("dockToGoal", "sends the robot to the goal", 
	      &myServerGotoTriangleCB, 
	      "string: goal", "none", "Navigation", "RETURN_NONE");
	 addModeData("undockFrom", "undocks the robot", &myServerUndockFromCB,
		"none", "none", "Navigation", "RETURN_NONE");
	myDriveTo.setGotoVertex(true);
	//Lets try to find a 90° triangle
	//myDriveTo.setTriangleParams(255,90,255);
	//myDriveTo.setAcquire(true);
	//myDriveTo.setMaxAngleMisalignment(20);
	myDriveTo.setLogging(true);
	myGroup.addAction(&myDriveTo, 55);
	myGroup.addAction(new ArActionStop, 50);
	clearInterrupted();
	

	
	//myDriveTo.setMaxAngleMisalignment(10);
	myDriveTo.setMaxLateralDist(500);
	myDriveTo.setMaxDistBetweenLinePoints(50);

	myBackGroup.addAction(new ArActionLimiterBackwards("backwards limiter",
						     -200, 0, -200, 1.2), 51);
	myBackGroup.addAction(&myGoto, 50);

	myHitDock = false;
	myDriveFromValid = false;
	myPosition = "unknown";
	lockMode(true);

	//TODO : A supprimer et checker position
	switchState(UNDOCKED);
	/*if(myServer != NULL){
		myServer->add

	}*/
}



AREXPORT ArServerMyMode::~ArServerMyMode(){
}

AREXPORT void ArServerMyMode::serverGotoTriangle(ArServerClient * /*client*/, 
					       ArNetPacket *packet)
{
  char buf[512];
  packet->bufToStr(buf, sizeof(buf)-1);
  //ArLog::log(ArLog::Normal, "Going to goal %s", buf);
  myRobot->lock();
  gotoTriangle(buf);
  myRobot->unlock();

  
}

AREXPORT void ArServerMyMode::serverUndockFrom(ArServerClient * /*client*/, 
					       ArNetPacket *packet)
{
  //char buf[512];
  //packet->bufToStr(buf, sizeof(buf)-1);
  ArLog::log(ArLog::Normal, "Undocking from");
  myRobot->lock();
  undockFrom();
  myRobot->unlock();
}

AREXPORT void ArServerMyMode::activate(){

	if (myIsActive)
		return;

	 ArServerMode *activeMode = getActiveMode();
  ArServerMode *lastActiveMode = getLastActiveMode();

  if (activeMode == NULL && lastActiveMode == ArServerMode::getIdleMode())
  {
    ArLog::log(ArLog::Normal, "DOCK: activeMode was NULL, but ourLastActiveMode was idle, so using that");
    activeMode = lastActiveMode;
  }

  if (activeMode != NULL)
    ArLog::log(ArLog::Normal, "DOCK: %s was activeMode", 
	       activeMode->getName());
  if (activeMode != NULL && activeMode == ArServerMode::getIdleMode())
  {
    activeMode = ArServerMode::getIdleMode()->getModeInterrupted();
    if (activeMode != NULL)
      ArLog::log(ArLog::Normal, "DOCK: Made %s new activeMode", 	       
		 activeMode->getName());
    else
      ArLog::log(ArLog::Normal, "DOCK: NULL new activeMode...");
  }
// exemple : mode stop
  myModeInterrupted = activeMode;
  clearInterrupted();
  
  //if (myForcedDock && activeMode != NULL)
  //{
  //  //printf("Interrupted %s\n", getActiveMode()->getName());
  //  if (activeMode->isAutoResumeAfterInterrupt()) 
  //  {
  //    myModeInterrupted = activeMode;
  //    ArLog::log(ArLog::Normal, "DOCK: Made %s new modeInterrupted",
		// myModeInterrupted->getName());
  //  }
  //}
  //else
  //{
  //  clearInterrupted();
  //}
  if (!baseActivate())
  {
    ArLog::log(ArLog::Normal, "DOCK: Couldn't activate, clearing interrupted");
    clearInterrupted();
    return;
  }

	lockMode(true);
	//dock();
  
	/*if(!baseActivate())
		return;*/
}

AREXPORT void ArServerMyMode::deactivate(){
 // if we're not forced docking and retry dock is set then cancel retry dock
  if (myRetryDock)
  {
    ArLog::log(ArLog::Normal, "Cancelling dock retry");
    myRetryDock = false;
  }
  myNeedToPathPlan = false;
   if (myState == UNDOCKED)
	{
		//ArLog::log(ArLog::Normal,"ArServerMyMode::deactivate(), STATE = UNDOCKED");
		//myForcedDock = false;
		//myShuttingDownSeconds = 0;
		//broadcastDockInfoChanged();
		baseDeactivate();
		//ArLog::log(ArLog::Normal,"ArServerMyMode::deactivate(), END");
	}
   else{
	   //ArLog::log(ArLog::Normal,"ArServerMyMode::deactivate(), STATE != UNDOCKED");
	   
		undockFrom();
   }
   //ArLog::log(ArLog::Normal,"ArServerMyMode::deactivate() : EXIT function");
   
}


/**
* Instruction to trigger Docking
* If Robot still docked at position return
* If Robot at position, activate DOCKING
* Else activate GOINGTOGOAL
*/
AREXPORT void ArServerMyMode::dock(){
	//ArLog::log(ArLog::Normal,"calling dock()");
	activate();
	//
	if (!myIsActive)
		return;
	//Actif et (State==UNDOCKED ou Retry)
	if(myState == DOCKED){
		undockFrom();
	}
	else{
	//undockFrom();
	//switchState(UNDOCKED);
		myNeedToPathPlan = true;
		switchState(UNDOCKED);
	}
	
}



AREXPORT void ArServerMyMode::undockFrom(){
	//ArLog::log(ArLog::Normal, "ArServerMyMode::undockFrom()");
	/*if(!myIsActive)
		activate();*/
  //myRobot->enableMotors();
	 /*if (myState == DOCKED && !myRetryDock)
		return;*/
	if(myState == UNDOCKED){
		//ArLog::log(ArLog::Normal, "ArServerMyMode::undockFrom() requested at STATE = UNDOCKED");
		resumeInterrupted(true);
	}
	else if (myState == DOCKED)
	  {
		  //ArLog::log(ArLog::Normal, "ArServerMyMode::undockFrom() requested at STATE = DOCKED");
		if (myDesiredBackOutDist > .1)
			myDistanceToBack = myDesiredBackOutDist;
		else if (myDriveFromValid)
			myDistanceToBack = myRobot->getEncoderPose().findDistanceTo(myDriveFrom);
		else
			myDistanceToBack = myRobot->getRobotRadius() + 1000;
		myStatus = "Undocking";
		myStartedBacking = false;
		
		switchState(UNDOCKING);
		ArLog::log(ArLog::Normal, "Undocking");
	  }
  else if (myState == DOCKING)
  {
	 // ArLog::log(ArLog::Normal, "ArServerMyMode::undockFrom(), STATE = DOCKING");
	myRobot->stop();
    ArLog::log(ArLog::Normal, "Undocking");
    if (myDesiredBackOutDist > .1)
      myDistanceToBack = myDesiredBackOutDist;
    else if (myDriveFromValid)
      myDistanceToBack = myRobot->getEncoderPose().findDistanceTo(myDriveFrom);
    else
      myDistanceToBack = myRobot->getRobotRadius() + 1000;
    myStartedBacking = false;
    if (myHitDock || myDriveTo.isActive())
    {
      myStatus = "Undocking";
      //myDisableDockCalled = false;
      switchState(UNDOCKING);
    }
    else
    {
      //myDisableDockCalled = true;
      switchState(UNDOCKED);
      resumeInterrupted(true);
    }
  }
}
AREXPORT void ArServerMyMode::beforeDriveInCallback()
{
  ArLog::log(ArLog::Verbose, "Setting ignore illegal pose flag (bd)");
  myLocTask->setIgnoreIllegalPoseFlag(true);
}

AREXPORT void ArServerMyMode::afterDriveOutCallback()
{
  ArLog::log(ArLog::Verbose, "Clearing ignore illegal pose flag");;
  myLocTask->setIgnoreIllegalPoseFlag(false);
}

AREXPORT void ArServerMyMode::userTask(){
	bool printing = false;
	int frontBump;
	int frontBumpMask = (ArUtil::BIT1 | ArUtil::BIT2 | ArUtil::BIT3 | 
		       ArUtil::BIT4 | ArUtil::BIT5 | ArUtil::BIT6);
	//ArLog::log(ArLog::Normal,toString(myState));
	switch(myState){
		case UNDOCKED:
			if (myNeedToPathPlan && !myServer->idleProcessingPending())
			{
				myGoalDone = false;
				myDriveTo.deactivate();
				myRobot->clearDirectMotion();
				myNeedToPathPlan = false;
				if(!myPathTask->pathPlanToGoal(myGoalName.c_str())){
					myStatus = "Failed to go to triangle";
					resumeInterrupted(false);
					return;
				}			
				myStatus = "Going to goal at ";
				myStatus += myGoalName;
				ArLog::log(ArLog::Normal, "Docking at %s", myGoalName.c_str());
				switchState(DOCKING);
			}
			break;
		
		case DOCKING:
			frontBump = ((myRobot->getStallValue() & 0xff00) >> 8) & frontBumpMask;
			if (myStallsAsBumps && (myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled()))
				frontBump |= ArUtil::BIT0;
			//Robot at goal and DriveTo not started
			if(myGoalDone && !myDriveTo.isActive()){
				//ArLog::log(ArLog::Normal, "Driving into dock");
				myNewState = false;
			//beforeDriveInCallback();
				myDriveFromValid = true;
				myDriveFrom = myRobot->getEncoderPose();
				myGroup.activateExclusive();
				myGoalDone = false;
				myStartedDriveTo.setToNow();
			}
			// Robot has hit the dock
			if ((myDriveTo.isActive() && 
				 frontBump) || myDriveTo.getState() == ArActionTriangleDriveTo::STATE_SUCCEEDED){
				ArLog::log(ArLog::Normal, "Hit dock");
				myHitDock = true;
				myStatus = "DockedTo";
				myRetryDock = false;
				//myHitDockTime.setToNow();
				//switchState(DOCKED);
				myDriveTo.deactivate();
				//break;
			}
			//Triangle drive to has failed
			if (myDriveTo.isActive() && myDriveTo.getState() == ArActionTriangleDriveTo::STATE_FAILED)
			{
			  ArLog::log(ArLog::Normal, 
				 "Dock Failed: Could not find dock target");
			  //myDriveTo.deactivate();
			  //switchState(UNDOCKING);
			  myRetryDock = true;
			  undockFrom();
			  break;
			}
			// if we're there and haven't sent the command
			//DOCKING state and hit dock
			if (myHitDock)
			{
				ArLog::log(ArLog::Normal, "DockedTo");		  
				switchState(DOCKED);
				break;
			}
			// if we sent the command but it didn't work
			/*else if (myHitDock && myHitDockTime.secSince() > 10)
			{
			  ArLog::log(ArLog::Normal, 
				 "Dock Failed");
			  myRetryDock = true;
			  undockFrom();
			}*/
			// if we sent the dock command just chill
			/*else if (myHitDock)
			{

			}*/
			//DOCKING State and bump and Triangle OK
			
			// if we failed getting there
			
			
			// if it took longer than 30 seconds to drive in
			if (myDriveTo.isActive() && myStartedDriveTo.secSince() > 30)
			{
			  ArLog::log(ArLog::Normal, 
				 "Dock Failed: Took too long to find target");
			  myStatus = "Failed to dock";
			  //myDriveTo.deactivate();
			  myRetryDock = true;
			  undockFrom();
			}
			//Robot reached the goal and have to dock
			//else if (myGoalDone && !myDriveTo.isActive())
			//{
			//  ArLog::log(ArLog::Normal, "Driving into dock");
			//  //beforeDriveInCallback();
			//  myDriveFromValid = true;
			//  myDriveFrom = myRobot->getEncoderPose();
			//  myGroup.activateExclusive();
			//  myGoalDone = false;
			//  myStartedDriveTo.setToNow();
			//  //myDrivingIntoDockCBList.invoke();
			//}
			break;
		case DOCKED:
			break;
		case UNDOCKING:
			//ArLog::log(ArLog::Normal, "UserTask STATE = UNDOCKING");
			//ArLog::log(ArLog::Normal, "myStartedBacking = %d", myStartedBacking);
			 myHitDock = false;
			if (printing)
				printf("Undocking %ld\n", myStartedState.secSince());

			// make it so that we're always trying to move while undocking, so
			// the sonar kick in before we start backing up	
   
			myRobot->forceTryingToMove();

	//TODO : check
			//if (myStartedState.secSince() >= 3 && !myStartedBacking)
			if (!myStartedBacking)
			{
				ArLog::log(ArLog::Normal, "Activating backing");
			  myStartedBacking = true;
			  //myGoto.setEncoderGoalRel(myDistanceToBack, -180, true);
			  myGoto.setDistance(-myDistanceToBack, true);
			  myRobot->clearDirectMotion();
			  myBackGroup.activateExclusive();
			}
			//if (myStartedBacking && myGoto.haveAchievedDistance())
			//{
			//  //backoutCallback();
			//}
			if (myStartedBacking && myGoto.haveAchievedDistance())
			{
				ArLog::log(ArLog::Normal, "Activating backing : distance achieved");
			  afterDriveOutCallback();
			  if (myRetryDock)
			  {
				myRetryDock = false;
				switchState(UNDOCKED);
				ArLog::log(ArLog::Normal, "Retrying dock");
				dock();
				return;
			  }
			  myStatus = "Undocked from " + myGoalName;
			  switchState(UNDOCKED);
			  ArLog::log(ArLog::Normal, "Undocked from");
			  resumeInterrupted(true);
			}
			if (myStartedState.secSince() >= 60)
			{
			  ArLog::log(ArLog::Normal, "Error undocking, taken too long");
			  myStartedState.setToNow();
			  myStatus = "Undocking failing";
			}
			break;
		}
	}





AREXPORT void ArServerMyMode::gotoTriangle(const char * goal){
	
  //reset();
	myGoalName = goal;
	myRobot->lock();
	dock();
	myRobot->unlock();
	
  /*myGoalName = goal;
  myMode = "Goto triangle";
  myStatus = "Going to ";
  myStatus += goal;
  activate();*/

}

AREXPORT const char *ArServerMyMode::toString(State s){
	switch (s) {
		case DOCKED:
			return "DOCKED";
		case DOCKING:
			return "DOCKING";
		case UNDOCKING:
			return "UNDOCKING";
		case UNDOCKED:
			return "UNDOCKED";
		/*case GOINGTOGOAL:
			return "GOINGTOGOAL";
		case FAILURE:
			return "FAILURE";*/
		} // end switch state

	return "unknown";
}

AREXPORT void ArServerMyMode::activateAsDocked(void)
{
  myStatus = "DockedTo";
  switchState(DOCKED);
  activate();
}




void ArServerMyMode::switchState(State state){
	State oldState = myState;
	myState = state;
	myNewState = true;
	ArLog::log(ArLog::Normal, "State = %s", toString(state));
	myStartedState.setToNow();
	//if (oldState != myState)
	//{
	//	if (myState == DOCKED)
	//	{
	//		//myStatus = "Docked";
	//		/*myDockedCBList.invoke();
	//		mySingleShotDockedCBList.invoke();*/
	//	}
	//	else if (myState == UNDOCKING){}
	//		
	//		//myUndockingCBList.invoke();
	//	else if (myState == UNDOCKED){}
	//		//myStatus = "UnDocked";
	//		//myUndockedCBList.invoke();

	////}
	stateChanged();
}


void ArServerMyMode::stateChanged(void){

	///*if((myLastState !=  myState) && myState == GOINGTOGOAL)
	//	myStatus = "Going to " + myGoalName;*/
	//if((myLastState !=  myState) && myState == DOCKING)
	//	myStatus = "Docking to " + myGoalName;
	//if((myLastState !=  myState) && myState == DOCKED)
	//	myStatus = "Docked to " + myGoalName;
	//if((myLastState !=  myState) && myState == UNDOCKED)
	//	myStatus = "Undocked";
	//if((myLastState !=  myState) && myState == UNDOCKING)
	//	myStatus = "Undocking from " + myGoalName;
	///*if((myLastState !=  myState) && myState == FAILURE)
	//	myStatus = "Fails";*/
	//
	 myLastState = myState;
}

AREXPORT void ArServerMyMode::clearInterrupted(void)
{
  myModeInterrupted = NULL;
  ArLog::log(ArLog::Normal, "DOCK: Clearing modeInterrupted");
}

AREXPORT void ArServerMyMode::resumeInterrupted(bool assureDeactivation)
{
  ArLog::log(ArLog::Normal, "DOCK: Resume interrupted start %d", assureDeactivation);
  std::list<ArServerMode *> *requestedActivateModes = getRequestedActivateModes();

  // if something else wanted to activate we can just deactivate and 
  // that'll get activated
  if (myIsActive && 
      (requestedActivateModes != NULL) &&
      requestedActivateModes->begin() != requestedActivateModes->end())
  {
    deactivate();
	if (getActiveMode() != NULL)
		
      ArLog::log(ArLog::Normal, 
		 "DOCK: Resume interrupted deactivating and returning... %s got activated", 
		 getActiveMode()->getName());
    else
      ArLog::log(ArLog::Normal, 
		 "DOCK: Resume interrupted deactivating and returning... nothing active...");
    return;
  }
  // if we're still active and we interrupted something when we
  // activated then start that up again, if it was stop we interrupted
  // just stay at the dock
  //TODO : remettre
 //if (myModeInterrupted != NULL)
  if (myModeInterrupted != NULL)
	//  ArLog::log(ArLog::Normal, "DOCK: Resume interrupted another... isActive %s modeInterrupted NULL", getActiveMode());
  //TODO : uncomment  
  ArLog::log(ArLog::Normal, "DOCK: Resume interrupted another... isActive %d modeInterrupted %p %s", 
	       myIsActive, myModeInterrupted, myModeInterrupted->getName());
  else
 
  if (myIsActive && myModeInterrupted != NULL && 
      strcmp(myModeInterrupted->getName(), "stop") != 0)
  {
    ArLog::log(ArLog::Normal, "DOCK: Trying to activate interrupted mode %s", 
	       myModeInterrupted->getName());
    myModeInterrupted->activate();
    myModeInterrupted = NULL;
    if (getActiveMode() != NULL)
      ArLog::log(ArLog::Normal, 
		 "DOCK: Did activate mode %s", 
		 getActiveMode()->getName());
    return;
  }

  ArLog::log(ArLog::Normal, "DOCK: Resume interrupted later");
  
  // if we're supposed to assure deactivation and we're still here
  // then deactivate
  if (myIsActive && assureDeactivation)
  {
    ArLog::log(ArLog::Normal, "DOCK: Deactivating");
    deactivate();
    return;
  }
  ArLog::log(ArLog::Normal, "DOCK: Resume interrupted end");
}


AREXPORT void ArServerMyMode::goalDone(ArPose goal){
	ArLog::log(ArLog::Normal,"ArServerMyMode::goalDone");
	//If mode is not activate
	if (!myIsActive)
		return;
	  //myStatus = "Driving into triangle";
	  myGoalDone = true;
	  myGoalFailed = false;
}

AREXPORT void ArServerMyMode::goalFailed(ArPose goal){
	ArLog::log(ArLog::Normal,"ArServerMyMode::goalFailed");
	if (!myIsActive)
		return;
	myGoalFailed = true;
	myGoalDone = false;
	resumeInterrupted(false);
	 // if we're still active try to redock
  if (myIsActive)
  {
    ArLog::log(ArLog::Normal, "Retrying dock");
	
    // MPL added this line for shutting down change
    myRetryDock = true;
    myNeedToPathPlan= true;
    switchState(UNDOCKED); 
    //dock();
  }
  // MPL added these two lines for shutting down change
  else
  {
    myNeedToPathPlan = false;
    switchState(UNDOCKED);
  }
	//switchState(UNDOCKED);
}

AREXPORT void ArServerMyMode::requestUnlock()
{
	//if (myState == UNDOCKED)
		deactivate();
  
}

AREXPORT void ArServerMyMode::forceUnlock(void)
{
  ArUtil::sleep(10);
  myState = UNDOCKED;
  ArServerMode::forceUnlock();
}


