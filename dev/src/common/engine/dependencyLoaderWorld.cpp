/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#if 0

#include "dependencyLoaderWorld.h"
#include "../core/dependencyLinkerFactory.h"
#include "world.h"

CDependencyLoaderWorld::CDependencyLoaderWorld( IFile& file, CDiskFile* diskFile )
	: CDependencyLoader( file, diskFile )
{
}


Bool CDependencyLoaderWorld::LoadDependencies( TDynArray< FileDependency >& dependencies, Bool includeSoftHandles )
{
	CDependencyLoader::LoadDependencies( dependencies, includeSoftHandles );

	// If we're coming from an actual file, grab extra files in the same directory tree.
	if ( m_loadedFile != nullptr )
	{
		String directory = m_loadedFile->GetDirectory()->GetAbsolutePath();

		TDynArray < String > paths;
		// Enumerate layers and layer groups
		GFileManager->FindFiles( directory, TXT("*.w2l"), paths, true );
		GFileManager->FindFiles( directory, TXT("*.w2lg"), paths, true );

		// Enumerate umbra occlusion data
		GFileManager->FindFiles( directory, TXT("*.w3occlusiondef"), paths, false );
		for ( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			dependencies.PushBack( localPath.ToLower() );
		}
	}

	return dependencies.Size() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////

CDependencyLoaderWorldFactory::CDependencyLoaderWorldFactory()
{
	SDependencyLinkerFactory::GetInstance().RegisterLoaderFactory( ResourceExtension< CWorld >(), this );
}

CDependencyLoaderWorldFactory::~CDependencyLoaderWorldFactory()
{
	SDependencyLinkerFactory::GetInstance().UnregisterLoaderFactory( this );
}

IDependencyLoader* CDependencyLoaderWorldFactory::CreateLoader( IFile& file, CDiskFile* diskFile ) const
{
	return new CDependencyLoaderWorld( file, diskFile );
}

#endif