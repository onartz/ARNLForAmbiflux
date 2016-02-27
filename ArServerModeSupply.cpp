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
  mySupplyDoneCB(this,&ArServerModeSupply::handleSupplyDone),
  mySupplyFailedCB(this,&ArServerModeSupply::handleSupplyFailed)
 
{
  myMode = "Supply";
  
  if (myServer != NULL)
  {
    addModeData("supply", "supply the robot", &myNetSupplyCB,
		"string: content", "none", "Supply", "RETURN_NONE");
	myASyncSupplyTask.setSupplyDoneCB(&mySupplyDoneCB);
	myASyncSupplyTask.setSupplyFailedCB(&mySupplyFailedCB);
	//myServer->addData("supplyInfos","Supply informations",
	//myServer->addData("supplyInfos",......

  }
}

AREXPORT ArServerModeSupply::~ArServerModeSupply()
{
}

void ArServerModeSupply::handleSupplyDone(char * res){
	myStatus = "Supply done by " + string(res);
	ArLog::log(ArLog::Normal, myStatus.c_str());
}

void ArServerModeSupply::handleSupplyFailed(char * res){
	
	myStatus = "Supply failed : " + string(res);
	ArLog::log(ArLog::Normal, myStatus.c_str());
}


AREXPORT void ArServerModeSupply::activate(void)
{
	//Modif ON
 if (isActive() || !baseActivate())
    return;

 /*if (!baseActivate())
    return;*/
  setActivityTimeToNow();
  //myStatus = "Starting supply operation";
  supplyTask();
}

AREXPORT void ArServerModeSupply::deactivate(void)
{
	if(myASyncSupplyTask.getRunning())
		myASyncSupplyTask.stopRunning();
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
  //reset();
	std::string strContent(content);
	boost::algorithm::replace_all(strContent, "_", " ");
	myContent = strContent.c_str();
	myMode = "Supply";
	myStatus = "Starting supply";
	myASyncSupplyTask.init(content);
	this->lockMode();
	activate();
}


AREXPORT void ArServerModeSupply::userTask(void)
{
 //ArLog::log(ArLog::Normal,"UserTask");
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



void ArServerModeSupply::supplyTask(){
	
	printf("Supply task started with content : %s",myContent);
	myASyncSupplyTask.runAsync();
	
}
