/*
MobileRobots Advanced Robotics Interface for Applications (ARIA)
Copyright (C) 2004, 2005 ActivMedia Robotics LLC
Copyright (C) 2006, 2007, 2008, 2009 MobileRobots Inc.
Copyright (C) 2010, 2011 Adept Technology, Inc.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

If you wish to redistribute ARIA under different terms, contact 
Adept MobileRobots for information about a commercial version of ARIA at 
robots@mobilerobots.com or 
Adept MobileRobots, 10 Columbia Drive, Amherst, NH 03031; 800-639-9481
*/
#include "Aria.h"
#include "ArExport.h"
#include "ArServerModeDeliver.h"
#include <boost/algorithm/string/replace.hpp>
#include "Globals.h"

static char* const greetingMessage[]={"Hello",
"Hi"};
static char* const deliveryMessage[]={"I have %s for you.",
"You can take %s from my back.",
"This is for you : %s"};

static char* const lostMessage[]={"Are you here?",
"Where are you?",
 "Is there anybody here?"};

void queueNowEmpty(){
	ArLog::log(ArLog::Normal,"The queue is empty");
}

void queueNowNotEmpty(){
	ArLog::log(ArLog::Normal,"The queue is not empty");
}

void soundTerminated(){
	ArLog::log(ArLog::Normal,"The sound is terminated");
}




//ArSoundsQueue soundsQueue;

static int const numGreetingMessage = sizeof(greetingMessage)/sizeof(greetingMessage[0]);
static int const numDeliveryingMessage = sizeof(deliveryMessage)/sizeof(deliveryMessage[0]);
static int const numLostMessage = sizeof(lostMessage)/sizeof(lostMessage[0]);

AREXPORT ArServerModeDeliver::ArServerModeDeliver(ArServerBase *server, 
					    ArRobot *robot,
					    bool defunct) : 
 ArServerMode(robot, server, "deliver"),
		myStopGroup(robot),
		myNetDeliverCB(this, &ArServerModeDeliver::netDeliver),
		myCardReadCB(this, &ArServerModeDeliver::handleCardRead),
		myHttpResponseCB(this, &ArServerModeDeliver::handleHttpResponse),
		myHttpFailedCB(this, &ArServerModeDeliver::handleHttpFailed),
		mySoundFinishedCB(this,&ArServerModeDeliver::handleSoundFinished),
		mySoundsQueueEmptyCB(this, &ArServerModeDeliver::handleSoundsQueueIsEmpty),
		myCardReader(&myCardReadCB),
		myHttpRequest(&myHttpResponseCB, &myHttpFailedCB),
		myHttpResponse(NULL),
		myCardRead(NULL)/*,
		myASyncSpeak(&myEndSpeakingCB)*/
 
{
  myMode = "Deliver";
  
  if (myServer != NULL)
  {
	 
    addModeData("deliver", "deliver the robot", &myNetDeliverCB,
		"string: content", "none", "Deliver", "RETURN_NONE");
	myNewCardRead = false;
	myHttpNewResponse = false;
	myHttpRequestFailed = false;
	myNewState = true;
	//myOperatorsName = "";
	attemptFailed = 0;
	strcpy(errorMessage, "No error\0");
	myDone = false;
	mySoundFinished = false;
	if(myCardReader.getRunning())
		myCardReader.stopRunning();
	myCardReader.close();

	speechSynthesizer = new ArCepstral();
	if(!speechSynthesizer) {
		ArLog::log(ArLog::Terse, "Error: no speech synthesizer is available. Are we linked to ArSpeechSynth_Cepstral or ArSpeechSynth_Festival? Are they broken?");
  }

	mySoundsQueue = new ArSoundsQueue(speechSynthesizer,
		speechSynthesizer->getInitCallback(),
		ArSoundPlayer::getPlayWavFileCallback(),
		speechSynthesizer->getInterruptCallback());
	
	mySoundsQueue->addSoundFinishedCallback(&mySoundFinishedCB);
	
	//Obligé de le mettre là, sinon ça ne marche pas
	mySoundsQueue->runAsync();
	
	
	//myServer->addData("deliverInfos","Deliver informations",
	//myServer->addData("deliverInfos",......

  }
}

AREXPORT ArServerModeDeliver::~ArServerModeDeliver()
{
	delete speechSynthesizer;
	delete mySoundsQueue;
}

void ArServerModeDeliver::handleSoundFinished(){
	//ArLog::log(ArLog::Normal,"The sound is finished");
	mySoundFinished = true;
}


//Triggered when card has been read
void ArServerModeDeliver::handleCardRead(char * cardID){
	myNewCardRead = true;
    myCardRead = cardID;
}

void ArServerModeDeliver::handleHttpResponse(char * response){
	myHttpNewResponse = true;
	myHttpResponse = response;
}

void ArServerModeDeliver::handleHttpFailed(){
	ArLog::log(ArLog::Normal,"Http request failed");
	strcpy(errorMessage,"Http request failed\0");
	myHttpRequestFailed = true;
}

