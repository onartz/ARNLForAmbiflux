#include "DeliverTask.h"

DeliverTask::DeliverTask():myState(FSM_START),
myRunning(true),
myCardReadCB(this, &DeliverTask::handleCardRead),
myHttpResponseCB(this, &DeliverTask::handleHttpResponse),
myHttpFailedCB(this, &DeliverTask::handleHttpFailed),
myCardReader(&myCardReadCB),
myHttpRequest(&myHttpResponseCB, &myHttpFailedCB),
myHttpResponse(NULL)
{
	// Notifications when the queue goes empty or non-empty.
  
	soundQueue.setPlayWavFileCallback(ArSoundPlayer::getPlayWavFileCallback());
	soundQueue.setInterruptWavFileCallback(ArSoundPlayer::getStopPlayingCallback());

	//soundQueue.runAsync();
	/*if(!(myCepstral.init())){
		ArLog::log(ArLog::Normal,"Cepstral failed");
	}*/
  
}

DeliverTask::~DeliverTask(){
	soundQueue.stopRunning();
}

void DeliverTask::init(const char * content){
	myContent = content;
	myHttpNewResponse = false;
	myNewCardRead = false;
	myHttpRequestFailed = false;
	myCardRead = NULL;
	myHttpResponse = NULL;
	switchState(FSM_START);
	soundQueue.runAsync();
}

void DeliverTask::queueNowEmpty() {
  //printf("The sound queue is now empty.\n");
}

void DeliverTask::queueNowNonempty() {
  //printf("The sound queue is now non-empty.\n");
}

bool DeliverTask::no() {
  // just a false tautology
  return false;
}



//Triggered when card has been read
void DeliverTask::handleCardRead(char * cardID){
	myNewCardRead = true;
    myCardRead = cardID;
}

void DeliverTask::handleHttpResponse(char * response){
	myHttpNewResponse = true;
	myHttpResponse = response;
}

void DeliverTask::handleHttpFailed(){
	ArLog::log(ArLog::Normal,"Http request failed");
	strcpy(errorMessage,"Http request failed\0");
	myHttpRequestFailed = true;
}


void DeliverTask::setDeliverDoneCB(ArFunctor1<char*> *f){
	myDeliverDoneCB = f;
}

void DeliverTask::setDeliverFailedCB(ArFunctor1<char*> *f){
	myDeliverFailedCB = f;
}

void DeliverTask::stopRunning(){
	soundQueue.stopRunning();
}


void *DeliverTask::runThread(void *arg)
{
	myRunning = true;
	ArLog::log(ArLog::Normal,"Deliver task thread started");
	while (myRunning==true)
	{
		switch(myState){
			case FSM_START:
				//Let's sound something or call using playSound
				ArLog::log(ArLog::Normal,"State FSM_START");
				//ArLog::log(ArLog::Normal,"Content : %s",myContent);
				soundQueue.play("c:\\temp\\ShortCircuit.wav");	
				switchState(FSM_WAITING_FOR_HUMAN_TO_START);
				attemptFailed = 0;
				break;
			case FSM_WAITING_FOR_HUMAN_TO_START:
				if(myNewState){
					ArLog::log(ArLog::Normal,"State FSM_WAITING_FOR_START");
					myCardReader.open();
					myCardReader.runAsync();
					myNewState = false;
				}
				if(myStartedState.secSince() > TIMEOUT_ATTENTE_HUMAIN){
					if(attemptFailed++ > MAX_ATTEMPTS_FAILED){
						myCardReader.stopRunning();
						myCardReader.close();
						strcpy(errorMessage,"Human not present\0");
						switchState(FSM_FAILED);
						
					}
					else{
						switchState(FSM_START);
					}
					break;
				}
				//New card detected
				if(myNewCardRead){
					myNewCardRead = false;
					myCardReader.stopRunning();
					myCardReader.close();
					switchState(FSM_GIVING_INFORMATIONS);		
				}	
				break;

			
				case FSM_GIVING_INFORMATIONS:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State FSM_GIVING_INFORMATIONS");
						myNewState = false;
						myCardReader.open();
						myCardReader.runAsync();
						myNewState = false;
						//Cepstral : "In formation + Use your badge to teminate"
					}
					switchState(FSM_WAITING_FOR_HUMAN_TO_END);

					break;
				case FSM_WAITING_FOR_HUMAN_TO_END:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State FSM_WAITING_FOR_HUMAN_TO_END");	
						myNewState = false;
					}
					if(myStartedState.secSince() > TIMEOUT_ATTENTE_HUMAIN){
						if(attemptFailed++ > MAX_ATTEMPTS_FAILED){
							myCardReader.stopRunning();
							myCardReader.close();
							strcpy(errorMessage,"Human forgot me\0");
							switchState(FSM_FAILED);
							
						}
						else{
							//Cepstral : "Are you still here"
							switchState(FSM_WAITING_FOR_HUMAN_TO_END);
						}
						break;
					}
					
					//New card detected
					if(myNewCardRead){
						myNewCardRead = false;
						myCardReader.stopRunning();
						myCardReader.close();
						//myDeliverDoneCB->invoke(myCardRead);
						switchState(FSM_END);	
					}
					break;

		
				case FSM_END:
					ArLog::log(ArLog::Normal,"State FSM_END");
					//Say ByeBye
					myRunning = false;
					char res[64];
					sprintf(res,"Done by %s",myCardRead);
					myDeliverDoneCB->invoke(res);
					break;
				case FSM_FAILED:
					ArLog::log(ArLog::Normal,"State FSM_END");
					//Say ByeBye
					myRunning = false;
					myDeliverFailedCB->invoke(errorMessage);
					break;
			}
		ArUtil::sleep(100);
	}

	ArLog::log(ArLog::Normal,"Deliver task thread stopped");
  // return out here, means the thread is done
  return NULL;
}

void DeliverTask::switchState(State state)
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

void DeliverTask::stateChanged(void)
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