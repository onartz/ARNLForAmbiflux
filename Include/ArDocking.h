/*
Adept MobileRobots Advanced Robotics Navigation and Localization (ARNL)
Version 1.7.3

Copyright (C) 2004, 2005 ActivMedia Robotics LLC
Copyright (C) 2006, 2007, 2008, 2009 MobileRobots Inc.
Copyright (C) 2010, 2011 Adept Technology, Inc.

All Rights Reserved.

Adept MobileRobots does not make any representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.

The license for this software is distributed as LICENSE.txt in the top
level directory.

robots@mobilerobots.com
Adept MobileRobots
10 Columbia Drive
Amherst, NH 03031
800-639-9481

*/
#ifndef ARDOCKING_H
#define ARDOCKING_H

#include "ArDockInterface.h"

#include "Aria.h"
#include "ArServerBase.h"
#include "ArServerMode.h"
#include "ArPathPlanningTask.h"
#include "ArLocalizationTask.h"

/// Base class to manage docking
/**
   This class sends the robot to a recharging "dock" station when
   a user commands it from a client, or when an automatic condition
   occurs.
   You need to have a dock object in your map.
   Some conditions for automatically docking, and for staying docked
   or allowing the robot to leave the dock, can be set via ArConfig (or directly).
 **/
class ArServerModeDock : public ArServerMode, public ArDockInterface
{
public:
  /// Constructor
  AREXPORT ArServerModeDock(
	  ArServerBase *serverBase, ArRobot *robot, 
	  ArLocalizationTask *locTask, ArPathPlanningTask *pathTask, 
	  bool useChargeState = false, ArFunctor *shutdownFunctor = NULL);
  /// Destructor
  AREXPORT virtual ~ArServerModeDock(void);

