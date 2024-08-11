#pragma once

#ifndef renderPostFxMicrosoftSSAO_INCLUDED
#define renderPostFxMicrosoftSSAO_INCLUDED

class CPostFXMicrosoftSSAO
{
//	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

public:
	// High quality (and better) is barely a noticeable improvement when modulated properly with ambient light.
	// However, in the debug view the quality improvement is very apparent.
	enum QualityLevel { kSsaoQualityVeryLow, kSsaoQualityLow, kSsaoQualityMedium, kSsaoQualityHigh, kSsaoQualityVeryHigh, kNumSsaoQualitySettings };
	QualityLevel m_QualityLevel;

	Uint32 m_verticalResolution;
	// This is necessary to filter out pixel shimmer due to bilateral upsampling with too much lost resolution.  High
	// frequency detail can sometimes not be reconstructed, and the noise filter fills in the missing pixels with the
	// result of the higher resolution SSAO.
	Float m_NoiseFilterTolerance;	// suggested values -8 to 0
	Float m_BlurTolerance;			// suggested values -8.0f to -1.0f
	Float m_UpsampleTolerance;		// suggested values -12.0f to -1.0f

	// Controls how aggressive to fade off samples that occlude spheres but by so much as to be unreliable.
	// This is what gives objects a dark halo around them when placed in front of a wall.  If you want to
	// fade off the halo, boost your rejection falloff.  The tradeoff is that it reduces overall AO.
	Float m_RejectionFalloff;		// suggested values 1.0f to 10.0f

	// The higher quality modes blend wide and narrow sampling patterns.  The wide
	// pattern is due to deinterleaving and requires blurring.  The narrow pattern is
	// not on a deinterleaved buffer, but it only samples every other pixel.  The blur
	// on it is optional.  If you combine the two before blurring, the narrow will get
	// blurred as well.  This creates a softer effect but can remove any visible noise
	// from having 50% sample coverage.
	Bool m_CombineResolutionsBeforeBlur;

	// When combining the wide and narrow patterns, a mul() operation can be used or
	// a min() operation.  Multiplication exaggerates the result creating even darker
	// creases.  This is an artistic choice.  I think it looks less natural, but often
	// art teams prefer more exaggerated contrast.  For me, it's more about having the
	// right AO falloff so that it's a smooth gradient rather than falling off precipitously
	// and forming overly dark recesses.
	Bool m_CombineResolutionsWithMul;

	Int32 m_HierarchyDepth;			// valid values 1 to 4
	Float m_NormalAOMultiply;		// more to darken
	Float m_NormalBackProjectionTolerance; // angle in degrees, 3.0f by default

	static Bool IsSSAOEnabled( const CEnvDisplaySettingsParams &displaySettings, const CAreaEnvironmentParamsAtPoint &areaParams )
	{
		// don't allow ssao in non main windows, because we are using temp textures which 
		// must be exactly the same size as the renderArea 
		// (nvidia hbao don't support partial texture usage, e.g. viewport smaller than renderTarget size)

		// ace_todo: determine if ssao would have visual impast based on ssao parameters
		return displaySettings.m_allowSSAO;
	}

	CPostFXMicrosoftSSAO();

	~CPostFXMicrosoftSSAO()
	{
		Deinit();
	}

	void Init();
	void Deinit();

	void Apply( CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, ERenderTargetName applyTarget, ERenderTargetName tempTarget, const CRenderFrameInfo& info );

protected:
	Bool EnsureTexturesAllocated( class CRenderSurfaces* surfaces );
	void ReleaseTextures();

	void GetScreenSize( Int32& outWidth, Int32& outHeight );

	struct Samplers
	{
		enum Enum
		{
			// order is important!
			PointBorder,
			MaxBorder,
			LinearClamp,
			Count
		};
	};

	struct BlurCBData
	{
		enum Enum
		{
			Depth0ToLinear,
			Depth1to0,
			Depth2to1,
			Depth3to2,
			Count,
		};
	};

	static const Int32 m_DepthDownSizeCount = 4;

	struct DepthData {  Float zMagicA; Float zMagicB; Uint32 TileCountX; Uint32 TileCountY; };
	struct DepthToPosData {  Vector cameraPos; Vector origin; Vector hDelta; Vector vDelta; };

