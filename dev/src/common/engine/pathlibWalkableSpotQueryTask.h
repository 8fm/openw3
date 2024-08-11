/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibTaskManager.h"
#include "pathlibWalkableSpotQueryRequest.h"

namespace PathLib
{

class CWalkableSpotQueryTask : public CTaskManager::CAsyncTask
{
	typedef CTaskManager::CAsyncTask Super;
protected:

	CWalkableSpotQueryRequest::Ptr			m_request;

	virtual void							Process() override;
	virtual void							PostProcessingSynchronous() override;

	Bool									PerformQuery();
public:
	CWalkableSpotQueryTask( CTaskManager& taskManager, CWalkableSpotQueryRequest* request );

	virtual void							DescribeTask( String& outName ) const override;
};

};