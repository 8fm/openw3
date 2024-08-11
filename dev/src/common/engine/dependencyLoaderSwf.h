/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/dependencyLoader.h"
#include "../core/dependencyLoaderFactory.h"

class IFile;
class CDiskFile;

/**********************************/
/* Dependency loader for SWF	  */
/**********************************/
class CDependencyLoaderSwf : public CDependencyLoader
{
	friend class CDependencyLinkerFactory;
	typedef CDependencyLoader TBaseClass;

public:
	CDependencyLoaderSwf( IFile& file, const CDiskFile* diskFile );
	virtual ~CDependencyLoaderSwf() {}

	// Deserialize objects with dependencies
	virtual Bool LoadObjects( DependencyLoadingContext& context ) override;
};

class CDependencyLoaderSwfFactory : public IDependencyLoaderFactory
{
public:
	CDependencyLoaderSwfFactory();
	virtual ~CDependencyLoaderSwfFactory();
	virtual IDependencyLoader* CreateLoader( IFile& file, const CDiskFile* fileBeingLoad ) const;
};
