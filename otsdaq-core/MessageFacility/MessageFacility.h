#ifndef OTSDAQ_CORE_MESSAGEFACILITY_MESSAGEFACILITY_H
#define OTSDAQ_CORE_MESSAGEFACILITY_MESSAGEFACILITY_H

#include <messagefacility/MessageLogger/MessageLogger.h>
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"

namespace ots
{

inline void INIT_MF(const char* name)
{
	char* logRootString = getenv("OTSDAQ_LOG_ROOT");
	if (logRootString == nullptr)
	{

		__COUT_ERR__ << "\n**********************************************************" << std::endl;
		__COUT_ERR__ << "WARNING: OTSDAQ_LOG_ROOT environment variable was not set!" << std::endl;
		__COUT_ERR__ << "**********************************************************\n" << std::endl;;
		//exit(0);
	}
	else
		setenv("ARTDAQ_LOG_ROOT", logRootString, 1);

	char* logFhiclCode  = getenv("OTSDAQ_LOG_FHICL");
	if (logFhiclCode == nullptr)
	{
		__COUT_ERR__ << "\n***********************************************************" << std::endl;
		__COUT_ERR__ << "WARNING: OTSDAQ_LOG_FHICL environment variable was not set!" << std::endl;
		__COUT_ERR__ << "***********************************************************\n" << std::endl;
		//exit(0);
	}
	else
		setenv("ARTDAQ_LOG_FHICL", logFhiclCode, 1);

	artdaq::configureMessageFacility(name,true,true);
	artdaq::setMsgFacAppName(name,0);
}


}

#endif
