/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../redMemory/include/scopedPtr.h"
#include "memoryFileReader.h"
#include "dependencyLoader2dArray.h"
#include "dependencyLinkerFactory.h"
#include "indexed2dArray.h"
#include "depot.h"

namespace credits
{
#include "credits.csv.h"
}
namespace credits_ep1
{
#include "credits_ep1.csv.h"
}
namespace credits_ep2
{
#include "credits_ep2.csv.h"
}

namespace
{
	struct BlobMapEntry
	{
		String m_fileName;
		const Uint8* m_blob;
		Uint32 m_blobSize;
	};

#define BLOBDEF(x) { MACRO_TXT("\\") MACRO_TXT(RED_STRINGIFY(x)) MACRO_TXT(".csv"), x::Data, sizeof(x::Data) }
	static BlobMapEntry BlobMap[] = {
		BLOBDEF( credits ),
		BLOBDEF( credits_ep1 ),
		BLOBDEF( credits_ep2 ),
	};
#undef BLOBDEF

	static Bool FindBlob( const String& depotPath, BlobMapEntry** ppOutEntry )
	{
		RED_FATAL_ASSERT( ppOutEntry, "" );
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32( BlobMap ); ++i )
		{
			if ( depotPath.EndsWith( BlobMap[i].m_fileName ) )
			{
				*ppOutEntry = &BlobMap[i];
				return true;
			}
		}
		return false;
	}
}

CDependencyLoader2dArray::CDependencyLoader2dArray( IFile& file, const CDiskFile* diskFile )
	: IDependencyLoader( file, diskFile )
	, m_filePath( diskFile->GetDepotPath() )
{
}

Bool CDependencyLoader2dArray::LoadObjects( DependencyLoadingContext& context )
{
	RED_FATAL_ASSERT( context.m_firstExportMemory != nullptr, "No resource memory specified" );
	RED_FATAL_ASSERT( context.m_firstExportMemorySize >= sizeof(C2dArray), "Resource memory is to small" );

	RED_FATAL_ASSERT( context.m_firstExportMemory->IsA< C2dArray >(), "Invalid resource class passed to dependency loader" );
	C2dArray* arr = static_cast< C2dArray* >( context.m_firstExportMemory );

	RED_FATAL_ASSERT( arr->IsLoading(), "Resource is not loading yet we are in LoadObjects" );
	RED_FATAL_ASSERT( arr->GetFile() != nullptr, "Resource is not bound yet we are in LoadObjects" );

	String fileData;

	// Overriding current CSV, *not* a fallback for a missing file
	BlobMapEntry* pBlob = nullptr;
	if ( FindBlob( m_filePath, &pBlob ) )
	{
		WARN_CORE( TXT("CDependencyLoader2dArray loaded binarized CSV for '%ls'"), m_filePath.AsChar() );
		red::TScopedPtr< IFile > reader( new CMemoryFileReaderExternalBuffer( pBlob->m_blob, pBlob->m_blobSize ) );
		GFileManager->LoadFileToString( reader.Get(), fileData );
	}
	else
	{
		GFileManager->LoadFileToString( this, fileData );
	}

	arr->ParseData( fileData );

	// return loaded objects
	context.m_loadedRootObjects.PushBack( arr );
	context.m_loadedResources.PushBack( arr );

	return true;
}

Bool CDependencyLoader2dArray::LoadDependencies( TDynArray< FileDependency >&, Bool )
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////

CDependencyLoader2dArrayFactory::CDependencyLoader2dArrayFactory()
{
	SDependencyLinkerFactory::GetInstance().RegisterLoaderFactory( ResourceExtension< C2dArray >(), this );
	SDependencyLinkerFactory::GetInstance().RegisterLoaderFactory( ResourceExtension< CIndexed2dArray >(), this );
}

CDependencyLoader2dArrayFactory::~CDependencyLoader2dArrayFactory()
{
	SDependencyLinkerFactory::GetInstance().UnregisterLoaderFactory( this );
}

IDependencyLoader* CDependencyLoader2dArrayFactory::CreateLoader( IFile& file, const CDiskFile* diskFile ) const
{
	return new CDependencyLoader2dArray( file, diskFile );
}
