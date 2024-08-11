/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibNodeSet.h"
#include "pathlibObstacleSpawnContext.h"

namespace PathLib
{

class CObstacle;
class CObstaclesMap;

class CObstacleGroup : public INodeSetPack
{
	friend class CObstacleGroupsCollection;													// m_id direct access
public:
	typedef Uint32				Id;
protected:
	typedef TDynArray< ExternalDepenentId > ObstaclesList;

	ObstaclesList					m_obstaclesList;
	Box								m_obstaclesBBox;
	Id								m_id;
	Bool							m_isAttached;
	
public:

	CObstacleGroup();
	~CObstacleGroup();

	Id							GetId() const												{ return m_id; }

	void						Attach( CObstaclesMap* obstaclesMap, CAreaDescription* area, CComponentRuntimeProcessingContext& context );
	void						Detach( CObstaclesMap* obstaclesMap, CAreaDescription* area, CComponentRuntimeProcessingContext& context );

	void						OnPostLoad( CAreaDescription* area );

	void						AddObstacle( CObstacle* obstacle );
	void						SimplifyShapes( CObstaclesMap* obstaclesMap );
	void						PostGraphGeneration( CNavGraph* navgraph );

	Bool						ReadFromBuffer( CSimpleBufferReader& reader );
	void						WriteToBuffer( CSimpleBufferWriter& writer ) const;
};


class CObstacleGroupsCollection
{
protected:
	typedef TArrayMap< SLayerMapping, CObstacleGroup* >	ObstaclesGroups;

	ObstaclesGroups					m_groups;

public:
	CObstacleGroupsCollection();
	~CObstacleGroupsCollection();

	CObstacleGroup*				GetObstacleGroup( const SLayerMapping& layer ) const;
	CObstacleGroup*				GetObstacleGroupById( CObstacleGroup::Id id ) const			{ return m_groups[ id ].m_second; }
	CObstacleGroup*				LazyCreateObstacleGroup( CObstaclesMap* obstacles, const SLayerMapping& layer );

	void						OnPostLoad( CObstaclesMap* obstacles, CAreaDescription* area, CComponentRuntimeProcessingContext& context );

	void						SimplifyShapes( CObstaclesMap* obstaclesMap );
	void						PostGraphGeneration( CNavGraph* navgraph );

	Bool						ReadFromBuffer( CSimpleBufferReader& reader );
	void						WriteToBuffer( CSimpleBufferWriter& writer ) const;
};




};		// namespace PathLib

