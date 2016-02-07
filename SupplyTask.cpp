#include "SupplyTask.h"
SupplyTask::SupplyTask():myState(CALLING), myRunning(true){
}

SupplyTask::~SupplyTask(){
}

void SupplyTask::setContent(const char * content){
	myContent = content;
}

void *SupplyTask::runThread(void *arg)
{
	while (myRunning==true)
	{
		switch(myState){
			case CALLING:
				ArLog::log(ArLog::Normal,"State CALLING");
				ArLog::log(ArLog::Normal,"Content : %s",myContent);
				switchState(SupplyTask::WAITINGFORCARD);
				break;
			case SupplyTask::WAITINGFORCARD:
				ArLog::log(ArLog::Normal,"State WAITINGFORCARD");
				switchState(SupplyTask::WAITINGFOREND);
				break;
			case SupplyTask::WAITINGFOREND:
				ArLog::log(ArLog::Normal,"State WAITINGFOREND");
				switchState(SupplyTask::END);
				break;
			case SupplyTask::END:
				ArLog::log(ArLog::Normal,"State END");
				myRunning = false;
				break;

		}
		ArUtil::sleep(2000);
	}

  // return out here, means the thread is done
  return NULL;
}

void SupplyTask::switchState(State state)
{
  State oldState = myState;
  myState = state;
  myStartedState.setToNow();
  /*if (oldState != myState)
  {
    if (myState == DOCKED)
    {
      myDockedCBList.invoke();
      mySingleShotDockedCBList.invoke();
    }
    else if (myState == UNDOCKING)
      myUndockingCBList.invoke();
    else if (myState == UNDOCKED)
      myUndockedCBList.invoke();
  }

  if (myShutdownFunctor != NULL && myState == DOCKING && 
      myShutdownLastPose.squaredFindDistanceTo(myRobot->getPose()) > 20 * 20)
  {
    myShutdownLastPose = myRobot->getPose();
    myShutdownLastMove.setToNow();
  }*/
  
  stateChanged();
}

void SupplyTask::stateChanged(void)
{
  // if we were and are docked see if forced toggled
 /* if (myLastState == DOCKED && myState == DOCKED && 
      myLastForcedDock != myForcedDock)
  {
    if (myForcedDock)
      myDockNowForcedCBList.invoke();
    else
      myDockNowUnforcedCBList.invoke();
  }
  myLastForcedDock = myForcedDock;*/
  myLastState = myState;

 /* std::list<ArFunctor *>::iterator it;
  for (it = myStateChangedCBList.begin(); 
       it != myStateChangedCBList.end(); 
       it++)
    (*it)->invoke();

  broadcastDockInfoChanged();*/
}