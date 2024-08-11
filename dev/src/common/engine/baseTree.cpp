/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "baseTree.h"
#include "renderer.h"
#include "../core/diskFile.h"
#include "../core/cooker.h"
#include "../core/scopedPtr.h"

// BEGIN HACK! Use to get bbox and tree type ... 
#include <Core/Core.h> 
#include "../renderer/speedTreeLinkage.h"
// END HACK

IMPLEMENT_ENGINE_CLASS( CSRTBaseTree );

CSRTBaseTree::CSRTBaseTree()
	: m_type( BaseTreeTypeUnknown )
{

}

CSRTBaseTree::~CSRTBaseTree()
{}

void CSRTBaseTree::OnSerialize( IFile &file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// No serialization for GC is needed
	if ( file.IsGarbageCollector() )
	{
		return;
	}

	SerializeBuffer( file );
	
}

Bool CSRTBaseTree::Reload( Bool confirm )
{
#ifdef USE_SPEED_TREE

	// Inform listeners to remove instances of the tree before reloading
	EDITOR_DISPATCH_EVENT( CNAME( PreReloadBaseTree ), CreateEventData( CReloadBaseTreeInfo( this, String::EMPTY ) ) );

	// Release old object
	m_loadedTree.Reset();

	// Create file reader, to read SRT again
	IFile* reader = GFileManager->CreateFileReader( GetFile()->GetAbsolutePath(), FOF_AbsolutePath, s_maxSRTBufferSize );
	if ( !reader )
	{
		ASSERT( TXT("Couldn't create file reader for SRT!") );
		return false;
	}

	SerializeBuffer( *reader );

	// Reader's work is done
	delete reader;

	CreateRenderObject();

	// Inform listeners that file has been reloaded
	EDITOR_DISPATCH_EVENT( CNAME( ReloadBaseTree ), CreateEventData( CReloadBaseTreeInfo( this, String::EMPTY ) ) );

	// Post Reload is needed for world to handle generic grass update, which needs to be done after base tree reload in foliageEditionController
	EDITOR_DISPATCH_EVENT( CNAME( PostReloadBaseTree ), CreateEventData( CReloadBaseTreeInfo( this, String::EMPTY ) ) );
#endif
	return true;
}

void CSRTBaseTree::ReleaseTextures()
{
	GRender->ReleaseSpeedTreeTextures( m_loadedTree.Get() );
}

void CSRTBaseTree::SerializeBuffer( IFile & file )
{
	if ( file.IsReader() )
	{
		Uint32 fileSize = static_cast< Uint32 >( file.GetSize() );
		RED_ASSERT( (Uint64)fileSize == file.GetSize(), TXT("Unexpectedly large file '%ls'"), file.GetFileNameForDebug() );

		m_buffer = Red::CreateUniqueBuffer( fileSize, 16, MC_FoliageTree );
		file.Serialize( m_buffer.Get(), fileSize );
	}
	else
	{
		file.Serialize( m_buffer.Get(), m_buffer.GetSize() );
	}
}

void CSRTBaseTree::OnPostLoad()
{
	// Hack ? Forcing the creation of the render resource since it will do some heavy sync IO operation
	CreateRenderObject();

	if( m_type == BaseTreeTypeUnknown )
	{
		// So we don't have renderer here. But we need to know bbox and tree type. 
		//BEGIN HACK!
		Red::TScopedPtr< SpeedTree::CTree > speedTree( new SpeedTree::CTree );
		if ( speedTree->LoadTree( (SpeedTree::st_byte*)GetSRTData(), GetSRTDataSize(), true ) )
		{
			m_type = speedTree->IsCompiledAsGrass() ? BaseTreeTypeGrass : BaseTreeTypeTree;
			SpeedTree::CExtents ext = speedTree->GetExtents();
			m_bbox = Box( Vector( ext.Min().x, ext.Min().y, ext.Min().z ), Vector( ext.Max().x, ext.Max().y, ext.Max().z ) );
		}
		// END HACK!
	}

	const char* pError = SpeedTree::CCore::GetError();
	while (pError)
	{
		fprintf( stderr, "%s\n", pError );
		pError = SpeedTree::CCore::GetError();
	}

#ifdef NO_EDITOR
	m_buffer.Reset(); // Only need to keep this buffer around if we might to save it. Else, discard!
#endif

}

#ifndef NO_RESOURCE_COOKING

void CSRTBaseTree::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

#ifdef USE_SPEED_TREE
	TDynArray< String > usedTextures;
	ReportUsedTextures( static_cast< const AnsiChar* >( GetSRTData() ), GetSRTDataSize(), usedTextures );
	CFilePath fullPath( GetDepotPath() );
	for( const String& texture : usedTextures )
	{
		// build full path
		const String fullTexturePath = fullPath.GetPathString(true) + texture;
		cooker.ReportHardDependency( fullTexturePath );
	}
#endif
}

void CSRTBaseTree::ReportUsedTextures( const AnsiChar* dataBuffer, Uint32 dataSize, TDynArray< String >& outList )
{
#ifdef USE_SPEED_TREE
	// find the textures used by the speed tree
	// HACK: this is being done by looking for the "xxxxx.dds" strings in the resource buffer
	// each found string is treated as texture in the same directory as the tree
	// TODO: expose the SpeedTree middleware 
	if ( dataBuffer )
	{
		const AnsiChar extension[4] = { '.', 'd', 'd', 's' };
		const Uint32 extensionSize = sizeof(extension);

		for ( Uint32 i=0; i<(dataSize - extensionSize); ++i )
		{
			if ( 0 == Red::MemoryCompare( dataBuffer + i, &extension, extensionSize ) )
			{
				// look for start of the file name
				const AnsiChar* start = dataBuffer + i;
				while (start > dataBuffer && start[-1] != 0)
					--start;

				// get the file name
				const String texturePath( ANSI_TO_UNICODE( start ) );
				outList.PushBack( texturePath );
			}
		}
	}
#endif
}

#endif

Bool CSRTBaseTree::CreateRenderObject() const
{
#ifdef USE_SPEED_TREE
	m_loadedTree.Reset( GRender->CreateSpeedTreeResource( this ) );
#endif
	return m_loadedTree;
}

RenderObjectHandle CSRTBaseTree::AcquireRenderObject() const
{
	if( !m_loadedTree )
	{
		CreateRenderObject();
	}

	return m_loadedTree;
}

void CSRTBaseTree::SetBBox( const Box& bbox ) const
{ 
	m_bbox = bbox; 
}

RenderObjectHandle CSRTBaseTree::GetRenderObject() const
{ 
	return m_loadedTree; 
}

const Box& CSRTBaseTree::GetBBox() const
{ 
	return m_bbox;
}

const void*	CSRTBaseTree::GetSRTData() const
{
	return m_buffer.Get(); 
}

Uint32 CSRTBaseTree::GetSRTDataSize() const
{ 
	return m_buffer.GetSize(); 
}

void CSRTBaseTree::SetSRTData( Red::UniqueBuffer buffer )
{ 
	m_buffer = std::move( buffer );
}

void CSRTBaseTree::SetTreeType(EBaseTreeType type) const
{
	m_type = type;
}

Bool CSRTBaseTree::IsGrassType() const
{
	return m_type == BaseTreeTypeGrass;
}

EBaseTreeType CSRTBaseTree::GetType() const
{
	return m_type;
}
