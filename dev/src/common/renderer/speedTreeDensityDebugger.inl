/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

// Add visualisations here

// Grass instance density
template<>
RED_INLINE Uint32 CSpeedTreeDensityDebugger::CalculateColour< CSpeedTreeDensityDebugger::EMODE_GrassInstances >( const CSpeedTreeDensityTrackingData& data ) const
{
	if( data.GetGrassInstanceCount() > 0 )
	{
		Float colourScale = Min( 1.0f, ( data.GetGrassInstanceCount() / m_grassDensityPerCellBudget ) );
		Float colourScaleSq  = colourScale * colourScale;	
		return MakeRGBA( (Uint8)( 255.0f * colourScaleSq ), (Uint8)( 255.0f * (1.0f - colourScaleSq) ), 0, (Uint8)(255.0f * colourScale) );
	}
	else
	{
		return 0;
	}
}

// Tree instance density
template<>
RED_INLINE Uint32 CSpeedTreeDensityDebugger::CalculateColour< CSpeedTreeDensityDebugger::EMODE_TreeInstances >( const CSpeedTreeDensityTrackingData& data ) const
{
	if( data.GetTreeInstanceCount() > 0 )
	{
		Float colourScale = Min( 1.0f, ( data.GetTreeInstanceCount() / m_treeDensityPerCellBudget ) );
		Float colourScaleSq  = colourScale * colourScale;	
		return MakeRGBA( (Uint8)( 255.0f * colourScaleSq ), (Uint8)( 255.0f * (1.0f - colourScaleSq) ), 0, (Uint8)(255.0f * colourScale) );
	}
	else
	{
		return 0;
	}
}

// Grass layer density
template<>
RED_INLINE Uint32 CSpeedTreeDensityDebugger::CalculateColour< CSpeedTreeDensityDebugger::EMODE_GrassLayers >( const CSpeedTreeDensityTrackingData& data ) const
{
	if( data.GetGrassLayerCount() > 0 )
	{
		Float colourScale = Min( 1.0f, ( data.GetGrassLayerCount() / m_grassLayerDensityPerCellBudget ) );
		Float colourScaleSq  = colourScale * colourScale;	
		return MakeRGBA( (Uint8)( 255.0f * colourScaleSq ), (Uint8)( 255.0f * (1.0f - colourScaleSq) ), 0, (Uint8)(255.0f * colourScale) );
	}
	else
	{
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

RED_INLINE CSpeedTreeDensityTrackingData::CSpeedTreeDensityTrackingData()
	: m_grassInstances( 0 )
	, m_treeInstances( 0 )
	, m_grassLayerCount( 0 )
{

}

RED_INLINE CSpeedTreeDensityTrackingData::~CSpeedTreeDensityTrackingData()
{

}

RED_INLINE void CSpeedTreeDensityTrackingData::IncrementGrassInstanceCount( Uint16 instanceCount )
{
	m_grassInstances += instanceCount;
}

RED_INLINE void CSpeedTreeDensityTrackingData::IncrementTreeInstanceCount( Uint16 instanceCount )
{
	m_treeInstances += instanceCount;
}

RED_INLINE void CSpeedTreeDensityTrackingData::IncrementGrassLayerCount( Uint16 layerCount )
{
	m_grassLayerCount += layerCount;
}

RED_INLINE Uint16 CSpeedTreeDensityTrackingData::GetGrassInstanceCount() const
{
	return m_grassInstances;
}

RED_INLINE Uint16 CSpeedTreeDensityTrackingData::GetTreeInstanceCount() const
{
	return m_treeInstances;
}

RED_INLINE Uint16 CSpeedTreeDensityTrackingData::GetGrassLayerCount() const
{
	return m_grassLayerCount;
}

////////////////////////////////////////////////////////////////////////////////////////

RED_INLINE void CSpeedTreeDensityDebugger::AddGrassInstance( SpeedTree::Vec3 position )
{
	Int32 cellIndex = CalculateCellIndex( position );
	m_data[ cellIndex ].IncrementGrassInstanceCount();
}

RED_INLINE void CSpeedTreeDensityDebugger::AddTreeInstance( SpeedTree::Vec3 position )
{
	Int32 cellIndex = CalculateCellIndex( position );
	m_data[ cellIndex ].IncrementTreeInstanceCount();
}

RED_INLINE Int32 CSpeedTreeDensityDebugger::CalculateCellIndex( SpeedTree::Vec3 position )
{
	Int32 cellIndexX = static_cast< Int32 >( Red::Math::MFloor( position.x / m_cellExtents ) );
	Int32 cellIndexY = static_cast< Int32 >( Red::Math::MFloor( position.y / m_cellExtents ) );
	return cellIndexX + ( cellIndexY * m_cellsPerSide );
}

RED_INLINE Int32 CSpeedTreeDensityDebugger::CalculateCellIndex( Int32 x, Int32 y )
{
	return x + ( y * m_cellsPerSide );
}

RED_INLINE void CSpeedTreeDensityDebugger::ClearLayerToucher()
{
	m_layerMap.ClearFast();
}

RED_INLINE void CSpeedTreeDensityDebugger::TouchLayer( SpeedTree::Vec3 position )
{
	Int32 index = CalculateCellIndex( position );
	if( m_layerMap.Insert( index, true ) )
	{
		m_data[index].IncrementGrassLayerCount();
	}
}

RED_INLINE Uint32 CSpeedTreeDensityDebugger::MakeRGBA( Uint8 r, Uint8 g, Uint8 b, Uint8 a ) const
{
	return (((Uint32)a) << 24) + (((Uint32)b) << 16) + (((Uint32)g) << 8) + ((Uint32)r);
}

template< CSpeedTreeDensityDebugger::FoliageVisualisationMode mode >
RED_INLINE void CSpeedTreeDensityDebugger::RedrawBitmap( Uint32* lockedData, Uint32 texturePitch, Int32 cameraOffsetX, Int32 cameraOffsetY )
{
	// Fill in the texture data - 1 pixel per cell of metrics data
	CSpeedTreeDensityTrackingData currentCellData;
	for( Int32 y = 0; y < m_debugTextureResolution; ++y )
	{
		Uint32* textureData = lockedData + ( y * ( texturePitch / 4 ) );
		Int32 worldSpaceCellY = cameraOffsetY + y;
		for( Int32 x = 0; x < m_debugTextureResolution; ++x )
		{
			Int32 worldSpaceCellX = cameraOffsetX + x;
			Uint32 resultColour = 0x00000000;
			Int32 cellHash = CalculateCellIndex( worldSpaceCellX, worldSpaceCellY );
			if( m_data.Find( cellHash, currentCellData ) )
			{
				resultColour = CalculateColour< mode >( currentCellData );
			}

			*textureData++ = resultColour;
		}
	}
}