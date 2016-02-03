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
#ifndef ARSERVEURMODESUPPLY_H
#define ARSERVEURMODESUPPLY_H

#include "Aria.h"
#include "ArServerMode.h"

/*
Mode in which the robot has to be supplied by an operator.
*/

class ArServerModeSupply : public ArServerMode
{
public:
  AREXPORT ArServerModeSupply(ArServerBase *server, ArRobot *robot, 
			    bool defunct = false);
  AREXPORT virtual ~ArServerModeSupply();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT void supply(void);
  AREXPORT void netSupply(ArServerClient *client, ArNetPacket *packet);
  AREXPORT virtual void userTask(void);
  AREXPORT virtual void checkDefault(void) { activate(); }
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return &myStopGroup; }
  /// Adds to the config
  AREXPORT void addToConfig(ArConfig *config, const char *section = "Teleop settings");
  /// Sets whether we're using the range devices that depend on location
  AREXPORT void setUseLocationDependentDevices(
	  bool useLocationDependentDevices, bool internal = false);
  /// Gets whether we're using the range devices that depend on location
  AREXPORT bool getUseLocationDependentDevices(void);
protected:
  ArActionDeceleratingLimiter *myLimiterForward;
  ArActionDeceleratingLimiter *myLimiterBackward;
  ArActionDeceleratingLimiter *myLimiterLateralLeft;
  ArActionDeceleratingLimiter *myLimiterLateralRight;
  ArActionGroupStop myStopGroup;
  bool myUseLocationDependentDevices;
  ArFunctor2C<ArServerModeSupply, ArServerClient *, ArNetPacket *> myNetSupplyCB;
};

#endif // ARSERVEURMODESUPPLY_H