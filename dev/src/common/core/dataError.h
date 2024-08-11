/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "sortedArray.h"
#include "hashmap.h"

#ifndef NO_DATA_VALIDATION

class CObject;
struct SDataError;

class IDataErrorListener
{
public:
	virtual void OnDataErrorReported( const SDataError& error ) = 0;
	virtual void StartProcessing() = 0;
	virtual void ProcessDataErrors( const TDynArray< SDataError >& errors ) = 0;
	virtual void StoreNonProcessedErrors( const TDynArray< SDataError >& errors ) = 0;
	virtual void StopProcessing() = 0;
};

// Data error priorities
enum EDataErrorSeverity
{
	DES_Uber,		//!< Uber error - game will probably not work
	DES_Major,		//!< Major error ( P1 )
	DES_Minor,		//!< Minor
	DES_Tiny,		//!< Really cosmetic thing

	DES_Count,
};

// Interface for data error system
class IDataErrorSystem
{
public:
	struct SAssertInfo
	{
		const CResource*	m_assertParent;

		EDataErrorSeverity	m_severity;
		const Char*			m_expression;
		const Char*			m_category;
		const Char*			m_file;
		Uint32				m_line;
	};

public:
	virtual ~IDataErrorSystem() = 0;

	virtual void Flush() = 0;
	virtual void GetCurrentCatchedFilteredForResource( TSortedArray< SDataError >& arrayForErrors, const TDynArray< String >& dependenciesPaths ) = 0;
	virtual void ClearCatchedForResource( const TDynArray< String >& dependenciesPaths ) = 0;
	virtual void ReportError( SAssertInfo& info, const Char* message, ... ) = 0;

	virtual void FillErrorInfoFromPerforce( SDataError& error ) = 0;

	virtual void RegisterListener( IDataErrorListener* listener ) = 0;
	virtual void UnregisterListener( IDataErrorListener* listener ) = 0;

	void ReportXMLError( const Char* filepath, const Char* error, ... );	
	void ClearXMLErrorsForFile( const Char* filepath )						{ m_xmlErrors.Erase( String ( filepath ) );		}
	const THashMap< String, TDynArray< String > >& GetXMLErrors()			{ return m_xmlErrors;							}

private:
	THashMap< String, TDynArray< String > >	m_xmlErrors;
	
};

// global variable of data error system
extern IDataErrorSystem* GDataError;

#else

#define NO_DATA_ASSERTS

#endif // NO_DATA_VALIDATION



//////////////////////////////////////////////////////////////////////////
// Predefine macros
//////////////////////////////////////////////////////////////////////////
#ifndef NO_DATA_ASSERTS

#define DATA_HALT( Severity, CResource, Category, Message, ... )				\
{																				\
	if( GDataError != nullptr )													\
	{																			\
		IDataErrorSystem::SAssertInfo info;										\
		info.m_assertParent = CResource;										\
		info.m_severity = Severity;												\
		info.m_file = MACRO_TXT( __FILE__ );									\
		info.m_line = __LINE__;													\
		info.m_expression = TXT( "Halt" );										\
		info.m_category = Category;												\
																				\
		GDataError->ReportError( info, Message, ##__VA_ARGS__ );				\
	}																			\
}

#define XML_ERROR( File, Message, ... )											\
{																				\
	if( GDataError != nullptr )													\
	{																			\
		GDataError->ReportXMLError( File, Message, ##__VA_ARGS__ );				\
	}																			\
}																				\

#define CLEAR_XML_ERRORS( File )												\
{																				\
	if ( GDataError != nullptr )												\
	{																			\
		GDataError->ClearXMLErrorsForFile( File );								\
	}																			\
}																				\

#else

#define DATA_HALT( Severity, CResource, Category, Message, ... )

#define XML_ERROR( File, Message, ... )

#define CLEAR_XML_ERRORS( File )

#endif	// NO_DATA_ASSERTS
