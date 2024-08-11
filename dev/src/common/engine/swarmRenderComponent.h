/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "drawableComponent.h"
#include "skeletonConsumer.h"
#include "meshEnum.h"
#include "renderSettings.h"

class CMeshTypeResource;
class IRenderSwarmData;
class CSwarmBoidData;

// Base for mesh-based components. Provides some basic things for forced LODs, coloring, etc.
class CSwarmRenderComponent	: public CDrawableComponent
							, public ISkeletonDataConsumer
{
	DECLARE_ENGINE_CLASS( CSwarmRenderComponent, CDrawableComponent, 0 );

	enum { MAX_NUM_BOIDS = 1024 };

protected:
	CMeshTypeResource*			m_mesh;
	CSkeletalAnimationSet*		m_animationSet;
	CSkeleton*					m_skeleton;

	TRenderObjectPtr< IRenderSwarmData>	m_swarmData;
	Uint32							m_boidCount;
	THashMap< CName, Float >		m_animationDurationMap;
	THandle< CEntityTemplate >		m_boidTemplateHandle;

	Bool							m_loadingDone;

	void RegisterTicks( Bool registerTick );
public:

	CSwarmRenderComponent();
	virtual ~CSwarmRenderComponent();

	virtual const ISkeletonDataConsumer* QuerySkeletonDataConsumer() const { return this; }

	void InitializeFromTemplate( CEntityTemplate *const entityTemplate );

	CMeshTypeResource* GetMesh() const							{ return m_mesh; }
	CSkeletalAnimationSet* GetAnimationSet() const				{ return m_animationSet; }
	CSkeleton* GetSkeleton() const								{ return m_skeleton; }
	void SetBoidCount( Uint32 boidCount )						{ m_boidCount = boidCount; }
	Float GetAnimationDuration( CName animationName )const;
	CEntityTemplate *const GetBoidTemplate();

	CSwarmBoidData* GetWriteData();

	// Explicitly set bounding box. The bounding box will not be updated automatically in OnUpdateBounds.
	void SetBoundingBox( const Box& b );
	void SetBoidTemplateHandle( const TSoftHandle<CEntityTemplate> & boidTemplateHandle );

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	virtual Float GetAutoHideDistance() const { return GetDefaultAutohideDistance(); }
	virtual Float GetDefaultAutohideDistance() const { return Config::cvSwarmHideDistance.Get(); }
	virtual Float GetMaxAutohideDistance() const { return 300.0f; }

	// tick
	void OnTickPostPhysics( Float timeDelta );

	virtual void OnInitializeProxy();

	virtual Bool IsDynamicGeometryComponent() const			{ return true; }

	// Should we update transform this node automatically when parent is update transformed /
	virtual Bool UsesAutoUpdateTransform() override		{ return true; }

	// Update world space bounding box based on the mesh returned by GetMesh.
	virtual void OnUpdateBounds();

	void OnLairActivated();
	void OnLairDeactivated();

	void OnAttached( CWorld* world ) override;
	void OnDetached( CWorld* world ) override;
};


BEGIN_CLASS_RTTI( CSwarmRenderComponent );
	PARENT_CLASS( CDrawableComponent );
	PROPERTY( m_boidTemplateHandle );
END_CLASS_RTTI();
