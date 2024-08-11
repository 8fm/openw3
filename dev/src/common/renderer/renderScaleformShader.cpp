/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "renderScaleformShader.h"
#include "renderScaleformHal.h"
//#include "Kernel/SF_Debug.h"

// HACK! Defined in D3D1x_shader.cpp Externalally generated for file D3D1x_ShaderDescs.cpp
extern const char* ScaleformGpuApi::ShaderUniformNames[Uniform::SU_Count];

bool CRenderScaleformVertexShader::Init(const ScaleformGpuApi::VertexShaderDesc* pd)
{
	pDesc = pd;
	m_prog = GpuApi::CreateShaderFromBinary( GpuApi::VertexShader, pDesc->pBinary, static_cast< GpuApi::Uint32 >( pDesc->BinarySize ) );
	if ( ! m_prog )
	{
		return false;
	}

	// GpuApi ShaderRefs are creating with zero refcount.
	GpuApi::AddRef( m_prog );
	GpuApi::SetShaderDebugPath( m_prog, "SF VS");

	for ( unsigned uniform = 0; uniform < ScaleformGpuApi::Uniform::SU_Count; ++uniform)
	{
		UniformOffset[uniform] = pd->Uniforms[uniform].Location * 16;
	}

	return true;
}

void CRenderScaleformVertexShader::Shutdown( )
{
	GpuApi::SafeRelease( m_prog );
}

SFBool CRenderScaleformFragShader::Init(const ScaleformGpuApi::FragShaderDesc* pd)
{
	pDesc = pd;

	m_prog = GpuApi::CreateShaderFromBinary(  GpuApi::PixelShader, pDesc->pBinary, static_cast< GpuApi::Uint32 >( pDesc->BinarySize ) );
	if ( ! m_prog )
	{
		return false;
	}

	// GpuApi ShaderRefs are creating with zero refcount.
	GpuApi::AddRef( m_prog );
	GpuApi::SetShaderDebugPath( m_prog, "SF PS");

	for ( unsigned uniform = 0; uniform < ScaleformGpuApi::Uniform::SU_Count; ++uniform)
	{
		UniformOffset[uniform] = pd->Uniforms[uniform].Location * 16;
	}

	return true;
}

void CRenderScaleformFragShader::Shutdown( )
{
	GpuApi::SafeRelease( m_prog );
}

namespace // anonymous
{

enum VertexAttributeName
{
	VATTRNAME_None		= FLAG(0),
	VATTRNAME_AColor	= FLAG(1),
	VATTRNAME_AFactor	= FLAG(2),
	VATTRNAME_ATC		= FLAG(3),
	VATTRNAME_Pos		= FLAG(4),
	VATTRNAME_VBatch	= FLAG(5),
};

// FIXME: String comparison is shit, or even hashing, but taking this from a large pregenerated file
VertexAttributeName FindVertexAttributeName( const SFChar* name )
{
	if ( Red::System::StringCompare( name, "acolor" ) == 0 )
	{
		return VATTRNAME_AColor;
	}
	if ( Red::System::StringCompare( name, "afactor" ) == 0 )
	{
		return VATTRNAME_AFactor;
	}
	if ( Red::System::StringCompare( name, "atc" ) == 0 )
	{
		return VATTRNAME_ATC;
	}
	if ( Red::System::StringCompare( name, "pos" ) == 0 )
	{
		return VATTRNAME_Pos;
	}
	if ( Red::System::StringCompare( name, "vbatch" ) == 0 )
	{
		return VATTRNAME_VBatch;
	}
	HALT( "Unknown vertex attribute name '%ls'.", name );
	return VATTRNAME_None;
}

GpuApi::eBufferChunkType FindBufferChunkType( const ScaleformGpuApi::VertexShaderDesc& vdesc )
{
	static Bool initialized = false;
	static ::THashMap< Uint32, GpuApi::eBufferChunkType > vertexAttrsChunkMap;

	if ( ! initialized )
	{
		initialized = true;
		static struct SInitData
		{
			Uint32 keysAttrs;
			GpuApi::eBufferChunkType chunkType;
		}
		initData[] =
		{
			{ ( VATTRNAME_AColor | VATTRNAME_AFactor | VATTRNAME_Pos ),					GpuApi::BCT_VertexScaleformAColorAFactorPos },
			{ ( VATTRNAME_AColor | VATTRNAME_ATC | VATTRNAME_Pos ),						GpuApi::BCT_VertexScaleformAColorATCPos },
			{ ( VATTRNAME_AColor | VATTRNAME_ATC | VATTRNAME_Pos | VATTRNAME_VBatch ),	GpuApi::BCT_VertexScaleformAColorATCPosVBatch },
			{ ( VATTRNAME_AColor | VATTRNAME_Pos | VATTRNAME_VBatch ),					GpuApi::BCT_VertexScaleformAColorPosVBatch },
			{ ( VATTRNAME_AFactor | VATTRNAME_Pos ),									GpuApi::BCT_VertexScaleformAFactorPos },
			{ ( VATTRNAME_ATC | VATTRNAME_Pos ),										GpuApi::BCT_VertexScaleformATCPos },
			{ ( VATTRNAME_ATC | VATTRNAME_Pos | VATTRNAME_VBatch ),						GpuApi::BCT_VertexScaleformATCPosVBatch },
			{ ( VATTRNAME_Pos ),														GpuApi::BCT_VertexScaleformPos },
			{ ( VATTRNAME_Pos | VATTRNAME_VBatch ),										GpuApi::BCT_VertexScaleformPosVBatch },
		};

		for ( Uint32 i = 0; i < ARRAY_COUNT(initData); ++i )
		{
			const SInitData& data = initData[ i ];
			vertexAttrsChunkMap.Insert( data.keysAttrs, data.chunkType );
 		}
 	}
	
	Uint32 lookupKey = 0;
	for ( Int32 attr = 0; attr < vdesc.NumAttribs; ++attr )
	{
		lookupKey |= FindVertexAttributeName( vdesc.Attributes[attr].Name );
	}

	GpuApi::eBufferChunkType chunkType = GpuApi::BCT_Max;
	VERIFY( vertexAttrsChunkMap.Find( lookupKey, chunkType ) );
	return chunkType;
}
} // namespace anonymous

