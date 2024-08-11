/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibAreaProcessingJob.h"

namespace PathLib
{


#ifndef NO_EDITOR_PATHLIB_SUPPORT
class CObstacleSimplificationProcessingThread : public CAreaProcessingJob
{
	typedef CAreaProcessingJob Super;
public:
	CObstacleSimplificationProcessingThread( CAreaDescription* area );

	virtual Bool					ProcessPathLibTask() override;

	virtual void					DescribeTask( String& task ) override;
};
#endif

};		// namespace PathLib