void ArServerModeDeliver::handleSoundsQueueIsEmpty(){
	ArLog::log(ArLog::Normal,"Queue is empty");
	mySoundsQueueEmpty = true;
}

//void ArServerModeDeliver::handleEndSpeaking(){
//	ArLog::log(ArLog::Normal,"End speaking");
//	myEndSpeaking = true;
//}

void ArServerModeDeliver::switchState(State state)
{
  State oldState = myState;
  myState = state;
  myNewState = true;
  myStartedState.setToNow();
  
  stateChanged();
}

void ArServerModeDeliver::stateChanged(void)
{
	//mySoundsQueue->play("c:\\temp\\ShortCircuit.wav");
	if((myLastState !=  myState) && myState == FSM_WAITING_FOR_HUMAN_TO_START)
		myStatus = "Waiting";
	/*if((myLastState !=  myState) && myState == FSM_SEND_IDENTIFICATION_REQ)
		myStatus = "Identifying";*/
	if((myLastState !=  myState) && myState == FSM_INFORM_FOR_DELIVERY){
		myStatus = "Deliverying";
	}
	if(myState == FSM_OK)
		myStatus = "Done  by " + std::string(myCardRead);
	if(myState == FSM_FAILED)
		myStatus = "Failed because " + std::string(errorMessage);
  myLastState = myState;
  
}



AREXPORT void ArServerModeDeliver::activate(void)
{
	//Modif ON
 if (isActive() || !baseActivate())
    return;
// myASyncSpeak.runAsync();

 /*if (!baseActivate())
    return;*/
  
	setActivityTimeToNow();
	mySoundsQueue->stop();
	mySoundsQueue->clearQueue();
	mySoundsQueue->runAsync();
	//myStatus = "Starting deliver operation";
	deliverTask();
}

AREXPORT void ArServerModeDeliver::deactivate(void)
{
	//myASyncSpeak.stopRunning();
	//ArLog::log(ArLog::Normal,"ASyncSpeak.running %d",myASyncSpeak.getRunningWithLock()?0:1);
	  myStopGroup.deactivate();
	  //mySoundsQueue->stop();
	  //delete speechSynthesizer;
	  baseDeactivate();
}


AREXPORT void ArServerModeDeliver::netDeliver(ArServerClient *client, 
				     ArNetPacket *packet)
{
   char buf[512];
	packet->bufToStr(buf, sizeof(buf)-1);
	//ArLog::log(ArLog::Normal, "Deliver content %s", buf);
    //myRobot->lock();
   deliver(buf);
}

AREXPORT void ArServerModeDeliver::deliver(const char *content)
{
 std::string strContent(content);
	boost::algorithm::replace_all(strContent, "_", " ");
	myContent = strContent.c_str();
	myMode = "Deliver";
	myStatus = "Starting delivery";
	//this->lockMode();
	activate();
}


