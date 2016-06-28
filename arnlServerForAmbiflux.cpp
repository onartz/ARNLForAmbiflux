/*

Copyright (c) 2014 Adept Technology Inc.
All rights reserved.  

Redistribution of this example source code, with or without modification, is 
permitted provided that the following conditions are met:   
-    Redistributions must retain the above copyright notice, 
     this list of conditions and the following disclaimer.  
-    Redistributions must be in source code form only

The information in this document is subject to change without notice and should
not be construed as a commitment by Adept Technology, Inc.

Adept Technology, Inc. makes no warranty as to the suitability of this material
for use by the recipient, and assumes no responsibility for any consequences
resulting from such use. 

Note: All other non-example software, including binary software objects
(libraries, programs), are prohibited from distribution under terms described
in LICENSE.txt (refer to LICENSE.txt for details).
*/
/** @example arnlServer.cpp example of almost all ARNL features */





/**
  This is an example of using ARNL and ArNetworking to provide a
  server program that remote clients, such as MobileEyes, can connect
  to, and which includes most of the features available in ARNL.

  You can make a copy of this program's source file in the 'examples' directory
  and use it as the basis for your own program.

  MobileEyes sends requests to this server program to go to goals,
  get the current map, set configuration parameters, manually relocalize, 
  request the current state of the robot and sensors, etc. This example server
  includes almost all features provided by ARNL (or SONARNL or MOGS), including
  localization, path planning and navigation to a map goal or abritrary point,
  sharing information with other servers peer-to-peer or through 
  a "central server", using IR and bumper sensors, stopping the robot
  if localization fails, global replanning if a path is not followable,
  use of special SICK laser reflector beacons, as well as various networking
  services for MobileEyes and other clients, such as diagnostic visualizations,
  access to configuration parameters, access to camera control and 
  video images (if SAVServer is running), special debugging commands 
  ("custom commands"), file uploading/downloading (Linux only) and gathering raw laser
  scans using Mapper3, and safe and unsafe teleoperation.

  When this program runs, ARNL will load its configuration file arnl.p. This
  file may specify the map file to use; if arnl.p does not yet have a map
  file name saved or has the wrong map, a map may be set using MobileEyes
  (Open Tools->Robot Configuration, and change the Map in the Files section).
 
  Note: This program creates an ArPoseStorage object which periodically saves 
  the robot position to a file called robotPose. On startup, the robot's 
  position (pose) in ARIA is set to this position, and if using the simulator, 
  the robot is moved to that position in the simulator.  ARNL will then use 
  that as a starting point when doing the initial localization (in
  localizeRobotAtHomeBlocking()) as well as any home points in the map.
  You can delete the 'robotPose' file to reset this behavior (then 0,0,0 is
  assumed to be the robot's initial position, and this is tried during initial
  localization as well as the home points).
*/

#include "Aria.h"
#include "ArNetworking.h"
#include "Arnl.h"
#include "ArPathPlanningInterface.h"
#include "ArLocalizationTask.h"
#include "ArDocking.h"
#include "ArSystemStatus.h"


#include "ArServerMyMode.h"
#include "ArServerModeSupply.h"
#include "ArServerModeDeliver.h"
//#include "ArServerModeBidon.h"
//#include "ArNetworking.h"



void logOptions(const char *progname)
{
  ArLog::log(ArLog::Normal, "Usage: %s [options]\n", progname);
  ArLog::log(ArLog::Normal, "[options] are any program options listed below, or any ARNL configuration");
  ArLog::log(ArLog::Normal, "parameters as -name <value>, see params/arnl.p for list.");
  ArLog::log(ArLog::Normal, "For example, -map <map file>.");
  Aria::logOptions();
}

bool gyroErrored = false;
const char* getGyroStatusString(ArRobot* robot)
{
  if(!robot || !robot->getOrigRobotConfig() || robot->getOrigRobotConfig()->getGyroType() < 2) return "N/A";
  if(robot->getFaultFlags() & ArUtil::BIT4)
  {
    gyroErrored = true;
    return "ERROR/OFF";
  }
  if(gyroErrored)
  {
    return "OK but error before";
  }
  return "OK";
}

// This function is called from the ArRobot task thread on every robot update,
// which is every 100ms.  You can check timers here and do any non-blocking
// monitoring of I/O.  Any long-blocking operations must not be done in this
// callback however, or it will impact the ArRobot task thread and
// communications with the robot (use ArASyncTask to run a new thread instead.)
void robotTaskExample(ArRobot* robot)
{
}

// This function is called whenever a new goal is reached. It will be attached 
// to the path planning task below in main() via ArPathPlanningTask::addGoalDoneCB()
void goalDone(ArPose goalPos) //, ArPathPlanningTask *pathTask)
{
  ArLog::log(ArLog::Normal, "ARNL server example: goal reached");
}

// This function is called if pathplanning fails. It will be attached 
// to the path planning task below in main() via
// ArPathPlanningTask::addGoalFailedCB()
void goalFailed(ArPose goalPos) //, ArPathPlanningTask *pathTask)
{
  ArLog::log(ArLog::Normal, "ARNL server example: goal failed");
}

// This function is called if localization fails. It will be attached 
// to the localization task below in main() via
// ArLocalizationTask::setFailedCallback()
void locFailed(int n) //, ArLocalizationTask* locTask)
{
  ArLog::log(ArLog::Normal, "ARNL server example: localization failed");
}
 
// This function is called whenever the path planning task changes its state
// (for example, from idle to planning a path, to following a planned path). It will be attached 
// to the path planning task below in main() via
// ArPathPlanningTask::addStateChangeCB()
//ArPathPlanningInterface *pathPlanningTask = NULL;
void pathPlanStateChanged(ArPathPlanningTask *pathPlanningTask)
{
  char s[256];
  pathPlanningTask->getFailureString(s, 256);
  ArLog::log(ArLog::Normal, "ARNL server example: Path planning state: %s", s);
}


class RobotMonitor
{
protected:
  ArRobot *robot;
  ArServerHandlerPopup *popupServer;
  ArTypes::Byte4 motorsDisabledPopupID;
  ArServerHandlerPopupInfo motorsDisabledPopupInfo;
  ArFunctor2C<RobotMonitor, ArTypes::Byte4, int> handleMotorsDisabledPopupResponseCB;
  ArFunctorC<RobotMonitor> robotMonitorCB;

public:
  RobotMonitor(ArRobot *r, ArServerHandlerPopup *ps) :
    robot(r),
    popupServer(ps),
    motorsDisabledPopupID(0),
    motorsDisabledPopupInfo( NULL,
      "Robot Motors Are Disabled",
      "The robot's motors are disabled", 
      ArServerHandlerPopup::WARNING,
      1,  // default button ID
      0,  // escape button ID
      -1, // timeout
      NULL, // timeout message
      "Enable Motors", "Enabling Motors...",
      "Ignore", "Ignore"
    ),
    handleMotorsDisabledPopupResponseCB(this, &RobotMonitor::handleMotorsDisabledResponse),
    robotMonitorCB(this, &RobotMonitor::robotMonitorTask)
  {
    robot->addUserTask("arnlServerRobotMonitor", 30, &robotMonitorCB);
  }

  ~RobotMonitor()
  {
    robot->remUserTask(&robotMonitorCB);
  }

protected:
  void handleMotorsDisabledResponse(ArTypes::Byte4 popupID, int button)
  {
    if(button == 0)
    {
      ArLog::log(ArLog::Normal, "Enabling motors...");
      robot->enableMotors();
    }
    popupServer->closePopup(motorsDisabledPopupID, "Closing motor disable popup.");
    motorsDisabledPopupID = 0;
  }


  // This function is called as a robot task (every 100ms) to check on the robot
  // state and perform feedback and interact with user as needed.
  void robotMonitorTask()
  {

    // a way for user to re-enable motors if disabled -- show a popup dialog in
    // MobileEyes.
    if(motorsDisabledPopupID == 0 && robot && !robot->areMotorsEnabled() && robot->isConnected())
    {
      motorsDisabledPopupID = popupServer->createPopup(&motorsDisabledPopupInfo, &handleMotorsDisabledPopupResponseCB);
    }

    // Set LX wheel light pattern based on robot activity. You could add more
    // conditions/light patterns here if you want.
    if(robot->isEStopPressed())
      robot->comDataN(ArCommands::WHEEL_LIGHT, "\x02\0\0\0", 4); // pattern #2, flash red
    else if(!robot->areMotorsEnabled())
      robot->comDataN(ArCommands::WHEEL_LIGHT, "\x03\0\0\0", 4); // pattern #3, flash yellow
    else if(fabs(robot->getVel()) < 5)
      robot->comDataN(ArCommands::WHEEL_LIGHT, "\x0A\0\0\0", 4);  // pattern #10, slow blue flash
    else
      robot->comDataN(ArCommands::WHEEL_LIGHT, "\x09\0\0\0", 4);  // pattern 9, blue sweep.
    
  }
};


/// Log messages from robot controller
bool handleDebugMessage(ArRobotPacket *pkt)
{
  if(pkt->getID() != ArCommands::MARCDEBUG) return false;
  char msg[256];
  pkt->bufToStr(msg, sizeof(msg));
  msg[255] = 0;
  ArLog::log(ArLog::Terse, "Controller Firmware: %s", msg);
  return true;
}


