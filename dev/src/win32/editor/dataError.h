/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_DATA_VALIDATION

#include "../../common/core/dataError.h"

struct SDataError
{
	Uint32				m_uid;			//!< 
	EDataErrorSeverity	m_severity;		//!< 
	String				m_resourcePath;	//!< 
	String				m_errorMessage;	//!< 
	String				m_category;		//!< 
	String				m_lastEditBy;	//!< 
	Uint32				m_errorCount;	//!< Number of times this error has occurred during the run of the program
	Bool				m_hit;			//!< Set to true if it has occurred again since the last Flush()

	Bool operator==( const SDataError& other )
	{
		return m_uid == other.m_uid;
	}

	RED_INLINE const String ToString() const
	{
		return String::Printf( TXT( "%d %ls (%ls)" ), m_severity + 1, m_errorMessage.AsChar(), m_resourcePath.AsChar() );
	}

	void FillNewDataErrorInfo( const String& message, const IDataErrorSystem::SAssertInfo& info, const String& resourcePath, int uid )
	{
		m_uid			= uid;
		m_errorMessage	= message;
		m_lastEditBy	= String::EMPTY;
		m_severity		= info.m_severity;
		m_resourcePath	= resourcePath;
		m_category		= info.m_category;
		m_hit			= true;
		m_errorCount	= 1;
	}

	struct BySeverity
	{ 
		static Bool Less( const SDataError& a, const SDataError& b )
		{
			return a.m_severity < b.m_severity;
		}
	};
};

namespace
{
	typedef THashMap< Uint32, SDataError > DataErrorMap;
}

class CDataErrorSystem : public IDataErrorSystem
{
public:
	CDataErrorSystem();
	virtual ~CDataErrorSystem();

	// implement IDataErrorSystem interface
	virtual void Flush();
	// Gets errors connected with resources specified in array dependenciesPaths
	virtual void GetCurrentCatchedFilteredForResource( TSortedArray< SDataError >& arrayForErrors, const TDynArray< String >& dependenciesPaths );
	virtual void ClearCatchedForResource( const TDynArray< String >& dependenciesPaths );
	virtual void ReportError( SAssertInfo& info, const Char* message, ... );

	virtual void RegisterListener( IDataErrorListener* listener );
	virtual void UnregisterListener( IDataErrorListener* listener );

	virtual void FillErrorInfoFromPerforce( SDataError& error );

private:
	// send error to listeners
	void ProcessErrors();
	void OnDataErrorReported( const SDataError& error );

	Uint32 GenerateHash( const Char* resourcePath, const Char* file, Uint32 line, const Char* message ) const;
	void InternalReportError( const SAssertInfo& info, const String& resourcePath, const String& message );

private:
	DataErrorMap						m_errors;
	Bool								m_hit;
	Uint32								m_pendingErrorsCount;
	TDynArray< Uint32 >					m_errorsWithoutPerforceInfo;
	DataErrorMap						m_nonProcessedErrors;
	TDynArray< IDataErrorListener* >	m_listeners;
	Red::Threads::CMutex				m_lock;
};

#endif // NO_DATA_VALIDATION
