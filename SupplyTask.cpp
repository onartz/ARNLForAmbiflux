#include "SupplyTask.h"




SupplyTask::SupplyTask():myState(CALLING), myRunning(true),
myCardReadCB(this, &SupplyTask::handleCardRead),
myCardReader(&myCardReadCB)
{

  // Create the sound queue.


  // Set WAV file callbacks 
  soundQueue.setPlayWavFileCallback(ArSoundPlayer::getPlayWavFileCallback());
  soundQueue.setInterruptWavFileCallback(ArSoundPlayer::getStopPlayingCallback());

  // Notifications when the queue goes empty or non-empty.
  //soundQueue.addQueueEmptyCallback(new ArGlobalFunctor(&queueNowEmpty));
  //soundQueue.addQueueNonemptyCallback(new ArGlobalFunctor(&queueNowNonempty));

  // Run the sound queue in a new thread
  soundQueue.runAsync();
}

SupplyTask::~SupplyTask(){
}

void SupplyTask::queueNowEmpty() {
  //printf("The sound queue is now empty.\n");
}

void SupplyTask::queueNowNonempty() {
  //printf("The sound queue is now non-empty.\n");
}

bool SupplyTask::no() {
  // just a false tautology
  return false;
}

void SupplyTask::setContent(const char * content){
	myContent = content;
}

//Triggered when card has been read
void SupplyTask::handleCardRead(char * cardID){
	myNewCardRead = true;
    myCardRead = cardID;
}

void *SupplyTask::runThread(void *arg)
{
	while (myRunning==true)
	{
		switch(myState){
			case CALLING:
				//Let's sound something or call using playSound
				ArLog::log(ArLog::Normal,"State CALLING");
				ArLog::log(ArLog::Normal,"Content : %s",myContent);
				soundQueue.play("c:\\temp\\ShortCircuit.wav");
				
				switchState(SupplyTask::WAITINGFORCARD);
				break;
			case SupplyTask::WAITINGFORCARD:
				if(myNewState){
					ArLog::log(ArLog::Normal,"State WAITINGFORCARD");
					myCardReader.open();
					myCardReader.runAsync();
					myNewState = false;
				}
				//New card detected
				if(myNewCardRead){
					myNewCardRead = false;
					myCardReader.stopRunning();
					myCardReader.close();
					printf("Card read : %s",myCardRead);
					//Identify Card Owner

					//identifyOwner();
					//Say hello to Card Owner
					//Tell content to charge

					switchState(SupplyTask::WAITINGFOREND);
					
				}
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
  myNewState = true;
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