/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTerrainUpdateData.h"

#include "..\..\common\engine\clipMap.h"

CRenderTerrainUpdateData::CRenderTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& updates, const STerrainTextureParameters* textureParameters, SClipmapStampDataUpdate* stampDataUpdate, Vector* colormapParams )
	: m_update( updates )
	, m_material( NULL )
	, m_stampUpdate( NULL )
	, m_colormapParams( *colormapParams )
#ifndef NO_EDITOR
	, m_isEditing( false )
#endif
{
	if ( textureParameters )
	{
		Red::System::MemoryCopy( (void*)&m_textureParams[0], (void*)textureParameters, NUM_TERRAIN_TEXTURES_AVAILABLE * sizeof( STerrainTextureParameters ) );
	}

	// Clone stamp data. Can't just keep the pointer, since it could potentially be freed at any time.
	if ( stampDataUpdate )
	{
		// Can use copy constructor to do most data
		m_stampUpdate = new SClipmapStampDataUpdate( *stampDataUpdate );
		// Now just need to re-allocate and copy data buffers. Only copy if that buffer is dirty, to avoid extra copies when nothing has changed.
		m_stampUpdate->m_heightData = NULL;
		m_stampUpdate->m_controlData = NULL;
		m_stampUpdate->m_colorData = NULL;
		if ( stampDataUpdate->m_heightData && stampDataUpdate->m_heightDataDirty )
		{
			m_stampUpdate->m_heightData = (Uint16*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderData, stampDataUpdate->m_heightDataSize );
			Red::System::MemoryCopy( m_stampUpdate->m_heightData, stampDataUpdate->m_heightData, stampDataUpdate->m_heightDataSize );
		}
		if ( stampDataUpdate->m_controlData && stampDataUpdate->m_controlDataDirty )
		{
			m_stampUpdate->m_controlData = (TControlMapType*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderData, stampDataUpdate->m_controlDataSize );
			Red::System::MemoryCopy( m_stampUpdate->m_controlData, stampDataUpdate->m_controlData, stampDataUpdate->m_controlDataSize );
		}
		if ( stampDataUpdate->m_colorData && stampDataUpdate->m_colorDataDirty )
		{
			m_stampUpdate->m_colorData = (TColorMapType*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderData, stampDataUpdate->m_colorDataSize );
			Red::System::MemoryCopy( m_stampUpdate->m_colorData, stampDataUpdate->m_colorData, stampDataUpdate->m_colorDataSize );
		}
	}
}

CRenderTerrainUpdateData::~CRenderTerrainUpdateData()
{
	for ( Uint32 i=0; i<m_update.Size(); ++i )
	{
		delete m_update[i];
	}

	if ( m_material )
	{
		m_material->Release();
	}

	if ( m_stampUpdate )
	{
		if ( m_stampUpdate->m_heightData )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_RenderData, m_stampUpdate->m_heightData );
			m_stampUpdate->m_heightData = NULL;
		}
		if ( m_stampUpdate->m_controlData )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_RenderData, m_stampUpdate->m_controlData );
			m_stampUpdate->m_controlData = NULL;
		}
		if ( m_stampUpdate->m_colorData )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_RenderData, m_stampUpdate->m_colorData );
			m_stampUpdate->m_colorData = NULL;
		}
		delete m_stampUpdate;
		m_stampUpdate = NULL;
	}
}

IRenderObject* CRenderInterface::CreateTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& updates, IRenderObject* material, const STerrainTextureParameters* textureParameters, SClipmapStampDataUpdate* stampUpdate, Vector* colormapParams )
{
	CRenderTerrainUpdateData* updateData = new CRenderTerrainUpdateData( updates, textureParameters, stampUpdate, colormapParams );
	if ( material )
	{
		updateData->m_material = material;
		updateData->m_material->AddRef();
	}

	return updateData;
}

#ifndef NO_EDITOR
IRenderObject* CRenderInterface::CreateEditedTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& updates, IRenderObject* material, const STerrainTextureParameters* textureParameters, SClipmapStampDataUpdate* stampUpdate, Vector* colormapParams )
{
	CRenderTerrainUpdateData* updateData = (CRenderTerrainUpdateData*)CreateTerrainUpdateData( updates, material, textureParameters, stampUpdate, colormapParams );
	updateData->SetIsEditing();

	return updateData;
}
#endif
