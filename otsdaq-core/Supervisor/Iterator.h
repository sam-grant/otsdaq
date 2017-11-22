#ifndef _ots_Iterator_h
#define _ots_Iterator_h


#include "otsdaq-core/ConfigurationPluginDataFormats/IterateConfiguration.h"
#include "otsdaq-core/XmlUtilities/HttpXmlDocument.h"
#include <mutex>        //for std::mutex
#include <string>

namespace ots
{

class Supervisor;
class ConfigurationManagerRW;

class Iterator
{
	friend class Supervisor;
	
public:
    Iterator (Supervisor* supervisor);
    ~Iterator(void);

    void									playIterationPlan			(HttpXmlDocument& xmldoc, const std::string& planName);
    void									pauseIterationPlan			(HttpXmlDocument& xmldoc);
    void									haltIterationPlan			(HttpXmlDocument& xmldoc);
    void									getIterationPlanStatus		(HttpXmlDocument& xmldoc);

    bool									handleCommandRequest		(HttpXmlDocument& xmldoc, const std::string& command, const std::string& parameter);

private:

    struct IteratorWorkLoopStruct
	{
    	IteratorWorkLoopStruct(Iterator* iterator, ConfigurationManagerRW* cfgMgr)
    	:theIterator_			(iterator)
    	,cfgMgr_			(cfgMgr)
    	,running_			(false)
    	,commandBusy_		(false)
    	,commandIndex_		((unsigned int)-1)
    	{}

    	Iterator *									theIterator_;
    	ConfigurationManagerRW* 					cfgMgr_;

    	bool 										running_, commandBusy_;

    	std::string 								activePlan_;
    	std::vector<IterateConfiguration::Command> 	commands_;
    	unsigned int 								commandIndex_;

    	//associated with FSM
    	std::string 								fsmName_, fsmRunAlias_;
    	unsigned int 								fsmNextRunNumber_;


	};

    static void								IteratorWorkLoop			(Iterator *iterator);
    static void								startCommand				(IteratorWorkLoopStruct *iteratorStruct);
    static bool								checkCommand				(IteratorWorkLoopStruct *iteratorStruct);
    static void								startCommandChooseFSM		(IteratorWorkLoopStruct *iteratorStruct, std::string fsmName);
    static void								startCommandConfigureAlias	(IteratorWorkLoopStruct *iteratorStruct, std::string systemAlias);
    static bool								checkCommandConfigureAlias	(IteratorWorkLoopStruct *iteratorStruct);

    std::mutex								accessMutex_;
    volatile bool							workloopRunning_;
    volatile bool							activePlanIsRunning_;
    volatile bool							iteratorBusy_;
    volatile bool							commandPlay_, commandPause_, commandHalt_; //commands are set by supervisor thread, and cleared by iterator thread
    std::string								activePlanName_, lastStartedPlanName_, lastFinishedPlanName_;
    volatile unsigned int					activeCommandIndex_;
    volatile time_t							activeCommandStartTime_;

    Supervisor* 							theSupervisor_;
};

}

#endif
