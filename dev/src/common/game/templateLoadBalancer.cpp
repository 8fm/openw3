/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "templateLoadBalancer.h"

#include "../core/depot.h"

const CTemplateLoadBalancer::Priority CTemplateLoadBalancer::IGNORED_PRIORITY =
{
	Priority::PRI_IGNORED, -1.f
};

IMPLEMENT_ENGINE_CLASS( CTemplateLoadRequest )


///////////////////////////////////////////////////////////////////////////////
// CTemplateLoadBalancer::RunningJobInfo
///////////////////////////////////////////////////////////////////////////////
CTemplateLoadBalancer::RunningJobInfo::RunningJobInfo()
	: m_job( nullptr )
	, m_templateId( INVALID_TEMPLATE_ID )
{

}

///////////////////////////////////////////////////////////////////////////////
// CTemplateLoadBalancer
///////////////////////////////////////////////////////////////////////////////
CTemplateLoadBalancer::CTemplateLoadBalancer()
	: m_nextRequestId( 1 )
	, m_jobsLocked( false )
	, m_requestQueueDirty( false )
{

}
CTemplateLoadBalancer::~CTemplateLoadBalancer()
{
	RED_ASSERT( m_templates.Empty(), TXT("LoadBalancer err 08: templates leaking") );
	RED_ASSERT( m_requests.Empty(), TXT("LoadBalancer err 09: requests leaking") );
}


CTemplateLoadBalancer::TemplateId CTemplateLoadBalancer::ComputeTemplateId( const String& templatePath )
{
	return Red::System::CalculatePathHash64( templatePath.AsChar() );
}

void CTemplateLoadBalancer::ClearRequest( CTemplateLoadRequest& request )
{
	auto it = m_requests.Find( request.m_requestId );
	if ( it != m_requests.End() )
	{
		SRequest& r = it->m_second;
		auto itTemplate = m_templates.Find( r.m_templateId );
		RED_ASSERT( itTemplate != m_templates.End(), TXT("LoadBalancer err 00: template<->requests inconsistency") );
		STemplateLoadInfo& templateInfo = itTemplate->m_second;
		// decrease template ref count
		if ( --templateInfo.m_requestsCount <= 0 )
		{
			// remove obsolate template loading request
			m_templates.Erase( itTemplate );
		}

		m_requests.Erase( it );
	}
	request.m_requestId = INVALID_REQUEST_ID;
}

void CTemplateLoadBalancer::RunJob()
{
	// Step 1. Get best template to spawn

	// 1.1. Clear template priorities
	Bool hasPendingTemplates = false;
	for ( auto it = m_templates.Begin(), end = m_templates.End(); it != end; ++it )
	{
		STemplateLoadInfo& t = it->m_second;
		t.m_priority.SetZero();
		if ( !t.m_loaded && !t.m_failed && !t.m_isBeingLoaded )
		{
			hasPendingTemplates = true;
		}
	}
	// detect situation when we have nothing to do - until further requests
	if ( !hasPendingTemplates )
	{
		m_requestQueueDirty = false;
		return;
	}

	// 1.2. Compute priorites based on requests (r * hashfind)
	for ( auto it = m_requests.Begin(), end = m_requests.End(); it != end; ++it )
	{
		const SRequest& r = it->m_second;
		m_templates.FindPtr( r.m_templateId )->m_priority.Maximize( r.m_priority );
	}

	// 1.3 Find template
	STemplateLoadInfo* bestTemplate = nullptr;
	TemplateId bestTemplateId = INVALID_TEMPLATE_ID;
	for ( auto it = m_templates.Begin(), end = m_templates.End(); it != end; ++it )
	{
		STemplateLoadInfo& t = it->m_second;
		if ( !t.m_loaded && !t.m_failed && !t.m_isBeingLoaded &&
			( !bestTemplate || bestTemplate->m_priority < t.m_priority ) )
		{
			bestTemplate = &t;
			bestTemplateId = it->m_first;
		}
	}

	// possibly no 
	if ( !bestTemplate )
	{
		m_requestQueueDirty = false;
		return;
	}

	Priority::Base requestPriority = bestTemplate->m_priority.m_basePriority;
	// 2. Look if we can spawn a job for template. Its possible that we are processing job of minor importance, and template also has minor importance
	for( Int32 i = Priority::PRI_HIGH; i >= requestPriority; --i )
	{
		if ( !m_runningJob[ i ].IsEmpty() )
		{
			// sorry, not this time.
			m_requestQueueDirty = false;
			return;
		}
	}

	bestTemplate->m_isBeingLoaded = true;

	// 3. Spawn job
	EJobPriority jobPriority =
		( requestPriority == Priority::PRI_HIGH )
		? JP_Immediate : JP_Resources;

	EResourceLoadingPriority resourceLoadingPriority =
		( requestPriority == Priority::PRI_HIGH )
		? eResourceLoadingPriority_High : eResourceLoadingPriority_Normal;


	CJobLoadResource* job = new CJobLoadResource( bestTemplate->m_templateHandle.GetPath(), jobPriority, false, nullptr, resourceLoadingPriority, this );

	m_runningJob[ requestPriority ].Set( job, bestTemplateId );

	SJobManager::GetInstance().Issue( job );
}

