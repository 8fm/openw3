/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "memory.h"
#include "dependencyLinkerFactory.h"
#include "resource.h"

class IFile;
class CDiskFile;

// Interface
class IDependencyLoaderFactory
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );
public:
	virtual ~IDependencyLoaderFactory() {}
	virtual IDependencyLoader* CreateLoader( IFile& file, const CDiskFile* loadedFile ) const = 0;
};
