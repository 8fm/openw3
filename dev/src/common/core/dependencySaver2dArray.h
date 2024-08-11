/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "dependencySaver.h"
#include "dependencySaverFactory.h"

/*********************************/
/* Dependency saver for 2d Array */
/*********************************/

class CDependencySaver2dArray : public IDependencySaver
{
	friend class CDependencyLinkerFactory;

public:
	CDependencySaver2dArray( IFile& file ) : IDependencySaver( file ) {}
	virtual ~CDependencySaver2dArray() {}

	// Save objects, returns true on success
	virtual Bool SaveObjects( DependencySavingContext& context );
};

class CDependencySaver2dArrayFactory : public IDependencySaverFactory
{
public:
	CDependencySaver2dArrayFactory();
	virtual ~CDependencySaver2dArrayFactory();

	virtual IDependencySaver* CreateSaver( IFile& file, const CDiskFile* fileBeingSaved ) const;
};