int main(int argc, char **argv)
{
  // Initialize Aria and Arnl global information
  Aria::init();
  Arnl::init();

  // You can change default ArLog options in this call, but the settings in the parameter file
  // (arnl.p) which is loaded below (Aria::getConfig()->parseFile())  will override the options.
  //ArLog::init(ArLog::File, ArLog::Normal, "log.txt", true, true);

  // Used to parse the command line arguments.
  ArArgumentParser parser(&argc, argv);
  
  // Load default arguments for this computer (from /etc/Aria.args, environment
  // variables, and other places)
  parser.loadDefaultArguments();

  // Tell the laser connector to always connect the first laser since
  // this program always requires a laser.
  parser.addDefaultArgument("-connectLaser");

  


  // The robot object
  ArRobot robot;

  // This is an example of a user task callback. This is called every ArRobot
  // task cycle (every 100ms). The function is defined at the top of this file.
  ArGlobalFunctor1<ArRobot*> robotTaskExampleCB(&robotTaskExample, &robot);
  robot.addUserTask("arnlServerExampleTask", 10, &robotTaskExampleCB);

  // handle messages from robot controller firmware and log the contents
  robot.addPacketHandler(new ArGlobalRetFunctor1<bool, ArRobotPacket*>(&handleDebugMessage));

  // This object is used to connect to the robot, which can be configured via
  // command line arguments.
  ArRobotConnector robotConnector(&parser, &robot);

  // Connect to the robot
  if (!robotConnector.connectRobot())
  {
    ArLog::log(ArLog::Normal, "Error: Could not connect to robot... exiting");
    Aria::exit(3);
  }



  // Set up where we'll look for files. Arnl::init() set Aria's default
  // directory to Arnl's default directory; addDirectories() appends this
  // "examples" directory.
  char fileDir[1024];
  ArUtil::addDirectories(fileDir, sizeof(fileDir), Aria::getDirectory(), 
			 "examples");
  
  
  // To direct log messages to a file, or to change the log level, use these  calls:
  //ArLog::init(ArLog::File, ArLog::Normal, "log.txt", true, true);
  //ArLog::init(ArLog::File, ArLog::Verbose);
 
  // Add a section to the configuration to change ArLog parameters
  ArLog::addToConfig(Aria::getConfig());

  // set up a gyro (if the robot is older and its firmware does not
  // automatically incorporate gyro corrections, then this object will do it)
  ArAnalogGyro gyro(&robot);

  // Our networking server
  ArServerBase server;
  

  // Set up our simpleOpener, used to set up the networking server
  ArServerSimpleOpener simpleOpener(&parser);

  // the laser connector
  ArLaserConnector laserConnector(&parser, &robot, &robotConnector);

  // used to connect to camera PTZ control
  ArPTZConnector ptzConnector(&parser, &robot);

  // Used to connect to a "central server" which can be used as a proxy 
  // for multiple robot servers, and as a way for them to also communicate with
  // each other.  (objects implementing some of these inter-robot communication
  // features are created below).  
  // NOTE: If the central server is running on the same host as robot server(s),
  // then you must use the -serverPort argument to instruct these robot-control
  // server(s) to use different ports than the default 7272, since the central
  // server will use that port.
  ArClientSwitchManager clientSwitch(&server, &parser);
  
  // Load default arguments for this computer (from /etc/Aria.args, environment
  // variables, and other places)
  parser.loadDefaultArguments();

  // Parse arguments 
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    logOptions(argv[0]);
    Aria::exit(1);
  }
  

  // This causes Aria::exit(9) to be called if the robot unexpectedly
  // disconnects
  ArGlobalFunctor1<int> shutdownFunctor(&Aria::exit, 9);
  robot.addDisconnectOnErrorCB(&shutdownFunctor);


  // Create an ArSonarDevice object (ArRangeDevice subclass) and 
  // connect it to the robot.
  ArSonarDevice sonarDev;
  robot.addRangeDevice(&sonarDev);



  // This object will allow robot's movement parameters to be changed through
  // a Robot Configuration section in the ArConfig global configuration facility.
  ArRobotConfig robotConfig(&robot);

  // Include gyro configuration options in the robot configuration section.
  robotConfig.addAnalogGyro(&gyro);

  // Start the robot thread.
  robot.runAsync(true);

  

  // connect the laser(s) if it was requested, this adds them to the
  // robot too, and starts them running in their own threads
  ArLog::log(ArLog::Normal, "Connecting to laser(s) configured in parameters...");
  if (!laserConnector.connectLasers())
  {
    ArLog::log(ArLog::Normal, "Error: Could not connect to laser(s). Exiting.");
    Aria::exit(2);
  }
  ArLog::log(ArLog::Normal, "Done connecting to laser(s).");

  // find the laser we should use for localization and/or mapping,
  // which will be the first laser
  robot.lock();
  ArLaser *firstLaser = robot.findLaser(1);
  if (firstLaser == NULL || !firstLaser->isConnected())
  {
    ArLog::log(ArLog::Normal, "Did not have laser 1 or it is not connected, cannot start localization and/or mapping... exiting");
    Aria::exit(2);
  }
  robot.unlock();


    /* Create and set up map object */
  
  // Set up the map object, this will look for files in the examples
  // directory (unless the file name starts with a /, \, or .
  // You can take out the 'fileDir' argument to look in the program's current directory
  // instead.
  // When a configuration file is loaded into ArConfig later, if it specifies a
  // map file, then that file will be loaded as the map.
  ArMap map(fileDir);
  // set it up to ignore empty file names (otherwise if a configuration omits
  // the map file, the whole configuration change will fail)
  map.setIgnoreEmptyFileName(true);
  // ignore the case, so that if someone is using MobileEyes or
  // MobilePlanner from Windows and changes the case on a map name,
  // it will still work.
  map.setIgnoreCase(true);

    
    /* Create localization and path planning threads */


  ArPathPlanningTask pathTask(&robot, &sonarDev, &map);
