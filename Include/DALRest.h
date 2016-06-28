#ifndef DEF_DALREST_H // Si la constante n'a pas été définie le fichier n'a jamais été inclus
#define DEF_DALREST_H // On définit la constante pour que la prochaine fois le fichier ne soit plus inclus

#define _WINSOCKAPI_ 

#include "ArSocket.h"
#include "ArASyncTask.h"
#include <sstream>

using namespace std;

//#define HOST	193.55.104.245	//aip-sqlaipl
#define WCFSERVICEPORT	4567
#define WCFHOSTIPADDRESS	"100.74.39.204"
#define HOSTWEBADDRESS	"100.74.39.204"
#define HOSTURL		"http://100.74.39.204"
//#define SERVICE	"/AmbifluxWcfService/Ambiflux.svc/json/"
#define SERVICE	"/Ambiflux.svc/json/"
#define REQUEST	"srma"
#define REQ_LOGSRMA	"logSRMA"
#define REQ_WORKORDERS	"workorders"

#define TIMEOUT	1000

class DALRest : public ArASyncTask
{
public:
	DALRest();
	DALRest(ArFunctor1<char*>* responseCB, ArFunctor* connectFailedCB);
	virtual ~DALRest();
	virtual void * runThread(void *arg);
	void lockMutex();
	void unlockMutex();
	void setRequest(char*);
	void sendRequest(string req, int param);
	void sendRequest(string req, string param);
	void sendRequest(ArFunctor1<char*> *, string req, int param);
	void sendRequest(ArFunctor1<char*> *, string req, string param);

private :
	//Send request to the server
	
	ArFunctor1<char*> *myResponseCB;
	ArFunctor *myConnectFailedCB;
	char myRequest[128];
};

#endif