#include "JSONParser.h"

using namespace std;

ptree JSONParser::parse(string str)
{
	ptree pt;
	std::istringstream is(str);
	std::string res;
	try
	{
		read_json(is, pt);
	}
	catch(std::exception const&  ex)
	{
		printf("JSON Error. %s", ex.what());
	}
	return pt;
}

