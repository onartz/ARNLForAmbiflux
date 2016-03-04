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
#ifndef ARSERVEURMODEDELIVER_H
#define ARSERVEURMODEDELIVER_H

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
Mode in which the robot has to deliver things
*/

class ArServerModeDeliver : public ArServerMode
{
public:

	enum State {
		FSM_START,
		FSM_WAITING_FOR_HUMAN_TO_START,
		FSM_INFORM_FOR_DELIVERY,
		FSM_WAITING_FOR_HUMAN_TO_END,
		FSM_OK,
		FSM_FAILED,
		FSM_OTHER
  };
  AREXPORT ArServerModeDeliver(ArServerBase *server, ArRobot *robot, 
			    bool defunct = false);
  AREXPORT virtual ~ArServerModeDeliver();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT void deliver(const char*);
  AREXPORT void netDeliver(ArServerClient *client, ArNetPacket *packet);
  //AREXPORT void handleDeliverDone(char *);
  //AREXPORT void handleDeliverFailed(char *);
  AREXPORT virtual void userTask(void);
  //AREXPORT virtual void checkDefault(void) { activate(); }
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return &myStopGroup; }
  /// Adds to the config
  //AREXPORT void addToConfig(ArConfig *config, const char *section = "Teleop settings");
  ///// Sets whether we're using the range devices that depend on location
  //AREXPORT void setUseLocationDependentDevices(
	 // bool useLocationDependentDevices, bool internal = false);
  ///// Gets whether we're using the range devices that depend on location
  //AREXPORT bool getUseLocationDependentDevices(void);
  ////funtion triggered when new card read
  ////void readCardCB(int *);
   

protected:
	/*ArActionDeceleratingLimiter *myLimiterForward;
	ArActionDeceleratingLimiter *myLimiterBackward;
	ArActionDeceleratingLimiter *myLimiterLateralLeft;
	ArActionDeceleratingLimiter *myLimiterLateralRight;*/
	ArActionGroupStop myStopGroup;
	//bool myUseLocationDependentDevices;
	ArFunctor2C<ArServerModeDeliver, ArServerClient *, ArNetPacket *> myNetDeliverCB;
  /// Change internal state of FSM
	void switchState(State state);
	/// Called a new card has been read by cardREader
	void handleCardRead(char * cardID);
	//Called when a valid http response comes from REST server
	void handleHttpResponse(char * response);
	void handleHttpFailed(void);
	void handleSoundsQueueIsEmpty(void);
	void handleSoundsQueueIsNotEmpty(void);
	//void handleEndSpeaking(void);
	/*const char * getRandomGreetingMessage();
	const char * getRandomDeliveryMessage();
	const char * getRandomLostMessage();
  */
	void stateChanged(void);
	void deliverTask();
	/// Checked if the current operation is ended (done or failed)
	bool myDone;
  //Functors passed to a class
	//ArFunctor1C<ArServerModeDeliver, char*> myDeliverDoneCB;
	//ArFunctor1C<ArServerModeDeliver, char*> myDeliverFailedCB;
	char myGreetingMessage[256];
	char myDeliveryMessage[256];
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
	//bool myEndSpeaking;
	bool mySoundFinished;
	// The card reader
	LecteurCarteTask myCardReader;

	State myState;
	State myLastState;
	bool myNewState;
	ArTime myStartedState;

	// Some Functors for events from card reader, soundQueue and REST server
	ArFunctor1C<ArServerModeDeliver, char *> myCardReadCB;
	ArFunctor1C<ArServerModeDeliver, char *> myHttpResponseCB;
	ArFunctorC<ArServerModeDeliver> myHttpFailedCB;
	ArFunctorC<ArServerModeDeliver> mySoundFinishedCB;
	ArFunctorC<ArServerModeDeliver> mySoundsQueueEmptyCB;
	ArFunctorC<ArServerModeDeliver> mySoundsQueueNonEmptyCB;

	bool mySoundsQueueEmpty;
	void handleSoundFinished();

	DALRest myHttpRequest;
	//ArSoundsQueue soundQueue;
	//Number of Attempt  to initiate communication with human
	int attemptFailed;
	char errorMessage[64];
	//ASyncSpeak myASyncSpeak;
  
	char myContent[256];
	ArSpeechSynth *speechSynthesizer;
	ArSoundsQueue *mySoundsQueue;

};

#endif // ARSERVEURMODEDELIVER_H
