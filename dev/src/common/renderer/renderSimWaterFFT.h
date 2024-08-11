
/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderSimWaterFFT
{
private:
	enum 
	{
		FFTStageFrameLatency = 3,
		FFTPingPongFrames = 2,
		FFTReadBackTexDiv = 4
	};

	EngineTime							m_time;
	Int32								m_resolution;
	Int32								m_steps;
	Int32								m_readBackTexRes;

	Int8								m_framesLeftBeforeFirstReadback;

	DebugVertexUV						m_points[6];

	// textures used by the FFT process
	GpuApi::TextureRef					m_order_texture;
	GpuApi::TextureRef					m_indices_texture;

	GpuApi::TextureRef					m_input;
	GpuApi::TextureRef					m_output;

	// ping-pong textures
	GpuApi::TextureRef					m_tex[FFTPingPongFrames];

	// variables used to read the data on the CPU
	GpuApi::TextureRef					m_stage[FFTStageFrameLatency];
	GpuApi::TextureRef					m_stageHelper;
	Uint32								m_stagingFlip;

	Uint64								m_computeFence;
	GpuApi::BufferRef					m_phillipsAnimationConstants;
	GpuApi::BufferRef					m_butterflyConstants[16];
	GpuApi::BufferRef					m_finalizationConstants;

public:
	CRenderSimWaterFFT();
	~CRenderSimWaterFFT();

	Int32 GetHeightmapResolution() const { return m_readBackTexRes; }
	Int32 GetReadBackResolution() const { return m_readBackTexRes/FFTReadBackTexDiv; }
	Float GetReadBackScale() const { return 1.0f / static_cast<Float>( FFTReadBackTexDiv ); }

	void Initialize( Int32 res ); //must be power of 2

	Bool IsAsyncCompute() const;

	GpuApi::TextureRef Calculate( Float wdir, Float wspeed, Float a, Float lambda, Float deltaTime, Float windScale, Bool& wasSimulated );
	void FillCPUData( void* heightReadback );

private:
	void FFTswap (Float* data, Int32 n);
	void FFTindices (Float* data, Int32 n);
	void FFTfactors (Float* data, Int32 n);
	void FFTsigns (Float* data, Int32 n);

	void ReadHeightMap( GpuApi::TextureRef dest , void* heightReadback );

	void FlipTarget( GpuApi::RenderTargetSetup *targets, GpuApi::TextureRef *textures, Uint32 & flipIdx );
	void Cleanup();
	void FlipTargetPixel( GpuApi::RenderTargetSetup *targets, GpuApi::TextureRef *textures, Uint32 & flipIdx );
	void FlipTargetCompute( GpuApi::TextureRef *textures, Uint32 & flipIdx );
	void CleanupCompute();
};
