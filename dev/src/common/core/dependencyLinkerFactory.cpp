/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dependencyLinkerFactory.h"
#include "diskFile.h"
#include "dependencyLoaderFactory.h"
#include "dependencyLoader.h"
#include "dependencySaver.h"
#include "dependencySaverFactory.h"

IDependencyLoader* CDependencyLinkerFactory::GetLoader( const CDiskFile* diskFile, IFile& file )
{
	ASSERT( diskFile );

	const Char * fileExtension = Red::System::StringSearchLast( diskFile->GetFileName().AsChar(), L'.' );

	if ( fileExtension )
	{
		++fileExtension;

		// Check for registered loaders
		for ( Uint32 i = 0; i < m_loaders.Size(); ++i )
		{
			if ( 0 == Red::System::StringCompare( m_loaders[ i ]->m_extension.AsChar(), fileExtension ) )
			{
				return m_loaders[ i ]->m_loader->CreateLoader( file, diskFile );
			}
		}
	}
	
	// Else create normal loader
	return new CDependencyLoader( file, diskFile );
}

IDependencySaver* CDependencyLinkerFactory::GetSaver( const CDiskFile* diskFile, IFile& file )
{
	ASSERT( diskFile );

	const Char * fileExtension = Red::System::StringSearchLast( diskFile->GetFileName().AsChar(), L'.' );

	if ( fileExtension )
	{
		++fileExtension;

		// Check for registered savers
		for ( Uint32 i = 0; i < m_savers.Size(); ++i )
		{
			if ( 0 == Red::System::StringCompare( m_savers[ i ]->m_extension.AsChar(), fileExtension ) )
			{
				return m_savers[ i ]->m_saver->CreateSaver( file, diskFile );
			}
		}
	}
	// Else create normal saver
	return new CDependencySaver( file, diskFile );
}

IDependencySaver* CDependencyLinkerFactory::GetSaver( const String& ext, IFile& file )
{
	// Check for registered savers
	for ( Uint32 i = 0; i < m_savers.Size(); ++i )
	{
		if ( ext == m_savers[ i ]->m_extension )
		{
			return m_savers[ i ]->m_saver->CreateSaver( file, NULL );
		}
	}

	// Else create normal saver
	return new CDependencySaver( file, NULL );
}

void CDependencyLinkerFactory::RegisterLoaderFactory( const String& fileExtension, IDependencyLoaderFactory* loaderFactory, Bool overrideLoader /*= false*/ )
{
	SExtensionLoaderPair * pair = new SExtensionLoaderPair();
	pair->m_extension	= fileExtension;
	pair->m_loader		= loaderFactory;

	if ( ! overrideLoader )
	{
		m_loaders.PushBack( pair );
	}
	else
	{
		m_loaders.Insert( 0, pair );
	}
}

void CDependencyLinkerFactory::UnregisterLoaderFactory( IDependencyLoaderFactory* loaderFactory )
{
	for ( Int32 i = (Int32)m_loaders.Size() - 1; i >= 0; --i )
	{
		if ( m_loaders[ i ]->m_loader == loaderFactory )
		{
			delete m_loaders[ i ];
			m_loaders.EraseFast( m_loaders.Begin() + i );
		}
	}
}

void CDependencyLinkerFactory::RegisterSaverFactory( const String& fileExtension, IDependencySaverFactory* saverFactory )
{
	SExtensionSaverPair * pair = new SExtensionSaverPair();
	pair->m_extension	= fileExtension;
	pair->m_saver		= saverFactory;
	m_savers.PushBack( pair );
}

void CDependencyLinkerFactory::UnregisterSaverFactory( IDependencySaverFactory* saverFactory )
{
	for ( Int32 i = (Int32)m_savers.Size() - 1; i >= 0; --i )
	{
		if ( m_savers[ i ]->m_saver == saverFactory )
		{
			delete m_savers[ i ];
			m_savers.EraseFast( m_savers.Begin() + i );
		}
	}
}
