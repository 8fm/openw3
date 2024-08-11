/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "dependencyLoader.h"
#include "dependencyLoaderFactory.h"
#include "dynarray.h"
#include "string.h"

class IFile;
class CDiskFile;

/**********************************/
/* Dependency loader for 2d Array */
/**********************************/
class CDependencyLoader2dArray : public IDependencyLoader
{
	friend class CDependencyLinkerFactory;

private:
	String	m_filePath;

public:
	CDependencyLoader2dArray( IFile& file, const CDiskFile* diskFile );
	virtual ~CDependencyLoader2dArray() {}

	// Deserialize objects with dependencies
	virtual Bool LoadObjects( DependencyLoadingContext& context );

	//! Load the list of dependencies of this file
	virtual Bool LoadDependencies( TDynArray< FileDependency >&, Bool );

	// Initialize loaded objects by calling OnPostLoad on them
	virtual Bool PostLoad() { return true; }
};

class CDependencyLoader2dArrayFactory : public IDependencyLoaderFactory
{
public:
	CDependencyLoader2dArrayFactory();
	virtual ~CDependencyLoader2dArrayFactory();
	virtual IDependencyLoader* CreateLoader( IFile& file, const CDiskFile* fileBeingLoad ) const;
};
