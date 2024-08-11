/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CRenderFrame;

enum EDynamicCollisionTypes
{	
	DYNAMIC_COLLIDERS_TYPE_GRASS,
	DYNAMIC_COLLIDERS_TYPE_WATER_DISPLACEMENT,
	DYNAMIC_COLLIDERS_TYPE_WATER_NORMAL,
	DYNAMIC_COLLIDERS_TYPES_NUMBER							//<! max
};

struct SDynamicCollider
{
	Float									m_intensity;
	Float									m_decaySpeed;
	Matrix									m_transformMatrix;
	CDynamicColliderComponent*				m_parent;
	Bool									m_useHideFactor;

	DECLARE_RTTI_STRUCT( SDynamicCollider );

	SDynamicCollider()
	{
		m_transformMatrix = Matrix::IDENTITY;
		m_decaySpeed = 0.0f;
		m_intensity = 0.0f;
		m_parent = nullptr;
		m_useHideFactor = false;
	};
};

BEGIN_CLASS_RTTI( SDynamicCollider );
END_CLASS_RTTI();

struct SDynamicCollidersCollection
{	
	THashMap< CDynamicColliderComponent*, Int32 >			m_colliderPair;
	TDynArray< SDynamicCollider >							m_activeCollisions;//!< This is the currently active collision list that goes to renderer
};

class CDynamicCollisionCollector
{	
public:

	TDynArray< Red::TUniquePtr< SDynamicCollidersCollection > >	m_dynamicCollection;

	void													Tick( CWorld* world, Float timeDelta );

	void													Add( CDynamicColliderComponent* cmp );
	void													Remove( CDynamicColliderComponent* cmp, Bool forceRemoval = false );
	void													Shutdown();
	void													GenerateEditorFragments( CRenderFrame* f );
	RED_INLINE TDynArray< SDynamicCollider >&				GetActiveColliders( Uint32 type ) { return m_dynamicCollection[ type ]->m_activeCollisions; };

	void													TryActivateCollider( Uint32 typeIndex, const SDynamicCollider &c );

	CDynamicCollisionCollector( CWorld* world );
	~CDynamicCollisionCollector();

private:

	Float													m_currentTimeAcum;
	Bool													m_forceOcclude;
										
	Uint32													m_pereferableTickRange;	//!< Optimization of OnTick() - in case someone want to insert millions of components
	Uint32													m_currentTickBlock;
	Uint32													m_currentTickBlockBegin;
	Uint32													m_currentTickBlockEnd;

	Int32													GetFreeSlot( Uint32 type );

	void													OccludeColliders( const Vector &cameraPosition, Bool forceOcclude );
	void													RefreshActiveCollider( Uint32 type, Float timeDelta );
	
	void													FetchToSpeedtree( CWorld* world, Float timeDelta );

	Bool													ReleaseCollider( Uint32 typeIndex, CDynamicColliderComponent* cmp, Bool forceRemoval = false );
	void													AddCollider( Uint32 typeIndex, CDynamicColliderComponent* cmp );
};
