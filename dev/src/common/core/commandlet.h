/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "object.h"
#include "list.h"

/// Base commandlet interface
class ICommandlet : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICommandlet, CObject );

protected:
	CName		m_commandletName;

public:

	class CommandletOptions
	{
	private:
		Bool IsAnOption( const String & argument ) const;

	private:
		TDynArray< String >			         m_arguments;
		THashMap< String, TList< String > >  m_options;

	public:

		const TDynArray< String > &GetFreeArguments() const { return m_arguments; }
		Bool HasOption( const String &shortVer, const String &longVer ) const;
		Bool HasOption( const String &longVer ) const;
		TList<String> GetOptionValues( const String &shortVer, const String &longVer ) const;
		TList<String> GetOptionValues( const String &longVer ) const;

		Bool GetSingleOptionValue( const String &longVer, String& outString ) const;
		Bool GetSingleOptionValue( const String &shortVer, const String &longVer, String& outString ) const;

	public:

		Bool ParseCommandline( Uint32 startIdx, const TDynArray< String > &arguments );

	public:

		RED_INLINE CommandletOptions() {};
	};

public:
	// Returns commandlet name
	RED_INLINE CName GetName() const { return m_commandletName; }

public:
	// Executes commandlet command
	virtual Bool Execute( const CommandletOptions& options ) = 0;

	// Returns commandlet one-liner
	virtual const Char* GetOneLiner() const = 0;
	
	// Prints commandlet help
	virtual void PrintHelp() const = 0;
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( ICommandlet, CObject );


