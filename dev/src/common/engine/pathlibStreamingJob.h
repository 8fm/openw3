/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibStreamingItem.h"
#include "../core/loadingJob.h"

namespace PathLib
{

class CStreamingManager;

///////////////////////////////////////////////////////////////////////////////
// Job loading a set of Areas
class CStreamingJob : public ILoadJob
{
	friend class CStreamingManager;
protected:
	IStreamingItem::List::Head						m_requests;
	Red::Threads::CAtomic< Bool >					m_shutdownRequest;
	CStreamingManager*								m_streamingManager;
public:
	CStreamingJob( IStreamingItem::List::Head&& requestsList, CStreamingManager* streamingManager );
	//! Process the job
	EJobResult Process() override;

	void PreLoadSync();
	void ShutdownRequest();

	virtual const Char* GetDebugName() const override;
};



};				// namespace PathLib