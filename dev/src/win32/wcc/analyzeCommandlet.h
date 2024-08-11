/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __ANALYZE_COMMANDLET_H__
#define __ANALYZE_COMMANDLET_H__

#include "../../common/core/commandlet.h"

class CAnalyzeCommandlet : public ICommandlet
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CAnalyzeCommandlet, ICommandlet );

public:
	CAnalyzeCommandlet();
	virtual ~CAnalyzeCommandlet();

	// Interface
	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const;
	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CAnalyzeCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI();

#endif // __ANALYZE_COMMANDLET_H__