CRenderScaleformSysVertexFormat::CRenderScaleformSysVertexFormat(const SF::Render::VertexFormat* vf, const ScaleformGpuApi::VertexShaderDesc* pvdesc )
	: m_chunkType( GpuApi::BCT_Max )
{	
	// The vertex format is ignored, except if it specifies a different format for the position; it is assumed that it will 
	// match the VertexShaderDesc's attribute definitions. They may not coincide exactly, for instance, the VSD's attributes 
	// for factors and batch indices may have already been combined.

	const SF::Render::VertexElement* ve = vf->GetElement(SF::Render::VET_Pos, SF::Render::VET_Usage_Mask);
	ASSERT(ve); SF_UNUSED(ve); // must have position.

	m_chunkType = FindBufferChunkType( *pvdesc );
	ASSERT( m_chunkType != GpuApi::BCT_Max );
}

bool CRenderScaleformShaderInterface::SetStaticShader(ScaleformGpuApi::ShaderDesc::ShaderType shader, const SF::Render::VertexFormat* pformat)
{
	CurShaders.pVFormat = pformat;
	CurShaders.pVS      = &pHal->SManager.StaticVShaders[ScaleformGpuApi::VertexShaderDesc::GetShaderIndex(shader, pHal->SManager.ShaderModel)];
	CurShaders.pVDesc   = CurShaders.pVS->pDesc;
	CurShaders.pFS      = &pHal->SManager.StaticFShaders[ScaleformGpuApi::FragShaderDesc::GetShaderIndex(shader, pHal->SManager.ShaderModel)];
	CurShaders.pFDesc   = CurShaders.pFS->pDesc;

	if ( pformat && !pformat->pSysFormat )
		(const_cast<SF::Render::VertexFormat*>(pformat))->pSysFormat = *SF_NEW CRenderScaleformSysVertexFormat(pformat, CurShaders.pVS->pDesc );

	return (bool)CurShaders;
}

void CRenderScaleformShaderInterface::SetTexture(CRenderScaleformShader, SFUInt var, SF::Render::Texture* ptexture, SF::Render::ImageFillMode fm, SFUInt index)
{
	CRenderScaleformTexture *pApiTexture = (CRenderScaleformTexture*)ptexture;
	ASSERT(CurShaders.pFDesc->Uniforms[var].Location >= 0 );
	ASSERT(CurShaders.pFDesc->Uniforms[var].Location + CurShaders.pFDesc->Uniforms[var].Size >= (short)(index + ptexture->TextureCount) );
	pApiTexture->ApplyTexture(CurShaders.pFDesc->Uniforms[var].Location + index, fm);
}

