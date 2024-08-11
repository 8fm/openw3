/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../core/dependencyLinkerFactory.h"
#include "../core/depot.h"
#include "dependencyLoaderSwf.h"
#include "swfResource.h"

CDependencyLoaderSwf::CDependencyLoaderSwf( IFile& file, const CDiskFile* diskFile )
	: CDependencyLoader( file, diskFile )
{
}

Bool CDependencyLoaderSwf::LoadObjects( DependencyLoadingContext& context )
{
	const Uint64 baseOffset = m_file->GetOffset();

	if ( TBaseClass::LoadObjects(context) )
	{
		return true;
	}

	// Mod support. Allow loading a *.swf or *.gfx renamed to .redswf
#ifdef RED_PLATFORM_WINPC
	RED_FATAL_ASSERT( context.m_firstExportMemory != nullptr, "No resource memory specified" );
	RED_FATAL_ASSERT( context.m_firstExportMemorySize >= sizeof(CSwfResource), "Resource memory is to small" );

	RED_FATAL_ASSERT( context.m_firstExportMemory->IsA< CSwfResource >(), "Invalid resource class passed to dependency loader" );

	CSwfResource* swfResource = static_cast< CSwfResource* >( context.m_firstExportMemory );

	RED_FATAL_ASSERT( swfResource->IsLoading(), "Resource is not loading yet we are in LoadObjects" );
	RED_FATAL_ASSERT( swfResource->GetFile() != nullptr, "Resource is not bound yet we are in LoadObjects" );

	m_file->Seek( baseOffset ); // rewind file, try loading as a raw SWF
	const DataBuffer::TSize bufferSize = static_cast< DataBuffer::TSize >( GetSize() );
	LOG_ENGINE(TXT("Loading raw SWF '%ls' of size %llu"), m_file->GetFileNameForDebug(), bufferSize );
	swfResource->m_dataBuffer.Allocate( bufferSize );
	m_file->Serialize( swfResource->m_dataBuffer.GetData(), bufferSize );

	if ( !CSwfResource::VerifySwf( swfResource->m_dataBuffer ) )
	{
		ERR_ENGINE(TXT("Not a valid SWF file: '%ls'"), m_file->GetFileNameForDebug());
		return false;
	}

	swfResource->m_linkageName = String::Printf(TXT("%ls [%ls]"), CSwfResource::RAW_SWF_LINKAGE_NAME, m_file->GetFileNameForDebug());

	WARN_ENGINE(TXT("Loaded raw SWF '%ls'."), m_file->GetFileNameForDebug());

	// return loaded objects
	context.m_loadedRootObjects.PushBack( swfResource );
	context.m_loadedResources.PushBack( swfResource );

	return true;
#else
	return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////

CDependencyLoaderSwfFactory::CDependencyLoaderSwfFactory()
{
	SDependencyLinkerFactory::GetInstance().RegisterLoaderFactory( ResourceExtension< CSwfResource >(), this );
}

CDependencyLoaderSwfFactory::~CDependencyLoaderSwfFactory()
{
	SDependencyLinkerFactory::GetInstance().UnregisterLoaderFactory( this );
}

IDependencyLoader* CDependencyLoaderSwfFactory::CreateLoader( IFile& file, const CDiskFile* diskFile ) const
{
	return new CDependencyLoaderSwf( file, diskFile );
}