//  pathPlanningTask = &pathTask;

  // The following are callback functions that pathTask will call 
  // on certain events such as reaching a goal, failing to reach a goal, etc.
  // The functions are defined at the top of this file. You can add code there
  // to take some action such as control something on the robot, choose the next
  // goal, send output elsewhele, etc.
  ArGlobalFunctor1<ArPose> /*, ArPathPlanningTask*>*/ goalDoneCB(&goalDone); //, &pathTask);
  pathTask.addGoalDoneCB(&goalDoneCB);

  ArGlobalFunctor1<ArPose> /*, ArPathPlanningTask*>*/ goalFailedCB(&goalFailed); //, &pathTask);
  pathTask.addGoalFailedCB(&goalFailedCB);

  ArGlobalFunctor1<ArPathPlanningTask*> stateCB(&pathPlanStateChanged, &pathTask);
  pathTask.addStateChangeCB(&stateCB);


  ArLog::log(ArLog::Normal, "Creating laser localization task");
  // Laser Monte-Carlo Localization
  ArLocalizationTask locTask(&robot, firstLaser, &map);
  


  // A callback function, which is called if localization fails
  ArGlobalFunctor1<int> locFailedCB(&locFailed);
  locTask.setFailedCallBack(&locFailedCB); //, &locTask);


  // Set some options  and callbacks on each laser that the laser connector 
  // connected to.
  std::map<int, ArLaser *>::iterator laserIt;
  for (laserIt = robot.getLaserMap()->begin();
       laserIt != robot.getLaserMap()->end();
       laserIt++)
  {
    int laserNum = (*laserIt).first;
    ArLaser *laser = (*laserIt).second;

    // Skip lasers that aren't connected
    if(!laser->isConnected())
      continue;

    // add the disconnectOnError CB to shut things down if the laser
    // connection is lost
    laser->addDisconnectOnErrorCB(&shutdownFunctor);
    // set the number of cumulative readings the laser will take
    laser->setCumulativeBufferSize(200);
    // add the lasers to the path planning task
    pathTask.addRangeDevice(laser, ArPathPlanningTask::BOTH);
    // set the cumulative clean offset (so that they don't all fire at once)
    laser->setCumulativeCleanOffset(laserNum * 100);
    // reset the cumulative clean time (to make the new offset take effect)
    laser->resetLastCumulativeCleanTime();

    // Add the packet count to the Aria info strings (It will be included in
    // MobileEyes custom details so you can monitor whether the laser data is
    // being received correctly)
    std::string laserPacketCountName;
    laserPacketCountName = laser->getName();
    laserPacketCountName += " Packet Count";
    Aria::getInfoGroup()->addStringInt(
	    laserPacketCountName.c_str(), 10, 
	    new ArRetFunctorC<int, ArLaser>(laser, 
					 &ArLaser::getReadingCount));
  }





    /* Start the server */

  // Open the networking server
  if (!simpleOpener.open(&server, fileDir, 240))
  {
    ArLog::log(ArLog::Normal, "Error: Could not open server.");
    exit(2);
  }



    /* Create various services that provide network access to clients (such as
     * MobileEyes), as well as add various additional features to ARNL */


  // ARNL can optionally get information about the positions of other robots from a
  // "central server" (see central server example program), if command
  // line options specifying the address of the central server was given.
  // If there is no central server, then the address of each other robot
  // can instead be given in the configuration, and the multirobot systems
  // will connect to each robot (or "peer") individually.

        // TODO move this to multirobot example?
  ArServerHandlerMultiRobot *handlerMultiRobot = NULL;
  ArMultiRobotRangeDevice *multiRobotRangeDevice = NULL;
  ArServerHandlerMultiRobotPeer *multiRobotPeer = NULL;
  ArMultiRobotPeerRangeDevice *multiRobotPeerRangeDevice = NULL;


  bool usingCentralServer = false;
  if(clientSwitch.getCentralServerHostName() != NULL)
    usingCentralServer = true;

  // if we're using the central server then we want to create the
  // multiRobot central classes
  if (usingCentralServer)
  {
    // Make the handler for multi robot information (this sends the
    // information to the central server)
    handlerMultiRobot = new ArServerHandlerMultiRobot(&server, &robot, 
						      &pathTask,
						      &locTask, &map);
    
    // Normally each robot, and the central server, must all have
    // the same map name for the central server to share robot
    // information.  (i.e. they are operating in the same space).
    // This changes the map name that ArServerHandlerMutliRobot 
    // reports to the central server, in case you want this individual
    // robot to load a different map file name, but still report 
    // the common map file to the central server.
    //handlerMultiRobot->overrideMapName("central.map");

    // the range device that gets the multi robot information from
    // the central server and presents it as virtual range readings
    // to ARNL
    multiRobotRangeDevice = new ArMultiRobotRangeDevice(&server);
    
    robot.addRangeDevice(multiRobotRangeDevice);
    pathTask.addRangeDevice(multiRobotRangeDevice, 
			    ArPathPlanningTask::BOTH);
    
    // Set up options for drawing multirobot information in MobileEyes.
    multiRobotRangeDevice->setCurrentDrawingData(
	    new ArDrawingData("polyDots", ArColor(125, 125, 0),
			      100, 73, 1000), true);
    multiRobotRangeDevice->setCumulativeDrawingData(
	    new ArDrawingData("polyDots", ArColor(125, 0, 125),
			      100, 72, 1000), true);

    // This sets up the localization to use the known poses of other robots
    // for its localization in cases where numerous robots crowd out the map.
    locTask.setMultiRobotCallback(multiRobotRangeDevice->getOtherRobotsCB());
  }
  // if we're not using a central server then create the multirobot peer classes
  else
  {
    // set the path planning so it uses the explicit collision range for how far its planning
    pathTask.setUseCollisionRangeForPlanningFlag(true);
    // make our thing that gathers information from the other servers
    multiRobotPeerRangeDevice = new ArMultiRobotPeerRangeDevice(&map);
    // make our thing that sends information to the other servers
    multiRobotPeer = new ArServerHandlerMultiRobotPeer(&server, &robot, 
						     &pathTask, &locTask);
    // hook the two together so they both know what priority this robot is
    multiRobotPeer->setNewPrecedenceCallback(
	    multiRobotPeerRangeDevice->getSetPrecedenceCallback());
    // hook the two together so they both know what priority this
    // robot's fingerprint is
    multiRobotPeer->setNewFingerprintCallback(
	    multiRobotPeerRangeDevice->getSetFingerprintCallback());
    // hook the two together so that the range device can call on the
    // server handler to change its fingerprint
    multiRobotPeerRangeDevice->setChangeFingerprintCB(
	    multiRobotPeer->getChangeFingerprintCB());
    // then add the robot to the places it needs to be
    robot.addRangeDevice(multiRobotPeerRangeDevice);
    pathTask.addRangeDevice(multiRobotPeerRangeDevice, 
			    ArPathPlanningTask::BOTH);

    // Set the range device so that we can see the information its using
    // to avoid, you can comment these out in order to not see them
    multiRobotPeerRangeDevice->setCurrentDrawingData(
	    new ArDrawingData("polyDots", ArColor(125, 125, 0),
			      100, 72, 1000), true);
    multiRobotPeerRangeDevice->setCumulativeDrawingData(
	    new ArDrawingData("polyDots", ArColor(125, 0, 125),
			      100, 72, 1000), true);
    // This sets up the localization to use the known poses of other robots
    // for its localization in cases where numerous robots crowd out the map.
    locTask.setMultiRobotCallback(
	    multiRobotPeerRangeDevice->getOtherRobotsCB());
  }




  /* Add additional range devices to the robot and path planning task (so it
     avoids obstacles detected by these devices) */
  
  // Add IR range device to robot and path planning task (so it avoids obstacles
  // detected by this device)
  robot.lock();
  ArIRs irs;
  robot.addRangeDevice(&irs);
  pathTask.addRangeDevice(&irs, ArPathPlanningTask::CURRENT);

  // Add bumpers range device to robot and path planning task (so it avoids obstacles
  // detected by this device)
  ArBumpers bumpers;
  robot.addRangeDevice(&bumpers);
  pathTask.addRangeDevice(&bumpers, ArPathPlanningTask::CURRENT);

  // Add range device which uses forbidden regions given in the map to give virtual
  // range device readings to ARNL.  (so it avoids obstacles
  // detected by this device)
  ArForbiddenRangeDevice forbidden(&map);
  robot.addRangeDevice(&forbidden);
  pathTask.addRangeDevice(&forbidden, ArPathPlanningTask::CURRENT);

  robot.unlock();


  // Action to slow down robot when localization score drops but not lost.
  ArActionSlowDownWhenNotCertain actionSlowDown(&locTask);
  pathTask.getPathPlanActionGroup()->addAction(&actionSlowDown, 140);

  // Action to stop the robot when localization is "lost" (score too low)
  ArActionLost actionLostPath(&locTask, &pathTask);
  pathTask.getPathPlanActionGroup()->addAction(&actionLostPath, 150);

  // Arnl uses this object when it must replan its path because its
  // path is completely blocked.  It will use an older history of sensor
  // readings to replan this new path.  This should not be used with SONARNL
  // since sonar readings are not accurate enough and may prevent the robot
  // from planning through space that is actually clear.
  ArGlobalReplanningRangeDevice replanDev(&pathTask);

  
  // Service to provide drawings of data in the map display :
  ArServerInfoDrawings drawings(&server);
  drawings.addRobotsRangeDevices(&robot);
  drawings.addRangeDevice(&replanDev);

  /* Draw a box around the local path planning area use this 
    (You can enable this particular drawing from custom commands 
    which is set up down below in ArServerInfoPath) */
  ArDrawingData drawingDataP("polyLine", ArColor(200,200,200), 1, 75);
  ArFunctor2C<ArPathPlanningTask, ArServerClient *, ArNetPacket *> 
    drawingFunctorP(&pathTask, &ArPathPlanningTask::drawSearchRectangle);
  drawings.addDrawing(&drawingDataP, "Local Plan Area", &drawingFunctorP); 

  /* Draw a box showing the safety clearance used by the path planning task to */
  drawings.addDrawing(
    new ArDrawingData("polySegments", ArColor(166, 166, 166), 1, 60, 100, "DefaultOn"),
    "Path Planning Clearances",
    new ArFunctor2C<ArPathPlanningTask, ArServerClient*, ArNetPacket*>(&pathTask, &ArPathPlanningTask::drawRobotBounds)
  );

  /* Show the sample points used by MCL */
  ArDrawingData drawingDataL("polyDots", ArColor(0,255,0), 100, 75);
  ArFunctor2C<ArLocalizationTask, ArServerClient *, ArNetPacket *> 
    drawingFunctorL(&locTask, &ArLocalizationTask::drawRangePoints);
  drawings.addDrawing(&drawingDataL, "Localization Points", &drawingFunctorL);


  // "Custom" commands. You can add your own custom commands here, they will
  // be available in MobileEyes' custom commands (enable in the toolbar or
  // access through Robot Tools)
  ArServerHandlerCommands commands(&server);


  // These provide various kinds of information to the client:
  ArServerInfoRobot serverInfoRobot(&server, &robot);
  ArServerInfoSensor serverInfoSensor(&server, &robot);
  ArServerInfoPath serverInfoPath(&server, &robot, &pathTask);
  serverInfoPath.addSearchRectangleDrawing(&drawings);
  serverInfoPath.addControlCommands(&commands);

  // Provides localization info and allows the client (MobileEyes) to relocalize at a given
  // pose:
  ArServerInfoLocalization serverInfoLocalization(&server, &robot, &locTask);
  ArServerHandlerLocalization serverLocHandler(&server, &robot, &locTask);

  // If you're using MobileSim, ArServerHandlerLocalization sends it a command
  // to move the robot's true pose if you manually do a localization through 
  // MobileEyes.  To disable that behavior, use this constructor call instead:
  // ArServerHandlerLocalization serverLocHandler(&server, &robot, true, false);
  // The fifth argument determines whether to send the command to MobileSim.

  // Provide the map to the client (and related controls):
  ArServerHandlerMap serverMap(&server, &map);

  // These objects add some simple (custom) commands to 'commands' for testing and debugging:
  ArServerSimpleComUC uCCommands(&commands, &robot);                   // Send any command to the microcontroller
  ArServerSimpleComMovementLogging loggingCommands(&commands, &robot); // configure logging
  ArServerSimpleComLogRobotConfig configCommands(&commands, &robot);   // trigger logging of the robot config parameters
//  ArServerSimpleServerCommands serverCommands(&commands, &server);     // monitor networking behavior (track packets sent etc.)


  // service that allows the client to monitor the communication link status
  // between the robot and the client.
  //
  ArServerHandlerCommMonitor handlerCommMonitor(&server);



  // service that allows client to change configuration parameters in ArConfig 
  ArServerHandlerConfig handlerConfig(&server, Aria::getConfig(),
				      Arnl::getTypicalDefaultParamFileName(),
				      Aria::getDirectory());


  // This service causes the client to show simple dialog boxes
  ArServerHandlerPopup popupServer(&server);

  // Monitor the robot for current state such as if the motors are disabled,
  // indicate this state by various means and show a popup dialog on clients
  // if user confirmation is needed.
  // (Namely, some robots  disable motors automatically on various error conditions,
  // and we want the user to manually re-enable them after resolving the
  // problem)
  RobotMonitor robotMonitor(&robot, &popupServer);



  /* Set up the possible modes for remote control from a client such as
   * MobileEyes:
   */

    // Mode To go to a goal or other specific point:
  ArServerMyMode myMode(&server, &robot,&locTask, &pathTask, &map, 300, 100);

  // Mode To go to a goal or other specific point:
  ArServerModeGoto modeGoto(&server, &robot, &pathTask, &map,
			    locTask.getRobotHome(),
			    locTask.getRobotHomeCallback());

  // Add a special command to Custom Commands that tours a list of goals rather
  // than all:
  modeGoto.addTourGoalsInListSimpleCommand(&commands);

  // Mode To stop and remain stopped:
  ArServerModeStop modeStop(&server, &robot);

  // Cause the sonar to turn off automatically
  // when the robot is stopped, and turn it back on when commands to move
  // are sent. (Note, if using SONARNL to localize, then don't do this
  // since localization may get lost)
  ArSonarAutoDisabler sonarAutoDisabler(&robot);

  // Teleoperation modes To drive by keyboard, joystick, etc:
  ArServerModeRatioDrive modeRatioDrive(&server, &robot);  
