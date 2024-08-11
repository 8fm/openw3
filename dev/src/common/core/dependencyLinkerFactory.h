/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "memory.h"
#include "dynarray.h"
#include "singleton.h"
#include "string.h"

// Forward declarations
class IDependencyLoader;
class IDependencySaver;
class IDependencyLoaderFactory;
class IDependencySaverFactory;

class CDiskFile;
class IFile;

class CDependencyLinkerFactory
{
public:
	IDependencyLoader* GetLoader( const CDiskFile* diskFile, IFile& file );
	IDependencySaver* GetSaver( const CDiskFile* diskFile, IFile& file );
	IDependencySaver* GetSaver( const String& ext, IFile& file );

	void RegisterLoaderFactory( const String& fileExtension, IDependencyLoaderFactory* loader, Bool overrideLoader = false );
	void UnregisterLoaderFactory( IDependencyLoaderFactory* loader );

	void RegisterSaverFactory( const String& fileExtension, IDependencySaverFactory* saver );
	void UnregisterSaverFactory( IDependencySaverFactory* saver );

private:
	struct SExtensionLoaderPair
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );
		String						m_extension;
		IDependencyLoaderFactory*	m_loader;
	};
	struct SExtensionSaverPair
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );
		String						m_extension;
		IDependencySaverFactory*	m_saver;
	};

	TDynArray< SExtensionLoaderPair* >	m_loaders;
	TDynArray< SExtensionSaverPair* >	m_savers;
};

typedef TSingleton<CDependencyLinkerFactory> SDependencyLinkerFactory;

