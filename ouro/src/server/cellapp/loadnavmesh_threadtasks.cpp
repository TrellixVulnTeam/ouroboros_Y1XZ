// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

#include "spacememory.h"	
#include "spacememorys.h"	
#include "loadnavmesh_threadtasks.h"
#include "server/serverconfig.h"
#include "common/deadline.h"
#include "navigation/navigation.h"

namespace Ouroboros{

//-------------------------------------------------------------------------------------
bool LoadNavmeshTask::process()
{
	Navigation::getSingleton().loadNavigation(resPath_, params_);
	return false;
}

//-------------------------------------------------------------------------------------
thread::TPTask::TPTaskState LoadNavmeshTask::presentMainThread()
{
	NavigationHandlePtr pNavigationHandle = Navigation::getSingleton().findNavigation(resPath_);
	
	SpaceMemory* pSpace = SpaceMemorys::findSpace(spaceID_);
	if(pSpace == NULL || !pSpace->isGood())
	{
		ERROR_MSG(fmt::format("LoadNavmeshTask::presentMainThread(): not found space({})\n",
			spaceID_));
	}
	else
	{
		pSpace->onLoadedSpaceGeometryMapping(pNavigationHandle);
	}
	
	return thread::TPTask::TPTASK_STATE_COMPLETED; 
}

//-------------------------------------------------------------------------------------
}