//  ArServerModeDrive modeDrive(&server, &robot);            // Older mode for compatability



  // Prevent normal teleoperation driving if localization is lost using
  // a high-priority action, which enables itself when the particular mode is
  // active.
  // (You have to enter unsafe drive mode to drive when lost.)
  ArActionLost actionLostRatioDrive(&locTask, &pathTask, &modeRatioDrive);
  modeRatioDrive.getActionGroup()->addAction(&actionLostRatioDrive, 110);

  // Add drive mode section to the configuration, and also some custom (simple) commands:
  modeRatioDrive.addToConfig(Aria::getConfig(), "Teleop settings");
  modeRatioDrive.addControlCommands(&commands);

  // Wander mode (also prevent wandering if lost):
  ArServerModeWander modeWander(&server, &robot);
  ArActionLost actionLostWander(&locTask, &pathTask, &modeWander);
  modeWander.getActionGroup()->addAction(&actionLostWander, 110);

  // Tool to log data periodically to a file
  ArDataLogger dataLogger(&robot, "datalog.txt");
  dataLogger.addToConfig(Aria::getConfig()); // make it configurable through ArConfig

  // Automatically add anything from the global info group to the data logger.
  Aria::getInfoGroup()->addAddStringCallback(dataLogger.getAddStringFunctor());

  // This provides a small table of interesting information for the client
  // to display to the operator. You can add your own callbacks to show any
  // data you want.
  ArServerInfoStrings stringInfo(&server);
  Aria::getInfoGroup()->addAddStringCallback(stringInfo.getAddStringFunctor());
  
  // The following statements add fields to a set of informational data called
  // the InfoGroup. These are served to MobileEyes for displayi (turn on by enabling Details
  // and Custom Details in the View menu of MobileEyes.)

  Aria::getInfoGroup()->addStringInt(
	  "Motor Packet Count", 10, 
	  new ArConstRetFunctorC<int, ArRobot>(&robot, 
					       &ArRobot::getMotorPacCount));

  Aria::getInfoGroup()->addStringDouble(
	  "Laser Localization Score", 8, 
	  new ArRetFunctorC<double, ArLocalizationTask>(
		  &locTask, &ArLocalizationTask::getLocalizationScore),
	  "%.03f");
  Aria::getInfoGroup()->addStringInt(
	  "Laser Loc Num Samples", 8, 
	  new ArRetFunctorC<int, ArLocalizationTask>(
		  &locTask, &ArLocalizationTask::getCurrentNumSamples),
	  "%4d");


  // Display gyro status if gyro is enabled and is being handled by the firmware (gyro types 2, 3, or 4).
  // (If the firmware detects an error communicating with the gyro or IMU it
  // returns a flag, and stops using it.)
  // (This gyro type parameter, and fault flag, are only in ARCOS, not Seekur firmware)
  if(robot.getOrigRobotConfig() && robot.getOrigRobotConfig()->getGyroType() > 1)
  {
    Aria::getInfoGroup()->addStringString(
          "Gyro/IMU Status", 10,
          new ArGlobalRetFunctor1<const char*, ArRobot*>(&getGyroStatusString, &robot)
      );
  }

  // Display system CPU and wireless network status
  ArSystemStatus::startPeriodicUpdate(1000); // update every 1 second
  Aria::getInfoGroup()->addStringDouble("CPU Use", 10, ArSystemStatus::getCPUPercentFunctor(), "% 4.0f%%");
  Aria::getInfoGroup()->addStringInt("Wireless Link Quality", 9, ArSystemStatus::getWirelessLinkQualityFunctor(), "%d");
  Aria::getInfoGroup()->addStringInt("Wireless Link Noise", 9, ArSystemStatus::getWirelessLinkNoiseFunctor(), "%d");
  Aria::getInfoGroup()->addStringInt("Wireless Signal", 9, ArSystemStatus::getWirelessLinkSignalFunctor(), "%d");
  

  // stats on how far its driven since software started
  Aria::getInfoGroup()->addStringDouble("Distance Travelled (m)", 20, new ArRetFunctorC<double, ArRobot>(&robot, &ArRobot::getOdometerDistanceMeters), "%.2f");
  Aria::getInfoGroup()->addStringDouble("Run time (min)", 20, new
ArRetFunctorC<double, ArRobot>(&robot, &ArRobot::getOdometerTimeMinutes),
"%.2f");


  ArServerModeSupply modeSupply(&server, &robot);
  ArServerModeDeliver modeDeliver(&server, &robot);


  // Setup the dock if there is a docking system on board.
  ArServerModeDock *modeDock = NULL;
  modeDock = ArServerModeDock::createDock(&server, &robot, &locTask, 
					  &pathTask);
  if (modeDock != NULL)
  {
    modeDock->checkDock();
    modeDock->addAsDefaultMode();
    modeDock->addToConfig(Aria::getConfig());
    modeDock->addControlCommands(&commands);
  }




  // Make Stop mode the default (If current mode deactivates without entering
  // a new mode, then Stop Mode will be selected)
  modeStop.addAsDefaultMode();
    // TODO move up near where stop mode is created?





  /* Services that allow the client to initiate scanning with the laser to
     create maps in Mapper3 (So not possible with SONARNL): */

  ArServerHandlerMapping handlerMapping(&server, &robot, firstLaser, 
					fileDir, "", true);

  // make laser localization stop while mapping
  handlerMapping.addMappingStartCallback(
	  new ArFunctor1C<ArLocalizationTask, bool>
	  (&locTask, &ArLocalizationTask::setIdleFlag, true));

  // and then make it start again when we're doine
  handlerMapping.addMappingEndCallback(
	  new ArFunctor1C<ArLocalizationTask, bool>
	  (&locTask, &ArLocalizationTask::setIdleFlag, false));


  // Make it so our "lost" actions don't stop us while mapping
  handlerMapping.addMappingStartCallback(actionLostPath.getDisableCB());
  handlerMapping.addMappingStartCallback(actionLostRatioDrive.getDisableCB());
  handlerMapping.addMappingStartCallback(actionLostWander.getDisableCB());

  // And then let them make us stop as usual when done mapping
  handlerMapping.addMappingEndCallback(actionLostPath.getEnableCB());
  handlerMapping.addMappingEndCallback(actionLostRatioDrive.getEnableCB());
  handlerMapping.addMappingEndCallback(actionLostWander.getEnableCB());

  // don't let forbidden lines show up as obstacles while mapping
  // (they'll just interfere with driving while mapping, and localization is off anyway)
  handlerMapping.addMappingStartCallback(forbidden.getDisableCB());

  // let forbidden lines show up as obstacles again as usual after mapping
  handlerMapping.addMappingEndCallback(forbidden.getEnableCB());


  /*
  // If we are on a simulator, move the robot back to its starting position,
  // and reset its odometry.
  // This will allow localizeRobotAtHomeBlocking() below will (probably) work (it
  // tries current odometry (which will be 0,0,0) and all the map
  // home points.
  // (Ignored by a real robot)
  //robot.com(ArCommands::SIM_RESET);
  */


  // create a pose storage class, this will let the program keep track
  // of where the robot is between runs...  after we try and restore
  // from this file it will start saving the robot's pose into the
  // file
  ArPoseStorage poseStorage(&robot);
  /// if we could restore the pose from then set the sim there (this
  /// won't do anything to the real robot)... if we couldn't restore
  /// the pose then just reset the position of the robot (which again
  /// won't do anything to the real robot)
  if (poseStorage.restorePose("robotPose"))
    serverLocHandler.setSimPose(robot.getPose());
  //else
 //   robot.com(ArCommands::SIM_RESET);



  /* File transfer services: */
  
#ifdef WIN32
  // Not implemented for Windows yet.
  ArLog::log(ArLog::Normal, "Note, file upload/download services are not implemented for Windows; not enabling them.");
#else
  // This block will allow you to set up where you get and put files
  // to/from, just comment them out if you don't want this to happen
  // /*
  ArServerFileLister fileLister(&server, fileDir);
  ArServerFileToClient fileToClient(&server, fileDir);
  ArServerFileFromClient fileFromClient(&server, fileDir, "/tmp");
  ArServerDeleteFileOnServer deleteFileOnServer(&server, fileDir);
  // */
