/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "dependencyLinkerFactory.h"
#include "memory.h"
#include "resource.h"

class IDependencySaverFactory
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	virtual ~IDependencySaverFactory() {}
	virtual IDependencySaver* CreateSaver( IFile& file, const CDiskFile* fileBeingSaved ) const = 0;
};

// Simple template class to register raw savers
template < class Resource, class SaverClass >
class TDependencySaverFactory : public IDependencySaverFactory
{
public:
	TDependencySaverFactory()
	{
		SDependencyLinkerFactory::GetInstance().RegisterSaverFactory( ResourceExtension< Resource >(), this );
	}

	~TDependencySaverFactory()
	{
		SDependencyLinkerFactory::GetInstance().UnregisterSaverFactory( this );
	}

	IDependencySaver* CreateSaver( IFile& file, const CDiskFile* fileBeingSaved ) const
	{
		return new SaverClass( file, fileBeingSaved );
	}
};
