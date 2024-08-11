/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DATA_VALIDATION

#include "dataError.h"
#include "dataErrorReporter.h"

#include "../../common/core/versionControl.h"

extern Bool GDataAsserts;

CDataErrorSystem::CDataErrorSystem()
	:m_hit( false )
	,m_pendingErrorsCount( 0 )
{
	/* intentionally empty */
}

CDataErrorSystem::~CDataErrorSystem()
{
	/* intentionally empty */
}

void CDataErrorSystem::ProcessErrors()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_lock );

	// Make sure data asserts are not disabled
	if ( GDataAsserts == false )
	{
		return;
	}

	if( m_hit == true )
	{
		// create temporary container with existing listeners because in StartProcessing or StopProcessing listener can unregister yourself
		TDynArray< IDataErrorListener* > tempListeners = m_listeners;
		const Uint32 listenerCount = tempListeners.Size();

		for( Uint32 i = 0; i < listenerCount; ++i )
		{
			IDataErrorListener* listener = tempListeners[i];
			if( listener != nullptr )
			{
				listener->StartProcessing();
			}
		}

		// ask control version system who the last modified resource
		for( Uint32 i = 0; i < m_errorsWithoutPerforceInfo.Size(); ++i )
		{
			const Uint32 hash = m_errorsWithoutPerforceInfo[i];
			FillErrorInfoFromPerforce( m_errors.GetRef( hash ) );
		}
		m_errorsWithoutPerforceInfo.ClearFast();

		TDynArray< SDataError > tmpErrors;
		tmpErrors.Reserve( m_pendingErrorsCount );
		for ( DataErrorMap::iterator it = m_errors.Begin(); it !=  m_errors.End(); ++it )
		{
			SDataError& error = it->m_second;
			if ( error.m_hit )
			{
				error.m_hit = false;
				tmpErrors.PushBack( error );
			}
		}

		TDynArray< SDataError > nonProcessed;
		m_nonProcessedErrors.GetValues( nonProcessed );
		for( Uint32 listenerInd = 0; listenerInd < listenerCount; ++listenerInd )
		{
			IDataErrorListener* listener = tempListeners[listenerInd];
			if( listener != nullptr )
			{
				listener->ProcessDataErrors( tmpErrors );
				listener->StoreNonProcessedErrors( nonProcessed );
			}
		}

		m_hit = false;
		m_pendingErrorsCount = 0;
		m_nonProcessedErrors.Clear();

		for( Uint32 i = 0; i < listenerCount; ++i )
		{
			IDataErrorListener* listener = tempListeners[i];
			if( listener != nullptr )
			{
				listener->StopProcessing();
			}
		}
	}
}

void CDataErrorSystem::FillErrorInfoFromPerforce( SDataError& error )
{
	String temporaryLastEditBy = TXT( "Unknown User" );

	if ( error.m_resourcePath != String::EMPTY )
	{
		String absoluteFilePath = GFileManager->GetDataDirectory() + error.m_resourcePath;
		GVersionControl->FileLastEditedBy( absoluteFilePath, temporaryLastEditBy );
	}

	error.m_lastEditBy = temporaryLastEditBy;
}

void CDataErrorSystem::OnDataErrorReported( const SDataError& error )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_lock );

	m_hit = true;

	TDynArray< IDataErrorListener* > tempListeners = m_listeners;
	const Uint32 listenerCount = tempListeners.Size();

	for( Uint32 i=0; i<listenerCount; ++i )
	{
		IDataErrorListener* listener = tempListeners[i];
		if( listener != nullptr )
		{
			listener->OnDataErrorReported( error );
		}
	}
}

void CDataErrorSystem::Flush()
{
	if( wxTheApp != nullptr )
	{
		RunLaterOnce( [ this ](){ ProcessErrors(); } );
	}
}

