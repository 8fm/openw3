/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderElement_ParticleEmitter;
class CRenderProxy_Particles;
class CRenderMesh;
struct MeshParticle;

struct SParticleInstanceDescriptor
{
	Float	m_localToWorld[12];
	Vector	m_color;
};

class CSimulateParticlesTask : public CTask
{
public:
	CSimulateParticlesTask( const CRenderCollector* collector, THashMap< CRenderProxy_Particles*, Box >* proxiesBoundingBoxes, Float velocityTimeDelta, Bool onScreen );

	//! OnScreen particles are going to be rendered in the upcoming frame
	//! OffScreen particles are within the showing distance but are outside of the camera frustum
	Bool m_onScreenParticles;
	Float m_velocityTimeDelta;

	const CRenderCollector* m_collector;
	THashMap< CRenderProxy_Particles*, Box >* m_proxiesBoundingBoxes;

#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override { return TXT("CSimulateOnScreenParticlesTask"); }
	virtual Uint32 GetDebugColor() const override { return COLOR_UINT32( 23, 233, 250 ); }
#endif

	virtual void Run() override;

private:
	void SimulateParticles();
};

/// Particle renderer
class CRenderPartricleBatcher : public IDynamicRenderResource
{
protected:
	GpuApi::BufferRef										m_indexBufferRef;

	CRenderMaterial*										m_lastMaterial;
	CRenderMaterialParameters*								m_lastMaterialParameters;

	TDynArray< TDynArray< SParticleInstanceDescriptor >>	m_instances;				//!< instances generated from an emitter <MeshIndex, TDynArray<InstanceDesc>>
	GpuApi::BufferRef										m_instancesBufferRef;		//!< Patch instancing buffer
	Int32													m_instancesDataElemOffset;	//!< Current instances write position
	Int32													m_frameInstancesDataElemOffset; //!< Current frame instances write position
	Bool													m_meshParticleBufferDiscardedThisFrame;
	Bool													m_simulationStartedThisFrame;

	Float													m_lastGameTime;				//!< Game time the last position was captured
	Float													m_velocityTimeDelta;		//!< velocity time delta

	THashMap< CRenderProxy_Particles*, Box >				m_proxiesBoundingBoxes;

	CSimulateParticlesTask* m_taskOnScreen;
	CSimulateParticlesTask* m_taskOffScreen;

public:
	CRenderPartricleBatcher();
	~CRenderPartricleBatcher();

	//! Render particle emitters
	void RenderParticles( const CRenderCollector& collector, const RenderingContext& context, const TDynArray< CRenderElement_ParticleEmitter* >& batchList );
	void RenderEmitter( const CRenderCollector& collector, const RenderingContext& context, CRenderElement_ParticleEmitter* element );

	//! Simulate on screen particle emitters
	void SimulateOnScreenParticles( const CRenderCollector& collector );
	//! Finish simulating onscreen particles
	void FinishSimulateOnScreenParticles();
	
	//! Finish simulating onscreen particles, it starts job thread for offscreen particles simulation
	void FinishOnScreenStartOffScreenSimulation(const CRenderCollector& collector );

	//! Simulate off screen particle emitters
	void SimulateOffScreenParticles( const CRenderCollector& collector );
	//! Finish simulating offscreen particles
	void FinishSimulateOffScreenParticles();

	//! finish simulating particles in this frame
	void FinishSimulateParticles();

	//! simulationFrequencyMultiplier cant be a whole number since then we can get into situations where this doesnt do anything
	void SimulateEmittersOverTime( CRenderProxy_Particles* proxyToUpdate, Float simulationTime, Float simulationFrequencyMultiplier );
	// New frame
	void OnNewFrame();
	void OnNewFrameScreenshot();

	// Relink proxies with updated bounding boxes
	void RelinkProxies();

protected:
	//! Bind batch types
	Bool SetBatchRenderStates( const RenderingContext& context, CRenderElement_ParticleEmitter* batch, Float proxyDistance );

	//! Draw mesh particles
	void DrawMeshParticles( CRenderElement_ParticleEmitter* emitterElement, const RenderingContext& context, Float proxyDistance );
	void ComputeMeshParticleFreeOrientation( const MeshParticle* particle, const Vector3& position, EulerAngles angles, Matrix& transform );
	void ComputeMeshParticleFreeNoRotOrientation( const MeshParticle* particle, const Vector3& position, Matrix& transform );
	void ComputeMeshParticleMovementDirectionOrientation( const MeshParticle* particle, const Vector3& position, Matrix& transform );
	void DrawMeshParticlesInstanced( CRenderMesh* mesh, Uint32 chunkIndex, const TDynArray< SParticleInstanceDescriptor >& instances, MeshDrawingStats& outMeshStats );

protected:
	// Device Reset/Lost handling
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
	virtual CName GetCategory() const;
	virtual Uint32 GetUsedVideoMemory() const;

	void CreateBuffers();
};