#endif

    /* Video image streaming, and camera controls (Requires SAVserver or ACTS) */

  // Forward one video stream if either ACTS, ArVideo videoSubServer, 
  // or SAV server are running.
  // ArHybridForwarderVideo allows this program to be separate from the ArVideo
  // library. You could replace videoForwarder and the PTZ connection code below
  // with a call to ArVideo::createVideoServers(), and link the program to the
  // ArVideo library if you want to include video capture in the same program
  // as robot control.
  ArHybridForwarderVideo videoForwarder(&server, "localhost", 7070);
  
  // connect to first configured camera PTZ controls (in robot parameter file and
  // command line options)
  ptzConnector.connect();
  ArServerHandlerCamera *cameraServer = NULL;
  ArCameraCollection cameraCollection;
  ArPTZ *ptz = ptzConnector.getPTZ(0);
  if(ptz)
  {
    ArLog::log(ArLog::Normal, "Connected to PTZ Camera");
    cameraCollection.addCamera("Camera1", ptz->getTypeName(), "Camera", ptz->getTypeName());

    videoForwarder.setCameraName("Camera1");
    videoForwarder.addToCameraCollection(cameraCollection);

    cameraServer = new ArServerHandlerCamera("Camera1", 
		                                           &server, 
					                                     &robot,
					                                     ptz, 
					                                     &cameraCollection);

    // ArServerHandlerCamera has a special feature that continuously moves the
    // camera to look at a position. This is an example of how to use that to 
    // look at the path planning task's next goal via its callbacks. You can 
    // also set the ArServerHandhlerCamera to look at any map position instead.
    pathTask.addNewGoalCB(
	    new ArFunctor1C<ArServerHandlerCamera, ArPose>(
		    cameraServer, 
		    &ArServerHandlerCamera::cameraModeLookAtGoalSetGoal));
    pathTask.addGoalFinishedCB(
	    new ArFunctorC<ArServerHandlerCamera>(
		    cameraServer, 
		    &ArServerHandlerCamera::cameraModeLookAtGoalClearGoal));

  } 

  // Allows client to find any camera servers created above
  ArServerHandlerCameraCollection cameraCollectionServer(&server, &cameraCollection);



    /* Load configuration values, map, and begin! */

  
  // When parsing the configuration file, also look at the program's command line options 
  // from the command-line argument parser as well as the configuration file.
  // (So you can use any argument on the command line, namely -map.) 
  Aria::getConfig()->useArgumentParser(&parser);

  // Read in parameter files.
  ArLog::log(ArLog::Normal, "Loading config file %s%s into ArConfig...", Aria::getDirectory(), Arnl::getTypicalParamFileName());
  if (!Aria::getConfig()->parseFile(Arnl::getTypicalParamFileName()))
  {
    ArLog::log(ArLog::Normal, "Could not load ARNL configuration file. Set ARNL environment variable to use non-default installation director.y");
    Aria::exit(5);
  }

  // Warn about unknown params.
  if (!simpleOpener.checkAndLog() || !parser.checkHelpAndWarnUnparsed())
  {
    logOptions(argv[0]);
    Aria::exit(6);
  }

  // Warn if there is no map
  if (map.getFileName() == NULL || strlen(map.getFileName()) <= 0)
  {
    ArLog::log(ArLog::Normal, "");
    ArLog::log(ArLog::Normal, "### No map file is set up, you can make a map with the following procedure");
    ArLog::log(ArLog::Normal, "   0) You can find this information in README.txt or docs/Mapping.txt");
    ArLog::log(ArLog::Normal, "   1) Connect to this server with MobileEyes");
    ArLog::log(ArLog::Normal, "   2) Go to Tools->Map Creation->Start Scan");
    ArLog::log(ArLog::Normal, "   3) Give the map a name and hit okay");
    ArLog::log(ArLog::Normal, "   4) Drive the robot around your space (see docs/Mapping.txt");
    ArLog::log(ArLog::Normal, "   5) Go to Tools->Map Creation->Stop Scan");
    ArLog::log(ArLog::Normal, "   6) Start up Mapper3");
    ArLog::log(ArLog::Normal, "   7) Go to File->Open on Robot");
    ArLog::log(ArLog::Normal, "   8) Select the .2d you created");
    ArLog::log(ArLog::Normal, "   9) Create a .map");
    ArLog::log(ArLog::Normal, "  10) Go to File->Save on Robot");
    ArLog::log(ArLog::Normal, "  11) In MobileEyes, go to Tools->Robot Config");
    ArLog::log(ArLog::Normal, "  12) Choose the Files section");
    ArLog::log(ArLog::Normal, "  13) Enter the path and name of your new .map file for the value of the Map parameter.");
    ArLog::log(ArLog::Normal, "  14) Press OK and your new map should become the map used");
    ArLog::log(ArLog::Normal, "");    
  }

  // Print a log message notifying user of the directory for map files
  ArLog::log(ArLog::Normal, "");
  ArLog::log(ArLog::Normal, 
	     "Directory for maps and file serving: %s", fileDir);
  
  ArLog::log(ArLog::Normal, "See the ARNL README.txt for more information");
  ArLog::log(ArLog::Normal, "");

  // Do an initial localization of the robot. ARNL and SONARNL try all the home points
  // in the map, as well as the robot's current odometric position, as possible
  // places the robot is likely to be at startup.   If successful, it will
  // also save the position it found to be the best localized position as the
  // "Home" position, which can be obtained from the localization task (and is
  // used by the "Go to home" network request).
  // MOGS instead just initializes at the current GPS position.
  // (You will stil have to drive the robot so it can determine the robot's
  // heading, however. See GPS Mapping instructions.)
  locTask.localizeRobotAtHomeBlocking();
  
  // Let the client switch manager (for multirobot) spin off into its own thread
  // TODO move to multirobot example?
  clientSwitch.runAsync();

  // Start the networking server's thread
  server.runAsync();

  // Enable the motors and wait until the robot exits (disconnection, etc.) or this program is
  // canceled.
  robot.enableMotors();
  ArLog::log(ArLog::Normal, "Server running. To exit, press Ctrl-C.");
  robot.waitForRunExit();
  Aria::exit(0);
}











