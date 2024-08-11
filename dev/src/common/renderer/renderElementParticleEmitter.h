/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderElement.h"

class IParticleData;
class CRenderParticleEmitter;
/// Single particle emitter
class CRenderElement_ParticleEmitter : public IRenderElement
{
	DECLARE_RENDER_OBJECT_MEMORYPOOL( MemoryPool_SmallObjects, MC_RenderElementParticleEmitter );

public:
	IParticleData*			m_data;							//!< Rendering data
	CRenderParticleEmitter*	m_emitter;						//!< Brand new render side particle emitter, hell yeah! :)
	Float					m_distanceFromCameraSquared;	//!< Squared distance of the emitter to camera
	Uint64					m_sortingKey;					//!< Sorting key for emitter
	EEnvColorGroup			m_envColorGroup;
	EMaterialVertexFactory	m_vertexFactory;

public:
	//! Get rendering data
	RED_INLINE IParticleData* GetData() const { return m_data; }

	//! Get render emitter
	RED_INLINE CRenderParticleEmitter* GetEmitter() const { return m_emitter; }

	//! Get squared distance from camera
	RED_INLINE Float GetDistanceFromCameraSquared() const { return m_distanceFromCameraSquared; }

	//! Get sorting key
	RED_INLINE Uint64 GetSortingKey() const { return m_sortingKey; }

	//! Get environment color group
	RED_INLINE EEnvColorGroup GetEnvColorGroup() const { return m_envColorGroup; }

	EMaterialVertexFactory GetVertexFactory() const { return m_vertexFactory; }

public:
	CRenderElement_ParticleEmitter( IRenderProxyDrawable* proxy, const IMaterial* material, EEnvColorGroup envColorGroup, Uint32 emitterID, CRenderParticleEmitter* renderEmitter );
	CRenderElement_ParticleEmitter( IRenderProxyDrawable* proxy, CRenderParticleEmitter* renderEmitter, CRenderMaterial* material, CRenderMaterialParameters* parameters, EEnvColorGroup envColorGroup );
	~CRenderElement_ParticleEmitter();

	//! Update distance from Point
	void UpdateDistanceFromPoint( const Vector & point , Uint8 internalOrder = 0 , Uint8 renderPriority = 4 );

#ifndef NO_EDITOR
	void ReplaceEmitter( CRenderParticleEmitter* renderParticleEmitter, CRenderMaterial* material, CRenderMaterialParameters* parameters, EEnvColorGroup envColorGroup );
#endif

protected:
	void CreateParticleData();
};