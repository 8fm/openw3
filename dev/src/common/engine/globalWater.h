/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "dynamicCollisionCollector.h"


class CGlobalWaterUpdateParams;
class IRenderScene;
class IRenderProxy;
class CGlobalWaterHeightmap;
class CLocalWaterShapesParams;

class CGlobalWater : public CObject
{
	friend class CGlobalWaterUpdateParams;

	DECLARE_ENGINE_CLASS( CGlobalWater, CObject, 0 );

public:
	CGlobalWater();
	CGlobalWater( CWorld* world );	
	~CGlobalWater();
		
	void											OnLocalShapeAttached( CWaterComponent* comp, Bool propertyChanged );
	void											Update( Float deltaTime );

	Float											GetWaterLevelAccurate( const Float X, const Float Y, Float* heightDepth = 0 ) const;
	Float											GetWaterLevelApproximate( const Float X, const Float Y, Float* heightDepth = 0 ) const;
	Float											GetWaterLevelBasic( const Float X, const Float Y ) const;
	Float											GetWaterLevelReference();
	Vector											GetWaterShoreProximityDampValue( Float hmapHeight, Float referenceWaterLevel ) const;

	Bool											GetWaterLevelBurst( Uint32 elementsCount, void* inputPos, void* outputPos, void* outputHeightDepth, size_t stride, Vector referencePosition, Bool useApproximation ) const;	

	void											Setup( IRenderScene* irs );
	void											DestroyProxy( IRenderScene* irs );

	void											Cooker_IncrementalShapeAddition( CWaterComponent* comp );

	void											NotifyTerrainOfLocalWaterChange();
	RED_FORCE_INLINE Bool							GetWaterRenderSurpass() const { return m_surpassWaterRender; }
	void											SetWaterRenderSurpass( Bool b ){ m_surpassWaterRender = b; }

private:
	IRenderProxy*									m_waterProxy;	// world global water render proxy
	CTextureArray*									m_waterShaderControlTexture;
	CGlobalWaterHeightmap*							m_waterHeightmap;
	CClipMap*										m_clipMap;
	
	TSortedArray< CGUID >							m_computedShapes;
	CLocalWaterShapesParams*						m_localShapesParams;
	TDynArray< Box >								m_localShapesBounds;
	Int32											m_numOfLocalShapesThatNeedsUpdate;

	CWorld*											m_world;
	Float											m_gameTime;

	CDynamicCollisionCollector*						m_dynamicCollector;
	TDynArray< SDynamicCollider >					m_activeNormalCollisions;
	//TDynArray< SDynamicCollider >					m_activeWaterDisplacementCollisions;

	Uint32											m_debugFrame;
	Uint32											m_debugAccess;

	Bool											m_needTerrainNotify;
	Bool											m_surpassWaterRender;

	Float											m_waterMaxLevel;

	Float											GetReferenceWaterLevel( const Float X, const Float Y ) const;

	void											Simulate( Float deltTime, CGlobalWaterUpdateParams* updateParams );
	void											GenerateLocalShapes();

	void											GenerateLocalShape( CWaterComponent* component, Uint32 shapeIndex );
};

BEGIN_CLASS_RTTI( CGlobalWater );
PARENT_CLASS( CObject );
END_CLASS_RTTI();
