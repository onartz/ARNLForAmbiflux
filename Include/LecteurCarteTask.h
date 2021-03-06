#ifndef DEF_LECTEURCARTETASK_H // Si la constante n'a pas �t� d�finie le fichier n'a jamais �t� inclus
#define DEF_LECTEURCARTETASK_H // On d�finit la constante pour que la prochaine fois le fichier ne soit plus inclus

//Obligatoire pour ne pas inclure winsock.h
//#include <WinSock2.h> 
#include "ArAsyncTask.h"
#include "AriaUtil.h"
#include "springprox.h"

#define DEFAULT_TIMEOUT 4000

class LecteurCarteTask : public ArASyncTask{
public :
	enum errorCodes
	{
		ERR_NOERROR=0, ERR_NOTFOUND=1, ERR_CONFIGURATIONFAILED=2
	};
	enum warningCodes
	{
		WARN_NOWARNING=0, WARN_INVALIDCARD=1, WARN_NOCARD = 2
	};
	enum statusCodes
	{
		STATUS_IDLE, STATUS_OPEN, STATUS_CLOSED
	};
	enum resultat
	{
		NOCARD, CARD
	};

	//Constructor
	LecteurCarteTask();
	LecteurCarteTask(ArFunctor1<char*>*);
	virtual ~LecteurCarteTask();
	virtual void * runThread(void *arg);
	void lockMutex();
	void unlockMutex();
	int open();
	//void invoke();
    void close();
    int read(long);
    int read();

  //Override base methods
  void stopRunning();
 
  WORD getErrorCode();
  WORD getWarningCode();
  int getStatusCode();
  
  //char* getCardId(); //Pointeur sur BYTE

  //void setCardReadCB(ArFunctor1<char*> *);
  //Permet � une classe appelante de s'abonner � l'�venement
  void setCardReadCB(ArFunctor1<char*>*);
  //void cardRead(char*);

protected :
	//Status
	//char myStatus[256];	
	resultat myResultat;
	//std::string myStrCardId;
	int myStatusCode;
	//Code Erreur
	WORD myErrorCode;
	//Code Warning
	WORD myWarningCode;
	//Timeout lecture en ms
	long myTimeout;
	//Message erreur
	//char myErrorMessage[256];
	//Message warning
	//char myWarningMessage[256];
	//Resultat de la lecture (4 premiers bytes de uid)
	//BYTE myCardId[4];

	char myCardId[5];
	//Etat du pooling
	bool myPoolingStatus;
	//Correspondances code erreur - message
	/*Error myErrorTable[2];*/
	SWORD rc;
	int i;
	char s_buffer[64];
	BOOL bench;
	//clock_t t0, t1;
	BYTE data[240];
	BYTE atq[2];
	BYTE sak[1];
	BYTE uid[12];
	BYTE uid_len;
	BYTE sect_count, sect;
	BYTE bloc_count, bloc;
	BYTE offset;
	char device[64]; 

	//Un pointeur sur une siganture de fonction qui prend un entier comme param�tre
	//doit �tre initialis�e par la classe qui s'abonne � cet event
	ArFunctor1<char*>* myCardReadCB;

};
#endif
