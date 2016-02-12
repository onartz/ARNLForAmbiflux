//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif

#include "LecteurCarteTask.h"

//Constructeur

LecteurCarteTask::LecteurCarteTask():
uid_len(12),myStatusCode(STATUS_IDLE),
myPoolingStatus(false),
myErrorCode(ERR_NOERROR),
myWarningCode(WARN_NOWARNING),
myTimeout(DEFAULT_TIMEOUT)
{		
	strcpy_s(device,"");
}

LecteurCarteTask::LecteurCarteTask(ArFunctor1<char*>* functor):
uid_len(12),myStatusCode(STATUS_IDLE),
myPoolingStatus(false),
myErrorCode(ERR_NOERROR),
myWarningCode(WARN_NOWARNING),
myCardReadCB(functor),
myTimeout(DEFAULT_TIMEOUT)
{		
	strcpy_s(device,"");
}

LecteurCarteTask::~LecteurCarteTask(){
}

void LecteurCarteTask::setCardReadCB(ArFunctor1<char*>* funct){
myCardReadCB = funct;
}

//Connexion au lecteur et ouverture du lecteur
int LecteurCarteTask::open()
{
	ArLog::log(ArLog::Verbose,"Ouverture lecteur carte");
  /* Open reader */ 
	rc=SPROX_ReaderOpenA(device);
	if (rc != MI_OK)
	{
		myErrorCode=ERR_NOTFOUND;
		myStatusCode=STATUS_IDLE;
		return(ERR_NOTFOUND);
	}
	SPROX_ControlLedY(1,0,0);
	
	/*SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);
	SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);
	SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);
	SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);*/

	//SPROX_ControlLedY(1,0,0);
	
	/* Configure reader in ISO 14443-A mode */
	rc = SPROX_SetConfig(CFG_MODE_ISO_14443_A);
	if (rc != MI_OK)
	{
		myErrorCode=ERR_CONFIGURATIONFAILED;	
		close();
		return(ERR_CONFIGURATIONFAILED);
	}
	/*Ca a marche*/
	//SPROX_ControlLedY(0,0,1);
	myStatusCode=STATUS_OPEN;
	ArLog::log(ArLog::Verbose,"Lecteur Ouvert");
	return(ERR_NOERROR);	
}


void LecteurCarteTask::close()
{
	/*SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);
	SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);
	SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);
	SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);*/
	//for(int i=0;i<12;i++)
		//uid[i]=0x00;
	SPROX_ControlLedY(0,0,0);
	SPROX_ReaderClose();
	myStatusCode=STATUS_CLOSED;
}

int LecteurCarteTask::read(long timeout)
{
	SPROX_ControlLedY(0,1,0);
	myTimeout = timeout;
	return(read());
}


void *LecteurCarteTask::runThread(void *arg)
{
	myRunning = true;
	int res = -1;
	while (myRunning==true)
	{
		res = read();
		ArUtil::sleep(1000);

	}

  // return out here, means the thread is done
  return NULL;
}

void LecteurCarteTask::stopRunning(){
	ArASyncTask::stopRunning();
	ArLog::log(ArLog::Normal, "Card reader stopped");
}

int LecteurCarteTask::read()
{
	myErrorCode = ERR_NOERROR;
	ArTime myStartTime;
	myStartTime.setToNow();
	//SPROX_ControlLedY(0, 1, 0);

	while(myStartTime.mSecSince() <= myTimeout)
	{	
		rc = MI_OK;	
		uid_len = sizeof(uid);
		//Lecture carte
		rc = SPROX_A_SelectIdle(atq, uid, &uid_len, sak);
		switch(rc)
		{
			//Pas de carte
			case MI_NOTAGERR:
			{
				myErrorCode = ERR_NOTFOUND;
				break;
			}
			//Une carte
			case MI_OK:
			{
				/* Halt currently active tag */
				rc = SPROX_A_Halt();
				if(rc==MI_OK)
				{
					myErrorCode = ERR_NOERROR;
					SPROX_ControlLedY(0,0,1);
					SPROX_ControlRF(FALSE);
					
					sprintf(myCardId,"%02X%02X%02X%02X\0",uid[3], uid[2], uid[1], uid[0]);
					//invocatiton de la function
					myCardReadCB->invoke(myCardId);
					
					return(CARD);
				}
				break;
			}
			/*default:
				{
				}*/
		}
		ArUtil::sleep(100);
	}
	SPROX_ControlRF(FALSE);
	return(NOCARD);
}
	





WORD LecteurCarteTask::getErrorCode()
{
	return myErrorCode;
}
WORD LecteurCarteTask::getWarningCode()
{
	return myWarningCode;
}
int LecteurCarteTask::getStatusCode()
{
	return myStatusCode;
}