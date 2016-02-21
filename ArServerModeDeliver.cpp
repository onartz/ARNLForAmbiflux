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

AREXPORT ArServerModeDeliver::ArServerModeDeliver(ArServerBase *server, 
					    ArRobot *robot,
					    bool defunct) : 
  ArServerMode(robot, server, "deliver"),
  myStopGroup(robot),
  myNetDeliverCB(this, &ArServerModeDeliver::netDeliver),
  myDeliverDoneCB(this,&ArServerModeDeliver::handleDeliverDone),
  myDeliverFailedCB(this,&ArServerModeDeliver::handleDeliverFailed)
 
{
  myMode = "Deliver";
  
  if (myServer != NULL)
  {
    addModeData("deliver", "deliver the robot", &myNetDeliverCB,
		"string: content", "none", "Deliver", "RETURN_NONE");
	myASyncDeliverTask.setDeliverDoneCB(&myDeliverDoneCB);
	myASyncDeliverTask.setDeliverFailedCB(&myDeliverFailedCB);
	
	//myServer->addData("deliverInfos","Deliver informations",
	//myServer->addData("deliverInfos",......

  }
}

AREXPORT ArServerModeDeliver::~ArServerModeDeliver()
{
}

void ArServerModeDeliver::handleDeliverDone(char * res){
	myStatus = "Deliver done by " + string(res);
	ArLog::log(ArLog::Normal, myStatus.c_str());
}

void ArServerModeDeliver::handleDeliverFailed(char * res){
	
	myStatus = "Deliver failed : " + string(res);
	ArLog::log(ArLog::Normal, myStatus.c_str());
}


AREXPORT void ArServerModeDeliver::activate(void)
{
	//Modif ON
 if (isActive() || !baseActivate())
    return;

 /*if (!baseActivate())
    return;*/
  
  setActivityTimeToNow();
  //myStatus = "Starting deliver operation";
  deliverTask();
}

AREXPORT void ArServerModeDeliver::deactivate(void)
{
	if(myASyncDeliverTask.getRunning())
		myASyncDeliverTask.stopRunning();
  myStopGroup.deactivate();
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
  //reset();
  myContent = content;
  myMode = "Deliver";
  myStatus = "Starting deliver";
  myASyncDeliverTask.init(content);
  this->lockMode();
  activate();
}


AREXPORT void ArServerModeDeliver::userTask(void)
{
 
}

AREXPORT void ArServerModeDeliver::addToConfig(ArConfig *config, 
					      const char *section)
{
}

AREXPORT void ArServerModeDeliver::setUseLocationDependentDevices(
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

AREXPORT bool ArServerModeDeliver::getUseLocationDependentDevices(void)
{
  return myUseLocationDependentDevices;
}



void ArServerModeDeliver::deliverTask(){
	
	printf("Deliver task started with content : %s",myContent);
	myASyncDeliverTask.runAsync();
	
}