void CTemplateLoadBalancer::BreakProcessing()
{
	m_mutex.Acquire();
	// lock processing not to start any new jobs (eg. because of shutdown order issues we could have some pending requests)
	m_jobsLocked = true;
	// finish up all Ur jobs
	for( Int32 i = Priority::PRI_HIGH; i >= 0; )
	{
		if ( !m_runningJob[ i ].IsEmpty() )
		{
			m_mutex.Release();
			Red::Threads::SleepOnCurrentThread( 1 );
			m_mutex.Acquire();
			continue;
		}

		--i;
	}

	m_jobsLocked = false;

	m_mutex.Release();
}

void CTemplateLoadBalancer::OnSerialize( IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		// full lock
		CScopedLock lock( m_mutex );

		for ( auto it = m_templates.Begin(), end = m_templates.End(); it != end; ++it )
		{
			STemplateLoadInfo& templateInfo = it->m_second;
			templateInfo.m_templateHandle.CollectForGC( file );
		}
	}
}

CTemplateLoadBalancer::SRequestInfo CTemplateLoadBalancer::RegisterTemplateRequest( CTemplateLoadRequest& request, Priority priority )
{
	// default function output
	SRequestInfo out;
	out.m_isLoaded = false;
	out.m_isInvalid = false;

	STemplateLoadInfo* t;

	// full lock
	CScopedLock lock( m_mutex );

	// check if request was already submitted
	if ( request.m_requestId != INVALID_REQUEST_ID )
	{
		// request already in processing
		SRequest* r = m_requests.FindPtr( request.m_requestId );
		RED_ASSERT( r, TXT("Load balancer er 01: Broken consistency with CTemplateLoadRequest and SRequest") );

		if ( !priority.IsIgnored() )
		{
			r->m_priority = priority;
		}

		t = m_templates.FindPtr( r->m_templateId );
		RED_ASSERT( t, TXT("Load balancer er 02: Broken consistency with SRequest and STemplateLoadInfo") );
	}
	else
	{
		// new request - check if template is already in procesing
		if ( request.m_templateId == INVALID_TEMPLATE_ID )
		{
			request.m_templateId = ComputeTemplateId( request.m_templateHandle.GetPath() );
			if ( request.m_templateId == INVALID_TEMPLATE_ID )
			{
				out.m_isInvalid = true;
				return out;
			}
		}

		// create new request and store it
		SRequest* r;
		{
			RequestId id = m_nextRequestId++;
			request.m_requestId = id;

			SRequest def;
			def.m_templateId = request.m_templateId;
			def.m_priority = priority;

			r = &m_requests.GetRef( id, def );
		}

		// check if the template is already loaded
		t = m_templates.FindPtr( r->m_templateId );
		if ( !t )
		{
			STemplateLoadInfo info;
			info.m_templateHandle = request.m_templateHandle;
			info.m_priority = priority;
			info.m_requestsCount = 0;
			info.m_loaded = false;
			info.m_failed = false;
			info.m_isBeingLoaded = false;
			t = &m_templates.GetRef( r->m_templateId, info );
			// test if template is already loaded
			CResource* depotResource = GDepot->FindResource( request.m_templateHandle.GetPath() );
			if ( depotResource )
			{
				if ( CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( depotResource ) )
				{
					request.m_templateHandle = entityTemplate;
					info.m_templateHandle = entityTemplate;

					info.m_loaded = true;
				}
			
			}

		}
		// add ref
		++t->m_requestsCount;
	}

	// process return value based on template info
	if ( t->m_loaded )
	{
		out.m_isLoaded = true;
		request.m_templateHandle = t->m_templateHandle;
	}
	out.m_isInvalid = t->m_failed;

	m_requestQueueDirty = true;
	// run job if our current request had a chance to be available
	if ( !out.m_isLoaded && !out.m_isInvalid && !t->m_isBeingLoaded && priority.m_basePriority != Priority::PRI_IGNORED && CanRunJob() )
	{
		RunJob();
	}
	return out;
}

