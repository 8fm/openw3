/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/commandlet.h"

/// Helper commandlet to build optimized sector data (HACK)
class CSectorDataBuilderCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CSectorDataBuilderCommandlet, ICommandlet, 0 );

public:
	CSectorDataBuilderCommandlet();
	~CSectorDataBuilderCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Optimize (resort) collision cache"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CSectorDataBuilderCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()
