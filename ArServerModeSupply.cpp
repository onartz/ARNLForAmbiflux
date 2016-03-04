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
#include "ArServerModeSupply.h"
#include <boost/algorithm/string/replace.hpp>

AREXPORT ArServerModeSupply::ArServerModeSupply(ArServerBase *server, 
					    ArRobot *robot,
					    bool defunct) : 
		ArServerMode(robot, server, "supply"),
		myStopGroup(robot),
		myNetSupplyCB(this, &ArServerModeSupply::netSupply),
		myCardReadCB(this, &ArServerModeSupply::handleCardRead),
		myHttpResponseCB(this, &ArServerModeSupply::handleHttpResponse),
		myHttpFailedCB(this, &ArServerModeSupply::handleHttpFailed),
		mySoundFinishedCB(this,&ArServerModeSupply::handleSoundFinished),
		mySoundsQueueEmptyCB(this, &ArServerModeSupply::handleSoundsQueueIsEmpty),
		mySoundsQueueNonEmptyCB(this,&ArServerModeSupply::handleSoundsQueueIsNotEmpty),
		myCardReader(&myCardReadCB),
		myHttpRequest(&myHttpResponseCB, &myHttpFailedCB)
		
{
  myMode = "Supply";

  if (myServer != NULL)
  {
    addModeData("supply", "supply the robot", &myNetSupplyCB,
		"string: content", "none", "Supply", "RETURN_NONE");
	
	myNewCardRead = false;
	myHttpNewResponse = false;
	myHttpRequestFailed = false;
	myNewState = true;
	myOperatorsName = "";
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

	//speechSynthesizer->setVoice("Diane");

	mySoundsQueue = new ArSoundsQueue(speechSynthesizer,
		speechSynthesizer->getInitCallback(),
		ArSoundPlayer::getPlayWavFileCallback(),
		speechSynthesizer->getInterruptCallback());
	
	mySoundsQueue->addSoundFinishedCallback(&mySoundFinishedCB);
	mySoundsQueue->addQueueEmptyCallback(&mySoundsQueueEmptyCB);
	mySoundsQueue->addQueueNonemptyCallback(&mySoundsQueueNonEmptyCB);
	
	
	//Obligé de le mettre là, sinon ça ne marche pas
	mySoundsQueue->runAsync();

  }
}

AREXPORT ArServerModeSupply::~ArServerModeSupply()
{
	delete speechSynthesizer;
	delete mySoundsQueue;
}

void ArServerModeSupply::handleSoundFinished(){
	//ArLog::log(ArLog::Normal,"The sound is finished");
	mySoundFinished = true;
}
//Triggered when card has been read
void ArServerModeSupply::handleCardRead(char * cardID){
	myNewCardRead = true;
	strcpy(myCardRead,cardID);
    //myCardRead = cardID;
}

void ArServerModeSupply::handleHttpResponse(char * response){
	myHttpNewResponse = true;
	myHttpResponse = response;
}

void ArServerModeSupply::handleHttpFailed(){
	ArLog::log(ArLog::Normal,"Http request failed");
	strcpy(errorMessage,"Http request failed\0");
	myHttpRequestFailed = true;
}

void ArServerModeSupply::handleSoundsQueueIsEmpty(){
	ArLog::log(ArLog::Normal,"Queue is empty");
	mySoundsQueueEmpty = true;
}

void ArServerModeSupply::handleSoundsQueueIsNotEmpty(){
	ArLog::log(ArLog::Normal,"Queue is not empty");
	mySoundsQueueEmpty = false;
}

void ArServerModeSupply::switchState(State state)
{
  //State oldState = myState;
	myState = state;
	myNewState = true;
	myStartedState.setToNow();
	stateChanged();
}

void ArServerModeSupply::stateChanged(void)
{
	//g_SoundsQueue.play("c:\\temp\\ShortCircuit.wav");
	if((myLastState !=  myState) && myState == FSM_WAITING_FOR_HUMAN_TO_START)
		myStatus = "Waiting";
	if((myLastState !=  myState) && myState == FSM_SEND_IDENTIFICATION_REQ)
		myStatus = "Identifying";
	if((myLastState !=  myState) && myState == FSM_INFORM_FOR_SUPPLY)
		myStatus = "Supplying";
	if(myState == FSM_OK)
		myStatus = "Done  by " + std::string(myCardRead);
	if(myState == FSM_FAILED)
		myStatus = "Failed because " + std::string(errorMessage);
	 myLastState = myState;
  
  
}


AREXPORT void ArServerModeSupply::activate(void)
{
	if (!baseActivate())
	return;
	mySoundsQueue->stop();
	mySoundsQueue->clearQueue();
	mySoundsQueue->runAsync();
	setActivityTimeToNow();
	supplyTask();
}

AREXPORT void ArServerModeSupply::deactivate(void)
{
  myStopGroup.deactivate();
  baseDeactivate();
}


