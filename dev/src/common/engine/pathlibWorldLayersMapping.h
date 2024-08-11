/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibConst.h"
#include "pathlibObstacleSpawnContext.h"

class CPathLibWorld;

namespace PathLib
{

// Class that maps every world layer group to areas that are interested of its visibility.
// Its constrained with layer system, as it holds current layer state, if they are visible or not.
// But its crucial for pathlib to be able to asynchronously query over enabled/disabled layers.
class CWorldLayersMapping
{
public:
	typedef TDynArray< SLayerMapping > LayersList;
	typedef TDynArray< AreaId > AreaList;

	struct LayerInfo
	{
		Bool							m_enabled;																						// mutable, asynchronously accessed
		AreaList						m_areaList;																						// const during gameplay
	};

	typedef THashMap< SLayerMapping, LayerInfo > LayersMap;

protected:

	LayersMap						m_layers;																							// const during gameplay
	LayersList						m_pendingLayersList;																				// modified on synchronous interface
	Bool							m_isTaskProcessing;																					// modified and read synchronously

	void						SpawnTask( CPathLibWorld& pathlib );

public:
	CWorldLayersMapping();
	~CWorldLayersMapping();

	void						Initialize( CWorld* world );

	void						AddAreaDependency( const SLayerMapping& mapping, AreaId areaId );
	void						OnLayerEnabled( CPathLibWorld& pathlib, CLayerGroup* layerGroup, Bool enabled );
	void						LayerProcessingFinished( CPathLibWorld& pathlib );														// synchronous interface for world layer processing task
	LayersList&&				StealPendingLayers()																					{ return Move( m_pendingLayersList ); }

	Bool						IsLayerEnabled( const SLayerMapping& layer ) const;														// IMPORTANT: possibly asynchronous calls
	const LayerInfo*			GetLayerInfo( const SLayerMapping& layer ) const;														// IMPORTANT: possibly asynchronous calls

	void						Generation_MapLayer( SLayerMapping& layer, AreaId areaId );

	Bool						ReadFromBuffer( CSimpleBufferReader& reader );
	void						WriteToBuffer( CSimpleBufferWriter& writer ) const;
};



};			// namespace PathLib

