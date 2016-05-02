/// @author Cedric Boudinet
/// @license GNU GPL Version 3
///
/// Distributed under the GNU GPL version 3 License
/// (See accompanying file LICENSE or copy at
/// http://www.gnu.org/licenses/)
///

#include "iec61850Exp_fun.h"
#include <iostream>
#include <algorithm>
std::vector<std::string> getLDList(IedConnection con)
{
	IedClientError error;
	std::vector<std::string> devices;
	LinkedList deviceList = IedConnection_getLogicalDeviceList(con, &error);
	LinkedList device=LinkedList_getNext(deviceList);
	while(device!=NULL)
	{
		devices.push_back((char*)device->data);
		device=LinkedList_getNext(device);
	}
	LinkedList_destroy(deviceList);
	return devices;
}

std::vector<std::string> getLNList(IedConnection con, const std::string & parentDevice)
{
	IedClientError error;
	std::vector<std::string> nodes;
	LinkedList logicalNodes = IedConnection_getLogicalDeviceDirectory(con, &error, parentDevice.c_str());

	LinkedList logicalNode = LinkedList_getNext(logicalNodes);
	while(logicalNode!=NULL)
	{
		nodes.push_back((char*)logicalNode->data);
		logicalNode = LinkedList_getNext(logicalNode);
	}
	LinkedList_destroy(logicalNodes);
	return nodes;
}

std::vector<std::string> getLNVars(IedConnection con, const std::string & parentLogicalNode)
{
	IedClientError error;
	std::vector<std::string> objects;
	LinkedList LNobjects = IedConnection_getLogicalNodeVariables(con, &error, parentLogicalNode.c_str());
	LinkedList LNobject = LinkedList_getNext(LNobjects);
	while(LNobject!=NULL)
	{
		objects.push_back((char*)LNobject->data);
		LNobject = LinkedList_getNext(LNobject);
	}
	LinkedList_destroy(LNobjects);
	return objects;
}

bool getVariableName(IedConnection con, const std::string & LNVarName, const std::string & parentDevice, const std::string & parentNode, std::string &varName, FunctionalConstraint &fc)
{
	varName = LNVarName;
	std::size_t found =varName.find("$");
	if(found!=std::string::npos)
	{
		std::string fcstr;
		fcstr=varName.substr(0,found);
		varName=varName.substr(found+1);
		std::replace(varName.begin(), varName.end(), '$', '.');
		varName = parentDevice+'/'+parentNode+'.'+varName;
		fc = FunctionalConstraint_fromString(fcstr.c_str());
		return true;
	}
	return false;
}

void dispLNVar(IedConnection con, const std::string & LNVarName, const std::string & parentDevice, const std::string & parentNode)
{
	//std::cout<<"  + "<<LNVarName<<std::endl;
	std::string varName; FunctionalConstraint fc;
	if(getVariableName(con, LNVarName, parentDevice, parentNode, varName, fc))
	{
		IedClientError error;
		char buffer[100];
		MmsValue* my_mms = IedConnection_readObject(con, &error, varName.c_str(), fc);
		std::cout<<"  + "<<varName<<" ["<<FunctionalConstraint_toString(fc)<<"]:";
		if(error!=0)
			std::cout<<"Error "<<error;
		std::cout<<"(type "<<MmsValue_getTypeString(my_mms)<<")= "<<MmsValue_printToBuffer(my_mms, buffer, 100);
		std::cout<<std::endl;
	}
}

std::map<std::string, FunctionalConstraint> getVariableList(IedConnection con)
{
	std::map<std::string, FunctionalConstraint> listVar;
	std::string varName;
	FunctionalConstraint fc;
	std::vector<std::string> devices = getLDList(con);
	for(std::vector<std::string>::iterator itLD=devices.begin();itLD<devices.end();itLD++)
	{
		std::vector<std::string> nodes = getLNList(con, (*itLD));
		for(std::vector<std::string>::iterator itLN=nodes.begin();itLN<nodes.end();itLN++)
		{
			std::vector<std::string> dataObjects=getLNVars(con, (*itLD)+"/"+(*itLN));
			for(std::vector<std::string>::iterator itDO=dataObjects.begin();itDO<dataObjects.end();itDO++)
			{
				//std::cout<<"  MMS : "<<(*itDO)<<std::endl;
				//dispLNVar(con, 
				if(getVariableName(con,*itDO, *itLD, *itLN, varName, fc))
					listVar[varName]=fc;
			}
		}
	}
	return listVar;
}