AREXPORT void ArServerModeSupply::netSupply(ArServerClient *client, 
				     ArNetPacket *packet)
{
   char buf[512];
	packet->bufToStr(buf, sizeof(buf)-1);
	//ArLog::log(ArLog::Normal, "Supply content %s", buf);
    //myRobot->lock();
   supply(buf);
}

AREXPORT void ArServerModeSupply::supply(const char *content)
{
	std::string strContent(content);
	boost::algorithm::replace_all(strContent, "_", " ");
	//myContent = strContent.c_str();
	strcpy(myContent,strContent.c_str());
	myMode = "Supply";
	myStatus = "Starting supply";
	//this->lockMode();
	activate();
}


AREXPORT void ArServerModeSupply::userTask(void)
{
	if(!myDone)
	{
		switch(myState){
			case FSM_START:
				if(myNewState){
					//Let's sound something or call using playSound
					ArLog::log(ArLog::Normal,"State FSM_START");
					myNewCardRead = false;
					myHttpNewResponse = false;
					myHttpRequestFailed = false;
					myNewState = false;
					myOperatorsName = "";
					//attemptFailed = 0;
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
					//myStatus = "Waiting";
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
					switchState(FSM_SEND_IDENTIFICATION_REQ);		
				}	
				break;

			case FSM_SEND_IDENTIFICATION_REQ:
				if(myNewState){
					ArLog::log(ArLog::Normal,"State FSM_IDENTIFICATION");
					myNewState = false;
					//Identify Card Owner	
					std::string req("employeeByCardId");
					std::string param(myCardRead);
							
					//Send http request to REST Server
					myHttpRequest.sendRequest(req, param);	
					break;
				}
				if(myHttpRequestFailed){
					myHttpRequestFailed = false;
					myOperatorsName = string("Guy");
					sprintf(myGreetingMessage,getRandomGreetingMessage(),myOperatorsName.c_str() );
					//mySoundsQueue->speak(myGreetingMessage);
					
					switchState(FSM_INFORM_FOR_SUPPLY);		
					break;
				}
				else if(myHttpNewResponse){
						myHttpNewResponse = false;

						//It seems to be a valid response from server
						try{	
							boost::property_tree::ptree pt = JSONParser::parse(myHttpResponse);		
							myOperatorsName = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("firstname").c_str()));						
							sprintf(myGreetingMessage,getRandomGreetingMessage(),myOperatorsName.c_str() );
							//mySoundsQueue->speak(myGreetingMessage );

							//lastName = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("lastname").c_str()));
							//cardId = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("cardID").c_str()));
							//emailAddress = string((char*)(pt.get_child("GetEmployeeByCardIdResult").get<std::string>("emailAddress").c_str()));	
							}
						catch(std::exception const&  ex){
							myHttpResponse = NULL;
							printf("JSON Error. %s", ex.what());
							ArLog::log(ArLog::Normal,"Unable to read JSON content");
							myOperatorsName = string("Guy");	
							sprintf(myGreetingMessage,getRandomGreetingMessage(),myOperatorsName.c_str() );			
						}
						switchState(FSM_INFORM_FOR_SUPPLY);	
							
						//myHttpResponse = NULL;
						
						//We have to wait until the end of speach
						
					}
				/*if(mySoundFinished || myStartedState.secSince() > 10){
						ArLog::log(ArLog::Normal,"Sound finished");
							mySoundFinished = false;
							switchState(FSM_INFORM_FOR_SUPPLY);
						}*/
				else
					break;
				break;
			
				case FSM_INFORM_FOR_SUPPLY:
					if(myNewState){
						ArLog::log(ArLog::Normal,"State FSM_INFORM_FOR_SUPPLY");
						myNewState = false;
						sprintf(mySupplyingMessage,getRandomSupplyingMessage(),myContent );
						ArLog::log(ArLog::Normal,"Greeting message : %s", myGreetingMessage);
						ArLog::log(ArLog::Normal,"Supplying message : %s", mySupplyingMessage);

						mySoundsQueue->speak(myGreetingMessage);
						ArUtil::sleep(100);
						mySoundsQueue->speak(mySupplyingMessage);
						ArUtil::sleep(100);
						//Cepstral : "In formation + Use your badge to teminate"
	
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
							
						}
						else{
							//Cepstral : "Are you still here "
							sprintf(myLostMessage,getRandomLostMessage());
							mySoundsQueue->speak(myLostMessage);
							switchState(FSM_WAITING_FOR_HUMAN_TO_END);
						}
						break;
					}
					
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
						myNewState = false;
						ArLog::log(ArLog::Normal,"State FSM_END");
						if(myCardReader.getRunning()){
							myCardReader.stopRunning();
						}
						myCardReader.close();
						mySoundsQueue->speak(getRandomByeMessage());
					}

					if((mySoundFinished && mySoundsQueueEmpty) || myStartedState.secSince() > 2){
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


void ArServerModeSupply::supplyTask(){
	
	printf("User task started with content : %s",myContent);
	myDone = false;
	switchState(FSM_START);
	
}