void CTemplateLoadBalancer::UnregisterTemplateRequest( CTemplateLoadRequest& request )
{
	if ( request.m_requestId != INVALID_REQUEST_ID )
	{
		// full lock
		CScopedLock lock( m_mutex );

		ClearRequest( request );
	}
}
CTemplateLoadBalancer::SRequestInfo CTemplateLoadBalancer::ForceTemplateRequest( CTemplateLoadRequest& request )
{
	SRequestInfo out = RegisterTemplateRequest( request );
	
	if ( !out.m_isLoaded && !out.m_isInvalid )
	{
		CEntityTemplate* entityTemplate = request.m_templateHandle.Get();

		// modify template base
		{
			CScopedLock lock( m_mutex );
			STemplateLoadInfo* t = m_templates.FindPtr( request.m_templateId );
			// !force load
			if ( entityTemplate )
			{
				t->m_loaded = true;
				t->m_templateHandle = entityTemplate;
				out.m_isLoaded = true;
			}
			else
			{
				t->m_failed = true;
				out.m_isInvalid = true;
			}
		}
	}
	return out;
}

void CTemplateLoadBalancer::LockJobSpawning()
{
	// NOTICE: no lock
	m_jobsLocked = true;
}

void CTemplateLoadBalancer::UnlockJobSpawning()
{
	// NOTICE: no lock
	m_jobsLocked = false;
	if ( CanRunJob() )
	{
		// full lock once we have something to spawn
		CScopedLock lock( m_mutex );
		if ( CanRunJob() )
		{
			RunJob();
		}
	}
}

void CTemplateLoadBalancer::OnJobFinished( CJobLoadResource* job, EJobResult result )
{
	CScopedLock lock( m_mutex );

	Uint32 i = 0;
	while ( m_runningJob[ i ].m_job != job )
	{
		++i;
		RED_ASSERT( i < Priority::PRI_COUNT, TXT("Job not recognized!") );
	}

	m_requestQueueDirty = true;

	STemplateLoadInfo* t = m_templates.FindPtr( m_runningJob[ i ].m_templateId );
	// its actually possible for template to be out of that list - in that instance it means its not requested anymore
	if ( t )
	{
		t->m_isBeingLoaded = false;
		// Extract loaded template
		CEntityTemplate* entityTemplate = ::Cast< CEntityTemplate >( job->GetResource() );
		if ( !entityTemplate )
		{
			t->m_failed = true;
		}
		else
		{
			t->m_loaded = true;
			t->m_templateHandle = entityTemplate;
		}
	}
	
	m_runningJob[ i ].m_job = nullptr;
	job->Release();

	m_requestQueueDirty = true;
	if ( CanRunJob() )
	{
		RunJob();
	}
}

