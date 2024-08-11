/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __RESOURCE_USAGE_COMMANDLET_H__
#define __RESOURCE_USAGE_COMMANDLET_H__

#include "../../common/core/commandlet.h"
#include "../../common/core/datetime.h"

/// Command let for exporting spatial resource usage information from world
class CResourceUsageCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CResourceUsageCommandlet, ICommandlet, 0 );

public:
	CResourceUsageCommandlet();
	~CResourceUsageCommandlet();

	// interface
	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const { return TXT("Export spatial resource usage information from a given world"); }
	virtual void PrintHelp() const;

private:
	struct Settings
	{
		String								m_outPath;
		String								m_worldPath;

		Settings();

		bool Parse( const CommandletOptions& options );
	};

	Settings								m_settings;

	class CResourceUsageDataBase*			m_usageDataBase;
};

BEGIN_CLASS_RTTI( CResourceUsageCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI();

#endif