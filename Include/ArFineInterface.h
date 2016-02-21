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

#ifndef ARFINEPOSITIONINTERFACE_H
#define ARFINEPOSITIONINTERFACE_H

#include "Aria.h"
#include "ArNetworking.h"

/// Interface for objects that provide information about a robot's docking status. 
/**
 *  ArFineInterface defines the methods that enable the AramScheduler to 
 *  determine the docking status of a robot.  For a local robot, these methods are
 *  implemented by ArServerModeDock.  With the central server and remote robots, 
 *  these methods are implemented by AramCentralDockProxy.  
**/
class ArFineInterface  
{
public:
	enum State{
		DOCKING,
		DOCKED,
		UNDOCKING,
		UNDOCKED
  };

	AREXPORT static const char *toString(State s) {
    switch (s) {
		case DOCKED:
			return "DOCKED";
		case DOCKING:
			return "DOCKING";
		case UNDOCKING:
			return "UNDOCKING";
		case UNDOCKED:
			return "UNDOCKED";
		} // end switch state

		return "unknown";

	} // end method toString


  /// Constructor
  AREXPORT ArFineInterface() {}
  /// Destructor
  AREXPORT virtual ~ArFineInterface() {}


  /// Gets the docking state we're in
  AREXPORT virtual State getState() const = 0;

}; // end class ArFineInterface


#endif // ARFINEPOSITIONINTERFACE_H
