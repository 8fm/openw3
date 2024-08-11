/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencyLoaderFactory.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/core/dependencySaverFactory.h"

/**********************************/
/* Dependency loader for Speed Tree base tree */
/**********************************/
class CDependencyLoaderSRT : public IDependencyLoader
{
	friend class CDependencyLinkerFactory;

private:
	String	m_filePath;
	static Red::Threads::CMutex m_lock;

public:
	CDependencyLoaderSRT( IFile& file, const CDiskFile* diskFile );
	virtual ~CDependencyLoaderSRT() {}

	// Deserialize objects with dependencies
	virtual Bool LoadObjects( DependencyLoadingContext& context );

	//! Load the list of dependencies of this file
	virtual Bool LoadDependencies( TDynArray< FileDependency >&, Bool ) { return true; }

	// Initialize loaded objects by calling OnPostLoad on them
	virtual Bool PostLoad() { return true; }
};

class CDependencyLoaderSRTFactory : public IDependencyLoaderFactory
{
public:
	CDependencyLoaderSRTFactory();
	virtual ~CDependencyLoaderSRTFactory();
	virtual IDependencyLoader* CreateLoader( IFile& file, const CDiskFile* fileBeingLoad ) const;
};


/*********************************/
/* Dependency saver for 2d Array */
/*********************************/

class CDependencySaverSRT : public IDependencySaver
{
	friend class CDependencyLinkerFactory;

public:
	CDependencySaverSRT( IFile& file ) : IDependencySaver( file ) {}
	virtual ~CDependencySaverSRT() {}

	// Save objects, returns true on success
	virtual Bool SaveObjects( DependencySavingContext& context );
};

class CDependencySaverSRTFactory : public IDependencySaverFactory
{
public:
	CDependencySaverSRTFactory();
	virtual ~CDependencySaverSRTFactory();

	virtual IDependencySaver* CreateSaver( IFile& file, const CDiskFile* fileBeingSaved ) const;
};