/**
 This is an extension of Arnl server.
 Custom commands has been added.
 The robot should be able to respond to Supply, deliver, talk commands...
*/
//
//#include <vector>
//#include <time.h>
//#include <stdio.h>
//#include <assert.h>
////#include "Aria.h"
//#include "Arnl.h"
//#include "ArDocking.h"
////#include "ArLog.h"
//#include "ArServerMyMode.h"
//#include "ArServerModeBidon.h"
////#include "ArNetworking.h"
//
//#include "ArServerModeSupply.h"
//#include "ArServerModeDeliver.h"
////#include "ArLocalizationTask.h"
//
//#include "Globals.h"
////#include "sqlite3.h"
//
//
//void logOptions(const char *progname)
//{
//  ArLog::log(ArLog::Normal, "Usage: %s [options]\n", progname);
//  ArLog::log(ArLog::Normal, "[options] are any program options listed below, or any ARNL configuration");
//  ArLog::log(ArLog::Normal, "parameters as -name <value>, see params/arnl.p for list.");
//  ArLog::log(ArLog::Normal, "For example, -map <map file>.");
//  Aria::logOptions();
//}
//
//bool gyroErrored = false;
//const char* getGyroStatusString(ArRobot* robot)
//{
//  if(!robot || !robot->getOrigRobotConfig() || robot->getOrigRobotConfig()->getGyroType() < 2) return "N/A";
//  if(robot->getFaultFlags() & ArUtil::BIT4)
//  {
//    gyroErrored = true;
//    return "ERROR/OFF";
//  }
//  if(gyroErrored)
//  {
//    return "OK but error before";
//  }
//  return "OK";
//}
//
//  //ArCepstral g_Cepstral;
//  //ArSoundsQueue g_SoundsQueue;
//
//int main(int argc, char **argv)
//{
//  
//  // Initialize Aria and Arnl global information
//  Aria::init();
//  Arnl::init();
//
// // ArSpeechSynth* speechSynthesizer = new ArCepstral();
// // if(!speechSynthesizer) {
// //   ArLog::log(ArLog::Terse, "Error: no speech synthesizer is available. Are we linked to ArSpeechSynth_Cepstral or ArSpeechSynth_Festival? Are they broken?");
// //   exit(-1);
// // }
// ////ArSoundsQueue soundQueue;
// // g_SoundsQueue.addInitCallback(speechSynthesizer->getInitCallback());
// // g_SoundsQueue.setSpeakCallback(speechSynthesizer->getSpeakCallback());
//
//  //g_SoundsQueue.speak("Hello guy how are you now??\0");
//
//
//  // Run the sound queue in a separate thread, and wait for it to stop:
//  /*printf("Running sounds queue in the background...\n");
//  g_SoundsQueue.runAsync();
//  while (g_SoundsQueue.isSpeakingOrPlaying() || !g_SoundsQueue.isInitialized())
//    ArUtil::sleep(100);*/
//
//
//
//
////sqlite3 *db;
////   char *zErrMsg = 0;
////   int  rc;
////   char *sql;
////
////   /* Open database */
////   rc = sqlite3_open("patrolbot.db", &db);
////   if( rc ){
////      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
////      exit(0);
////   }else{
////      fprintf(stdout, "Opened database successfully\n");
////   }
////    /* Create SQL statement */
////   sql = "CREATE TABLE USER("  \
////         "ID CARDID PRIMARY KEY     NOT NULL," \
////         "NAME           TEXT    NOT NULL\")";
////         
////
////   /* Execute SQL statement */
////   rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
////   if( rc != SQLITE_OK ){
////   fprintf(stderr, "SQL error: %s\n", zErrMsg);
////      sqlite3_free(zErrMsg);
////   }else{
////      fprintf(stdout, "Table created successfully\n");
////   }
////   sqlite3_close(db);
////
//
//
//	//if(!(g_Cepstral.init())){
//	//		ArLog::log(ArLog::Normal,"Cepstral failed");
//	//	}
//
//	//else{
//	//	//g_Cepstral.speakf("Hello");
//	//	
//	//	//g_Cepstral.speak("State FSM_START");
//	//}
//
//	//g_SoundsQueue.setPlayWavFileCallback(ArSoundPlayer::getPlayWavFileCallback());
//	//g_SoundsQueue.setInterruptWavFileCallback(ArSoundPlayer::getStopPlayingCallback());
//
//	//g_SoundsQueue.runAsync();
//
//
//  // The robot object
//  ArRobot robot;
//
//  // Parse the command line arguments.
//  ArArgumentParser parser(&argc, argv);
//
//  // Set up our simpleConnector, to connect to the robot and laser
//  //ArSimpleConnector simpleConnector(&parser);
//  ArRobotConnector robotConnector(&parser, &robot);
//
//  // Connect to the robot
//  if (!robotConnector.connectRobot())
//  {
//    ArLog::log(ArLog::Normal, "Error: Could not connect to robot... exiting");
//    Aria::exit(3);
//  }
//
//
//
//  // Set up where we'll look for files. Arnl::init() set Aria's default
//  // directory to Arnl's default directory; addDirectories() appends this
//  // "examples" directory.
//  char fileDir[1024];
//  ArUtil::addDirectories(fileDir, sizeof(fileDir), Aria::getDirectory(), 
//			 "examples");
//  
//  
//  // To direct log messages to a file, or to change the log level, use these  calls:
//  //ArLog::init(ArLog::File, ArLog::Normal, "log.txt", true, true);
//  //ArLog::init(ArLog::File, ArLog::Verbose);
// 
//  // Add a section to the configuration to change ArLog parameters
//  ArLog::addToConfig(Aria::getConfig());
//
//  // set up a gyro (if the robot is older and its firmware does not
//  // automatically incorporate gyro corrections, then this object will do it)
//  ArAnalogGyro gyro(&robot);
//
//  // Our networking server
//  ArServerBase server;
//  
//
//  // Set up our simpleOpener, used to set up the networking server
//  ArServerSimpleOpener simpleOpener(&parser);
//
//  // the laser connector
//  ArLaserConnector laserConnector(&parser, &robot, &robotConnector);
//
//  // Tell the laser connector to always connect the first laser since
//  // this program always requires a laser.
//  parser.addDefaultArgument("-connectLaser");
//  
//  // Load default arguments for this computer (from /etc/Aria.args, environment
//  // variables, and other places)
//  parser.loadDefaultArguments();
//
//  // Parse arguments 
//  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
//  {
//    logOptions(argv[0]);
//    Aria::exit(1);
//  }
//  
//
//  // This causes Aria::exit(9) to be called if the robot unexpectedly
//  // disconnects
//  ArGlobalFunctor1<int> shutdownFunctor(&Aria::exit, 9);
//  robot.addDisconnectOnErrorCB(&shutdownFunctor);
//
//
//  // Create an ArSonarDevice object (ArRangeDevice subclass) and 
//  // connect it to the robot.
//  ArSonarDevice sonarDev;
//  robot.addRangeDevice(&sonarDev);
//
//
//
//  // This object will allow robot's movement parameters to be changed through
//  // a Robot Configuration section in the ArConfig global configuration facility.
//  ArRobotConfig robotConfig(&robot);
//
//  // Include gyro configuration options in the robot configuration section.
//  robotConfig.addAnalogGyro(&gyro);
//
//  // Start the robot thread.
//  robot.runAsync(true);
//  
//
//  // connect the laser(s) if it was requested, this adds them to the
//  // robot too, and starts them running in their own threads
//  if (!laserConnector.connectLasers())
//  {
//    ArLog::log(ArLog::Normal, "Could not connect to all lasers... exiting\n");
//    Aria::exit(2);
//  }
//
//  // find the laser we should use for localization and/or mapping,
//  // which will be the first laser
//  robot.lock();
//  ArLaser *firstLaser = robot.findLaser(1);
//  if (firstLaser == NULL || !firstLaser->isConnected())
//  {
//    ArLog::log(ArLog::Normal, "Did not have laser 1 or it is not connected, cannot start localization and/or mapping... exiting");
//    Aria::exit(2);
//  }
//  robot.unlock();
//
//
//    /* Create and set up map object */
//  
//  // Set up the map object, this will look for files in the examples
//  // directory (unless the file name starts with a /, \, or .
//  // You can take out the 'fileDir' argument to look in the program's current directory
//  // instead.
//  // When a configuration file is loaded into ArConfig later, if it specifies a
//  // map file, then that file will be loaded as the map.
//  ArMap map(fileDir);
//  // set it up to ignore empty file names (otherwise if a configuration omits
//  // the map file, the whole configuration change will fail)
//  map.setIgnoreEmptyFileName(true);
//  // ignore the case, so that if someone is using MobileEyes or
//  // MobilePlanner from Windows and changes the case on a map name,
//  // it will still work.
//  map.setIgnoreCase(true);
//
//    
//    /* Create localization and path planning threads */
//
//
//  ArPathPlanningTask pathTask(&robot, &sonarDev, &map);
//
//  ArLog::log(ArLog::Normal, "Creating laser localization task");
//  // Laser Monte-Carlo Localization
//  ArLocalizationTask locTask(&robot, firstLaser, &map);
//
//
//
//  // Set some options on each laser that the laser connector 
//  // connected to.
//  std::map<int, ArLaser *>::iterator laserIt;
//  for (laserIt = robot.getLaserMap()->begin();
//       laserIt != robot.getLaserMap()->end();
//       laserIt++)
//  {
//    int laserNum = (*laserIt).first;
//    ArLaser *laser = (*laserIt).second;
//
//    // Skip lasers that aren't connected
//    if(!laser->isConnected())
//      continue;
//
//    // add the disconnectOnError CB to shut things down if the laser
//    // connection is lost
//    laser->addDisconnectOnErrorCB(&shutdownFunctor);
//    // set the number of cumulative readings the laser will take
//    laser->setCumulativeBufferSize(200);
//    // add the lasers to the path planning task
//    pathTask.addRangeDevice(laser, ArPathPlanningTask::BOTH);
//    // set the cumulative clean offset (so that they don't all fire at once)
//    laser->setCumulativeCleanOffset(laserNum * 100);
//    // reset the cumulative clean time (to make the new offset take effect)
//    laser->resetLastCumulativeCleanTime();
//
//    // Add the packet count to the Aria info strings (It will be included in
//    // MobileEyes custom details so you can monitor whether the laser data is
//    // being received correctly)
//    std::string laserPacketCountName;
//    laserPacketCountName = laser->getName();
//    laserPacketCountName += " Packet Count";
//    Aria::getInfoGroup()->addStringInt(
//	    laserPacketCountName.c_str(), 10, 
//	    new ArRetFunctorC<int, ArLaser>(laser, 
//					 &ArLaser::getReadingCount));
//  }
//
//
//  // Used for optional multirobot features (see below) (TODO move to multirobot
//  // example?)
//  ArClientSwitchManager clientSwitch(&server, &parser);
//
//
//
//
//    /* Start the server */
//
//  // Open the networking server
//  if (!simpleOpener.open(&server, fileDir, 240))
//  {
//    ArLog::log(ArLog::Normal, "Error: Could not open server.");
//    exit(2);
//  }
//
//
//
//    /* Create various services that provide network access to clients (such as
//     * MobileEyes), as well as add various additional features to ARNL */
//
//
//  // ARNL can optionally get information about the positions of other robots from a
//  // "central server" (see central server example program), if command
//  // line options specifying the address of the central server was given.
//  // If there is no central server, then the address of each other robot
//  // can instead be given in the configuration, and the multirobot systems
//  // will connect to each robot (or "peer") individually.
//
//        // TODO move this to multirobot example?
//  ArServerHandlerMultiRobot *handlerMultiRobot = NULL;
//  ArMultiRobotRangeDevice *multiRobotRangeDevice = NULL;
//  ArServerHandlerMultiRobotPeer *multiRobotPeer = NULL;
//  ArMultiRobotPeerRangeDevice *multiRobotPeerRangeDevice = NULL;
//
//
//  bool usingCentralServer = false;
//  if(clientSwitch.getCentralServerHostName() != NULL)
//    usingCentralServer = true;
//
//  // if we're using the central server then we want to create the
//  // multiRobot central classes
//  if (usingCentralServer)
//  {
//    // Make the handler for multi robot information (this sends the
//    // information to the central server)
//    handlerMultiRobot = new ArServerHandlerMultiRobot(&server, &robot, 
//						      &pathTask,
//						      &locTask, &map);
//    
//    // Normally each robot, and the central server, must all have
//    // the same map name for the central server to share robot
//    // information.  (i.e. they are operating in the same space).
//    // This changes the map name that ArServerHandlerMutliRobot 
//    // reports to the central server, in case you want this individual
//    // robot to load a different map file name, but still report 
//    // the common map file to the central server.
//    //handlerMultiRobot->overrideMapName("central.map");
//
//    // the range device that gets the multi robot information from
//    // the central server and presents it as virtual range readings
//    // to ARNL
//    multiRobotRangeDevice = new ArMultiRobotRangeDevice(&server);
//    
//    robot.addRangeDevice(multiRobotRangeDevice);
//    pathTask.addRangeDevice(multiRobotRangeDevice, 
//			    ArPathPlanningTask::BOTH);
//    
//    // Set up options for drawing multirobot information in MobileEyes.
//    multiRobotRangeDevice->setCurrentDrawingData(
//	    new ArDrawingData("polyDots", ArColor(125, 125, 0),
//			      100, 73, 1000), true);
//    multiRobotRangeDevice->setCumulativeDrawingData(
//	    new ArDrawingData("polyDots", ArColor(125, 0, 125),
//			      100, 72, 1000), true);
//
//    // This sets up the localization to use the known poses of other robots
//    // for its localization in cases where numerous robots crowd out the map.
//    locTask.setMultiRobotCallback(multiRobotRangeDevice->getOtherRobotsCB());
//  }
//  // if we're not using a central server then create the multirobot peer classes
//  else
//  {
//    // set the path planning so it uses the explicit collision range for how far its planning
//    pathTask.setUseCollisionRangeForPlanningFlag(true);
//    // make our thing that gathers information from the other servers
//    multiRobotPeerRangeDevice = new ArMultiRobotPeerRangeDevice(&map);
//    // make our thing that sends information to the other servers
//    multiRobotPeer = new ArServerHandlerMultiRobotPeer(&server, &robot, 
//						     &pathTask, &locTask);
//    // hook the two together so they both know what priority this robot is
//    multiRobotPeer->setNewPrecedenceCallback(
//	    multiRobotPeerRangeDevice->getSetPrecedenceCallback());
//    // hook the two together so they both know what priority this
//    // robot's fingerprint is
//    multiRobotPeer->setNewFingerprintCallback(
//	    multiRobotPeerRangeDevice->getSetFingerprintCallback());
//    // hook the two together so that the range device can call on the
//    // server handler to change its fingerprint
//    multiRobotPeerRangeDevice->setChangeFingerprintCB(
//	    multiRobotPeer->getChangeFingerprintCB());
//    // then add the robot to the places it needs to be
//    robot.addRangeDevice(multiRobotPeerRangeDevice);
//    pathTask.addRangeDevice(multiRobotPeerRangeDevice, 
//			    ArPathPlanningTask::BOTH);
//
//    // Set the range device so that we can see the information its using
//    // to avoid, you can comment these out in order to not see them
//    multiRobotPeerRangeDevice->setCurrentDrawingData(
//	    new ArDrawingData("polyDots", ArColor(125, 125, 0),
//			      100, 72, 1000), true);
//    multiRobotPeerRangeDevice->setCumulativeDrawingData(
//	    new ArDrawingData("polyDots", ArColor(125, 0, 125),
//			      100, 72, 1000), true);
//    // This sets up the localization to use the known poses of other robots
//    // for its localization in cases where numerous robots crowd out the map.
//    locTask.setMultiRobotCallback(
//	    multiRobotPeerRangeDevice->getOtherRobotsCB());
//  }
//
//
//
//
//  /* Add additional range devices to the robot and path planning task (so it
//     avoids obstacles detected by these devices) */
//  
//  // Add IR range device to robot and path planning task (so it avoids obstacles
//  // detected by this device)
//  robot.lock();
//  ArIRs irs;
//  robot.addRangeDevice(&irs);
//  pathTask.addRangeDevice(&irs, ArPathPlanningTask::CURRENT);
//
//  // Add bumpers range device to robot and path planning task (so it avoids obstacles
//  // detected by this device)
//  ArBumpers bumpers;
//  robot.addRangeDevice(&bumpers);
//  pathTask.addRangeDevice(&bumpers, ArPathPlanningTask::CURRENT);
//
//  // Add range device which uses forbidden regions given in the map to give virtual
//  // range device readings to ARNL.  (so it avoids obstacles
//  // detected by this device)
//  ArForbiddenRangeDevice forbidden(&map);
//  robot.addRangeDevice(&forbidden);
//  pathTask.addRangeDevice(&forbidden, ArPathPlanningTask::CURRENT);
//
//  robot.unlock();
//
//
//  // Action to slow down robot when localization score drops but not lost.
//  ArActionSlowDownWhenNotCertain actionSlowDown(&locTask);
//  pathTask.getPathPlanActionGroup()->addAction(&actionSlowDown, 140);
//
//  // Action to stop the robot when localization is "lost" (score too low)
//  ArActionLost actionLostPath(&locTask, &pathTask);
//  pathTask.getPathPlanActionGroup()->addAction(&actionLostPath, 150);
//
//  // Arnl uses this object when it must replan its path because its
//  // path is completely blocked.  It will use an older history of sensor
//  // readings to replan this new path.  This should not be used with SONARNL
//  // since sonar readings are not accurate enough and may prevent the robot
//  // from planning through space that is actually clear.
//  ArGlobalReplanningRangeDevice replanDev(&pathTask);
//
//  
//  // Service to provide drawings of data in the map display :
//  ArServerInfoDrawings drawings(&server);
//  drawings.addRobotsRangeDevices(&robot);
//  drawings.addRangeDevice(&replanDev);
//
//  /* Draw a box around the local path planning area use this 
//    (You can enable this particular drawing from custom commands 
//    which is set up down below in ArServerInfoPath) */
//  ArDrawingData drawingDataP("polyLine", ArColor(200,200,200), 1, 75);
//  ArFunctor2C<ArPathPlanningTask, ArServerClient *, ArNetPacket *> 
//    drawingFunctorP(&pathTask, &ArPathPlanningTask::drawSearchRectangle);
//  drawings.addDrawing(&drawingDataP, "Local Plan Area", &drawingFunctorP); 
//
//  /* Show the sample points used by MCL */
//  ArDrawingData drawingDataL("polyDots", ArColor(0,255,0), 100, 75);
//  ArFunctor2C<ArLocalizationTask, ArServerClient *, ArNetPacket *> 
//    drawingFunctorL(&locTask, &ArLocalizationTask::drawRangePoints);
//  drawings.addDrawing(&drawingDataL, "Localization Points", &drawingFunctorL);
//
//
//  // "Custom" commands. You can add your own custom commands here, they will
//  // be available in MobileEyes' custom commands (enable in the toolbar or
//  // access through Robot Tools)
//  ArServerHandlerCommands commands(&server);
//  
// 
//  
// // Add our custom command. ArServerHandlerCommands also has other methods
//  // for adding commands taht take different kinds of arguments, or no
//  // arguments.
// 
//
//
//  // These provide various kinds of information to the client:
//  ArServerInfoRobot serverInfoRobot(&server, &robot);
//  ArServerInfoSensor serverInfoSensor(&server, &robot);
//  ArServerInfoPath serverInfoPath(&server, &robot, &pathTask);
//  serverInfoPath.addSearchRectangleDrawing(&drawings);
//  serverInfoPath.addControlCommands(&commands);
//
//  // Provides localization info and allows the client (MobileEyes) to relocalize at a given
//  // pose:
//  ArServerInfoLocalization serverInfoLocalization(&server, &robot, &locTask);
//  ArServerHandlerLocalization serverLocHandler(&server, &robot, &locTask);
//
//  // If you're using MobileSim, ArServerHandlerLocalization sends it a command
//  // to move the robot's true pose if you manually do a localization through 
//  // MobileEyes.  To disable that behavior, use this constructor call instead:
//  // ArServerHandlerLocalization serverLocHandler(&server, &robot, true, false);
//  // The fifth argument determines whether to send the command to MobileSim.
//
//  // Provide the map to the client (and related controls):
//  ArServerHandlerMap serverMap(&server, &map);
//
//  // These objects add some simple (custom) commands to 'commands' for testing and debugging:
//  ArServerSimpleComUC uCCommands(&commands, &robot);                   // Send any command to the microcontroller
//  ArServerSimpleComMovementLogging loggingCommands(&commands, &robot); // configure logging
//  ArServerSimpleComLogRobotConfig configCommands(&commands, &robot);   // trigger logging of the robot config parameters
////  ArServerSimpleServerCommands serverCommands(&commands, &server);     // monitor networking behavior (track packets sent etc.)
//
//
//  // service that allows the client to monitor the communication link status
//  // between the robot and the client.
//  //
//  ArServerHandlerCommMonitor handlerCommMonitor(&server);
//
//
//
//  // service that allows client to change configuration parameters in ArConfig 
//  ArServerHandlerConfig handlerConfig(&server, Aria::getConfig(),
//				      Arnl::getTypicalDefaultParamFileName(),
//				      Aria::getDirectory());
//
//
//
//  /* Set up the possible modes for remote control from a client such as
//   * MobileEyes:
//   */
//
//  // Mode To go to a goal or other specific point:
//  ArServerModeGoto modeGoto(&server, &robot, &pathTask, &map,
//			    locTask.getRobotHome(),
//			    locTask.getRobotHomeCallback());
//
//   // Mode To go to a goal or other specific point:
//  ArServerMyMode myMode(&server, &robot,&locTask, &pathTask, &map, 300, 100);
// 
// 
//	//ArServerModeBidon myModeBidon(&server, &robot);
//	//myModeBidon.addControlCommands(&commands);
// 
//
//
//  // Mode To stop and remain stopped:
//  ArServerModeStop modeStop(&server, &robot);
//
//  // Cause the sonar to turn off automatically
//  // when the robot is stopped, and turn it back on when commands to move
//  // are sent. (Note, if using SONARNL to localize, then don't do this
//  // since localization may get lost)
//  ArSonarAutoDisabler sonarAutoDisabler(&robot);
//
//  // Teleoperation modes To drive by keyboard, joystick, etc:
//  ArServerModeRatioDrive modeRatioDrive(&server, &robot);  
////  ArServerModeDrive modeDrive(&server, &robot);            // Older mode for compatability
//
//
//
//  // Prevent normal teleoperation driving if localization is lost using
//  // a high-priority action, which enables itself when the particular mode is
//  // active.
//  // (You have to enter unsafe drive mode to drive when lost.)
//  ArActionLost actionLostRatioDrive(&locTask, &pathTask, &modeRatioDrive);
//  modeRatioDrive.getActionGroup()->addAction(&actionLostRatioDrive, 110);
//
//  // Add drive mode section to the configuration, and also some custom (simple) commands:
//  modeRatioDrive.addToConfig(Aria::getConfig(), "Teleop settings");
//  modeRatioDrive.addControlCommands(&commands);
//
//  // Wander mode (also prevent wandering if lost):
//  ArServerModeWander modeWander(&server, &robot);
//  ArActionLost actionLostWander(&locTask, &pathTask, &modeWander);
//  modeWander.getActionGroup()->addAction(&actionLostWander, 110);
//
//
//  // This provides a small table of interesting information for the client
//  // to display to the operator. You can add your own callbacks to show any
//  // data you want.
//  ArServerInfoStrings stringInfo(&server);
//  Aria::getInfoGroup()->addAddStringCallback(stringInfo.getAddStringFunctor());
//  
//  // Provide a set of informational data (turn on in MobileEyes with
//  // View->Custom Details)
//
//  Aria::getInfoGroup()->addStringInt(
//	  "Motor Packet Count", 10, 
//	  new ArConstRetFunctorC<int, ArRobot>(&robot, 
//					       &ArRobot::getMotorPacCount));
//
//  Aria::getInfoGroup()->addStringDouble(
//	  "Laser Localization Score", 8, 
//	  new ArRetFunctorC<double, ArLocalizationTask>(
//		  &locTask, &ArLocalizationTask::getLocalizationScore),
//	  "%.03f");
//  Aria::getInfoGroup()->addStringInt(
//	  "Laser Loc Num Samples", 8, 
//	  new ArRetFunctorC<int, ArLocalizationTask>(
//		  &locTask, &ArLocalizationTask::getCurrentNumSamples),
//	  "%4d");
//
//
//  // Display gyro status if gyro is enabled and is being handled by the firmware (gyro types 2, 3, or 4).
//  // (If the firmware detects an error communicating with the gyro or IMU it
//  // returns a flag, and stops using it.)
//  // (This gyro type parameter, and fault flag, are only in ARCOS, not Seekur firmware)
//  if(robot.getOrigRobotConfig() && robot.getOrigRobotConfig()->getGyroType() > 1)
//  {
//    Aria::getInfoGroup()->addStringString(
//          "Gyro/IMU Status", 10,
//          new ArGlobalRetFunctor1<const char*, ArRobot*>(&getGyroStatusString, &robot)
//      );
//  }
//  
//   ArServerModeSupply modeSupply(&server, &robot);
//  //modeSupply.addAsDefaultMode();
//  //modeSupply
//
//   ArServerModeDeliver modeDeliver(&server, &robot);
//   //modeDeliver.addAsDefaultMode();
//  
//  // Setup the dock if there is a docking system on board.
//  ArServerModeDock *modeDock = NULL;
//  modeDock = ArServerModeDock::createDock(&server, &robot, &locTask, 
//					  &pathTask);
//  if (modeDock != NULL)
//  {
//    modeDock->checkDock();
//    modeDock->addAsDefaultMode();
//    modeDock->addToConfig(Aria::getConfig());
//    modeDock->addControlCommands(&commands);
//  }
//
//
//
//
//  // Make Stop mode the default (If current mode deactivates without entering
//  // a new mode, then Stop Mode will be selected)
//  modeStop.addAsDefaultMode();
//    // TODO move up near where stop mode is created?
//
//
//
//
//
//  /* Services that allow the client to initiate scanning with the laser to
//     create maps in Mapper3 (So not possible with SONARNL): */
//
//  ArServerHandlerMapping handlerMapping(&server, &robot, firstLaser, 
//					fileDir, "", true);
//
//  // make laser localization stop while mapping
//  handlerMapping.addMappingStartCallback(
//	  new ArFunctor1C<ArLocalizationTask, bool>
//	  (&locTask, &ArLocalizationTask::setIdleFlag, true));
//
//  // and then make it start again when we're doine
//  handlerMapping.addMappingEndCallback(
//	  new ArFunctor1C<ArLocalizationTask, bool>
//	  (&locTask, &ArLocalizationTask::setIdleFlag, false));
//
//
//  // Make it so our "lost" actions don't stop us while mapping
//  handlerMapping.addMappingStartCallback(actionLostPath.getDisableCB());
//  handlerMapping.addMappingStartCallback(actionLostRatioDrive.getDisableCB());
//  handlerMapping.addMappingStartCallback(actionLostWander.getDisableCB());
//
//  // And then let them make us stop as usual when done mapping
//  handlerMapping.addMappingEndCallback(actionLostPath.getEnableCB());
//  handlerMapping.addMappingEndCallback(actionLostRatioDrive.getEnableCB());
//  handlerMapping.addMappingEndCallback(actionLostWander.getEnableCB());
//
//  // don't let forbidden lines show up as obstacles while mapping
//  // (they'll just interfere with driving while mapping, and localization is off anyway)
//  handlerMapping.addMappingStartCallback(forbidden.getDisableCB());
//
//  // let forbidden lines show up as obstacles again as usual after mapping
//  handlerMapping.addMappingEndCallback(forbidden.getEnableCB());
//
//
//  /*
//  // If we are on a simulator, move the robot back to its starting position,
//  // and reset its odometry.
//  // This will allow localizeRobotAtHomeBlocking() below will (probably) work (it
//  // tries current odometry (which will be 0,0,0) and all the map
//  // home points.
//  // (Ignored by a real robot)
//  //robot.com(ArCommands::SIM_RESET);
//  */
//
//
//  // create a pose storage class, this will let the program keep track
//  // of where the robot is between runs...  after we try and restore
//  // from this file it will start saving the robot's pose into the
//  // file
//  ArPoseStorage poseStorage(&robot);
//  /// if we could restore the pose from then set the sim there (this
//  /// won't do anything to the real robot)... if we couldn't restore
//  /// the pose then just reset the position of the robot (which again
//  /// won't do anything to the real robot)
//  if (poseStorage.restorePose("robotPose"))
//    serverLocHandler.setSimPose(robot.getPose());
//  else
//    robot.com(ArCommands::SIM_RESET);
//
//
//
//  /* File transfer services: */
//  
//#ifdef WIN32
//  // Not implemented for Windows yet.
//  ArLog::log(ArLog::Normal, "Note, file upload/download services are not implemented for Windows; not enabling them.");
//#else
//  // This block will allow you to set up where you get and put files
//  // to/from, just comment them out if you don't want this to happen
//  // /*
//  ArServerFileLister fileLister(&server, fileDir);
//  ArServerFileToClient fileToClient(&server, fileDir);
//  ArServerFileFromClient fileFromClient(&server, fileDir, "/tmp");
//  ArServerDeleteFileOnServer deleteFileOnServer(&server, fileDir);
//  // */
//#endif
//
//    /* Video image streaming, and camera controls (Requires SAVserver or ACTS) */
//
//  // Forward any video if either ACTS or SAV server are running.
//  // You can find out more about SAV and ACTS on our website
//  // http://robots.activmedia.com. ACTS is for color tracking and is
//  // a seperate product. SAV just does software A/V transmitting and is
//  // free to all our customers. Just run ACTS or SAV server before you
//  // start this program and this class here will forward video from the
//  // server to the client.
//  ArHybridForwarderVideo videoForwarder(&server, "localhost", 7070);
//  
//  // make a camera to use in case we have video. the camera collection collects
//  // multiple ptz cameras 
//  ArPTZ *camera = NULL;
//  ArServerHandlerCamera *handlerCamera = NULL;
//  ArCameraCollection *cameraCollection = NULL;
//
//  // if we have video then set up a camera 
//  if (videoForwarder.isForwardingVideo())
//  {
//
//    cameraCollection = new ArCameraCollection();
//    cameraCollection->addCamera("Cam1", "VCC4", "Camera", "VCC4");
//
//    videoForwarder.setCameraName("Cam1");
//    videoForwarder.addToCameraCollection(*cameraCollection);
//
//    bool invertedCamera = false;
//    camera = new ArVCC4(&robot,	invertedCamera, 
//			                  ArVCC4::COMM_UNKNOWN, true, true);
//    camera->init();
//
//    handlerCamera = new ArServerHandlerCamera("Cam1", 
//		                                           &server, 
//					                                     &robot,
//					                                     camera, 
//					                                     cameraCollection);
//
//    pathTask.addGoalFinishedCB(
//	    new ArFunctorC<ArServerHandlerCamera>(
//		    handlerCamera, 
//		    &ArServerHandlerCamera::cameraModeLookAtGoalClearGoal));
//  }
//
//  // After all of the cameras / videos have been created and added to the collection,
//  // then start the collection server.
//  //
//  if (cameraCollection != NULL) {
//    new ArServerHandlerCameraCollection(&server, cameraCollection);
//  }
//
//
//
//
//    /* Load configuration values, map, and begin! */
//
//  
//  // When parsing the configuration file, also look at the program's command line options 
//  // from the command-line argument parser as well as the configuration file.
//  // (So you can use any argument on the command line, namely -map.) 
//  Aria::getConfig()->useArgumentParser(&parser);
//
//  // Read in parameter files.
//  ArLog::log(ArLog::Normal, "Loading config file %s into ArConfig...", Arnl::getTypicalParamFileName());
//  if (!Aria::getConfig()->parseFile(Arnl::getTypicalParamFileName()))
//  {
//    ArLog::log(ArLog::Normal, "Trouble loading configuration file, exiting");
//    Aria::exit(5);
//  }
//
//  // Warn about unknown params.
//  if (!simpleOpener.checkAndLog() || !parser.checkHelpAndWarnUnparsed())
//  {
//    logOptions(argv[0]);
//    Aria::exit(6);
//  }
//
//  // Warn if there is no map
//  if (map.getFileName() == NULL || strlen(map.getFileName()) <= 0)
//  {
//    ArLog::log(ArLog::Normal, "");
//    ArLog::log(ArLog::Normal, "### No map file is set up, you can make a map with the following procedure");
//    ArLog::log(ArLog::Normal, "   0) You can find this information in README.txt or docs/Mapping.txt");
//    ArLog::log(ArLog::Normal, "   1) Connect to this server with MobileEyes");
//    ArLog::log(ArLog::Normal, "   2) Go to Tools->Map Creation->Start Scan");
//    ArLog::log(ArLog::Normal, "   3) Give the map a name and hit okay");
//    ArLog::log(ArLog::Normal, "   4) Drive the robot around your space (see docs/Mapping.txt");
//    ArLog::log(ArLog::Normal, "   5) Go to Tools->Map Creation->Stop Scan");
//    ArLog::log(ArLog::Normal, "   6) Start up Mapper3");
//    ArLog::log(ArLog::Normal, "   7) Go to File->Open on Robot");
//    ArLog::log(ArLog::Normal, "   8) Select the .2d you created");
//    ArLog::log(ArLog::Normal, "   9) Create a .map");
//    ArLog::log(ArLog::Normal, "  10) Go to File->Save on Robot");
//    ArLog::log(ArLog::Normal, "  11) In MobileEyes, go to Tools->Robot Config");
//    ArLog::log(ArLog::Normal, "  12) Choose the Files section");
//    ArLog::log(ArLog::Normal, "  13) Enter the path and name of your new .map file for the value of the Map parameter.");
//    ArLog::log(ArLog::Normal, "  14) Press OK and your new map should become the map used");
//    ArLog::log(ArLog::Normal, "");    
//  }
//
//  // Print a log message notifying user of the directory for map files
//  ArLog::log(ArLog::Normal, "");
//  ArLog::log(ArLog::Normal, 
//	     "Directory for maps and file serving: %s", fileDir);
//  
//  ArLog::log(ArLog::Normal, "See the ARNL README.txt for more information");
//  ArLog::log(ArLog::Normal, "");
//
//  // Do an initial localization of the robot. It tries all the home points
//  // in the map, as well as the robot's current odometric position, as possible
//  // places the robot is likely to be at startup.   If successful, it will
//  // also save the position it found to be the best localized position as the
//  // "Home" position, which can be obtained from the localization task (and is
//  // used by the "Go to home" network request).
//  locTask.localizeRobotAtHomeBlocking();
//  
//  // Let the client switch manager (for multirobot) spin off into its own thread
//  // TODO move to multirobot example?
//  clientSwitch.runAsync();
//
//  // Start the networking server's thread
//  server.runAsync();
//
//
//  // Add a key handler so that you can exit by pressing
//  // escape. Note that this key handler, however, prevents this program from
//  // running in the background (e.g. as a system daemon or run from 
//  // the shell with "&") -- it will lock up trying to read the keys; 
//  // remove this if you wish to be able to run this program in the background.
//  ArKeyHandler *keyHandler;
//  if ((keyHandler = Aria::getKeyHandler()) == NULL)
//  {
//    keyHandler = new ArKeyHandler;
//    Aria::setKeyHandler(keyHandler);
//    robot.lock();
//    robot.attachKeyHandler(keyHandler);
//    robot.unlock();
//    puts("Server running. To exit, press escape.");
//  }
//
//  // Enable the motors and wait until the robot exits (disconnection, etc.) or this program is
//  // canceled.
//  robot.enableMotors();
//  robot.waitForRunExit();
//
//  //while(true);
// 
//  Aria::exit(0);
//}

