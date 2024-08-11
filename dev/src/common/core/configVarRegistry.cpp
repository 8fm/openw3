/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "configVar.h"
#include "configVarStorage.h"
#include "configVarRegistry.h"

namespace Config
{

CConfigVarRegistry::CConfigVarRegistry()
{
}

CConfigVarRegistry::~CConfigVarRegistry()
{
}


void CConfigVarRegistry::Refresh( const class CConfigVarStorage& storage, const EConfigVarSetMode setMode )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// set all values we found a value for
	for ( auto it = m_vars.Begin(); it != m_vars.End(); ++it )
	{
		IConfigVar* var = (*it).m_second;

		// do we have a value for this property in storage ?
		String configValue;
		if ( storage.GetEntry( var->GetGroup(), var->GetName(), configValue ) )
		{
			// set only if different
			String currentValue;
			if ( !var->GetText( currentValue ) || (currentValue != configValue) )
			{
				var->SetText( configValue, setMode );
			}
		}
	}
}

void CConfigVarRegistry::Capture( class CConfigVarStorage& storage ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// we should care only about saveable values
	for ( auto it = m_vars.Begin(); it != m_vars.End(); ++it )
	{
		const IConfigVar* var = (*it).m_second;

		if ( var->HasFlag( eConsoleVarFlag_Save ) )
		{
			// get current value
			String currentValue;
			if ( var->GetText( currentValue ) )
			{
				// store in the storage
				storage.SetEntry( var->GetGroup(), var->GetName(), currentValue );
			}
		}
	}
}

CConfigVarRegistry::TNameHash CConfigVarRegistry::CalcNameHash( const AnsiChar* name, const AnsiChar* groupName )
{
	TNameHash hash = Red::CalculateHash32( name );
	hash = Red::CalculateHash32( groupName, hash );
	return hash;
}

void CConfigVarRegistry::Register( IConfigVar& var )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	
	// calculate name hash
	const TNameHash hash = CalcNameHash( var.GetName(), var.GetGroup() );
	IConfigVar* existingVar = nullptr;
	if ( m_vars.Find( hash, existingVar ) )
	{
		if ( existingVar != &var )
		{
			if ( 0 == Red::StringCompare(var.GetName(), existingVar->GetName()) )
			{
				RED_FATAL( "Console variable '%ls', group '%ls' is duplicated", ANSI_TO_UNICODE( var.GetName() ), ANSI_TO_UNICODE( var.GetGroup() ) );
			}
			else
			{
				RED_FATAL( "Console variable '%ls', group '%ls' has hash collision!", ANSI_TO_UNICODE( var.GetName() ), ANSI_TO_UNICODE( var.GetGroup() ) );
			}
		}

		// do not add twice
		return;
	}

	// add to list
	m_vars.Insert( hash, &var );
}

void CConfigVarRegistry::Unregister( IConfigVar& var )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// calculate name hash
	const TNameHash hash = CalcNameHash( var.GetName(), var.GetGroup() );
	IConfigVar* existingVal = nullptr;
	if ( m_vars.Find( hash, existingVal ) )
	{
		RED_FATAL_ASSERT( existingVal == &var, "Different console variable is registered" );
		m_vars.Erase( hash );
	}
}

IConfigVar* CConfigVarRegistry::Find( const AnsiChar* groupName, const AnsiChar* name ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	const TNameHash hash = CalcNameHash( name, groupName );
	IConfigVar* existingVal = nullptr;
	if ( m_vars.Find( hash, existingVal ) )
	{
		if ( 0 == Red::StringCompare( groupName, existingVal->GetGroup() ) )
		{
			if ( 0 == Red::StringCompare( name, existingVal->GetName() ) )
			{
				return existingVal;
			}
		}
	}

	// not found
	return nullptr;
}

void CConfigVarRegistry::EnumVars( TDynArray< IConfigVar* >& outVars, const AnsiChar* groupMatch /*= ""*/, const AnsiChar* nameMatch /*= ""*/, const Uint32 includeFlags /*= 0*/, const Uint32 excludeFlags /*= 0*/ ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// linear scan
	for ( auto it = m_vars.Begin(); it != m_vars.End(); ++it )
	{
		IConfigVar* var = (*it).m_second;

		// check group match
		if ( groupMatch && *groupMatch )
		{
			if ( nullptr == Red::StringSearch( var->GetGroup(), groupMatch ) )
				continue;
		}

		// check name match
		if ( nameMatch && *nameMatch )
		{
			if ( nullptr == Red::StringSearch( var->GetName(), nameMatch ) )
				continue;
		}

		// check inclusion flags
		if ( includeFlags && !var->HasFlag( (EConfigVarFlags)includeFlags ) )
		{
			continue;
		}

		// check exclusion flags
		if ( var->HasFlag( (EConfigVarFlags) excludeFlags ) )
		{
			continue;
		}

		// add to list
		outVars.PushBack( var );
	}
}

} // Console