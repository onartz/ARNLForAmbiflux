#ifndef DEF_JSONPARSER_H
#define DEF_JSONPARSER_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;


using namespace std;


class JSONParser{	

public:
	//Return ptree from string
	static ptree JSONParser::parse(string);

};

#endif