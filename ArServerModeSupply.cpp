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

AREXPORT ArServerModeSupply::ArServerModeSupply(ArServerBase *server, 
					    ArRobot *robot,
					    bool defunct) : 
  ArServerMode(robot, server, "supply"),
  myStopGroup(robot),
  myNetSupplyCB(this, &ArServerModeSupply::netSupply)
{
  myMode = "Supply";
  if (myServer != NULL)
  {
    addModeData("supply", "supply the robot", &myNetSupplyCB,
		"none", "none", "Supply", "RETURN_NONE");
  }
}

AREXPORT ArServerModeSupply::~ArServerModeSupply()
{
}

AREXPORT void ArServerModeSupply::activate(void)
{
  if (isActive() || !baseActivate())
    return;
  setActivityTimeToNow();
  myStatus = "StartingSupply";
}

AREXPORT void ArServerModeSupply::deactivate(void)
{
  myStopGroup.deactivate();
  baseDeactivate();
}

AREXPORT void ArServerModeSupply::supply(void)
{
  activate();
}

AREXPORT void ArServerModeSupply::netSupply(ArServerClient *client, 
				     ArNetPacket *packet)
{
  setActivityTimeToNow();
  myRobot->lock();
  ArLog::log(ArLog::Verbose, "StoppingSupply");
  supply();
  myRobot->unlock();
}

AREXPORT void ArServerModeSupply::userTask(void)
{
 /* if (myRobot->getVel() < 2 && myRobot->getRotVel() < 2)
  {
    myStatus = "Stopped";
  }
  else
  {
    setActivityTimeToNow();
    myStatus = "Stopping";
  }*/
}

AREXPORT void ArServerModeSupply::addToConfig(ArConfig *config, 
					      const char *section)
{
 
}

AREXPORT void ArServerModeSupply::setUseLocationDependentDevices(
	bool useLocationDependentDevices, bool internal)
{
  if (!internal)
    myRobot->lock();
  // if this is a change then print it
  if (useLocationDependentDevices != myUseLocationDependentDevices)
  {
    myUseLocationDependentDevices = useLocationDependentDevices;
    myLimiterForward->setUseLocationDependentDevices(
	    myUseLocationDependentDevices);
    myLimiterBackward->setUseLocationDependentDevices(
	    myUseLocationDependentDevices);
    if (myLimiterLateralLeft != NULL)
      myLimiterLateralLeft->setUseLocationDependentDevices(
	      myUseLocationDependentDevices);
    if (myLimiterLateralRight != NULL)
      myLimiterLateralRight->setUseLocationDependentDevices(
	      myUseLocationDependentDevices);
  }
  if (!internal)
    myRobot->unlock();
}

AREXPORT bool ArServerModeSupply::getUseLocationDependentDevices(void)
{
  return myUseLocationDependentDevices;
}
