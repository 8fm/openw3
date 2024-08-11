/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/areaComponent.h"

#include "behTreeAreaSelection.h"
#include "behTreeCustomMoveData.h"
#include "behTreeDecorator.h"

class CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaInstance;

///////////////////////////////////////////////////////////////////////////////
// Thats just initial implementation of algorithm to find random point in convex area component. If it would start to get more useful it could get generalized and moved somewhere else.
struct SRandomPointInConvexAreaAlgorithm
{
private:
	THandle< CAreaComponent >				m_area;																// NOTICE: we are assuming that area world points are NOT changing in runtime

	TDynArray< Float >						m_triangleAreas;

	static void GetTriangle( const CAreaComponent::TAreaPoints& points, Uint32 triangle, Vector2* verts );
public:
	SRandomPointInConvexAreaAlgorithm();
	~SRandomPointInConvexAreaAlgorithm();

	Bool									PrecomputeForArea( CAreaComponent* area );
	void									ComputeRandomPoint( CAreaComponent* area, Vector2& outPoint );
};


///////////////////////////////////////////////////////////////////////////////
// Node that selects random position on clip map in given convex area
class CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaInstance, SelectRandomTerrainPositionInArea )

protected:
	SBehTreeAreaSelection					m_areaSelection;

	IBehTreeNodeDecoratorInstance*			SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaDefinition()											{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_areaSelection, TXT("Selected area") )
END_CLASS_RTTI()

class CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	SBehTreeSelectedAreaInstance			m_areaSelection;
	CBehTreeCustomMoveDataPtr				m_positionPtr;
	SRandomPointInConvexAreaAlgorithm		m_pointSelectionAlgorithm;
public:
	typedef CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaDefinition Definition;

	CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool									Activate() override;
};