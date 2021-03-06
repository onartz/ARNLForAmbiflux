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
#ifndef ARSERVERMODESUPPLY_H
#define ARSERVERMODESUPPLY_H


#include "Aria.h"
#include "ArServerMode.h"
#include "LecteurCarteTask.h"
#include "DALRest.h"
#include "JSONParser.h"
#include "ArSpeech.h"
#include "ArCepstral.h"
#include "ArSoundsQueue.h"
#include "Globals.h"

/*
ArServerMode in which the robot has to be supplied by an human operator.
*/

class ArServerModeSupply : public ArServerMode
{
public:
	enum State {
		FSM_START,
		FSM_WAITING_FOR_HUMAN_TO_START,
		FSM_SEND_IDENTIFICATION_REQ,
		FSM_INFORM_FOR_SUPPLY,
		FSM_WAITING_FOR_HUMAN_TO_END,
		FSM_OK,
		FSM_FAILED,
		FSM_OTHER
  };
  AREXPORT ArServerModeSupply(ArServerBase *server, ArRobot *robot, 
			    bool defunct = false);
  AREXPORT virtual ~ArServerModeSupply();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT void supply(const char*);
  AREXPORT void netSupply(ArServerClient *client, ArNetPacket *packet);
  //AREXPORT void handleSupplyDone(char *);
  //AREXPORT void handleSupplyFailed(char *);
  AREXPORT virtual void userTask(void);
  //AREXPORT virtual void checkDefault(void) { activate(); }
  
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return &myStopGroup; }
  
   

protected:
	
	ArActionGroupStop myStopGroup;
	ArFunctor2C<ArServerModeSupply, ArServerClient *, ArNetPacket *> myNetSupplyCB;
	/// Change internal state of FSM
	void switchState(State state);
	/// Called a new card has been read by cardREader
	void handleCardRead(char * cardID);
	/// Called when a valid http response comes from REST server
	void handleHttpResponse(char * response);
	/// Called when failed to contact REST Server
	void handleHttpFailed(void);
	void handleSoundsQueueIsEmpty(void);
	void handleSoundsQueueIsNotEmpty(void);
	void handleSoundFinished();
	
	void supplyTask();
 
	void stateChanged(void);

	/// Checked if the current operation is ended (done or failed)
	bool myDone;
	// used by the base class for the interrupted handling
	ArServerMode *myModeInterrupted;
	
	/// The content to be spupply to the robot
	char myContent[256];

	char myGreetingMessage[256];
	char mySupplyingMessage[256];
	char myLostMessage[256];

	//A new card has been read
	bool myNewCardRead;
	//Card ID
	char myCardRead[12];
	/// Set when a new Http response comes from REST server. Reset when read.
	bool myHttpNewResponse;
	/// Content of the response
	char * myHttpResponse;
	/// Set when Http request failed. Reset when read.
	bool myHttpRequestFailed;
	/// Set when speaking is finished
	bool mySoundFinished;
	/// The card reader
	LecteurCarteTask myCardReader;

	/// FSM Variables for States
	State myState;
	State myLastState;
	bool myNewState;
	ArTime myStartedState;

	// Variables to trigger the FSM
	bool mySoundsQueueEmpty;

	// Some Functors for events from card reader, soundQueue and REST server
	ArFunctor1C<ArServerModeSupply, char *> myCardReadCB;
	ArFunctor1C<ArServerModeSupply, char *> myHttpResponseCB;
	ArFunctorC<ArServerModeSupply> myHttpFailedCB;
	ArFunctorC<ArServerModeSupply> mySoundFinishedCB;
	ArFunctorC<ArServerModeSupply> mySoundsQueueEmptyCB;
	ArFunctorC<ArServerModeSupply> mySoundsQueueNonEmptyCB;

	/// An object to send requests to REST Server	
	DALRest myHttpRequest;


	std::string myOperatorsName;
	//Number of Attempt  to initiate communication with human
	int attemptFailed;
	
	char errorMessage[64];

	ArSpeechSynth *speechSynthesizer;
	ArSoundsQueue *mySoundsQueue;
	
 
};

#endif // ARSERVEURMODESUPPLY_H