AREXPORT void ArServerModeDeliver::userTask(void)
{
	//ArLog::log(ArLog::Normal,"SoundsQueue %s", mySoundsQueue->isPlaying() ? "is speaking.":"is not speaking");
	if(!myDone)
	{
		switch(myState){
			case FSM_START:
				if(myNewState){
					ArLog::log(ArLog::Normal,"State FSM_START");
					myNewCardRead = false;
					myHttpNewResponse = false;
					myHttpRequestFailed = false;
					myNewState = false;
					strcpy(errorMessage, "No error\0");
					switchState(FSM_WAITING_FOR_HUMAN_TO_START);
					
					mySoundsQueue->play("c:\\temp\\scifi025.wav");
					break;
				}
				if(mySoundFinished || myStartedState.secSince() > 8){
					mySoundFinished = false;
					switchState(FSM_WAITING_FOR_HUMAN_TO_START);
				}

				
				
				break;

			case FSM_WAITING_FOR_HUMAN_TO_START:
				if(myNewState){
					ArLog::log(ArLog::Normal,"State FSM_WAITING_FOR_HUMAN_TO_START");
					if(!myCardReader.getRunning()){
						myCardReader.open();
						myCardReader.runAsync();
					}
					myNewState = false;
				}
				if(myStartedState.secSince() > TIMEOUT_ATTENTE_HUMAIN){
					if(attemptFailed++ >= MAX_ATTEMPTS_FAILED){
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
					switchState(FSM_INFORM_FOR_DELIVERY);		
				}	
				break;
			
				case FSM_INFORM_FOR_DELIVERY:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State FSM_INFORM_FOR_DELIVERY");
						myNewState = false;
						
						//sprintf(myDeliveryMessage,getRandomDeliveryMessage(),myContent );
						//sprintf(myDeliveryMessage,myContent);
						//ArLog::log(ArLog::Normal,"Delivery message : %s", myDeliveryMessage);
						//std::string str = "Hello";
						mySoundsQueue->speak(getRandomGreetingMessage());
						//mySoundsQueue->speak("Hello");
						//ArUtil::sleep(100);
						
						//mySoundsQueue->speak("Hello hehe");
						//ArUtil::sleep(100);
						//mySoundsQueue->speak("Nice to see you again");
						//mySoundsQueue->runAsync();
					
					}
					if(mySoundFinished || myStartedState.secSince() > 3){
						mySoundFinished = false;
						switchState(FSM_WAITING_FOR_HUMAN_TO_END);
				}
					/*if(!mySoundsQueue->isPlaying())
						switchState(FSM_WAITING_FOR_HUMAN_TO_END);*/

					//|| myStartedState.secSince()> 5
					//if((!mySoundsQueue->isPlaying() && mySoundsQueue->isInitialized())|| myStartedState.secSince()> 30){
					//////if(mySoundFinished == true){
					////	//mySoundFinished = false;
					//	switchState(FSM_WAITING_FOR_HUMAN_TO_END);
					//}
				
					
					//if(!(mySoundsQueue->isSpeakingOrPlaying())){
					//	ArLog::log(ArLog::Normal,"Speech terminated.");
					//	//mySoundsQueue->stop();
					//	switchState(FSM_WAITING_FOR_HUMAN_TO_END);
					//}
					break;

				case FSM_WAITING_FOR_HUMAN_TO_END:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State FSM_WAITING_FOR_HUMAN_TO_END");	
						myNewState = false;
					}
					if(myStartedState.secSince() > TIMEOUT_ATTENTE_HUMAIN){
						if(attemptFailed++ >= MAX_ATTEMPTS_FAILED){
							myCardReader.stopRunning();
							myCardReader.close();
							strcpy(errorMessage,"Human forgot me\0");
							switchState(FSM_FAILED);
							break;
							
						}
						else{
							//Cepstral : "Are you still here "
							//sprintf(myLostMessage,getRandomLostMessage(),myContent );
							//ArLog::log(ArLog::Normal,myLostMessage);
							//mySoundsQueue->speak(myLostMessage);
							//mySoundsQueue->runAsync();
							switchState(FSM_WAITING_FOR_HUMAN_TO_END);
							break;	
						}
						//if((!(mySoundsQueue->isSpeaking()) && mySoundsQueue->isInitialized())|| myStartedState.secSince()> 2){
						////if(mySoundFinished == true){
						//	//mySoundFinished = false;
						//	switchState(FSM_WAITING_FOR_HUMAN_TO_END);
						//	break;
						//}
						
					}
					
				
				/*	if(mySoundFinished == true){
						mySoundFinished = false;
						switchState(FSM_WAITING_FOR_HUMAN_TO_END);
					}*/

					
					//New card detected
					if(myNewCardRead){
						myNewCardRead = false;
						myCardReader.stopRunning();
						myCardReader.close();
						switchState(FSM_OK);	
					}
					break;


				case FSM_OK:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State FSM_END");
						myNewState = false;
						if(myCardReader.getRunning()){
							myCardReader.stopRunning();
						}
						myCardReader.close();
						//mySoundsQueue->clearQueue();
						mySoundsQueue->speak("It was a pleasure. Bye.");
						//break;
					}

					if(mySoundFinished || myStartedState.secSince() > 8){
						mySoundFinished = false;
						deactivate();
					}
					
					break;
				case FSM_FAILED:
					ArLog::log(ArLog::Normal,"State FSM_FAILED");
					if(myCardReader.getRunning()){
						myCardReader.stopRunning();
						myCardReader.close();
					}
					myDone = true;
					deactivate();
					//Say ByeBye
					break;
				default:
					break;
				}
		}
					
 
}

const char* ArServerModeDeliver::getRandomGreetingMessage(){
	return greetingMessage[rand()%numGreetingMessage];
}

const char* ArServerModeDeliver::getRandomDeliveryMessage(){
	return deliveryMessage[rand()%numDeliveryingMessage];
}

const char* ArServerModeDeliver::getRandomLostMessage(){
	return lostMessage[rand()%numLostMessage];
}

//AREXPORT void ArServerModeDeliver::addToConfig(ArConfig *config, 
//					      const char *section)
//{
//}

//AREXPORT void ArServerModeDeliver::setUseLocationDependentDevices(
//	bool useLocationDependentDevices, bool internal)
//{
//  if (!internal)
//    myRobot->lock();
//  // if this is a change then print it
//  if (useLocationDependentDevices != myUseLocationDependentDevices)
//  {
//    myUseLocationDependentDevices = useLocationDependentDevices;
//    myLimiterForward->setUseLocationDependentDevices(
//	    myUseLocationDependentDevices);
//    myLimiterBackward->setUseLocationDependentDevices(
//	    myUseLocationDependentDevices);
//    if (myLimiterLateralLeft != NULL)
//      myLimiterLateralLeft->setUseLocationDependentDevices(
//	      myUseLocationDependentDevices);
//    if (myLimiterLateralRight != NULL)
//      myLimiterLateralRight->setUseLocationDependentDevices(
//	      myUseLocationDependentDevices);
//  }
//  if (!internal)
//    myRobot->unlock();
//}


void ArServerModeDeliver::deliverTask(){
	
	printf("Deliver task started with content : %s",myContent);
	myDone = false;
	
	switchState(FSM_START);
	
}
