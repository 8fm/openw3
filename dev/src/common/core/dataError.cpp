/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DATA_VALIDATION

#include "dataError.h"

IDataErrorSystem::~IDataErrorSystem()
{
	/* intentionally empty */
}

void IDataErrorSystem::ReportXMLError( const Char* filepath, const Char* error, ... )
{ 
	va_list arglist;
	va_start( arglist, error );
	m_xmlErrors.GetRef( String ( filepath ) ).PushBackUnique( String::PrintfV( error, arglist ) );	
	va_end( arglist );
}

IDataErrorSystem* GDataError = nullptr;

#endif // NO_DATA_VALIDATION
