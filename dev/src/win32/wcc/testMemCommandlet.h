/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __TESTMEM_COMMANDLET_H__
#define __TESTMEM_COMMANDLET_H__

#include "../../common/core/commandlet.h"

/// Commandlet that tests the memory framework + GC + resource management + streaming
/// The testing is simple: load world, start jumping around to random locations (in loop), stream in/out, call GC
/// Compare memory allocations each time we visit each location and print if we leak resources
class CTestMemCommandlet : public ICommandlet
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CTestMemCommandlet, ICommandlet );

public:
	CTestMemCommandlet();
	
	// ICommandlet interface 
	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const { return TXT("Test memory framework, streaming, GC and resource management stability"); }
	virtual void PrintHelp() const;

private:
/*	struct MemoryReport
	{

	};*/

	struct Settings
	{
		String		m_worldPath;
		Float		m_worldExtents;
		Uint32		m_numTestPoints;
		Uint32		m_numIterations;

		Settings();

		Bool Parse( const CommandletOptions& options );
	};

	struct TestPoint
	{
		Vector		m_position;

	};

	typedef TDynArray< TestPoint > TTestPoints;

	Settings			m_settings;
	TTestPoints			m_testPoints;

	THandle< CWorld >	m_world;
};

BEGIN_CLASS_RTTI( CTestMemCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI();

#endif // __TESTMEM_COMMANDLET_H__