#include "DALRest.h"

using namespace std;

DALRest::DALRest(){
}

DALRest::DALRest(ArFunctor1<char*> *responseCB, ArFunctor * connectFailedCB):
myResponseCB(responseCB),
myConnectFailedCB(connectFailedCB)
{
}

void *DALRest::runThread(void *arg)
{
	//Buffer de réception
	char buff[512];
	const char *ptr=NULL;
	 // The size of the string the server sent
	size_t strSize;
	ArSocket sock;
	
	if(!(sock.connect(WCFHOSTIPADDRESS,WCFSERVICEPORT,ArSocket::TCP,0)))
	{
		myConnectFailedCB->invoke();
		return NULL;
	}
	string response="";
	//Envoi de la requete
	sock.write(myRequest,sizeof(myRequest));

	//lecture de la réponse
	if(strcmp(sock.readString(),"HTTP/1.1 200 OK")!=0)
	{
			sock.close();
			ArLog::log(ArLog::Verbose, "socketClientExample: Server error: \r\n%s", buff);
			myConnectFailedCB->invoke();
			return NULL;

	}

	//Réponse OK, on lit ce qui arrive 
	while((strSize=sock.read(buff,sizeof(buff)-1))>0)
	{
		buff[strSize]='\0';		
		response+=string(buff);
	}

	//Si réponse vide
	ptr=strstr(response.c_str(),"\r\n\r\n");
	
	if(ptr==NULL)
	{
		sock.close();
		myConnectFailedCB->invoke();
		return NULL;
	}

	char *ptrFin=strstr(&response[0],"}HTTP");
	if(ptrFin!=NULL)
		*(ptrFin+sizeof(char))='\0';

	std::string s(ptr+sizeof("\r\n\r\n")-1);
	sock.close();
    char *cstr = new char[s.length() + 1];
	strcpy(cstr, s.c_str());

    myResponseCB->invoke(cstr);
	return NULL;

}


void DALRest::sendRequest(ArFunctor1<char*> *funct,string requestType, int param)
{
	//Conversion int to string
	string s = static_cast<ostringstream*>( &(ostringstream() << param) )->str();
	sendRequest(funct, requestType, s);
	//return(sendRequest(requestType, s));
}

void DALRest::sendRequest(string requestType, int param)
{
	//Conversion int to string
	string s = static_cast<ostringstream*>( &(ostringstream() << param) )->str();
	sendRequest(requestType, s);
}
void DALRest::sendRequest(string requestType, string param)
{
	//Création de la requête
	sprintf(myRequest,"GET %s%s/%s HTTP/1.1\r\nHost: %s\r\n\r\n",SERVICE,requestType.c_str(),param.c_str(),HOSTWEBADDRESS);
	runAsync();
}

DALRest::~DALRest(){

}
void DALRest::sendRequest(ArFunctor1<char*> *funct, string requestType, string param)
{
	//Création de la requête
	sprintf(myRequest,"GET %s%s/%s HTTP/1.1\r\nHost: %s\r\n\r\n",SERVICE,requestType.c_str(),param.c_str(),HOSTWEBADDRESS);
	runAsync();
	
}

void DALRest::setRequest(char * value){
	strcpy(myRequest, value);
	//myRequest = value;
}

//string DALRest::getWorkorders(Workorder::WORKORDER_STATUS status)
//{
//	string str = sendRequest("workorders",int(status));
//	return(str);
//}


//string DALRest::getEmployeeByCardId(string cardId)
//{
//	return(sendRequest("employeeByCardId",cardId));
//}

//string DALRest::getResourceById(string id)
//{
//	return(sendRequest("resource",id));
//}
//
//void DALRest::updateWorkorder(Workorder* w){
//	char buf[256];
//	sprintf(buf, "%d/%d",w->getWorkorderId(), w->getStatusId());
//	string str = sendRequest("updateWorkorder", string(buf) );
//
//}
//
//void DALRest::updateWorkorderRouting(WorkorderRouting * wor){
//	char buf[256];
//	sprintf(buf, "%s/%d/%d",wor->getWorkorderRoutingNo(), wor->getStatusId(), wor->getStateId());
//	string str = sendRequest("updateWorkorderRouting", string(buf) );
//	
//}

