
/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/globalWaterUpdateParams.h"
#include "renderProxyTerrain.h"
#include "../engine/renderSettings.h"

class CRenderSimDynamicWater
{
private:
	Vector						m_lastPos;
	Int32						m_resolution;
	Bool						m_flipped			: 1;
	Bool						m_appliedImpulses	: 1;
	Bool						m_initialClearDone	: 1;

	GpuApi::VertexLayoutRef		m_impulseVertexLayout;
	GpuApi::BufferRef			m_impulseVertexBuffer;

	GpuApi::BufferRef			m_simulateConstants;

	DebugVertexUV				m_points[6];
	GpuApi::TextureRef			m_tex1;
	GpuApi::TextureRef			m_tex2;
	GpuApi::TextureRef			m_input;
	GpuApi::TextureRef			m_output;
	Int32						m_framesSinceImpulse;
	Float						m_lastdt;
	Float						m_time;
	TDynArray<Matrix>			m_impulses;
	Vector						m_snappedCameraPosition;
	Float						m_rainIntensity;

	Uint64						m_simulationFence;

public:
	CRenderSimDynamicWater();
	~CRenderSimDynamicWater();

	const Vector&			GetSnappedCameraPosition() const { return m_snappedCameraPosition; }
	void					SetRainIntensity( Float rainIntensity ) { m_rainIntensity = rainIntensity; }

	// - - - - - - - - - - - - - - - - - 

	//!< Initializes dynamic water helper textures
	void					Initialize();
	void					InitialClear();

	//!< Calculates dynamic water response based on given impulse forces
	GpuApi::TextureRef		Calculate( GpuApi::TextureRef terrainHeightMapTextureArray, Float tim );

	//!< Render helper impulse texture
	void					AddImpulse( const Matrix& matrix );
	void					ApplyImpulses();

	void					UpdateCamera( const Vector& campos );


	void					FinishAsyncSimulate();

private:

	//!< If simulation should be keep going
	Bool					ShouldSimulate() const { return ( ( m_framesSinceImpulse < 2500 || m_rainIntensity>0.0001f ) && Config::cvUseDynamicWaterSimulation.Get() ); }

	void					DoSimulateCompute( Float timeDelta, const Vector& cameraDelta );
	void					DoSimulatePixel( Float timeDelta, const Vector& cameraDelta );
	void					DoFinalizeCompute();
	void					DoFinalizePixel();
};