 /// This will create a dock of the appropriate type for the robot
  AREXPORT static ArServerModeDock *createDock(
	  ArServerBase *serverBase, ArRobot *robot,
	  ArLocalizationTask *locTask, ArPathPlanningTask *pathTask,
	  ArFunctor *shutdownFunctor = NULL);
  /// Gets the docking state we're in
  AREXPORT virtual State getState(void) const { return myState; }
  /// Sends the robot to the dock (subclass needs to implement)
  AREXPORT virtual void dock(void) = 0;
  /// Has the robot leave the dock (subclass needs to implement)
  AREXPORT virtual void undock(void) = 0;
  /// Sees if the robot is already docked and activates itself if it is (subclass should implement)
  virtual void checkDock(void) { }
  /// Forces the robot to think that it is already docked and to activate it 
  AREXPORT virtual void activateAsDocked(void);
  /// For default modes, sees if it should activate itself
  AREXPORT virtual void checkDefault(void) { checkDock(); }
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT virtual void requestUnlock(void);
  /// Gets whether our docking is forced or not
  AREXPORT virtual bool getForcedDock(void) { return myForcedDock; }
  /// Requests a forced dock
  AREXPORT void requestForcedDock(void);
  AREXPORT virtual void forceUnlock(void);
  /// Gets the name of the dock we're using
  AREXPORT const char *getDockName(void) { return myDockName.c_str(); }
  /// Set the voltage we go dock at
  AREXPORT void setDockingVoltage(double dockingVoltage);
  /// Get the voltage we go dock at
  AREXPORT double getDockingVoltage(void) const;
  /// Set the voltage we're done docking at (does first of voltage or time)
  AREXPORT void setDoneChargingVoltage(double doneChargingVoltage);
  /// Get the voltage we're done docking at (does first of voltage or time)
  AREXPORT double getDoneChargingVoltage(void) const;
  /// Sets the number of minutes we dock for (does first of voltage or time)
  AREXPORT void setDoneChargingMinutes(int doneChargingMinutes);
  /// Gets the number of minutes we dock for (does first of voltage or time)
  AREXPORT int getDoneChargingMinutes(void);
  /// Gets whether we're using the charge state or not
  AREXPORT bool getUseChargeState();
  /// Sets whether we're done docking at float or not (this ONLY has meaning if getUseChargeState is true)
  AREXPORT void setDoneChargingAtFloat(bool doneChargingAtFloat);
  /// Gets whether we're done docking at float or not (this ONLY has meaning if getUseChargeState is true)
  AREXPORT bool getDoneChargingAtFloat(void);
  /// Sets the number of minutes we won't auto redock for this many minutes after the last dock
  AREXPORT void setMinimumMinutesBetweenAutoDock(int minutesBetween);
/// Sets the number of minutes we won't auto redock for this many minutes after the last dock
  AREXPORT int getMinimumMinutesBetweenAutoDock(void);
  /// Sets whether we auto dock or not 
  AREXPORT void setAutoDock(bool autoDocking);
  /// Enables the auto docking
  AREXPORT bool getAutoDock(void);
  /// Adds the docking information to the given config
  AREXPORT virtual void addToConfig(ArConfig *config);
  /// Adds simple commands to 
  AREXPORT virtual void addControlCommands(ArServerHandlerCommands *handlerCommands);
  /// Takes the server command to undock
  AREXPORT void serverDock(ArServerClient *client, ArNetPacket *packet);
  /// Takes the server command to undock
  AREXPORT void serverUndock(ArServerClient *client, ArNetPacket *packet);
  /// Returns out the state of the docking
  AREXPORT void serverDockInfo(ArServerClient *client, ArNetPacket *packet);
  /// Returns whether we're auto docking or not
  AREXPORT void serverGetAutoDock(ArServerClient *client, ArNetPacket *packet);
  /// Sets whether we're auto docking or not 
  AREXPORT void serverSetAutoDock(ArServerClient *client, ArNetPacket *packet);
  /// Adds a functor to get called when the state changes
  AREXPORT void addStateChangedCB(ArFunctor *functor, 
				 ArListPos::Pos position = ArListPos::LAST);
  /// Removes a functor to get called when the state changes
  AREXPORT void remStateChangedCB(ArFunctor *functor);
  /// Gets the number of times goto has failed
  AREXPORT unsigned int getFailedGotoNum(void) { return myFailedGotoNum; }
  /// Adds a callback for when forced docking (mostly voltage) occurs
  AREXPORT void addForcedDockCB(ArFunctor *functor, int position = 50)
    { myForcedDockCBList.addCallback(functor, position); }
  /// Removes a callback for when forced docking (mostly voltage) occurs
  AREXPORT void remForcedDockCB(ArFunctor *functor)
    { myForcedDockCBList.remCallback(functor); }
  /// Adds a callback for when idle docking occurs
  AREXPORT void addIdleDockCB(ArFunctor *functor, int position = 50)
    { myIdleDockCBList.addCallback(functor, position); }
  /// Removes a callback for when idle docking occurs
  AREXPORT void remIdleDockCB(ArFunctor *functor)
    { myIdleDockCBList.remCallback(functor); }
  /// Adds a callback for when normal requested docking occurs
  AREXPORT void addRequestedDockCB(ArFunctor *functor, int position = 50)
    { myRequestedDockCBList.addCallback(functor, position); }
  /// Removes a callback for when normal requested docking occurs 
  AREXPORT void remRequestedDockCB(ArFunctor *functor)
    { myRequestedDockCBList.remCallback(functor); }
  /// Adds a callback for when the robot is driving to the docking goal
  AREXPORT void addDrivingToDockCB(ArFunctor *functor, int position = 50)
    { myDrivingToDockCBList.addCallback(functor, position); }
  /// Removes a callback for when the robot is driving to the docking goal
  AREXPORT void remDrivingToDockCB(ArFunctor *functor)
    { myDrivingToDockCBList.remCallback(functor); }
  /// Adds a callback for when the robot is driving into the actual dock 
  AREXPORT void addDrivingIntoDockCB(ArFunctor *functor, int position = 50)
    { myDrivingIntoDockCBList.addCallback(functor, position); }
  /// Removes a callback for when the robot is driving into the actual dock
  AREXPORT void remDrivingIntoDockCB(ArFunctor *functor)
    { myDrivingIntoDockCBList.remCallback(functor); }
  /// Adds a callback for when the robot is finally docked
  AREXPORT void addDockedCB(ArFunctor *functor, int position = 50)
    { myDockedCBList.addCallback(functor, position); }
  /// Removes a callback for when the robot is finally docked
  AREXPORT void remDockedCB(ArFunctor *functor)
    { myDockedCBList.remCallback(functor); }
  AREXPORT void addSingleShotDockedCB(ArFunctor *functor, int position = 50)
    { mySingleShotDockedCBList.addCallback(functor, position); }
  AREXPORT void remSingleShotDockedCB(ArFunctor *functor)
    { mySingleShotDockedCBList.remCallback(functor); }
  /// Adds a callback for when the robot is not forced docked anymore
  AREXPORT void addDockNowUnforcedCB(ArFunctor *functor, int position = 50)
    { myDockNowUnforcedCBList.addCallback(functor, position); }
  /// Removes a callback for when the robot is not forced anymore
  AREXPORT void remDockNowUnforcedCB(ArFunctor *functor)
    { myDockNowUnforcedCBList.remCallback(functor); }
  /// Adds a callback for when the robot is forced again
  AREXPORT void addDockNowForcedCB(ArFunctor *functor, int position = 50)
    { myDockNowForcedCBList.addCallback(functor, position); }
  /// Removes a callback for when the robot is forced again
  AREXPORT void remDockNowForcedCB(ArFunctor *functor)
    { myDockNowForcedCBList.remCallback(functor); }
  /// Adds a callback for when the robot is undocking
  AREXPORT void addUndockingCB(ArFunctor *functor, int position = 50)
    { myUndockingCBList.addCallback(functor, position); }
  /// Removes a callback for when the robot is undocking
  AREXPORT void remUndockingCB(ArFunctor *functor)
    { myUndockingCBList.remCallback(functor); }
  /// Adds a callback for when the robot is undocked
  AREXPORT void addUndockedCB(ArFunctor *functor, int position = 50)
    { myUndockedCBList.addCallback(functor, position); }
  /// Removes a callback for when the robot is undocked
  AREXPORT void remUndockedCB(ArFunctor *functor)
    { myUndockedCBList.remCallback(functor); }
protected:
  AREXPORT bool processFile(void);
  void switchState(State state);
  AREXPORT void dockUserTask(void);
  void stateChanged(void);
  void broadcastDockInfoChanged(void);
  void makeDockInfoPacket(ArNetPacket *packet);
    
