#ifndef ARSERVERMYMODE_H
#define ARSERVERMYMODE_H
#include "ArServerMode.h"
#include "Aria.h"
#include "ArServerBase.h"
#include "ArServerMode.h"
#include "ArPathPlanningTask.h"
#include "ArLocalizationTask.h"
#include "ArFineInterface.h"

class ArServerMyMode : public ArServerMode{
public:
	enum State{
		DOCKING,
		DOCKED,
		UNDOCKING,
		UNDOCKED
	};
	AREXPORT const char *toString(State s);
	AREXPORT ArServerMyMode(ArServerBase *server, ArRobot *robot, 
		ArLocalizationTask *locTask,
		ArPathPlanningTask *pathTask,
		ArMapInterface *arMap,
		double approachDist,
		double backOutDist);
	AREXPORT virtual ~ArServerMyMode();
	AREXPORT void serverGotoTriangle(ArServerClient * /*client*/, 
					       ArNetPacket *packet);
	AREXPORT void serverUndockFrom(ArServerClient * /*client*/, 
					       ArNetPacket *packet);
	AREXPORT virtual void activate(void);
	AREXPORT void activateAsDocked(void);
	AREXPORT virtual void deactivate(void);
	AREXPORT virtual void userTask(void);
	AREXPORT void serverDock(ArServerClient*, ArNetPacket *);
	AREXPORT void dock(void);
	AREXPORT void undockFrom(void);
	AREXPORT void gotoTriangle(const char*);
	//AREXPORT
	AREXPORT void setStallsAsBumps(bool stallsAsBumps) 
    { myStallsAsBumps = stallsAsBumps; }
  /// get whether to treat stalls as bumps or not (mostly for the simulator)
	AREXPORT bool getStallsAsBumps(void) { return myStallsAsBumps; }

protected:
	AREXPORT void goalDone(ArPose pose);
	AREXPORT void goalFailed(ArPose pose);
	void switchState(State);
	void stateChanged(void);
	bool myNeedToPathPlan;
	ArActionDriveDistance myGoto;
	ArActionGroup myGroup;
	ArActionGroup myBackGroup;
	ArTime myStartedDriveTo;
	ArTime myStartedState;
	double myApproachDist;
	double myBackOutDist;
	double myDistanceToBack;
	double myDesiredBackOutDist;
	ArPose myStartedUndocking;
	bool myStallsAsBumps;
	bool myHitDock;
	bool myDriveFromValid;
	bool myStartedBacking;

	ArPose myGoal;
	ArPose myDriveFrom;
	std::string myGoalName;
	State myState;
	State myLastState;
	bool myGoalDone;
	ArPathPlanningTask * myPathTask;
	ArLocalizationTask * myLocTask;
	ArMapInterface * myMap;
	ArActionTriangleDriveTo myDriveTo;
	ArFunctor1C<ArServerMyMode, ArPose> myGoalDoneCB;
	ArFunctor1C<ArServerMyMode, ArPose> myGoalFailedCB;

	ArFunctor2C<ArServerMyMode, ArServerClient*, ArNetPacket*> myServerGotoTriangleCB;
	ArFunctor2C<ArServerMyMode, ArServerClient*, ArNetPacket*> myServerUndockFromCB;

};

#endif