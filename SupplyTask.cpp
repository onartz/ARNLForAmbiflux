#include "SupplyTask.h"

SupplyTask::SupplyTask():myState(CALLING), myRunning(true),
myCardReadCB(this, &SupplyTask::handleCardRead),
myHttpResponseHandler(this, &SupplyTask::handleHttpResponse),
myCardReader(&myCardReadCB)
{
	/*if(!(myCepstral.init())){
		ArLog::log(ArLog::Normal,"Cepstral failed");
	}*/
  // Create the sound queue.
  // Set WAV file callbacks 
  soundQueue.setPlayWavFileCallback(ArSoundPlayer::getPlayWavFileCallback());
  soundQueue.setInterruptWavFileCallback(ArSoundPlayer::getStopPlayingCallback());

  // Notifications when the queue goes empty or non-empty.
  //soundQueue.addQueueEmptyCallback(new ArGlobalFunctor(&queueNowEmpty);
  //soundQueue.addQueueNonemptyCallback(new ArGlobalFunctor(&queueNowNonempty));

  // Run the sound queue in a new thread
  soundQueue.runAsync();
}

SupplyTask::~SupplyTask(){
	soundQueue.stopRunning();
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

void SupplyTask::handleHttpResponse(char * response){
	myHttpResponseReceived = true;
	myHttpResponse = response;
}


void SupplyTask::setSupplyDoneCB(ArFunctor1<char*> *f){
	mySupplyDoneCB = f;
}

void SupplyTask::setSupplyFailedCB(ArFunctor1<char*> *f){
	mySupplyFailedCB = f;
}

//void SupplyTask::addSupplyFailedCB(ArFunctor1C<SupplyTask, char*>* f){
//	mySupplyFailedCB = f;
//}

void *SupplyTask::runThread(void *arg)
{
	DALRest dr(&myHttpResponseHandler);
	
	while (myRunning==true)
	{
		switch(myState){
			case CALLING:
				//Let's sound something or call using playSound
				ArLog::log(ArLog::Normal,"State CALLING");
				//ArLog::log(ArLog::Normal,"Content : %s",myContent);
				soundQueue.play("c:\\temp\\ShortCircuit.wav");
		
				switchState(SupplyTask::WAITING_FOR_CARD);
				break;
			case WAITING_FOR_CARD:
				if(myNewState){
					ArLog::log(ArLog::Normal,"State WAITING_FOR_CARD");
					myCardReader.open();
					myCardReader.runAsync();
					myNewState = false;
				}
				//New card detected
				if(myNewCardRead){
					myNewCardRead = false;
					myCardReader.stopRunning();
					myCardReader.close();

					//Identify Card Owner
					std::string req("employeeByCardId");
					std::string param(myCardRead);
					dr.sendRequest(&myHttpResponseHandler, req, param);	
					switchState(SupplyTask::WAITING_FOR_RESPONSE);	
				}
				break;

				case SupplyTask::WAITING_FOR_RESPONSE:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State WAITING_FOR_RESPONSE");
						myNewState = false;
					}
					if(!myHttpResponseReceived)
						break;
					if(myHttpResponse == NULL)
						break;
					if(strcmp(myHttpResponse,"")){
						
						try{	
							boost::property_tree::ptree pt = JSONParser::parse(myHttpResponse);
							
							myOperatorsName = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("firstname").c_str()));
							
							//printf("Operators name : %s",myOperatorsName.c_str());

							//lastName = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("lastname").c_str()));
							//cardId = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("cardID").c_str()));
							//emailAddress = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("emailAddress").c_str()));	
						}
						catch(std::exception const&  ex)
						{
							myHttpResponse = NULL;
							printf("JSON Error. %s", ex.what());
						}
						
						myHttpResponse = NULL;
						switchState(SupplyTask::TELLING_WHAT_TO_DO);

					}
					break;
				case TELLING_WHAT_TO_DO:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State TELLING_WHAT_TO_DO");
						myNewState = false;
					}
					switchState(WAITING_FOR_CARD_TO_CLOSE);


					break;
				case WAITING_FOR_CARD_TO_CLOSE:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State WAITING_FOR_CARD_TO_CLOSE");
						myCardReader.open();
						myCardReader.runAsync();
						myNewState = false;
					}
					//New card detected
					if(myNewCardRead){
						myNewCardRead = false;
						myCardReader.stopRunning();
						myCardReader.close();
						switchState(SupplyTask::END);	
					}
					break;

		
			case SupplyTask::END:
				ArLog::log(ArLog::Normal,"State END");
				//Say ByeBye
				myRunning = false;
				
				//char * result = new char[myOperatorsName.size() + 1];
				//std::copy(myOperatorsName.begin(), myOperatorsName.end(), result);
				//result[myOperatorsName.size()] = '\0'; // don't forget the terminating 0

			// don't forget to free the string after finished using it
				//delete[] myOperatorsName;

				mySupplyDoneCB->invoke(myCardRead);
				break;
		}
		ArUtil::sleep(500);
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