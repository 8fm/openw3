/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "speedTreeDensityDebugger.h"

#ifdef USE_SPEED_TREE

CSpeedTreeDensityDebugger::CSpeedTreeDensityDebugger()
	: m_cellExtents( 0.0f )
	, m_worldExtents( 0.0f )
	, m_cellsPerSide( 0 )
	, m_totalCells( 0 )
	, m_debugTextureResolution( 0 )
	, m_renderTexture( NULL )
	, m_visualisation( EMODE_GrassInstances )
{
}

CSpeedTreeDensityDebugger::~CSpeedTreeDensityDebugger()
{
	GpuApi::RemoveDynamicTexture( m_renderTexture );
	GpuApi::SafeRelease( m_renderTexture );
}

void CSpeedTreeDensityDebugger::SetGrassDensityBudget( Float instancesPerSqMeter )
{
	// Average the density over the entire visualisation cells
	m_grassDensityPerCellBudget = instancesPerSqMeter * ( m_cellExtents * m_cellExtents );
}

void CSpeedTreeDensityDebugger::SetTreeDensityBudget( Float instancesPerSqMeter )
{
	// Average the density over the entire visualisation cells
	m_treeDensityPerCellBudget = instancesPerSqMeter * ( m_cellExtents * m_cellExtents );
}

void CSpeedTreeDensityDebugger::SetGrassLayerDensityBudget( Float layersPerSqMeter )
{
	// We don't multiply up over cell scale - treat as an average density target
	m_grassLayerDensityPerCellBudget = layersPerSqMeter;
}

void CSpeedTreeDensityDebugger::Initialise( Float worldExtents, Float cellExtents, Uint32 debugTextureResolution )
{
	RED_ASSERT( m_renderTexture.isNull(), TXT( "Texture already exists! Calling this twice is a really bad idea!" ) );
	m_worldExtents = worldExtents;
	m_cellExtents = cellExtents;
	m_cellsPerSide = static_cast< Uint32 >( Red::Math::MCeil( m_worldExtents / m_cellExtents ) );
	m_totalCells = m_cellsPerSide * m_cellsPerSide;
	m_debugTextureResolution = (Int32)debugTextureResolution;
	ResetAll();
}

void CSpeedTreeDensityDebugger::ResetAll()
{
	m_data.ClearFast();
}

void CSpeedTreeDensityDebugger::SetMode( FoliageVisualisationMode m )
{
	m_visualisation = m;
}

void CSpeedTreeDensityDebugger::CreateRenderResources()
{
	if( !m_renderTexture )
	{
		// Create the visualisation texture if it doesn't exist yet
		GpuApi::TextureDesc texDesc;
		texDesc.type		= GpuApi::TEXTYPE_2D;
		texDesc.format		= GpuApi::TEXFMT_R8G8B8A8;
		texDesc.initLevels	= 1;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Dynamic;
		texDesc.width		= m_debugTextureResolution;
		texDesc.height		= m_debugTextureResolution;
		texDesc.sliceNum	= 1;		// No mips
		m_renderTexture	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_System );
		ASSERT( m_renderTexture );
		GpuApi::SetTextureDebugPath(m_renderTexture, "SpeedtreeDensity"); 
		GpuApi::AddDynamicTexture(m_renderTexture, "SpeedtreeDensity");
	}
}

void CSpeedTreeDensityDebugger::UpdateTexture( const Vector& cameraPosition, const Vector& cameraForward )
{
	if( m_visualisation != EMODE_None )
	{
		CreateRenderResources();

		// Calculate world space rectangle covered by the texture
		const Float rectExtents = m_debugTextureResolution * m_cellExtents;
		m_currentArea.X = Red::Math::MFloor( cameraPosition.X / m_cellExtents ) * m_cellExtents - rectExtents/2.0f;
		m_currentArea.Y = Red::Math::MFloor( cameraPosition.Y / m_cellExtents ) * m_cellExtents - rectExtents/2.0f;
		m_currentArea.Z = m_currentArea.X + rectExtents;
		m_currentArea.W = m_currentArea.Y + rectExtents;

		// Calculate the camera position in density-space (is that a thing? it is now)
		Int32 cameraCenterCellX = (Int32)Red::Math::MFloor( cameraPosition.X / m_cellExtents );
		Int32 cameraCenterCellY = (Int32)Red::Math::MFloor( cameraPosition.Y / m_cellExtents );
		Int32 cameraCenterOffset = m_debugTextureResolution >> 1;		// Camera is centered on the texture
		
		Uint32 texturePitch = 0;
		Uint32* lockedData = ( Uint32* )GpuApi::LockLevel( m_renderTexture, 0, 0, GpuApi::BLF_Discard, texturePitch );
		Uint32* textureData = lockedData;
	
		// Fill the bitmap
		switch( m_visualisation )
		{
		case EMODE_GrassInstances:
			RedrawBitmap<EMODE_GrassInstances>( lockedData, texturePitch, cameraCenterCellX - cameraCenterOffset, cameraCenterCellY - cameraCenterOffset );
			break;
		case EMODE_GrassLayers:
			RedrawBitmap<EMODE_GrassLayers>( lockedData, texturePitch, cameraCenterCellX - cameraCenterOffset, cameraCenterCellY - cameraCenterOffset );
			break;
		case EMODE_TreeInstances:
			RedrawBitmap<EMODE_TreeInstances>( lockedData, texturePitch, cameraCenterCellX - cameraCenterOffset, cameraCenterCellY - cameraCenterOffset );
			break;
		}

		GpuApi::UnlockLevel( m_renderTexture, 0, 0 );
	}
}

#endif