  // this should be called when the goto fails
  void gotoFailed(void) { myFailedGotoNum++; }
  unsigned int myFailedGotoNum;
  /// this is a helper function that will find the dock from the preferred dock and whats in the map 
  AREXPORT ArMapObject *findDock(ArMapInterface *arMap);
  /// This is for clearing the interrupted mode
  AREXPORT void clearInterrupted(void);
  /// This is for resuming the interrupted mode if we should
  AREXPORT void resumeInterrupted(bool assureDeactivation);

  ArLocalizationTask *myLocTask;
  ArPathPlanningTask *myPathTask;
  bool myUseChargeState;
  ArFunctor *myShutdownFunctor;

  // used by the base class for the interrupted handling
  ArServerMode *myModeInterrupted;
  State myState;
  bool myDrivingToDock;
  ArTime myStartedState;
  std::string myDockName;
  std::string myDockType;

  double myDockingVoltage;
  double myDockingStateOfCharge;
  int myDockingIdleTime;
  double myDoneChargingVoltage;
  double myDoneChargingStateOfCharge;
  int myDoneChargingMinutes;
  bool myDoneChargingAtFloat;
  int myMinimumMinutesBetweenAutoDock;

  bool myWasCharging;

  char myPreferredDock[512];

  // these callbacks are for the different docking states... and are
  // called automatically
  ArCallbackList myForcedDockCBList;
  ArCallbackList myIdleDockCBList;
  ArCallbackList myRequestedDockCBList;
  ArCallbackList myDockedCBList;
  ArCallbackList myUndockingCBList;
  ArCallbackList myUndockedCBList;
  ArCallbackList myDockNowUnforcedCBList;
  ArCallbackList myDockNowForcedCBList;
  ArCallbackList mySingleShotDockedCBList;

  // these are for the different parts of docking and have to be
  // called by the subclass
  ArCallbackList myDrivingToDockCBList;
  ArCallbackList myDrivingIntoDockCBList;

  std::list<ArFunctor *> myStateChangedCBList;
  bool myForcedDock;
  bool myForcedDockRequested;
  bool myIdleDock;
  bool myAutoDock;
  bool myLastAutoDock;
  bool myUsingAutoDock;
  ArTime myLastDocked;
  bool myHaveDocked;
  ArServerHandlerCommands *myHandlerCommands;

  bool myLastForcedDock;
  State myLastState;

  // information for shutting down if the robot can't dock 
  int myShutdownMinutesIdle;
  int myShutdownMinutesForced;
  int myShuttingDownSeconds;
  int myLastShuttingDownSeconds;
  ArTime myShutdownStartedForced;
  ArTime myShutdownStartedIdle;
  ArTime myShutdownLastMove;
  ArPose myShutdownLastPose;