// Record the min/max shader constant registers, and just set the entire buffer in one call,
// instead of doing them individually. This could result in some uninitialized data to be sent,
// but it should be unreferenced by the shaders, and should be more efficient.
struct CRenderScaleformShaderConstantRange
{
	CRenderScaleformShaderConstantRange(CRenderScaleformHAL* phal, Float* data) : UniformData(data), pHal(phal) { };
	void Update( SFInt offset, SFInt size, SFInt shadowLocation)
	{
		if ( offset < 0 )
			return;

		if ( m_constantBuffer.isNull() )
		{
			m_constantBuffer = pHal->getNextConstantBuffer();
			const GpuApi::BufferDesc& bufferDesc = GpuApi::GetBufferDesc( m_constantBuffer );
			const GpuApi::Uint32 bufferSize = bufferDesc.size;
			m_mappedBufferData = GpuApi::LockBuffer( m_constantBuffer, GpuApi::BLF_Discard, 0, bufferSize );
		}

#ifdef RED_PLATFORM_ORBIS
		if ( shadowLocation > 0 || offset > 0 || size > 0 )
		{
			//ERR_RENDERER( TXT("shadowLocation: %i, offset: %i, size: %i"), (Int32)shadowLocation, (Int32)offset, (Int32)size );
		}
		Int32 sizeToCopy = Min( offset + size * sizeof(Float), 128 * 4 * sizeof(Float) ) - offset;
		Red::System::MemoryCopy((Float*)m_mappedBufferData + (offset/sizeof(Float)), UniformData + shadowLocation, sizeToCopy );
#else
		Red::System::MemoryCopy((Float*)m_mappedBufferData + (offset/sizeof(Float)), UniformData + shadowLocation, size*sizeof(Float) );
#endif
	}
	void Finish(SFBool bfragment)
	{
		if ( m_constantBuffer )
		{
			GpuApi::UnlockBuffer( m_constantBuffer );
			if ( bfragment )
			{
				GpuApi::BindConstantBuffer( 0, m_constantBuffer, GpuApi::PixelShader );
			}
			else
			{
				GpuApi::BindConstantBuffer( 0, m_constantBuffer, GpuApi::VertexShader );
			}
		}
	}

	Float* UniformData;
	void*  m_mappedBufferData;
	GpuApi::BufferRef	 m_constantBuffer;
	CRenderScaleformHAL*                 pHal;
};

void CRenderScaleformShaderInterface::Finish(SFUInt meshCount)
{
	ShaderInterfaceBase::Finish(meshCount);

	CRenderScaleformShaderConstantRange shaderConstantRangeFS(pHal, UniformData);
	CRenderScaleformShaderConstantRange shaderConstantRangeVS(pHal, UniformData);
	for (int i = 0; i < ScaleformGpuApi::Uniform::SU_Count; i++)
	{
		if (UniformSet[i])
		{
			if (CurShaders.pFS->UniformOffset[i] >= 0)
			{
				shaderConstantRangeFS.Update( CurShaders.pFS->UniformOffset[i],
					CurShaders.pFDesc->Uniforms[i].Size,
					CurShaders.pFDesc->Uniforms[i].ShadowOffset);
			}
			else if (CurShaders.pVS->UniformOffset[i] >= 0 )
			{
				shaderConstantRangeVS.Update( CurShaders.pVS->UniformOffset[i],
					CurShaders.pVDesc->Uniforms[i].Size,
					CurShaders.pVDesc->Uniforms[i].ShadowOffset);
			}
		}
	}


	shaderConstantRangeVS.Finish(false);
	shaderConstantRangeFS.Finish(true);
	Red::System::MemorySet(UniformSet, 0, sizeof(UniformSet));

	// FIXME:/NOTE: THIS *MUST* COME BEFORE GpuAPi::SetVertexFormat raw for the input lookup
	GetRenderer()->GetStateManager().SetShader( CurShaders.pVS->m_prog, RST_VertexShader );

	// Makes redundant changes anyway when GpuApi draws
	GpuApi::eBufferChunkType chunkType = ((CRenderScaleformSysVertexFormat*)(CurShaders.pVFormat->pSysFormat.GetPtr()))->m_chunkType;
	GpuApi::SetVertexFormatRaw( chunkType );

	GetRenderer()->GetStateManager().SetShader( CurShaders.pFS->m_prog, RST_PixelShader );
}

void CRenderScaleformShaderInterface::BeginScene()
{
}

// *** ShaderManager
CRenderScaleformShaderManager::CRenderScaleformShaderManager( SF::Render::ProfileViews* prof ) : 
StaticShaderManager(prof), ShaderModel( SCALEFORM_GPUAPI_SHADER_VERSION )
{
	Red::System::MemorySet(StaticVShaders, 0, sizeof(StaticVShaders));
	Red::System::MemorySet(StaticFShaders, 0, sizeof(StaticFShaders));
}

bool CRenderScaleformShaderManager::HasInstancingSupport() const
{
#if defined( RED_PLATFORM_DURANGO )
	// Durango has instancing.
	return true;
#elif defined( RED_PLATFORM_ORBIS )
	return true;
#elif defined( RED_PLATFORM_WINPC )
	// Only FeatureLevel 10.0+ has instancing (but it always has it).
	//return ShaderModel >= ScaleformGpuApi::ShaderDesc::ShaderVersion_D3D1xFL10X;
	ASSERT( ShaderModel >= ScaleformGpuApi::ShaderDesc::ShaderVersion_D3D1xFL10X );
#else
#error Unsupported platform
#endif
	return true;
}

