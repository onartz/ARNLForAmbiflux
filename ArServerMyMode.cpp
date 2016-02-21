

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
myBackOutDist(backOutDist)
{
	myPathTask = pathTask;
	myLocTask = locTask;
	myMap = arMap;
	myMode = "Goto triangle";
	myStatus = "Starting";

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
	addModeData("dockTo", "sends the robot to the goal", 
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

	
	//myDriveTo.setMaxAngleMisalignment(10);
	myDriveTo.setMaxLateralDist(500);
	myDriveTo.setMaxDistBetweenLinePoints(50);

	myBackGroup.addAction(new ArActionLimiterBackwards("backwards limiter",
						     -200, 0, -200, 1.2), 51);
	myBackGroup.addAction(&myGoto, 50);

	myHitDock = false;
	myDriveFromValid = false;


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
  ArLog::log(ArLog::Normal, "Going to goal %s", buf);
  //myRobot->lock();
  gotoTriangle(buf);
  //myRobot->unlock();
}

AREXPORT void ArServerMyMode::serverUndockFrom(ArServerClient * /*client*/, 
					       ArNetPacket *packet)
{
  //char buf[512];
  //packet->bufToStr(buf, sizeof(buf)-1);
  ArLog::log(ArLog::Normal, "Undocking from");
  //myRobot->lock();
  undockFrom();
  //myRobot->unlock();
}

AREXPORT void ArServerMyMode::activate(){
	if(!baseActivate())
		return;
}

AREXPORT void ArServerMyMode::deactivate(){
	 if (myState == UNDOCKED)
  {
    baseDeactivate();
  }
  else
    undockFrom();
}

AREXPORT void ArServerMyMode::dock(){
	if(myIsActive && myState == DOCKED)
		return;
	activate();
	if (!myIsActive || (myState != UNDOCKED))// && !myRetryDock))
		return;
	myNeedToPathPlan = true;
	switchState(UNDOCKED);
}


AREXPORT void ArServerMyMode::userTask(){
	bool printing = false;
	int frontBump;
	int frontBumpMask = (ArUtil::BIT1 | ArUtil::BIT2 | ArUtil::BIT3 | 
		       ArUtil::BIT4 | ArUtil::BIT5 | ArUtil::BIT6);

	if(myState == UNDOCKED){
		if (myNeedToPathPlan && !myServer->idleProcessingPending())
		{
			myGoalDone = false;
			myDriveTo.deactivate();
			myRobot->clearDirectMotion();
			myNeedToPathPlan = false;
			if(!myPathTask->pathPlanToGoal(myGoalName.c_str())){
				myStatus = "Failed to go to triangle";
				return;
			}
			myStatus = "Going to goal at ";
			myStatus += myGoalName;
			ArLog::log(ArLog::Normal, "Docking at %s", myGoalName.c_str());
			switchState(DOCKING);
		}	
	}
	else if(myState == DOCKING)
	{
		frontBump = ((myRobot->getStallValue() & 0xff00) >> 8) & frontBumpMask;
		if (myStallsAsBumps && (myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled()))
		  frontBump |= ArUtil::BIT0;
		// if we're there and haven't sent the command
		if (myHitDock)
		{
			ArLog::log(ArLog::Normal, "DockedTo");		  
			myStatus = "DockedTo";
			switchState(DOCKED);
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
		else if ((myDriveTo.isActive() && 
			 frontBump) || myDriveTo.getState() == ArActionTriangleDriveTo::STATE_SUCCEEDED)
		{
			ArLog::log(ArLog::Normal, "Hit dock");
			myHitDock = true;
			//myHitDockTime.setToNow();
			switchState(DOCKED);
			myDriveTo.deactivate();
		}
		// if we failed getting there
		else if (myDriveTo.isActive() && myDriveTo.getState() == ArActionTriangleDriveTo::STATE_FAILED)
		{
		  ArLog::log(ArLog::Normal, 
			 "Dock Failed: Could not find dock target");
		  myDriveTo.deactivate();
		  //myRetryDock = true;
		  undockFrom();
		}
		
		// if it took longer than 30 seconds to drive in
		else if (myDriveTo.isActive() && myStartedDriveTo.secSince() > 30)
		{
		  ArLog::log(ArLog::Normal, 
			 "Dock Failed: Took too long to find target");
		  myStatus = "Failed dock";
		  myDriveTo.deactivate();
		  //myRetryDock = true;
		}
		// if we've gotten to our goal and need to drive into the dock
		else if (myGoalDone && !myDriveTo.isActive())
		{
		  ArLog::log(ArLog::Normal, "Driving into dock");
		  //beforeDriveInCallback();
		  myDriveFromValid = true;
		  myDriveFrom = myRobot->getEncoderPose();
		  myGroup.activateExclusive();
		  myGoalDone = false;
		  myStartedDriveTo.setToNow();
		  //myDrivingIntoDockCBList.invoke();
		}
	}
	else if(myState == UNDOCKING)
	{
		myHitDock = false;
		if (printing)
		  printf("Undocking %ld\n", myStartedState.secSince());

		// make it so that we're always trying to move while undocking, so
		// the sonar kick in before we start backing up
		myRobot->forceTryingToMove();
		if (myStartedState.secSince() >= 3 && !myStartedBacking)
		{
		  myStartedBacking = true;
		  //myGoto.setEncoderGoalRel(myDistanceToBack, -180, true);
		  myGoto.setDistance(-myDistanceToBack, true);
		  myRobot->clearDirectMotion();
		  myBackGroup.activateExclusive();
		}
		if (myStartedBacking && myGoto.haveAchievedDistance())
		{
		  //backoutCallback();
		}

		if (myStartedBacking && myGoto.haveAchievedDistance())
		{
		  //afterDriveOutCallback();
		  /*if (myRetryDock)
		  {
			myRetryDock = false;
			switchState(UNDOCKED);
			ArLog::log(ArLog::Normal, "Retrying dock");
			dock();
			return;
		  }*/
		  myStatus = "Undocked from";
		  switchState(UNDOCKED);
		  ArLog::log(ArLog::Normal, "Undocked from");
		  //resumeInterrupted(true);
		}
		if (myStartedState.secSince() >= 60)
		{
		  ArLog::log(ArLog::Normal, "Error undocking, taken too long");
		  myStartedState.setToNow();
		  myStatus = "Undocking failing";
		}
	}
	else if (myState == UNDOCKED){
	}
}


AREXPORT void ArServerMyMode::undockFrom(){
  //myRobot->enableMotors();
  if (myState == UNDOCKED)
  {
    //resumeInterrupted(true);
  }
  else if (myState == DOCKED)
  {
    if (myDriveFromValid)
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
    myRobot->stop();
    ArLog::log(ArLog::Normal, "Undocking");
	if (myDriveFromValid)
      myDistanceToBack = myRobot->getEncoderPose().findDistanceTo(myDriveFrom);
    else
      myDistanceToBack = myRobot->getRobotRadius() + 1000;
    myStartedBacking = false;
    if (myHitDock || myDriveTo.isActive())
    {
      myStatus = "Undocking";
      switchState(UNDOCKING);
    }
    else
    {
      switchState(UNDOCKED);
	  myStatus = "Undocked";
      //resumeInterrupted(true);
    }
  }
  // if we're already undocking ignore this
  else if (myState == UNDOCKING)
  {
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
	ArLog::log(ArLog::Normal, "State = %s", toString(state));
	myStartedState.setToNow();
	if (oldState != myState)
	{
		if (myState == DOCKED)
		{
			//myStatus = "Docked";
			/*myDockedCBList.invoke();
			mySingleShotDockedCBList.invoke();*/
		}
		else if (myState == UNDOCKING){}
			
			//myUndockingCBList.invoke();
		else if (myState == UNDOCKED){}
			//myStatus = "UnDocked";
			//myUndockedCBList.invoke();

	//}
		stateChanged();
	}
}

void ArServerMyMode::stateChanged(void){
}

AREXPORT void ArServerMyMode::goalDone(ArPose goal){
	ArLog::log(ArLog::Normal,"ArServerMyMode::goalDone");
	//If mode is not activate
	if (!myIsActive)
		return;
	  myStatus = "Driving into triangle";
	  myGoalDone = true;
}

AREXPORT void ArServerMyMode::goalFailed(ArPose goal){
	ArLog::log(ArLog::Normal,"ArServerMyMode::goalFailed");
	if (!myIsActive || myState != DOCKING)
		return;
	switchState(UNDOCKED);
}