void CDataErrorSystem::ReportError( SAssertInfo& info, const Char* message, ... )
{
	if( message == nullptr )
	{
		message = TXT( "FIX ME: A message needs to be defined with this assert!" );
	}

	va_list arglist;
	va_start( arglist, message );

	String formattedMessage = String::PrintfV( message, arglist );

	va_end( arglist );

	String resourcePath;
	const CResource* res = info.m_assertParent; 
	if ( res == nullptr )
	{
		resourcePath = TXT("No resource path");
	}
	else
	{
		resourcePath = res->GetDepotPath();
	}

	if( wxTheApp != nullptr )
	{
		if ( GDataAsserts )
		{
			InternalReportError( info, resourcePath, formattedMessage );
		}
	}
}

void CDataErrorSystem::InternalReportError( const SAssertInfo& info, const String& resourcePath, const String& message )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_lock );

	// generate hash code
	Uint32 hash = GenerateHash( resourcePath.AsChar(), info.m_file, info.m_line, message.AsChar() );

	SDataError* existingError = m_errors.FindPtr( hash );

	if ( existingError != nullptr )
	{
		// if we didnt meet reported errors limit yet or the error was already reported in this batch
		if ( m_pendingErrorsCount < 200 || existingError->m_hit )
		{
			existingError->m_errorCount++;
			if ( !existingError->m_hit )
			{
				m_pendingErrorsCount++;
			}
			existingError->m_hit = true;
		}
		else
		{
			existingError = m_nonProcessedErrors.FindPtr( hash );
			if ( !existingError )
			{
				SDataError& error = m_nonProcessedErrors.GetRef( hash );
				error.FillNewDataErrorInfo( message, info, resourcePath, hash );
			}
			else
			{
				existingError->m_errorCount++;
			}
		}
		OnDataErrorReported( *existingError );
	}
	else
	{
		SDataError& newError = m_pendingErrorsCount < 200 ? m_errors.GetRef( hash ) : m_nonProcessedErrors.GetRef( hash );
		newError.FillNewDataErrorInfo( message, info, resourcePath, hash );
		OnDataErrorReported( newError );

		if ( m_pendingErrorsCount < 200 )
		{
			m_errorsWithoutPerforceInfo.PushBackUnique( hash );
			m_pendingErrorsCount++;
		}
	}
}

RED_INLINE Uint32 CDataErrorSystem::GenerateHash( const Char* resourcePath, const Char* file, Uint32 line, const Char* message ) const
{
	String hashString = String::Printf( TXT( "%s%u%s%s" ), file, line, resourcePath, message );
	Uint32 hash = GetHash( hashString );

	return hash;
}

void CDataErrorSystem::RegisterListener( IDataErrorListener* listener )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_lock );
	m_listeners.PushBackUnique( listener );
}

void CDataErrorSystem::UnregisterListener( IDataErrorListener* listener )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_lock );
	m_listeners.Remove( listener );
}

void CDataErrorSystem::GetCurrentCatchedFilteredForResource( TSortedArray< SDataError >& arrayForErrors, const TDynArray< String >& dependenciesPaths )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_lock );

	if ( GDataAsserts == false )
	{
		return;
	}

	for( DataErrorMap::iterator iter = m_errors.Begin(); iter != m_errors.End(); ++iter )
	{
		const SDataError& storedError = iter->m_second;
		if( dependenciesPaths.Exist( storedError.m_resourcePath ) )
		{
			arrayForErrors.PushBackUnique( storedError );
		}
	}
}

void CDataErrorSystem::ClearCatchedForResource( const TDynArray< String >& dependenciesPaths )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopeLock( m_lock );

	if ( GDataAsserts == false )
	{
		return;
	}

	TDynArray< Uint32 > errorsToRemove;
	for( DataErrorMap::iterator iter = m_errors.Begin(); iter != m_errors.End(); ++iter )
	{
		const SDataError& storedError = iter->m_second;
		if( dependenciesPaths.Exist( storedError.m_resourcePath ) )
		{
			errorsToRemove.PushBackUnique( iter->m_first );
		}
	}
	for( Uint32 i = 0; i < errorsToRemove.Size(); ++i )
	{
		m_errors.Erase( errorsToRemove[i] );
	}
}

#endif // NO_DATA_VALIDATION
