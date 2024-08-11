/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/resource.h"
#include "../../common/core/cooker.h"
#include "cookDataBase.h"
#include "cookFramework.h"

//---

CCookerErrorReporter::CCookerErrorReporter()
{
}

void CCookerErrorReporter::DumpErrorsToLog()
{
	Red::Threads::CScopedLock<Mutex> mutex( m_lock );
}

void CCookerErrorReporter::DumpErrorsToReport()
{
	Red::Threads::CScopedLock<Mutex> mutex( m_lock );
}

void CCookerErrorReporter::ReportError( const CResource* resource, const CObject* object, const String& txt )
{
	ERR_WCC( TXT("Cooking error for resource '%ls': %ls"), 
		resource ? resource->GetDepotPath().AsChar() : TXT("NULL"), txt.AsChar() );

	// add to report
	{
		Red::Threads::CScopedLock<Mutex> mutex( m_lock );

		Message* msg = new ( m_errors) Message;
		msg->m_resource = resource ? resource->GetDepotPath() : String::EMPTY;
		msg->m_object = (object == resource) ? String(TXT("this")) : (object ? object->GetFriendlyName() : String::EMPTY);
		msg->m_text = txt;
	}
}

void CCookerErrorReporter::ReportWarning( const CResource* resource, const CObject* object, const String& txt )
{
	WARN_WCC( TXT("Cooking warning for resource '%ls': %ls"), 
		resource ? resource->GetDepotPath().AsChar() : TXT("NULL"), txt.AsChar() );

	// add to report
	{
		Red::Threads::CScopedLock<Mutex> mutex( m_lock );

		Message* msg = new ( m_warnings ) Message;
		msg->m_resource = resource ? resource->GetDepotPath() : String::EMPTY;
		msg->m_object = (object == resource) ? String(TXT("this")) : (object ? object->GetFriendlyName() : String::EMPTY);
		msg->m_text = txt;
	}
}

//---

CCookerDependencyTracker::CCookerDependencyTracker()
{
}

void CCookerDependencyTracker::ReportHardDependency( const String& depotPath )
{
	if ( !depotPath.Empty() )
		m_hardDependencies.Insert( depotPath );
}

void CCookerDependencyTracker::ReportSoftDependency( const String& depotPath )
{
	if ( !depotPath.Empty() )
		m_softDependencies.Insert( depotPath );
}

void CCookerDependencyTracker::GetHardDependencies( TDynArray< String >& outPaths ) const
{
	for ( auto it = m_hardDependencies.Begin(); it != m_hardDependencies.End(); ++it )
	{
		outPaths.PushBack( *it );
	}

	::Sort( outPaths.Begin(), outPaths.End() );
}

void CCookerDependencyTracker::GetSoftDependencies( TDynArray< String >& outPaths ) const
{
	for ( auto it = m_softDependencies.Begin(); it != m_softDependencies.End(); ++it )
	{
		outPaths.PushBack( *it );
	}

	::Sort( outPaths.Begin(), outPaths.End() );
}

//---

CCookerFramework::CCookerFramework( const CResource* resource, const ECookingPlatform platform,  CCookerErrorReporter* errorReporter, CCookerDependencyTracker* dependencyTracker, ICookerStatCollector* statCollector )
	: m_platform( platform )
	, m_errorReporter( errorReporter )
	, m_dependencyTracker( dependencyTracker )
	, m_cookedResource( resource )
	, m_statCollector( statCollector )
{
}

void CCookerFramework::CookingError( const CObject* subObject, const Char* message, ... )
{
	if ( m_errorReporter )
	{
		Char buffer[MAX_MESSAGE_LENGTH];
		va_list args;

		va_start( args, message );
		Red::VSNPrintF( buffer, MAX_MESSAGE_LENGTH, message, args );
		va_end( args );

		m_errorReporter->ReportError( m_cookedResource, subObject, buffer );
	}
}

void CCookerFramework::CookingWarning( const CObject* subObject, const Char* message, ... )
{
	if ( m_errorReporter )
	{
		Char buffer[MAX_MESSAGE_LENGTH];
		va_list args;

		va_start( args, message );
		Red::VSNPrintF( buffer, MAX_MESSAGE_LENGTH, message, args );
		va_end( args );

		m_errorReporter->ReportWarning( m_cookedResource, subObject, buffer );
	}
}

void CCookerFramework::ReportHardDependency( const String& depotPath )
{
	if ( m_dependencyTracker )
		m_dependencyTracker->ReportHardDependency( depotPath );
}

void CCookerFramework::ReportSoftDependency( const String& depotPath )
{
	if ( m_dependencyTracker )
		m_dependencyTracker->ReportSoftDependency( depotPath );
}

void CCookerFramework::ReportCookingTime( const CClass* objectClass, const Double timeTaken )
{
	if ( m_statCollector )
		m_statCollector->ReportCookingTime( objectClass, timeTaken );
}

//---
