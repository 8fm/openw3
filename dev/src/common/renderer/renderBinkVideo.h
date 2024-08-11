/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifdef USE_BINK_VIDEO

#include "../../../external/Bink2SDKWindows/include/bink.h"

struct SVideoParamsBink;

class CRenderBinkVideo : public IRenderVideo
{
private:
	typedef U32 TBinkSize; // Even for 64 bit
	typedef U32 TBinkThreadIndex;

private:
	static const TBinkThreadIndex INVALID_BINK_THREAD_INDEX = TBinkThreadIndex(-1);
	static const Int32			  BINK_CBUFFER_SIZE = 8;
	static const Float			  REALTIME_BUFFER_FILL_PERCENTAGE;

private:
	struct SBinkShaderParams
	{
		Uint32 m_width;
		Uint32 m_height;
		Float m_alphaLevel;
		Float m_xScale;
		Float m_yScale;
		Float m_xOffset;
		Float m_yOffset;
		Bool m_isPremultAlpha;
	};

	struct SMappedBinkFrameBuffer
	{
		TBinkSize m_ySize;
		TBinkSize m_cRSize;
		TBinkSize m_cBSize;
		TBinkSize m_aSize;

		Uint8*			  m_yPlane;
		Uint8*			  m_cRPlane;
		Uint8*			  m_cBPlane;
		Uint8*			  m_aPlane;
	};

	struct SBinkTextureGroup
	{
		GpuApi::TextureRef m_yTexture;
		GpuApi::TextureRef m_cRTexture;
		GpuApi::TextureRef m_cBTexture;
		GpuApi::TextureRef m_aTexture;
	};

private:
	String				   m_fileName;
	Bool				   m_playLooped;
	Bool				   m_playRealtime;
	TBinkThreadIndex	   m_yPlaneDecodeThreadIndex;
	TBinkThreadIndex	   m_otherDecodeThreadIndex;
	HBINK				   m_bink;
	BINKFRAMEBUFFERS	   m_binkBuffers;
	SMappedBinkFrameBuffer m_mappedBinkBuffer[ BINKMAXFRAMEBUFFERS ];
	SBinkTextureGroup	   m_drawTexture;
	
	GpuApi::BufferRef	   m_cbuffer;
	GpuApi::BufferRef	   m_vertexBuffer;
	GpuApi::BufferRef	   m_indexBuffer;

	Bool				   m_hasValidFrame;
	Bool				   m_decodingFrame;
	Bool				   m_finishedVideo;
	Bool				   m_binkLoadSuspended;
	Bool				   m_gameLoadSuspended;

public:
	CRenderBinkVideo( const SVideoParamsBink& videoParams );
	~CRenderBinkVideo();

	virtual Bool IsValid() const;
	virtual void Render( CRenderFrame* frame );

private:
	void Init();
	void Cleanup();
	void SetShaderParams( const SBinkShaderParams& shaderParams );
	Bool CreateBinkDrawTexture( TBinkSize width, TBinkSize height, GpuApi::TextureRef& outTex /*[out]*/ );
	Bool CreateBinkResources();
	void ReleaseBinkResources();
	void LockMovieFrame();
	void UnlockMovieFrame();
	void DrawMovieFrame( CRenderFrame* frame );
	void StartNextFrame();
	Bool UpdateFrame();
	void WaitForDecode();
	void StartDecodingThreads();
	void StopDecodingThreads();
	void BalanceRealtimeBufferFill();
	void SuspendBinkLoad();
	void ResumeBinkLoad();
	void TrySuspendBinkLoad();
	void TrySuspendGameLoad();
	void TryResumeBinkLoad();
	void TryResumeGameLoad();
	void UnsuspendLoadAll();
};

#endif //! USE_BINK_VIDEO