	struct ConstantBufferData
	{
		Float				m_DepthOnlySsaoCBData[2][m_DepthDownSizeCount][2][30];
		Float				m_WithNormalsCBData[2][m_DepthDownSizeCount][2][18];
		Float				m_BlurCBData[2][BlurCBData::Count][8];
		DepthData			m_DepthDecompressCBData[2];
		DepthToPosData		m_DepthToPosData[2];
		Float				m_InverseRTSizeData[4];
	};

	ConstantBufferData *		m_CBData;

	Int32						m_CurrentAODoubleBuffer;
	Int32						m_CurrentBlurDoubleBuffer;
	Int32						m_CurrentFrameIndex;

	GpuApi::TextureRef			m_LinearDepth;
	GpuApi::TextureRef			m_NormalBufferDownsize;
	GpuApi::TextureRef			m_NormalBufferTiled;
	GpuApi::TextureRef			m_DepthDownsize[m_DepthDownSizeCount];
	GpuApi::TextureRef			m_DepthTiled[m_DepthDownSizeCount];
	GpuApi::TextureRef			m_AOMerged[m_DepthDownSizeCount];
	GpuApi::TextureRef			m_AOSmooth[m_DepthDownSizeCount - 1];
	GpuApi::TextureRef			m_AOHighQuality[m_DepthDownSizeCount];
#ifdef RED_PLATFORM_DURANGO
	void *						m_Samplers[Samplers::Count];
#else
	GpuApi::SamplerStateRef		m_Samplers[Samplers::Count];
#endif
	GpuApi::BufferRef			m_DepthOnlyCB;
	GpuApi::BufferRef			m_WithNormalsCB;
	GpuApi::BufferRef			m_DepthDecompressCB;
	GpuApi::BufferRef			m_DepthToPosCB;
	GpuApi::BufferRef			m_InverseRTSizeCB;

	Float						m_LastTanHalfFovH;
	Float						m_LastRejectionFalloff;
	Float						m_LastNormalAOMultiply;
	Float						m_LastBlurTolerance;
	Float						m_LastUpsampleTolerance;
	Float						m_LastNoiseFilterTolerance;
	Float						m_LastNormalBackProjectionTolerance;

	Int32						m_LastScreenWidth;
	Int32						m_LastScreenHeight;

	Float						m_LastNormalToDepthBrightnessEqualiser;	// includes compensation for fade by depth, possibly ought to be a far plane multiply?
	Float						m_NormalToDepthBrightnessEqualiser;	// includes compensation for fade by depth, possibly ought to be a far plane multiply?

	Float						m_SampleThickness[12];	// Pre-computed sample thicknesses

	CRenderShaderCompute*		m_DepthPrepare1WithNormalsCS;
	CRenderShaderCompute*		m_DepthPrepare1CS;
	CRenderShaderCompute*		m_DepthPrepare2CS;
	CRenderShaderCompute*		m_Render1CS;
	CRenderShaderCompute*		m_Render2CS;
	CRenderShaderCompute*		m_RenderNormals1CS;
	CRenderShaderCompute*		m_RenderNormals2CS;
	CRenderShaderCompute*		m_BlurUpsampleBlend[5];	// Blend the upsampled result with the next higher resolution
	CRenderShaderCompute*		m_BlurUpsampleFinal[5];	// Don't blend the result, just upsample it
	CRenderShaderCompute*		m_LinearizeDepthCS;

	//template <size_t TBinarySize>
	//void CreateShader( CRenderShaderCompute *&shader, const uint8_t (&shaderBinary)[TBinarySize] );

	void ComputeAO( CRenderShaderCompute* shader, const GpuApi::TextureRef& Destination, const Int32 depthCount, const Int32 tiledIndex, const GpuApi::TextureRef& DepthBuffer, const Bool useNormals = false );
	void UpdateCBs( const Float TanHalfFovH, Float zMagicA, Float zMagicB );
	void WriteCommonCBData( Float *dest, Uint32 BufferWidth, Uint32 BufferHeight );
	void BlurAndUpsample( GpuApi::TextureRef Destination, 
		const GpuApi::TextureRef& HiResDepth, 
		const GpuApi::TextureRef& LoResDepth, 
		const BlurCBData::Enum cbIndex, 
		GpuApi::TextureRef InterleavedAO, 
		GpuApi::TextureRef HighQualityAO, 
		GpuApi::TextureRef HiResAO );
};
#endif