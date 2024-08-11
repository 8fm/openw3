/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dynarray.h"
#include "names.h"
#include "variant.h"

class CProperty;
class CScriptThread;
struct SSavePointValue;
class IGameSaver;
class IGameLoader;
class CScriptStackFrame;

///////////////////////////////////////////////////////////////////////////////

/// A tool for serializing the thread execution state.
class CScriptThreadSerializer
{
private:
	CName									m_name;
	TDynArray< SSavePointValue* >			m_params;

public:
	~CScriptThreadSerializer();

	 // Records the parameters at the specified SavePoint.
	void Record( const CName& savepointName, const TDynArray< SSavePointValue* >& params );

	 // Save the state of the thread using save point
	void SaveState( IGameSaver* saver );

	// Restores the SavePoint description from an XML, and resets
	void RestoreState( CScriptStackFrame& stack, IGameLoader* loader );

private:
	void RestoreThreadState( CScriptStackFrame& stack );
};

///////////////////////////////////////////////////////////////////////////////

/// A structure holding information about a single parameter from script save point
struct SSavePointValue
{
	CName											m_name;
	CName											m_type;
	CVariant										m_value;

	SSavePointValue();
	SSavePointValue( CScriptStackFrame& frame, CProperty& property );

	void SaveState( IGameSaver* saver );
	void RestoreState( IGameLoader* loader );
};