void CTemplateLoadBalancer::OnGenerateDebugFragments( CRenderFrame* frame )
{
	struct T
	{
		Float					m_order;
		TemplateId				m_templateId;

		Bool					operator<( const T& t ) const { return m_order > t.m_order; }
	};
	static TSortedArray< T > list;

	Int32 x = 512;
	Int32 y = 130;


	CScopedLock lock( m_mutex );

	String text = String::Printf( TXT("Templates: %d requests: %d"), m_templates.Size(), m_requests.Size() );
	frame->AddDebugScreenText( x, y, text, Color::WHITE );
	y += 24;

	list.ResizeFast( m_templates.Size() );

	Uint32 i = 0;

	for ( auto it = m_templates.Begin(), end = m_templates.End(); it != end; ++it )
	{
		TemplateId templateId = it->m_first;
		STemplateLoadInfo& info = it->m_second;

		list[ i ].m_order =
			info.m_priority.m_orderingPriority
			+ ( (!info.m_loaded && !info.m_failed) ? 10000.f : 0.f )
			+ ( info.m_failed ? 1000.f : 0.f );

		list[ i ].m_templateId = templateId;

		++i;
	}

	list.Sort();

	for ( Uint32 i = 0; i < list.Size(); ++i )
	{
		auto it = m_templates.Find( list[ i ].m_templateId );

		TemplateId templateId = it->m_first;
		STemplateLoadInfo& info = it->m_second;

		Color textColor;
		switch ( info.m_priority.m_basePriority )
		{
		case Priority::PRI_HIGH:
			textColor = Color::LIGHT_BLUE;
			break;
		case Priority::PRI_MED:
			textColor = Color::BLUE;
			break;
		case Priority::PRI_LOW:
			textColor = Color::DARK_BLUE;
			break;
		default:
		case Priority::PRI_IGNORED:
			textColor = Color::GRAY;
			break;
		}

		if ( info.m_loaded )
		{
			textColor = Color::WHITE;
		}
		else if ( info.m_failed )
		{
			textColor = Color::MAGENTA;
		}
		else if ( info.m_isBeingLoaded )
		{
			textColor = Color::LIGHT_RED;
		}
		

		text = String::Printf( TXT("Requests: %d template: %s"), info.m_requestsCount, info.m_templateHandle.GetPath().AsChar() );

		frame->AddDebugScreenText( x, y, text, textColor );

		y += 17;
	}
}

CTemplateLoadBalancer::Priority CTemplateLoadBalancer::ActorPriority( Bool canSpawn, Int32 group, Float distance )
{
	Priority p;
	p.m_basePriority =
		canSpawn
		? ( ( group < 3 )
		? Priority::PRI_HIGH
		: Priority::PRI_MED )
		: ( ( group < 2 )
		? Priority::PRI_MED
		: Priority::PRI_LOW );
	p.m_orderingPriority =
		10.f
		- distance * ( 10.f / 128.f )							// distance impact
		+ 10.f - Float ( group * 2.f )							// group impact
		+ (canSpawn ? 50.f : 0.f);								// can spawn impact

	return p;
}

///////////////////////////////////////////////////////////////////////////////
// CTemplateLoadRequest
///////////////////////////////////////////////////////////////////////////////
void CTemplateLoadRequest::Initialize( const CTemplateLoadBalancer::TemplateHandle& templateHandle )
{
	if ( m_templateHandle.IsEmpty() )
	{
		m_templateHandle = templateHandle;
	}
}
void CTemplateLoadRequest::Clear()
{
	if ( m_requestId != CTemplateLoadBalancer::INVALID_REQUEST_ID )
	{
		if ( CTemplateLoadBalancer* templateLoadBalancer = GCommonGame->GetTemplateLoadBalancer() )
		{
			templateLoadBalancer->UnregisterTemplateRequest( *this );
		}
		m_requestId = CTemplateLoadBalancer::INVALID_REQUEST_ID;
		
	}
	m_templateHandle.Release();
}