  ArFunctorC<ArServerModeDock> myDockUserTaskCB; 
  ArFunctor2C<ArServerModeDock, ArServerClient *, ArNetPacket *> myServerDockCB;
  ArFunctor2C<ArServerModeDock, ArServerClient *, ArNetPacket *> myServerUndockCB;
  ArFunctor2C<ArServerModeDock, ArServerClient *, ArNetPacket *> myServerDockInfoCB;
  ArFunctor2C<ArServerModeDock, ArServerClient *, ArNetPacket *> myServerGetAutoDockCB;
  ArFunctor2C<ArServerModeDock, ArServerClient *, ArNetPacket *> myServerSetAutoDockCB;
  ArFunctor1C<ArServerModeDock, bool> myServerAutoDockingDisableCB;
  ArFunctor1C<ArServerModeDock, bool> myServerAutoDockingEnableCB;
  ArRetFunctorC<bool, ArServerModeDock> myProcessFileCB;

};

/// Class for docking a Pioneer DX on a Pioneer dock
class ArServerModeDockPioneer : public ArServerModeDock
{
public:
  AREXPORT ArServerModeDockPioneer(
	  ArServerBase *serverBase, ArRobot *robot, 
	  ArLocalizationTask *locTask, ArPathPlanningTask *pathTask,
	  ArFunctor *shutdownFunctor = NULL);
  AREXPORT virtual ~ArServerModeDockPioneer();
  AREXPORT virtual void dock(void);
  AREXPORT virtual void undock(void);
  AREXPORT virtual void checkDock(void);
  AREXPORT virtual void forceUnlock(void);
  AREXPORT virtual void deactivate(void);
protected:
  AREXPORT virtual void userTask(void);
  AREXPORT void goalDone(ArPose pose);
  AREXPORT void goalFailed(ArPose pose);
  bool myGoalDone;
  ArActionGroup myGroup;
  ArActionTriangleDriveTo myDriveTo;
  bool mySentDockCommand;
  int myFindingDockTry;
  bool myNeedToResendDockCommand;
  bool myNeedToPathPlanToDock;
  bool myUndockingMoveSent;
  ArTime mySentDockCommandTime;
  ArTime myStartedDriveTo;
  ArFunctor1C<ArServerModeDockPioneer, ArPose> myGoalDoneCB;
  ArFunctor1C<ArServerModeDockPioneer, ArPose> myGoalFailedCB;
}; 

/// Base class for a dock in which the robot drives into the dock until a bumper hits the dock.
/**

   This class is set up so that it can be used for both a patrolbot
   and a powerbot, it'll also work as a base for the simulator (so
   that it has a docking mode too).

   This class takes care of all of the driving, sub classes only need
   to implement 3 functions.  The first is 'isDocked' which'll return
   true if power is being received.  The second is 'enableDocking'
   which is called when the dock is bumped and should engage docking
   if needed.  The third is 'disableDocking' which'll be
   called before the robot backs out.

   So basically this class'll drive to the dock goal, then it'll find
   the triangular docking target and drive to that until it bumps and
   then call 'enableDocking' and then wait 10 seconds while
   checking 'isDocked' to see if the robot is docked.  If it is docked
   it'll switch to the dock state.  Note that if the robot stalls
   'isDocked' will be checked on its own, if it is docked then it'll
   just go to docked too, this is for the simulator and you shouldn't
   really use this for anything else.

   When trying to undock this'll call 'disableDocking' and
   then wait 5 seconds and then back out from the dock (using the
   sonar to not run into things).
 **/
class ArServerModeDockTriangleBump : public ArServerModeDock
{
public:
  AREXPORT ArServerModeDockTriangleBump(ArServerBase *serverBase, 
					ArRobot *robot, 
					ArLocalizationTask *locTask, 
					ArPathPlanningTask *pathTask,
					bool useChargeState = false,
					double approachDist = 1000,
					double backOutDist = 0,
					ArFunctor *shutdownFunctor = NULL);
  AREXPORT virtual ~ArServerModeDockTriangleBump();
  AREXPORT virtual void dock(void);
  AREXPORT virtual void undock(void);
  AREXPORT virtual void checkDock(void);
  AREXPORT virtual void forceUnlock(void);
  AREXPORT virtual void deactivate(void);
  /// This should return true if the robot has power from the dock and false otherwise
  AREXPORT virtual bool isDocked(void) = 0;
  /// This should enable docking (turning on the charger or what not) 
  /**
     This should enable docking (turn on charger or what not), it'll
     be called after the robot bumps into the dock.  After this the
     code'll wait a while for isDocked to become true.
   **/
  AREXPORT virtual void enableDock(void) = 0;
  /// This should disable docking (turn off charger or what not)
  /**
     This should disable docking (turn off charger or what not), it'll
     be called, this'll wait 5 seconds, and then the robot'll back away 
     from the dock.
   **/
  AREXPORT virtual void disableDock(void) = 0;
  /// This is a function that will be called before the robot drives in
  /**
     This should do anything that needs to be done before the robot
     can drive into the dock.
   **/
  AREXPORT virtual void beforeDriveInCallback(void) {}
  /// This is a function that will be called after the robot drives out
  /**
     This should do anything that needs to be done after the robot
     can drives out of the dock
   **/
  AREXPORT virtual void afterDriveOutCallback(void) {}
  /// This function is called if the robot has backed out but is still docked
  AREXPORT virtual void backoutCallback(void) {}
  /// set whether to treat stalls as bumps or not (mostly for the simulator)
  AREXPORT void setStallsAsBumps(bool stallsAsBumps) 
    { myStallsAsBumps = stallsAsBumps; }
  /// get whether to treat stalls as bumps or not (mostly for the simulator)
  AREXPORT bool getStallsAsBumps(void) { return myStallsAsBumps; }
protected:
  AREXPORT virtual void userTask(void);
  AREXPORT void goalDone(ArPose pose);
  AREXPORT void goalFailed(ArPose pose);
  bool myGoalDone;
  ArActionGroup myGroup;
  ArActionTriangleDriveTo myDriveTo;
  ArActionGroup myBackGroup;
  //ArActionGotoStraight myGoto;
  ArActionDriveDistance myGoto;