void CRenderScaleformShaderManager::MapVertexFormat(SF::Render::PrimitiveFillType fill, const SF::Render::VertexFormat* sourceFormat,
	const SF::Render::VertexFormat** single, const SF::Render::VertexFormat** batch, 
	const SF::Render::VertexFormat** instanced)
{
	// D3D1x's shader input types need to match their input layouts. So, just convert all position data
	// to float (instead of using short). Extra shaders could be generated to support both, however, that
	// would require double the number of vertex shaders.
	SF::Render::VertexElement floatPositionElements[8];
	SF::Render::VertexFormat floatPositionFormat;
	floatPositionFormat.pElements = floatPositionElements;
	floatPositionFormat.pSysFormat = 0;
	Uint32 offset = 0;
	Uint32 i;
	for ( i = 0; sourceFormat->pElements[i].Attribute != SF::Render::VET_None; ++i )
	{
		floatPositionElements[i].Attribute = sourceFormat->pElements[i].Attribute;
		if ( (floatPositionElements[i].Attribute&SF::Render::VET_Usage_Mask) == SF::Render::VET_Pos)
		{
			floatPositionElements[i].Attribute &= ~SF::Render::VET_CompType_Mask;
			floatPositionElements[i].Attribute |= SF::Render::VET_F32;
			floatPositionElements[i].Offset = sourceFormat->pElements[i].Offset + offset;
			if ( (floatPositionElements[i].Attribute&SF::Render::VET_CompType_Mask) == SF::Render::VET_S16)
				offset += 4;
		}
		else
		{
			floatPositionElements[i].Offset = sourceFormat->pElements[i].Offset + offset;
		}
	}
	floatPositionElements[i].Attribute = SF::Render::VET_None;
	floatPositionElements[i].Offset    = 0;

	// Now let the base class actually do the mapping.
	Base::MapVertexFormat(fill, &floatPositionFormat, single, batch, instanced, 
		(HasInstancingSupport() ? SF::Render::MVF_HasInstancing : 0) | SF::Render::MVF_Align);
}

bool CRenderScaleformShaderManager::Initialize(CRenderScaleformHAL* phal)
{
	ShaderModel = SCALEFORM_GPUAPI_SHADER_VERSION;

#if !defined( RED_PLATFORM_DURANGO )
	if ( !ScaleformGpuApi::ShaderDesc::IsShaderVersionSupported(ShaderModel))
	{
		ASSERT( ! "Support for D3D_FEATURE_LEVEL_10_0+ was not included when running GFxShaderMaker." );
		return false;
	}
#endif

	// Now, initialize all the shaders that use our current ShaderModel version.
	for (unsigned i = 0; i < ScaleformGpuApi::VertexShaderDesc::VSI_Count; i++)
	{
		const ScaleformGpuApi::VertexShaderDesc* desc = ScaleformGpuApi::VertexShaderDesc::Descs[i];
		if ( desc && desc->Version == ShaderModel && desc->pBinary)
		{
			if (!StaticVShaders[i].Init(desc))
				return false;
		}
	}

	for (unsigned i = 0; i < ScaleformGpuApi::FragShaderDesc::FSI_Count; i++)
	{
		const ScaleformGpuApi::FragShaderDesc* desc = ScaleformGpuApi::FragShaderDesc::Descs[i];
		if (desc && desc->Version == ShaderModel && desc->pBinary)
		{
			if ( !StaticFShaders[i].Init(desc) )
				return false;
		}
	}

	return true;
}

void CRenderScaleformShaderManager::BeginScene()
{

}

void CRenderScaleformShaderManager::EndScene()
{

}

void CRenderScaleformShaderManager::Reset()
{
	for (unsigned i = 0; i < ScaleformGpuApi::VertexShaderDesc::VSI_Count; i++)
	{
		const ScaleformGpuApi::VertexShaderDesc* desc = ScaleformGpuApi::VertexShaderDesc::Descs[i];
		if ( desc && desc->pBinary)
			StaticVShaders[i].Shutdown();
	}

	for (unsigned i = 0; i < ScaleformGpuApi::FragShaderDesc::FSI_Count; i++)
	{
		const ScaleformGpuApi::FragShaderDesc* desc = ScaleformGpuApi::FragShaderDesc::Descs[i];
		if (desc && desc->pBinary)
			StaticFShaders[i].Shutdown();
	}

	VFormats.Clear();
}

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
