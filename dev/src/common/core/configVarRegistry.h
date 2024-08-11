/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "hashmap.h"

namespace Config
{
	class IConfigVar;

	/// Registry to IConsoleVars
	class CConfigVarRegistry
	{
		DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	public:
		CConfigVarRegistry();
		~CConfigVarRegistry();

		// pull the config values from the given storage, reload flag is there to indicate in-game reload
		void Refresh( const class CConfigVarStorage& storage, const EConfigVarSetMode setMode );

		// dump values that can be saved to a given storage
		void Capture( class CConfigVarStorage& storage ) const;

		// register variable in registry
		void Register( IConfigVar& var );

		// unregister variable from registry
		void Unregister( IConfigVar& var );

		// enumerate variables matching given search pattern
		void EnumVars( TDynArray< IConfigVar* >& outVars, const AnsiChar* groupMatch = "", const AnsiChar* nameMatch = "", const Uint32 includeFlags = 0, const Uint32 excludeFlags = 0 ) const;

		// find variable
		IConfigVar* Find( const AnsiChar* groupName, const AnsiChar* name ) const;

	private:
		// name hashing
		typedef Uint32 TNameHash;
		static TNameHash CalcNameHash( const AnsiChar* name, const AnsiChar* groupName );

		// mapped variables
		typedef THashMap< TNameHash, IConfigVar* >	TConsoleVarMap;
		TConsoleVarMap		m_vars;

		// thread safety lock (eh...)
		mutable Red::Threads::CMutex	m_lock;
	};

} // Config