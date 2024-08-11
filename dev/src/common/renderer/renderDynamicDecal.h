/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderDynamicResource.h"
#include "../../common/engine/dynamicDecal.h"
#include "renderTextureBase.h"
#include "renderFramePrefetch.h"


#ifdef RED_PLATFORM_DURANGO
// Temporary hack until we can get proper solution from MS. GS used for culling the decal triangles is spitting out garbage.
#	define DYNAMIC_DECAL_NO_GS_CULLING
#endif


class CRenderDynamicDecalChunk;
class CRenderSceneEx;
struct SDynamicDecalInitInfo;
class IRenderProxyDrawable;
class CRenderProxy_Decal;


class CRenderDynamicDecal : public IDynamicRenderResource, public IRenderPrefetchable
{
protected:
	TDynArray< CRenderDynamicDecalChunk* >	m_chunks;			// List of chunks for this decal.
	SDynamicDecalInitInfo					m_initInfo;			// Initialization info, basically the decal's current state.
	Matrix									m_parentToDecal;	// Parent space to Decal. Most of the time, the parent is world space or mesh local.
																// Calculated from initInfo, but cached to avoid re-calculations.

	CRenderProxy_Decal*						m_staticDecal;		// Screen-space decal applied to static objects.

	CRenderSceneEx*							m_scene;

	Float									m_lifetime;			// total lifetime of decal

	static Red::Threads::CMutex				m_internalMutex;	// Internal mutex for chunks

public:
	CRenderDynamicDecal( const SDynamicDecalInitInfo& initInfo );
	~CRenderDynamicDecal();

	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override;

	virtual String GetDisplayableName() const;
	virtual CName GetCategory() const;
	virtual Uint32 GetUsedVideoMemory() const;

	virtual void OnDeviceLost();
	virtual void OnDeviceReset();

	RED_FORCE_INLINE CRenderTextureBase* GetDiffuseTexture() const { return static_cast< CRenderTextureBase* >( m_initInfo.m_diffuseTexture ); }
	RED_FORCE_INLINE CRenderTextureBase* GetNormalTexture() const { return static_cast< CRenderTextureBase* >( m_initInfo.m_normalTexture ); }

	RED_FORCE_INLINE Bool ApplyInLocalSpace() const { return m_initInfo.m_applyInLocalSpace; }
	RED_FORCE_INLINE Bool IsDoubleSided() const { return m_initInfo.m_doubleSided; }
	RED_FORCE_INLINE Bool IsEllipsoid() const { return m_initInfo.m_projectionMode == RDDP_Sphere; }

	RED_FORCE_INLINE const Matrix& GetParentToDecal() const { return m_parentToDecal; }
	RED_FORCE_INLINE const Matrix& GetWorldToDecalParent() const { return m_initInfo.m_worldToDecalParent; }

	RED_FORCE_INLINE Bool HasNormalTexture() const { return m_initInfo.m_normalTexture != nullptr; }

	RED_FORCE_INLINE Bool HasAdditiveNormals() const { return m_initInfo.m_additiveNormals; }

	RED_FORCE_INLINE Float GetTimeToLive() const { return m_initInfo.m_timeToLive; }
	RED_FORCE_INLINE Float GetAutoHideDistance() const { return m_initInfo.m_autoHideDistance; }
	RED_FORCE_INLINE Vector GetAtlasVector() const { return m_initInfo.m_atlasVector; }
	RED_FORCE_INLINE EDynamicDecalSpawnPriority GetSpawnPriority() const { return m_initInfo.m_spawnPriority; }

	// Get the AABB of the decal when it was created. Only really useful at/near creation time.
	Box GetInitialBounds() const;

	RED_FORCE_INLINE Uint32 GetNumChunks() const { return m_chunks.Size(); }

	// Collect decal chunks. Only needed when not using umbra.
	void CollectChunks( CRenderCollector& collector );

	// Register the decal chunks with Umbra, for scene collection. There isn't a matching UnregisterChunks(), because
	// each chunk will be unregistered when it is destroyed (since they can be destroyed at different times).
	void RegisterChunks();

	virtual void AttachToScene( CRenderSceneEx* scene, Bool createStaticDecal );
	virtual void DetachFromScene( CRenderSceneEx* scene );

	// Return false if the decal can be removed (TTL has expired, or has no chunks and no screenspace decal).
	Bool Tick( CRenderSceneEx* scene, Float timeDelta );

	// Send generation parameters to renderer. worldToDecal transforms from the generation space (whether it's world, or local) to the decal.
	void BindGenerateParameters( const Matrix& worldToDecal ) const;

	// Send rendering parameters to renderer.
	void BindRenderParameters() const;


	// Destroy all dynamic chunks in this decal. Returns true if the decal should still be kept alive (if it has
	// a screenspace decal), false if it can be removed entirely.
	Bool DestroyChunks();


	// Add a created chunk to the decal. Will addref the chunk, so caller can release without it being destroyed.
	void AddChunk( CRenderDynamicDecalChunk* chunk );

	// Destroy this decal. All chunks will be destroyed. Should only be called by the scene, so that the scene can properly remove it as well.
	void DestroyDecal();

	// Called by CRenderDynamicDecalChunk when it is destroyed. Allows us to remove the chunk from our own list.
	void OnDynamicDecalChunkDestroyed( CRenderDynamicDecalChunk* chunk );

protected:
	Float GetAlpha() const;
	Float GetScale() const;
};
