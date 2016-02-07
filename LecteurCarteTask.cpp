//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif

#include "LecteurCarteTask.h"

//Constructeur

LecteurCarteTask::LecteurCarteTask():
uid_len(12),myStatusCode(STATUS_IDLE),myPoolingStatus(false),myErrorCode(ERR_NOERROR),myWarningCode(WARN_NOWARNING)
{		
	strcpy_s(device,"");
	//myCardReadCB->invoke(23);
}
LecteurCarteTask::LecteurCarteTask(ArFunctor1<int>* functor):
uid_len(12),myStatusCode(STATUS_IDLE),myPoolingStatus(false),myErrorCode(ERR_NOERROR),myWarningCode(WARN_NOWARNING),
myCardReadCB(functor)
//myCardReadCB(this, &ArServerModeSupply::handleCardRead)
{		
	strcpy_s(device,"");
	//myCardReadCB->invoke(23);
}

LecteurCarteTask::~LecteurCarteTask(){
}

void LecteurCarteTask::setCardReadCB(ArFunctor1<int>* funct){
myCardReadCB = funct;
}

void LecteurCarteTask::invoke(){
	myCardReadCB->invoke(23);

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
	ArUtil::sleep(100);
	SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);
	
	/* Configure reader in ISO 14443-A mode */
	rc = SPROX_SetConfig(CFG_MODE_ISO_14443_A);
	if (rc != MI_OK)
	{
		myErrorCode=ERR_CONFIGURATIONFAILED;	
		close();
		return(ERR_CONFIGURATIONFAILED);
	}
	/*Ca a marche*/
	SPROX_ControlLedY(0,0,1);
	myStatusCode=STATUS_OPEN;
	ArLog::log(ArLog::Verbose,"Lecteur Ouvert");
	return(ERR_NOERROR);	
}


void LecteurCarteTask::close()
{
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
	ArUtil::sleep(100);
	SPROX_ControlLedY(1,1,1);
	ArUtil::sleep(100);
	SPROX_ControlLedY(0,0,0);
	ArUtil::sleep(100);
	//for(int i=0;i<12;i++)
		//uid[i]=0x00;
	SPROX_ReaderClose();
	myStatusCode=STATUS_CLOSED;
}

int LecteurCarteTask::read(long timeout)
{
	myTimeout = timeout;
	return(read());
}


void *LecteurCarteTask::runThread(void *arg)
{
	int res = -1;
	int i = 0;
	while (myRunning==true)
	{
		res = read();
		ArUtil::sleep(2000);
		myCardReadCB->invoke(i);

	}

  // return out here, means the thread is done
  return NULL;
}


int LecteurCarteTask::read()
{
	//BYTE * bufCardId;
	myErrorCode = ERR_NOERROR;
	ArTime myStartTime;
	myStartTime.setToNow();
	SPROX_ControlLedY(0, 1, 0);
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
					SPROX_ControlLedY(1,0,0);
					SPROX_ControlRF(FALSE);
					//memcpy(bufCardId,uid,5);
					sprintf(myStrCardId,"%02X%02X%02X%02X\0",uid[3], uid[2], uid[1], uid[0]);	
					//invocatiton de la function avec un entier
					myCardReadCB->invoke(23);
					//myFunc2->invoke(23);
	
					//for(int i=0;i<4;i++)
						//bufCardId[i]=uid[i];
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
		///* une carte a été trouvée */
		//if (rc != MI_NOTAGERR)
		//{
		//	/* Carte Mais pb de lecture */
		//	if (rc != MI_OK) 
		//	{
		//		printf("Pb de lecture\n");
		//		/* Other error */
		//		myWarningCode=WARN_INVALIDCARD;
		//		if (rc < -128)
		//			goto done; 
		//		else 
		//			goto close;
		//	}
		//	myWarningCode=WARN_NOWARNING;
		//	
		//	/* Halt currently active tag */
		//	rc = SPROX_A_Halt();
		//	if (rc != MI_OK)
		//	{
		//		printf("Warning\n");
		//		myWarningCode=WARN_INVALIDCARD;
		//		//strcpy_s(myWarningMessage,"Carte non reconnue Halt");
		//		if (rc < -128)
		//		goto close;           /* Fatal error */
		//	}
		//	//On signale l evenement cartelue
		//	carteLue.signal();
		//	myMutex.unlock();
		//	break;
		//	}
		//else
		//{
		//	myMutex.unlock();
		//}	
		
	
	//close:
	//{
	//	/*Fin de la boucle*/
	//	/* Red LED on */
	//	SPROX_ControlLedY(1,0,0);	

	//	myMutex.unlock();		
	//	stopPooling();
	//	myPoolingStatus=false;
	//}




//Accesseurs
char * LecteurCarteTask::getCardId()
{
	return myStrCardId;
}

//char * LecteurCarte::getStatus()
//{
//	return myStatus;
//}
//
//char * LecteurCarte::getErrorMessage()
//{
//	return myErrorMessage;
//}
//char * LecteurCarte::getWarningMessage()
//{
//	return myWarningMessage;
//}


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