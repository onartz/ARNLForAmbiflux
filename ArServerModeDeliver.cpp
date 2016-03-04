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
		mySoundsQueueNonEmptyCB(this,&ArServerModeDeliver::handleSoundsQueueIsNotEmpty),
		myCardReader(&myCardReadCB),
		myHttpRequest(&myHttpResponseCB, &myHttpFailedCB)
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
	mySoundsQueue->addQueueEmptyCallback(&mySoundsQueueEmptyCB);
	mySoundsQueue->addQueueNonemptyCallback(&mySoundsQueueNonEmptyCB);
	
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
	strcpy(myCardRead,cardID);
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

void ArServerModeDeliver::handleSoundsQueueIsNotEmpty(){
	ArLog::log(ArLog::Normal,"Queue is not empty");
	mySoundsQueueEmpty = false;
}

//void ArServerModeDeliver::handleEndSpeaking(){
//	ArLog::log(ArLog::Normal,"End speaking");
//	myEndSpeaking = true;
//}

void ArServerModeDeliver::switchState(State state)
{
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
  
	setActivityTimeToNow();
	mySoundsQueue->stop();
	mySoundsQueue->clearQueue();
	mySoundsQueue->runAsync();
	deliverTask();
}

AREXPORT void ArServerModeDeliver::deactivate(void)
{
	  myStopGroup.deactivate();
	  baseDeactivate();
}


AREXPORT void ArServerModeDeliver::netDeliver(ArServerClient *client, 
				     ArNetPacket *packet)
{
   char buf[512];
	packet->bufToStr(buf, sizeof(buf)-1);
    //myRobot->lock();
   deliver(buf);
}

AREXPORT void ArServerModeDeliver::deliver(const char *content)
{
	std::string strContent(content);
	boost::algorithm::replace_all(strContent, "_", " ");
	strcpy(myContent,strContent.c_str());
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
					mySoundsQueue->play("c:\\temp\\ShortCircuit.wav");
					break;
				}
				//We have to wait until the end of speach
				if((mySoundFinished && mySoundsQueueEmpty) || myStartedState.secSince() > 8){
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
						sprintf(myGreetingMessage,getRandomGreetingMessage(),"");
						sprintf(myDeliveryMessage,getRandomDeliveryingMessage(),myContent );
						ArLog::log(ArLog::Normal,"Greeting message : %s", myGreetingMessage);
						ArLog::log(ArLog::Normal,"Deliverying message : %s", myDeliveryMessage);

						mySoundsQueue->speak(myGreetingMessage);
						ArUtil::sleep(100);
						mySoundsQueue->speak(myDeliveryMessage);
						ArUtil::sleep(100);
						break;
					
					}
					//We have to wait until the end of speach
					if(mySoundsQueueEmpty && mySoundFinished){
						ArLog::log(ArLog::Normal,"Speak ended");
						mySoundFinished = false;
						switchState(FSM_WAITING_FOR_HUMAN_TO_END);
						break;
					}
					else if(myStartedState.secSince() > 10){
						mySoundFinished = false;
						switchState(FSM_WAITING_FOR_HUMAN_TO_END);
						break;
					}
					else
						break;
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
							//break;
							
						}
						else{
							//Cepstral : "Are you still here "
							sprintf(myLostMessage,getRandomLostMessage());
							mySoundsQueue->speak(myLostMessage);
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
						myDone = true;
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
					break;
				default:
					break;
				}
		}					
 
}



void ArServerModeDeliver::deliverTask(){
	
	printf("Deliver task started with content : %s",myContent);
	myDone = false;
	
	switchState(FSM_START);
	
}
