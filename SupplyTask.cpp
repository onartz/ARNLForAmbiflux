#include "SupplyTask.h"

SupplyTask::SupplyTask():myState(FSM_START),
myRunning(true),
myCardReadCB(this, &SupplyTask::handleCardRead),
myHttpResponseCB(this, &SupplyTask::handleHttpResponse),
myHttpFailedCB(this, &SupplyTask::handleHttpFailed),
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

SupplyTask::~SupplyTask(){
	soundQueue.stopRunning();
}

void SupplyTask::init(const char * content){
	myContent = content;
	myHttpNewResponse = false;
	myNewCardRead = false;
	myHttpRequestFailed = false;
	myCardRead = NULL;
	myHttpResponse = NULL;
	switchState(FSM_START);
	soundQueue.runAsync();
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



//Triggered when card has been read
void SupplyTask::handleCardRead(char * cardID){
	myNewCardRead = true;
    myCardRead = cardID;
}

void SupplyTask::handleHttpResponse(char * response){
	myHttpNewResponse = true;
	myHttpResponse = response;
}

void SupplyTask::handleHttpFailed(){
	ArLog::log(ArLog::Normal,"Http request failed");
	myHttpRequestFailed = true;

}


void SupplyTask::setSupplyDoneCB(ArFunctor1<char*> *f){
	mySupplyDoneCB = f;
}

void SupplyTask::setSupplyFailedCB(ArFunctor1<char*> *f){
	mySupplyFailedCB = f;
}

void SupplyTask::stopRunning(){
	soundQueue.stopRunning();
}


void *SupplyTask::runThread(void *arg)
{
	myRunning = true;
	ArLog::log(ArLog::Normal,"Supply task thread started");
	while (myRunning==true)
	{
		switch(myState){
			case FSM_START:
				//Let's sound something or call using playSound
				ArLog::log(ArLog::Normal,"State FSM_START");
				//ArLog::log(ArLog::Normal,"Content : %s",myContent);
				soundQueue.play("c:\\temp\\ShortCircuit.wav");	
				switchState(FSM_WAITING_FOR_HUMAN_TO_START);
				break;
			case FSM_WAITING_FOR_HUMAN_TO_START:
				if(myNewState){
					ArLog::log(ArLog::Normal,"State FSM_WAITING_FOR_START");
					myCardReader.open();
					myCardReader.runAsync();
					myNewState = false;
				}
				if(myStartedState.secSince() > TIMEOUT_ATTENTE_HUMAIN){
					myCardReader.stopRunning();
					myCardReader.close();
					mySupplyFailedCB->invoke("Human not present\0");
					switchState(FSM_END);
					break;
				}
				//New card detected
				if(myNewCardRead){
					myNewCardRead = false;
					myCardReader.stopRunning();
					myCardReader.close();

					//Identify Card Owner
					std::string req("employeeByCardId");
					std::string param(myCardRead);
					//Send http request to REST Server
					//myHttpRequest.sendRequest(&myHttpResponseCB, req, param);	
					myHttpRequest.sendRequest(req, param);	
					switchState(FSM_WAITING_FOR_IDENTIFICATION);	
				}	
				break;

				case FSM_WAITING_FOR_IDENTIFICATION:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State FSM_WAITING_FOR_IDENTIFICATION");
						myNewState = false;
					}	
					if(myHttpRequestFailed){
						switchState(FSM_END);
						mySupplyFailedCB->invoke("Http server failed\0");
						break;
					}

					if(!myHttpNewResponse)
						break;
					if(myHttpResponse == NULL)
						break;
					if(strcmp(myHttpResponse,""))
					{
						try{	
							boost::property_tree::ptree pt = JSONParser::parse(myHttpResponse);		
							myOperatorsName = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("firstname").c_str()));						
							//printf("Operators name : %s",myOperatorsName.c_str());

							//lastName = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("lastname").c_str()));
							//cardId = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("cardID").c_str()));
							//emailAddress = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("emailAddress").c_str()));	
							}
						catch(std::exception const&  ex){
							myHttpResponse = NULL;
							printf("JSON Error. %s", ex.what());
						}
							
						myHttpResponse = NULL;
						switchState(FSM_GIVING_INFORMATIONS);
					}
					break;
				case FSM_GIVING_INFORMATIONS:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State FSM_GIVING_INFORMATIONS");
						myNewState = false;
					}
					switchState(FSM_WAITING_FOR_HUMAN_TO_END);

					break;
				case FSM_WAITING_FOR_HUMAN_TO_END:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State FSM_WAITING_FOR_HUMAN_TO_END");
						myCardReader.open();
						myCardReader.runAsync();
						myNewState = false;
					}
					if(myStartedState.secSince() > TIMEOUT_ATTENTE_HUMAIN){
						myCardReader.stopRunning();
						myCardReader.close();
						mySupplyFailedCB->invoke("Human forgot me\0");
						switchState(FSM_END);
						break;
					}
					
					//New card detected
					if(myNewCardRead){
						myNewCardRead = false;
						myCardReader.stopRunning();
						myCardReader.close();
						mySupplyDoneCB->invoke(myCardRead);
						switchState(FSM_END);	
					}
					break;

		
				case FSM_END:
					ArLog::log(ArLog::Normal,"State FSM_END");
					//Say ByeBye
					myRunning = false;
					break;
			}
		ArUtil::sleep(100);
	}

	ArLog::log(ArLog::Normal,"Supply task thread stopped");
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