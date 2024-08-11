/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/jobGenericJobs.h"

class CTemplateLoadBalancer : public CJobLoadResource::CCallback
{
	friend class CTemplateLoadRequest;
public:
	struct Priority
	{
		enum Base
		{
			PRI_IGNORED					= -1,
			PRI_LOW						= 0,
			PRI_MED						= 1,
			PRI_HIGH					= 2,
			PRI_COUNT					= 3
			
		}				m_basePriority;
		Float			m_orderingPriority;

		void			SetZero()													{ m_basePriority = PRI_IGNORED; m_orderingPriority = -FLT_MAX; }
		void			Maximize( const Priority& p )								{ m_basePriority = Max( m_basePriority, p.m_basePriority ); m_orderingPriority = Max( m_orderingPriority, p.m_orderingPriority ); }
		Bool			IsIgnored() const											{ return m_basePriority == PRI_IGNORED; }

		Bool			operator<( const Priority& p ) const						{ return m_orderingPriority < p.m_orderingPriority; }

	};
	
	typedef Uint64 TemplateId;
	typedef Uint32 RequestId;
	typedef TSoftHandle< CEntityTemplate > TemplateHandle;

	static const TemplateId INVALID_TEMPLATE_ID = 0xffffffffffffffff;
	static const RequestId INVALID_REQUEST_ID = 0xffffffff;
	static const Priority IGNORED_PRIORITY;

protected:
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;
	

	struct STemplateLoadInfo
	{
		TemplateHandle	m_templateHandle;
		Priority		m_priority;
		Int16			m_requestsCount;
		Bool			m_loaded;
		Bool			m_failed;
		Bool			m_isBeingLoaded;
	};

	struct SRequest
	{
		TemplateId		m_templateId;
		Priority		m_priority;
	};

	struct RunningJobInfo
	{
		CJobLoadResource*						m_job;
		TemplateId								m_templateId;

		RunningJobInfo();

		void									Clear()															{ m_job = nullptr; }
		void									Set( CJobLoadResource* job, TemplateId templateId )				{ m_job = job; m_templateId = templateId; }

		Bool									IsEmpty() const													{ return m_job == nullptr; }
	};

	CMutex										m_mutex;
	THashMap< TemplateId, STemplateLoadInfo >	m_templates;
	THashMap< RequestId, SRequest >				m_requests;
	RunningJobInfo								m_runningJob[ Priority::PRI_COUNT ];								// We hold table of jobs, in order to never get blocked with major case by a job of minor importance (as it could wait a lot for processing)
	RequestId									m_nextRequestId;												// Unique request id factory
	Bool										m_jobsLocked;													// NOTICE: Its modified only from main thread. Tested from both. It may be touched (turned on) without a full mutex lock.
	Bool										m_requestQueueDirty;											// Modified only in critical section. Tested asynchronously, also without full lock.

	// Internal interface. Used only inside critical section.
	TemplateId			ComputeTemplateId( const String& templatePath );
	void				ClearRequest( CTemplateLoadRequest& request );

	RED_INLINE Bool		CanRunJob() const																		{ return m_requestQueueDirty == true && m_jobsLocked == false && m_runningJob[ Priority::PRI_HIGH ].IsEmpty() && !m_templates.Empty(); }
	void				RunJob();

public:
	struct SRequestInfo
	{
		Bool			m_isLoaded;
		Bool			m_isInvalid;
	};

	CTemplateLoadBalancer();
	virtual ~CTemplateLoadBalancer();
	
	void				BreakProcessing();

	void				OnSerialize( IFile& file );

	// Main external interface. Impose critical section.
	SRequestInfo		RegisterTemplateRequest( CTemplateLoadRequest& request, Priority priority = IGNORED_PRIORITY );
	SRequestInfo		UpdateTemplateRequest( CTemplateLoadRequest& request, Priority priority )				{ return RegisterTemplateRequest( request, priority ); }
	void				UnregisterTemplateRequest( CTemplateLoadRequest& request );
	SRequestInfo		ForceTemplateRequest( CTemplateLoadRequest& request );									// synchronous forceful interface

	// Job spawn locking mechanism. Used when system is gathering the requests, so we would fire the best one fit - not the first one hit. NOTICE: if not in a mood for spawning a job, its not locking a thing.
	void				LockJobSpawning();
	void				UnlockJobSpawning();

	// Loading resource callback
	virtual void		OnJobFinished( CJobLoadResource* job, EJobResult result ) override;

	// debug stuff
	void				OnGenerateDebugFragments( CRenderFrame* frame );

	// set of custom functions with hardcoded priorities heuristics
	static Priority		ActorPriority( Bool canSpawn, Int32 group, Float distance );
	static Float		PriorityUpdateDelay()																	{ return 1.f; }
};

class CTemplateLoadRequest : public Red::System::NonCopyable
{
	friend class CTemplateLoadBalancer;
	DECLARE_RTTI_STRUCT( CTemplateLoadRequest )
protected:
	CTemplateLoadBalancer::RequestId				m_requestId;
	CTemplateLoadBalancer::TemplateId				m_templateId;
	CTemplateLoadBalancer::TemplateHandle			m_templateHandle;
public:
	CTemplateLoadRequest()
		: m_requestId( CTemplateLoadBalancer::INVALID_REQUEST_ID )
		, m_templateId( CTemplateLoadBalancer::INVALID_TEMPLATE_ID )												{}

	~CTemplateLoadRequest()																						{ Clear(); }

	CEntityTemplate*	GetEntityTemplate()																		{ RED_FATAL_ASSERT( m_templateHandle.IsLoaded(), "LoadBalancer err 06: requested not-loaded template" ); return m_templateHandle.Get(); }

	void				Initialize( const CTemplateLoadBalancer::TemplateHandle& templateHandle );
	void				Initialize( const String& templatePath )												{ if ( m_templateHandle.IsEmpty() ) { m_templateHandle = templatePath; } RED_FATAL_ASSERT( m_templateHandle.GetPath() == templatePath, "LoadBalancer err 07: template path<->request inconsistency" ); }
	Bool				IsRequesting()																			{ return m_requestId != CTemplateLoadBalancer::INVALID_REQUEST_ID; }

	void				Clear();
};

BEGIN_NOCOPY_CLASS_RTTI( CTemplateLoadRequest )
END_CLASS_RTTI()