  bool myIsSim;

  bool myRetryDock;
  bool myNeedToPathPlanToDock;
  bool myStartedBacking;
  bool myStallsAsBumps;
  bool myHitDock;
  int myFindingDockTry;
  bool myDisableDockCalled;
  ArTime myHitDockTime;
  ArTime myStartedDriveTo;
  ArTime myLastPowerGood;
  ArPose myDriveFrom;
  bool myDriveFromValid;
  double myDistanceToBack;
  double myDesiredBackOutDist;
  ArPose myStartedUndocking;
  ArFunctor1C<ArServerModeDockTriangleBump, ArPose> myGoalDoneCB;
  ArFunctor1C<ArServerModeDockTriangleBump, ArPose> myGoalFailedCB;
}; 

/// Docking class for the Patrolbot dock
class ArServerModeDockPatrolBot : public ArServerModeDockTriangleBump
{
public:
  AREXPORT ArServerModeDockPatrolBot(ArServerBase *serverBase, ArRobot *robot, 
				     ArLocalizationTask *locTask, 
				     ArPathPlanningTask *pathTask,
				     ArFunctor *shutdownFunctor = NULL);
  AREXPORT virtual ~ArServerModeDockPatrolBot();

  AREXPORT virtual bool isDocked(void);
  AREXPORT virtual void enableDock(void);
  AREXPORT virtual void disableDock(void);
  AREXPORT virtual void checkDock(void);
  AREXPORT virtual void beforeDriveInCallback(void);
  AREXPORT virtual void afterDriveOutCallback(void);
};

/// Docking class for the simulator (various robots)
class ArServerModeDockSimulator : public ArServerModeDockTriangleBump
{
public:
  AREXPORT ArServerModeDockSimulator(ArServerBase *serverBase, ArRobot *robot, 
				     ArLocalizationTask *locTask, 
				     ArPathPlanningTask *pathTask,
				     ArFunctor *shutdownFunctor = NULL);
  AREXPORT virtual ~ArServerModeDockSimulator();

  AREXPORT virtual bool isDocked(void);
  AREXPORT virtual void enableDock(void);
  AREXPORT virtual void disableDock(void);
  AREXPORT virtual void checkDock(void);
};

/// Docking class for the PowerBot dock (rare)
class ArServerModeDockPowerBot : public ArServerModeDockTriangleBump
{
public:
  AREXPORT ArServerModeDockPowerBot(ArServerBase *serverBase, ArRobot *robot, 
				    ArLocalizationTask *locTask, 
				    ArPathPlanningTask *pathTask, 
				    bool isOldDock,
				    ArFunctor *shutdownFunctor = NULL,
				    bool useChargeState = false, 
				    int oldDockAnalogPort = 0);
  AREXPORT virtual ~ArServerModeDockPowerBot();

  AREXPORT virtual bool isDocked(void);
  AREXPORT virtual void enableDock(void);
  AREXPORT virtual void disableDock(void);
  AREXPORT virtual void backoutCallback(void);
  /// Adds the docking information to the given config
  AREXPORT virtual void addToConfig(ArConfig *config);
protected:
  bool myIsOldDock;
  int myOldDockAnalogPort;
  double myBatteryChargingGood;
};

#endif // ARDOCKING
