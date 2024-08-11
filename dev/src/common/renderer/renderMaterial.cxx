/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderHelpers.h"
#include "renderTextureArray.h"
#include "renderCube.h"
#include "renderMaterial.h"
#include "renderShader.h"

#include "../core/depot.h"
#include "../core/taskManager.h"
#include "../core/dependencyMapper.h"
#include "../engine/baseEngine.h"
#include "../engine/renderFragment.h"
#include "../engine/shaderCacheManager.h"
#include "../engine/materialDefinition.h"
#include "../engine/materialGraph.h"
#include "../engine/materialCompilerDefines.h"
#include "../engine/cubeTexture.h"
#include "../engine/texture.h"
#include "../engine/textureArray.h"


// Get/Release could happen from different threads, so protect the map.
THashMap< Uint32, GpuApi::BufferRef > CRenderMaterialParameters::m_sharedParamBuffers;
Red::Threads::CMutex CRenderMaterialParameters::m_sharedParamBuffersMutex;

GpuApi::BufferRef CRenderMaterialParameters::GetSharedParamBuffer( const void* data, Uint32 dataSize )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_sharedParamBuffersMutex );

	Uint32 hash = Red::System::CalculateHash32( data, dataSize );

	GpuApi::BufferRef& buffer = m_sharedParamBuffers.GetRef( hash );
	if ( buffer.isNull() )
	{
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = data;
		buffer = GpuApi::CreateBuffer( dataSize, GpuApi::BCC_Constant, GpuApi::BUT_Default, 0, &bufInitData );
	}

	RED_ASSERT( !buffer.isNull(), TXT("Failed to create material params constant buffer with size %u"), dataSize );

	// Caller gets a reference.
	GpuApi::AddRefIfValid( buffer );

	return buffer;
}

void CRenderMaterialParameters::SafeReleaseSharedParamBuffer( GpuApi::BufferRef& buffer )
{
	if ( buffer )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_sharedParamBuffersMutex );

		// Release for the caller
		Int32 refs = GpuApi::Release( buffer );
		RED_ASSERT( refs > 0 );
		// If there's just 1 reference left, then it's the reference we're holding, and we can destroy the buffer.
		if ( refs == 1 )
		{
			if ( m_sharedParamBuffers.EraseByValue( buffer ) )
			{
				// Release a second time, for the reference we hold ourselves.
				GpuApi::Release( buffer );
			}
			else
			{
				RED_HALT( "Buffer was not in the sharedParamBuffers map" );
			}
		}
		buffer = GpuApi::BufferRef();
	}
}


// This could be a non-static member, to avoid cases where on material finishing compilation blocks another material
// trying to bind. But, that sort of case probably doesn't come up enough to justify the extra members in every material.
Red::Threads::CMutex			CRenderMaterial::m_techniquesMutex;

TDynArray< CRenderMaterial* >	CRenderMaterial::st_currentlyRecompilingMaterials;
Red::Threads::CMutex			CRenderMaterial::st_recompileMutex;

CRenderMaterial::CompiledTechnique::CompiledTechnique( CRenderShader* pixelShader, CRenderShader* vertexShader, const TUsedParameterArray& pixelParameters, const TUsedParameterArray& vertexParameters, const TSamplerStateArray& psSamplerStates, const TSamplerStateArray& vsSamplerStates, CRenderShader* hullShader, CRenderShader* domainShader )
	: m_pixelShader( pixelShader )
	, m_vertexShader( vertexShader )
	, m_hullShader( hullShader )
	, m_domainShader( domainShader )
	, m_pixelParameters( pixelParameters )
	, m_vertexParameters( vertexParameters )
	, m_vsSamplerStates( vsSamplerStates )
	, m_psSamplerStates( psSamplerStates )
{
}

CRenderMaterial::CompiledTechnique::~CompiledTechnique()
{
	SAFE_RELEASE( m_pixelShader );
	SAFE_RELEASE( m_vertexShader );
	SAFE_RELEASE( m_hullShader );
	SAFE_RELEASE( m_domainShader );

	for ( Uint32 samplerIndex = 0; samplerIndex < m_psSamplerStates.Size(); ++samplerIndex )
	{
		GpuApi::SafeRelease( m_psSamplerStates[samplerIndex].m_second );
	}
	for ( Uint32 samplerIndex = 0; samplerIndex < m_vsSamplerStates.Size(); ++samplerIndex )
	{
		GpuApi::SafeRelease( m_vsSamplerStates[samplerIndex].m_second );
	}
}

void CRenderMaterial::CompiledTechnique::Bind( const MaterialRenderingContext& context )
{
	// Bind shaders
	m_vertexShader->Bind();

	if (m_hullShader)
	{
		// if we have a hull shader we must have a domain shader too
		ASSERT(m_domainShader);
		m_hullShader->Bind();
		m_domainShader->Bind();
	}

	// Don't use a pixel shader when drawing depth-only pass for non-masked geometry.
	const Bool bindPixelShader = ( context.m_renderingContext->m_pass != RP_ShadowDepthSolid );
	if ( bindPixelShader )
	{
		m_pixelShader->Bind();
	}
	else
	{
		// Make sure no PS is bound
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );
	}
}

Bool CRenderMaterial::CompiledTechnique::BindParams( const MaterialRenderingContext& context, const CRenderMaterialParameters* params, Float distance )
{
	ASSERT( params );

	if ( !SetParamsForShaderType( params, m_vsSamplerStates, RST_VertexShader, context.CalcID(), distance ) )
	{
		return false;
	}

	// Don't use a pixel shader when drawing depth-only pass for non-masked geometry.
	const Bool bindPixelShader = ( context.m_renderingContext->m_pass != RP_ShadowDepthSolid );
	if ( bindPixelShader )
	{
		if ( !SetParamsForShaderType( params, m_psSamplerStates, RST_PixelShader, context.CalcID(), distance ) )
		{
			return false;
		}

#ifndef NO_EDITOR // UV DENSITY RENDERING MODE HACK
		Vector maxTexSize = Vector::ZEROS;
		for ( Uint32 i=0; i<m_pixelParameters.Size(); i++ )
		{
			const UsedParameter& param = m_pixelParameters[i];
			switch ( param.m_type )
			{
			case IMaterialDefinition::PT_Texture:
				{
					ASSERT( !params->m_textures.Empty() );

					CRenderTexture* dx9texture = *(CRenderTexture**) &params->m_pixelData[ param.m_offset ];
					if ( dx9texture )
					{
						if ( context.m_materialDebugMode == MDM_UVDensity )
						{
							if ( dx9texture->GetTextureGroupName() == CNAME( WorldDiffuse ) || 
								dx9texture->GetTextureGroupName() == CNAME( WorldDiffuseWithAlpha ) || 
								dx9texture->GetTextureGroupName() == CNAME( CharacterDiffuse ) || 
								dx9texture->GetTextureGroupName() == CNAME( CharacterDiffuseWithAlpha ) || 
								dx9texture->GetTextureGroupName() == CNAME( SpeedTreeDiffuse ) || 
								dx9texture->GetTextureGroupName() == CNAME( SpeedTreeDiffuseWithAlpha ) ||
								dx9texture->GetTextureGroupName() == CNAME( TerrainDiffuseAtlas ) )
							{
								Vector size;
								dx9texture->GetSizeVector( size );
								if ( (size.X * size.Y) > (maxTexSize.X * maxTexSize.Y) )
								{
									maxTexSize = size;
								}
							}
						}
					}
				}
			}
		}
		if ( context.m_materialDebugMode == MDM_UVDensity )
		{
			GetRenderer()->GetStateManager().SetPixelConst( PSC_SelectionEffect, maxTexSize );
		}
#endif
	}

	return true;
}

void CRenderMaterial::CompiledTechnique::UnbindParams()
{
	// Bind dynamic parameters
	for ( Uint32 i=0; i<m_pixelParameters.Size(); i++ )
	{
		const UsedParameter& param = m_pixelParameters[i];
		switch ( param.m_type )
		{
			case IMaterialDefinition::PT_Cube:
			case IMaterialDefinition::PT_Atlas:
			case IMaterialDefinition::PT_Texture:
			case IMaterialDefinition::PT_TextureArray:
			{
				GpuApi::BindTextures( param.m_register, 1, nullptr, GpuApi::PixelShader );
			}
		}
	}
}

Bool CRenderMaterial::CompiledTechnique::SetParamsForShaderType( const CRenderMaterialParameters* params, const TSamplerStateArray& samplerStates, ERenderShaderType shaderType, Uint32 contextID, Float distance )
{
	const TUsedParameterArray& usedParams = (shaderType == RST_PixelShader) ? m_pixelParameters : m_vertexParameters;

	if (!usedParams.Empty())
	{
		GpuApi::BufferRef paramsBuffer = params->GetParamsBuffer( contextID, shaderType );
		Bool initParamsBuffer = paramsBuffer.isNull();

		const TDynArray< Uint8, MC_RenderData >& data = (shaderType == RST_PixelShader) ? params->m_pixelData : params->m_vertexData;

		Vector paramsTemp[100];
		Int32 lastParam = -1;

		// Bind dynamic parameters
		for ( Uint32 i=0; i<usedParams.Size(); i++ )
		{
			const UsedParameter& param = usedParams[i];
			switch ( param.m_type )
			{
			case IMaterialDefinition::PT_Texture:			// Fall-through
			case IMaterialDefinition::PT_TextureArray:		// All texture share the same base
			case IMaterialDefinition::PT_Cube:
				{
					RED_ASSERT( !params->m_textures.Empty() );
					RED_ASSERT( param.m_register < 8, TXT("Material using too many textures") );

					if ( !params->m_textures.Empty() && param.m_register < 8 )
					{
						CRenderTextureBase* texture = *(CRenderTextureBase**) &data[ param.m_offset ];
						if ( texture != nullptr )
						{
							texture->BindNoSamplerFast( param.m_register, shaderType, distance );
						}
						else
						{
							WARN_RENDERER( TXT("Binding null texture in material! Was it intended? Expect graphical errors") );
							GpuApi::BindTextures( param.m_register, 1, nullptr, (GpuApi::eShaderType)shaderType );
						}
					}

					break;
				}

			case IMaterialDefinition::PT_Color:
				{
					if(lastParam < param.m_register)
					{
						lastParam = param.m_register;
					}

					if (initParamsBuffer)
					{
						const Color& color = *(const Color*) &data[ param.m_offset ];
						paramsTemp[param.m_register] = color.ToVector();
					}
					break;
				}

			case IMaterialDefinition::PT_Vector:
				{
					if(lastParam < param.m_register)
					{
						lastParam = param.m_register;
					}

					if (initParamsBuffer)
					{
						const Vector& vector = *(const Vector*) &data[ param.m_offset ];
						paramsTemp[param.m_register] = vector;
					}
					break;
				}

			case IMaterialDefinition::PT_Scalar:
				{
					if(lastParam < param.m_register)
					{
						lastParam = param.m_register;
					}

					if (initParamsBuffer)
					{
						const Float& scalar = *(const Float*) &data[ param.m_offset ];
						paramsTemp[param.m_register] = Vector( scalar, scalar, scalar, scalar );
					}
					break;
				}
			}
		}

		// since we only count non-texture params, it is possible to have usedParams only textures in which case we won't have any constants
		if ( initParamsBuffer && lastParam > -1 )
		{
			paramsBuffer = CRenderMaterialParameters::GetSharedParamBuffer( &paramsTemp[0], ( lastParam + 1 ) * sizeof(Vector) );

			if ( paramsBuffer.isNull() )
			{
				ERR_RENDERER( TXT("Failed to get parameters buffer") );
				return false;
			}

			CRenderMaterialParameters* tempNonConstParams = const_cast<CRenderMaterialParameters*>(params);
			tempNonConstParams->SetParamsBuffer( contextID, shaderType, paramsBuffer );
		}

		params->Bind( contextID, shaderType );
	}

	// Bind all sampler states
	GpuApi::eShaderType gpuApiShaderType = Map(shaderType);
	for ( Uint32 s=0; s<samplerStates.Size(); ++s )
	{
		const TPair< Uint32, GpuApi::SamplerStateRef >& statePair = samplerStates[s];
		GpuApi::SetSamplerState( statePair.m_first, statePair.m_second, gpuApiShaderType );
	}

	return true;
}

Red::Threads::CAtomic< Uint32 > CRenderMaterial::MaterialCompilationJob::m_currentlyProcessingCount( 0 );

CRenderMaterial::MaterialCompilationJob::MaterialCompilationJob( CRenderMaterial* material, const MaterialRenderingContext& context )
	: m_material( material )
	, m_renderingContext( new RenderingContext( *context.m_renderingContext ) )
	, m_technique( nullptr )
{
	m_context = new MaterialRenderingContext( *m_renderingContext );
	m_context->m_vertexFactory			= context.m_vertexFactory;
	m_context->m_selected				= context.m_selected;
	m_context->m_uvDissolveSeparateUV	= context.m_uvDissolveSeparateUV;
	m_context->m_useInstancing			= context.m_useInstancing;
	m_context->m_useParticleInstancing  = context.m_useParticleInstancing;
	m_context->m_materialDebugMode		= context.m_materialDebugMode;
	m_context->m_hasVertexCollapse		= context.m_hasVertexCollapse;
	m_context->m_hasExtraStreams		= context.m_hasExtraStreams;
	m_context->m_discardingPass			= context.m_discardingPass;
	m_context->m_pregeneratedMaps		= context.m_pregeneratedMaps;
	m_context->m_lowQuality				= context.m_lowQuality;

	m_material->AddRef();
}

CRenderMaterial::MaterialCompilationJob::~MaterialCompilationJob()
{
	SAFE_RELEASE( m_material );

	if ( m_context )
	{
		delete m_context;
		m_context = nullptr;
	}
	if ( m_renderingContext )
	{
		delete m_renderingContext;
		m_renderingContext = nullptr;
	}
}

CRenderMaterial::CompiledTechnique* CRenderMaterial::MaterialCompilationJob::DoCompilation( CRenderMaterial* material, const MaterialRenderingContext& context )
{
	// Try to compile using given material
	CRenderMaterial::CompiledTechnique* technique = material->CompileTechnique( context );

	// Get fallback material
	if ( !technique )
	{
		// Remove all gameplay specific shit from material context and retry
		MaterialRenderingContext tempContext = context;

		// Try to get simpler technique
		technique = material->CompileTechnique( tempContext );
		if ( technique )
		{
			// We dropped some gameplay flags - log it
			WARN_RENDERER( TXT("Dropped gameplay flags for technique '%ls' ( %d ) of '%ls'"), context.ToString().AsChar(), context.CalcID(), material->GetDisplayableName().AsChar() );
		}
		else
		{
			// We really need a fallback here....
			WARN_RENDERER( TXT("Fallback needed for technique '%ls' ( %d ) of '%ls'"), context.ToString().AsChar(), context.CalcID(), material->GetDisplayableName().AsChar() );
			technique = CRenderMaterial::CompileFallback( context );
		}
	}

	return technique;
}

void CRenderMaterial::MaterialCompilationJob::Run()
{
	PC_SCOPE_PIX( MaterialCompilationJob );

	m_technique = DoCompilation( m_material, *m_context );
	m_currentlyProcessingCount.Decrement();
}

IMPLEMENT_RENDER_RESOURCE_ITERATOR_WITH_CACHE( CRenderMaterialParameters );

CRenderMaterialParameters::CRenderMaterialParameters( const IMaterial* material )
	: m_batchNext( nullptr )
	, m_batchList( nullptr )
	, m_dataCubeTextureOffset( 255 )
	, m_textureArrayCubeTextureOffset( 255 )
{
	SetupParameters( material );

#if 0
	m_debugPath = material->GetDepotPath();

	if ( m_debugPath.Empty() && material->GetParent() )
	{
		CResource* parentResource = material->GetParent()->FindParent< CResource >();
		if ( parentResource )
		{
			m_debugPath = TXT("Instance in ");

			String parentPath = parentResource->GetDepotPath();
			if ( parentPath.Empty() )
			{
				m_debugPath += TXT("Unknown of class ");
				m_debugPath += parentResource->GetClass()->GetName().AsChar();
			}
			else
			{
				m_debugPath += parentPath;
			}
		}
	}
#endif
}

CRenderMaterialParameters::CRenderMaterialParameters( const CRenderMaterialParameters& paramsToCopy )
	: m_batchNext( nullptr )
	, m_batchList( nullptr )
	, m_dataCubeTextureOffset( paramsToCopy.m_dataCubeTextureOffset )
	, m_textureArrayCubeTextureOffset( paramsToCopy.m_textureArrayCubeTextureOffset )
	, m_isMasked( paramsToCopy.m_isMasked )
{
	m_textures = paramsToCopy.m_textures;
	m_pixelData = paramsToCopy.m_pixelData;
	m_vertexData = paramsToCopy.m_vertexData;

	for ( Uint32 i = 0; i < m_textures.Size(); ++i )
	{
		m_textures[i]->AddRef();
	}
}

CRenderMaterialParameters* CRenderMaterialParameters::Create( const IMaterial* material, Uint64 partialRegistrationHash )
{
	TScopedRenderResourceCreationObject< CRenderMaterialParameters > renderMaterial ( partialRegistrationHash );

	renderMaterial.InitResource( new CRenderMaterialParameters( material ) );
	
	return renderMaterial.RetrieveSuccessfullyCreated();
}

void CRenderMaterialParameters::ReplaceCubeTexture( CCubeTexture* cubeTexture )
{
	IRenderResource* resource = cubeTexture->GetRenderResource();

	ASSERT( resource );

	if ( resource )
	{
		resource->AddRef();

		m_textures[ m_textureArrayCubeTextureOffset ]->Release();
		m_textures[ m_textureArrayCubeTextureOffset ] = resource;

		* ( IRenderResource** ) &m_pixelData[ m_dataCubeTextureOffset ] = resource;
	}
}

Bool CRenderMaterialParameters::HasCubeTexture()
{
	return (m_dataCubeTextureOffset != 255) && (m_textureArrayCubeTextureOffset != 255);
}

CRenderMaterialParameters::~CRenderMaterialParameters()
{
	// Release references to textures
	for ( Uint32 i=0; i<m_textures.Size(); i++ )
	{
		SAFE_RELEASE( m_textures[i] );
	}
	m_textures.Clear();

	for ( Uint32 i=0; i < m_techniqueParams.Size(); i++ )
	{
		SafeReleaseSharedParamBuffer( m_techniqueParams[i].m_vertexParamsBuffer );
		SafeReleaseSharedParamBuffer( m_techniqueParams[i].m_pixelParamsBuffer );
	}
	m_techniqueParams.Clear();
}

void CRenderMaterialParameters::SetupParameters( const IMaterial* material )
{
	TDynArray< IRenderResource* > oldTextures = m_textures;
	m_textures.Clear();

	m_isMasked = material->IsMasked();

	// Get source material definition
	const IMaterialDefinition* definition = const_cast< IMaterial* >( material )->GetMaterialDefinition();
	if ( definition )
	{
		CompileDataBuffer(definition, material, MSH_PixelShader);
		CompileDataBuffer(definition, material, MSH_VertexShader);
	}

	// Release references to textures
	for ( Uint32 i=0; i<oldTextures.Size(); i++ )
	{
		IRenderResource* res = oldTextures[i];
		res->Release();
	}
}

CName CRenderMaterialParameters::GetCategory() const
{
	return CNAME( RenderMaterialParameters );
}

Uint32 CRenderMaterialParameters::GetUsedVideoMemory() const
{
	return m_pixelData.Size() + m_vertexData.Size();
}

void CRenderMaterialParameters::OnDeviceLost()
{
	// Release references to textures
	for ( Uint32 i=0; i<m_textures.Size(); i++ )
	{
		SAFE_RELEASE( m_textures[i] );
	}

	m_textures.Clear();
}

void CRenderMaterialParameters::OnDeviceReset()
{
	// Nothing, resources NEED to be recreated on engine side
}

void CRenderMaterialParameters::CompileDataBuffer( const IMaterialDefinition* definition, const IMaterial* material, EMaterialShaderTarget shaderTarget )
{
	// Create data buffer
	const Uint32 blockSize = (shaderTarget==MSH_PixelShader)?definition->GetPixelParamBlockSize():definition->GetVertexParamBlockSize();
	TDynArray< Uint8, MC_RenderData >& data = (shaderTarget==MSH_PixelShader)?m_pixelData:m_vertexData;
	data.Resize( blockSize );
	Red::System::MemorySet( data.TypedData(), 0, blockSize );

	// Compile values to single data buffer
	const IMaterialDefinition::TParameterArray& params = (shaderTarget==MSH_PixelShader)?definition->GetPixelParameters():definition->GetVertexParameters();
	for ( Uint32 i=0; i<params.Size(); i++ )
	{
		const CMaterialGraph::Parameter& param = params[i];

		// Grab internal references to textures
		if ( param.m_type == CMaterialGraph::PT_Texture || param.m_type == CMaterialGraph::PT_Atlas )
		{
			// Load texture pointer
			THandle< ITexture > texture;
			material->ReadParameter( param.m_name, texture );
			if ( texture )
			{
				// Get render resource
				IRenderResource* resource = texture->GetRenderResource();
				if ( resource )
				{
					// Set as parameter
					* ( IRenderResource** ) &data[ param.m_offset ] = resource;

					// Add internal reference
					m_textures.PushBack( resource );
					resource->AddRef();
				}
			}

			// Skip
			continue;
		}

		// Grab internal references to texture arrays
		if ( param.m_type == CMaterialGraph::PT_TextureArray )
		{
			// Load texture pointer
			THandle< CTextureArray > texture;
			material->ReadParameter( param.m_name, texture );
			if ( texture )
			{
				// Get render resource
				IRenderResource* resource = texture->GetRenderResource();
				if ( resource )
				{
					// Set as parameter
					* ( IRenderResource** ) &data[ param.m_offset ] = resource;

					// Add internal reference
					m_textures.PushBack( resource );
					resource->AddRef();
				}
			}

			// Skip
			continue;
		}

		// Grab internal references to cubemaps
		if ( param.m_type == CMaterialGraph::PT_Cube )
		{
			// Load texture pointer
			THandle< CCubeTexture > texture;
			material->ReadParameter( param.m_name, texture );
			if ( texture )
			{
				// Get render resource
				IRenderResource* resource = texture->GetRenderResource();
				if ( resource )
				{
					// Set as parameter
					* ( IRenderResource** ) &data[ param.m_offset ] = resource;

					m_dataCubeTextureOffset = param.m_offset;
					m_textureArrayCubeTextureOffset = (Uint8) m_textures.Size();

					// Add internal reference
					m_textures.PushBack( resource );
					resource->AddRef();
				}
			}

			// Skip
			continue;
		}

		// Normal parameter, read directly
		material->ReadParameterRaw( param.m_name, &data[ param.m_offset ] );
	}
}

void CRenderMaterialParameters::Bind( Uint32 contextID, ERenderShaderType shaderType ) const
{
	for ( Uint32 i = 0; i < m_techniqueParams.Size(); ++i )
	{
		if ( m_techniqueParams[i].m_contextID == contextID )
		{
			if ( shaderType == RST_PixelShader ) 
			{
				GpuApi::BindConstantBuffer( 4, m_techniqueParams[i].m_pixelParamsBuffer, GpuApi::PixelShader );
			}
			else
			{
				GpuApi::BindConstantBuffer( 4, m_techniqueParams[i].m_vertexParamsBuffer, GpuApi::VertexShader );
				GpuApi::BindConstantBuffer( 4, m_techniqueParams[i].m_vertexParamsBuffer, GpuApi::GeometryShader );
				GpuApi::BindConstantBuffer( 4, m_techniqueParams[i].m_vertexParamsBuffer, GpuApi::HullShader );
				GpuApi::BindConstantBuffer( 4, m_techniqueParams[i].m_vertexParamsBuffer, GpuApi::DomainShader );
			}
		}
	}
}

void CRenderMaterialParameters::SetParamsBuffer( Uint32 contextID, ERenderShaderType shaderType, GpuApi::BufferRef bufferRef )
{
	for ( Uint32 i = 0; i < m_techniqueParams.Size(); ++i )
	{
		if ( m_techniqueParams[i].m_contextID == contextID )
		{
			if ( shaderType == RST_PixelShader ) 
			{
				GpuApi::SafeRelease( m_techniqueParams[i].m_pixelParamsBuffer );
				m_techniqueParams[i].m_pixelParamsBuffer = bufferRef;
				return;
			}
			else
			{
				GpuApi::SafeRelease( m_techniqueParams[i].m_vertexParamsBuffer );
				m_techniqueParams[i].m_vertexParamsBuffer = bufferRef;
				return;
			}
		}
	}

	//not found, add a new one
	m_techniqueParams.Grow();
	m_techniqueParams[ m_techniqueParams.Size()-1 ].m_contextID = contextID;
	if ( shaderType == RST_PixelShader ) 
	{
		m_techniqueParams[m_techniqueParams.Size()-1].m_pixelParamsBuffer = bufferRef;
		return;
	}
	else
	{
		m_techniqueParams[m_techniqueParams.Size()-1].m_vertexParamsBuffer = bufferRef;
		return;
	}
}

GpuApi::BufferRef CRenderMaterialParameters::GetParamsBuffer( Uint32 contextID, ERenderShaderType shaderType ) const
{
	for ( Uint32 i = 0; i < m_techniqueParams.Size(); ++i )
	{
		if ( m_techniqueParams[i].m_contextID == contextID )
		{
			if ( shaderType == RST_PixelShader ) 
			{
				return m_techniqueParams[i].m_pixelParamsBuffer;
			}
			else
			{
				return m_techniqueParams[i].m_vertexParamsBuffer;
			}
		}
	}

	return GpuApi::BufferRef::Null();
}

CRenderMaterial::CRenderMaterial( const IMaterialDefinition* material )
	: CRenderMaterialParameters( material )
	, m_engineMaterial( nullptr )
	, m_flags( 0 )
{
	SetupParameters( material );
}

CRenderMaterial::~CRenderMaterial()
{
	// Delete compiled techniques
	m_techniques.ClearPtr();
	m_techniquesValid.Clear();

#ifndef NO_ASYNCHRONOUS_MATERIALS
	for ( auto it = m_compilationJobs.Begin(); it != m_compilationJobs.End(); ++it )
	{
		MaterialCompilationJob* job = it->m_second;
		job->Release();
	}
	m_compilationJobs.ClearFast();
#endif

	// Cleanup parameters
	m_pixelParameters.Clear();
	m_vertexParameters.Clear();
}

CRenderMaterial* CRenderMaterial::Create( const IMaterialDefinition *definition, Uint64 partialRegistrationHash )
{
	TScopedRenderResourceCreationObject< CRenderMaterial > renderMaterial ( partialRegistrationHash );

	renderMaterial.InitResource( new CRenderMaterial( definition ) );

	return renderMaterial.RetrieveSuccessfullyCreated();
}

Bool CRenderMaterial::CanReplace( CRenderMaterial* material )
{
	// Skin can replace other skin. Non-skin can replace non-skin.
	return IsSkin() == material->IsSkin();
}

#ifndef NO_ASYNCHRONOUS_MATERIALS
Bool CRenderMaterial::ClearCompilationJobs()
{
	TDynArray< Uint32 > idsToRemove;
	
	// collect the jobs to remove
	for ( auto it = m_compilationJobs.Begin(); it != m_compilationJobs.End(); ++it )
	{
		MaterialCompilationJob* job = it->m_second;
		if ( job )
		{
			Uint32 id = job->GetContextID();
			if ( job->IsFinished() )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_techniquesMutex );

				// store result if finished
				CompiledTechnique* technique = job->GetCompiledTechnique();
				m_techniquesValid.Insert( id, technique != nullptr );
				m_techniques.Insert( id, technique );
				idsToRemove.PushBack( id );
			}
			else if ( job->IsCancelled() )
			{
				idsToRemove.PushBack( id );
			}
			else if ( job->IsReady() )
			{
				Int32 count = MaterialCompilationJob::GetCurrentlyProcessingCount();
				Int32 maxCount = GTaskManager->GetNumDedicatedTaskThreads() - 1;
				maxCount = Max( maxCount, 1 );
				if ( count < maxCount )
				{
					MaterialCompilationJob::IncreaseProcessingCount();
					GTaskManager->Issue( *job, TSP_VeryHigh );
				}
			}

			// job still running
		}
	}

	// remove jobs
	for ( Uint32 i = 0; i < idsToRemove.Size(); ++i )
	{
		MaterialCompilationJob* job = nullptr;
		VERIFY( m_compilationJobs.Find( idsToRemove[ i ], job ) );
		ASSERT( job );
		job->Release();
		m_compilationJobs.Erase( idsToRemove[ i ] );
	}

	return m_compilationJobs.Empty();
}
#endif

void CRenderMaterial::SetupParameters( const IMaterialDefinition* material )
{
	// Extract parameters
	m_pixelParameters = material->GetPixelParameters();
	m_vertexParameters = material->GetVertexParameters();

	// Remember sort group
	m_sortGroup = material->GetRenderingSortGroup();

	// Remember material friendly name
	m_engineMaterialName = material->GetFriendlyName();
	m_engineMaterialHash = Red::System::CalculateAnsiHash32LowerCase( material->GetDepotPath().AsChar() );

	// Delete compiled data
	m_techniques.ClearPtr();
	m_techniquesValid.Clear();

	RED_ASSERT( material );
	THandle< IMaterialDefinition > materialHandle( const_cast< IMaterialDefinition* >( material ) );
	m_engineMaterial = materialHandle;
	RED_ASSERT( m_engineMaterial );
	if ( !material->IsCooked() )
	{
		m_crcMap = m_engineMaterial->GetCRCMap();
	}

	// Material flags
	m_flags  = 0;
	m_flags |= material->IsTwoSided() ? RMF_TwoSided : 0;
	m_flags |= material->IsEmissive() ? RMF_Emissive : 0;
	m_flags |= material->IsForwardRendering() ? RMF_Forward : 0;
	m_flags |= material->IsAccumulativelyRefracted() ? RMF_AccumRefracted : 0;
	m_flags |= material->IsMimicMaterial() ? RMF_MimicMaterial : 0;
	m_flags |= material->IsSkin() ? RMF_Skin : 0;
	m_flags |= material->CanUseOnMeshes() ? RMF_CanUseOnMeshes : 0;
	m_flags |= material->CanUseOnParticles() ? RMF_CanUseOnParticles : 0;
	m_flags |= material->IsReflectiveMasked() ? RMF_ReflectiveMasked : 0;
	m_flags |= material->IsEye() ? RMF_Eye : 0;
	m_flags |= material->CanUseSkinnedInstancing() ? RMF_SkinnedInstancing : 0;
	m_flags |= material->CanUseOnMorphMeshes() ? RMF_CanUseOnMorphMeshes : 0;

	// Material parameter mask
	m_flags |= ( material->GetRenderingFragmentParameterMask() & RFMP_ColorShiftMatrices ) ? RMF_ColorShift : 0;
	m_flags |= ( material->GetRenderingFragmentParameterMask() & RFMP_CustomMaterialParameter0 ) ? RMF_EffectParam0 : 0;
	m_flags |= ( material->GetRenderingFragmentParameterMask() & RFMP_CustomMaterialParameter1 ) ? RMF_EffectParam1 : 0;
	m_flags |= ( material->GetRenderingFragmentParameterMask() & RFMP_FoliageColor ) ? RMF_UsesFoliageColor : 0;	

	// Call base implementation
	CRenderMaterialParameters::SetupParameters( material );
}

CName CRenderMaterial::GetCategory() const
{
	return CNAME( RenderMaterial );
}

Uint32 CRenderMaterial::GetUsedVideoMemory() const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_techniquesMutex );

	// Count techniques data size
	Uint32 dataSize = m_techniques.DataSize();

	// Count local params data size
	dataSize += static_cast< Uint32 >( m_pixelParameters.DataSize() );
	dataSize += static_cast< Uint32 >( m_vertexParameters.DataSize() );
	return dataSize + CRenderMaterialParameters::GetUsedVideoMemory();
}

Bool CRenderMaterial::Bind( const MaterialRenderingContext& context, const CRenderMaterialParameters* params, Float distance ) 
{
	CRenderMaterial::CompiledTechnique* technique = nullptr;
	if ( BindShaders( context, technique ) )
	{
		return BindParams( context, params, technique, distance );
	}

	return false;
}

Bool CRenderMaterial::BindShaders( const MaterialRenderingContext& context, CRenderMaterial::CompiledTechnique*& techniqueBound ) 
{
	// Find existing technique to use
	CompiledTechnique* technique = nullptr;

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_techniquesMutex );
		m_techniques.Find( context.CalcID(), technique );
		if ( !technique )
		{
			Bool techniqueValid = false;
			if ( m_techniquesValid.Find( context.CalcID(), techniqueValid ) )
			{
				if ( !techniqueValid )
				{
					// failed to compile
					return false;
				}
			}
		}
	}

#ifdef SYNCHRONOUS_MATERIAL_CACHE
	if ( !technique )
	{
		technique = GetTechniqueFromCache( context );
		if ( technique )
		{
			RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("'%ls' Compiled from cache synchronously"), m_engineMaterialName.AsChar() );
			m_techniques.Insert( context.CalcID(), technique );
			m_techniquesValid.Insert( context.CalcID(), true );
		}
	}
#endif // SYNCHRONOUS_MATERIAL_CACHE

	if ( !technique )
	{
#ifndef NO_ASYNCHRONOUS_MATERIALS
		// m_techniquesMutex must be released before we try to lock st_recompileMutex, or else deadlock can happen -- main thread has nested
		// locks: recompile, and then techniques a few times.
		if ( GetRenderer()->GetAsyncCompilationMode() )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( st_recompileMutex );

			MaterialCompilationJob* job = nullptr;
			m_compilationJobs.Find( context.CalcID(), job );

			if ( !job )
			{
				MaterialCompilationJob* job = new ( CTask::Root ) MaterialCompilationJob( this, context );
				m_compilationJobs.Insert( context.CalcID(), job );
				{
					// register material as being currently recompiled
					if ( !st_currentlyRecompilingMaterials.Exist( this ) )
					{
						st_currentlyRecompilingMaterials.PushBack( this );
						this->AddRef();
					}
				}
			}
		}
		else
#endif // NO_ASYNCHRONOUS_MATERIALS
		{
			// If async compilation is not enabled, then we only need to access m_techniques. No need to worry about deadlock here.
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_techniquesMutex );
			technique = MaterialCompilationJob::DoCompilation( this, context );
			m_techniques.Insert( context.CalcID(), technique );
			m_techniquesValid.Insert( context.CalcID(), technique != nullptr );
		}
	}

	// Activate
	if ( technique )
	{
		technique->Bind( context );
		techniqueBound = technique;
		return true;
	}

	// Not bound
	return false;
}

Bool CRenderMaterial::BindParams( const MaterialRenderingContext& context, const CRenderMaterialParameters* params, CRenderMaterial::CompiledTechnique* technique, Float distance )
{
	if ( technique )
	{
		return technique->BindParams( context, params, distance );
	}
	return false;
}

CRenderMaterial::CompiledTechnique* CRenderMaterial::CompileFallback( const MaterialRenderingContext& context )
{
	// Remove all gameplay specific shit from material context before compilation
	MaterialRenderingContext tempContext( *context.m_renderingContext );
	tempContext.m_vertexFactory = context.m_vertexFactory;
	tempContext.m_selected = false;

	// Get fallback material
	CMaterialGraph* fallbackMaterial = GRender->GetFallbackShader();
	if ( fallbackMaterial )
	{
		// Get rendering resources
		CRenderMaterial* material = static_cast< CRenderMaterial* >( fallbackMaterial->GetRenderResource() );
		if ( material )
		{
			// Compile fallback technique
			CompiledTechnique* fallbackTechnique = material->CompileTechnique( tempContext );
			return fallbackTechnique;
		}
	}

	// Unable to compile the material technique :(
	return nullptr;
}

static void MapUsedParameters( const TMaterialUsedParameterArray& materialUsedParameters, const IMaterialDefinition::TParameterArray& localParameters, CRenderMaterial::TUsedParameterArray& usedParameters )
{
	// Convert material dynamic data to list of used parameters
	for ( Uint32 i=0; i<materialUsedParameters.Size(); i++ )
	{
		const MaterialUsedParameter& up = materialUsedParameters[i];
		for ( Uint32 j=0; j<localParameters.Size(); j++ )
		{
			const IMaterialDefinition::Parameter& param = localParameters[j];
			if ( param.m_name == up.m_name )
			{
				// Add definition of used parameter
				const IMaterialDefinition::EParameterType paramType = (IMaterialDefinition::EParameterType) param.m_type;
				new ( usedParameters ) CRenderMaterial::UsedParameter( paramType, param.m_offset, up.m_register );
			}
		}
	}
}

CRenderMaterial::CompiledTechnique* CRenderMaterial::GetTechniqueFromCache( const MaterialRenderingContext& context )
{
	const Uint64 materialHash = CalculateMaterialHash( m_engineMaterialHash, context.CalcID() );
	Uint64 includesCRC = GShaderCache->GetCachedIncludesCRC();

	MaterialEntry* entry = nullptr;
	IShaderCache::EResult res = GShaderCache->GetMaterial( materialHash, entry );
	if ( res == IShaderCache::eResult_Pending )
	{
		return nullptr;
	}
	if ( entry )
	{
		Bool createFromCache = true;
		if ( m_engineMaterial && !m_engineMaterial->IsCooked() )
		{
			TPair< Uint64, Uint64 > crcs;
			if ( m_crcMap.Find( context.CalcID(), crcs ) )
			{
#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
				Uint64 materialCRC = crcs.m_first;
				materialCRC = ACalcBufferHash64Merge( &crcs.m_second, sizeof( crcs.m_second ), materialCRC );

				Bool isMaterialUpToDate = materialCRC == entry->m_crc;
				if ( !isMaterialUpToDate )
				{
					RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("Material '%ls' with context '%ls' differs from cache, recompilation required"), m_engineMaterialName.AsChar(), context.ToString().AsChar() );
					RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("    MaterialCRC [%") RED_PRIWu64 TXT("] (VSCRC [%") RED_PRIWu64 TXT("] PSCRC [%") RED_PRIWu64 TXT("]), cachedCRC [%") RED_PRIWu64 TXT("]"), materialCRC, crcs.m_first, crcs.m_second, entry->m_crc );
					createFromCache = false;
				}

				Bool isIncludeDirUpToDate = includesCRC == entry->m_includesCRC;
				if ( !isIncludeDirUpToDate )
				{
					RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("Includes directory differ from those the cache was created") );
					RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("    Includes CRC [%") RED_PRIWu64 TXT("], cached CRC [%") RED_PRIWu64 TXT("]"), includesCRC, entry->m_includesCRC );
					createFromCache = false;
				}
#endif // !RED_OPTIMIZED_MATERIAL_ENTRY
			}
			else
			{
				RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("Material '%ls' does not have cached CRCs for context '%ls', recompilation required"), m_engineMaterialName.AsChar(), context.ToString().AsChar() );
				createFromCache = false;
			}
		}

		if ( createFromCache )
		{
			return CreateTechniqueFromCache( entry, context, m_engineMaterialName );
		}
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("Material '%ls' [context: '%ls'] [hash: %") RED_PRIWu64 TXT("] not found in the cache"), m_engineMaterialName.AsChar(), context.ToString().AsChar(), materialHash );
	}

	return nullptr;
}

CRenderShader* CreateShader( const MaterialEntry* entry, GpuApi::eShaderType type, const String& filename )
{
	CRenderShader* shader = nullptr;
	Uint64 hash = entry->GetShaderHash( type );
	// try already created one
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( CRenderShaderMap::s_mutex );
		if ( GRenderShaderMap->Find( hash, shader ) )
		{
			RED_ASSERT( shader, TXT("Retrieved null RenderShader from RenderShaderMap %") RED_PRIWu64 TXT(" - DEBUG THIS!"), hash );
			if ( shader )
			{
				shader->AddRef();
				return shader;
			}
		}

		DataBuffer byteCode;
		IShaderCache::EResult sRes = GShaderCache->GetShaderData( hash, byteCode );
		if ( sRes != IShaderCache::eResult_Valid )
		{
			ERR_RENDERER( TXT("Unable to find bytecode for %ls [shader:%d]: %") RED_PRIWu64, filename.AsChar(), type, hash );
			return nullptr;
		}

		ERenderShaderType rst = (ERenderShaderType)type;
		shader = CRenderShader::Create( rst, byteCode, hash, filename );
		if ( shader )
		{
			RED_VERIFY( GRenderShaderMap->Insert( hash, shader ) );
		}
	}
	
	return shader;
}

CRenderMaterial::CompiledTechnique* CRenderMaterial::CreateTechniqueFromCache( const MaterialEntry* entry, const MaterialRenderingContext& context, const String& filename )
{
	CTimeCounter timer;
	RED_ASSERT( entry );

	CRenderShader* ps = nullptr;
	CRenderShader* vs = nullptr;
	CRenderShader* hs = nullptr;
	CRenderShader* ds = nullptr;
		
	ps = CreateShader( entry, GpuApi::PixelShader, filename );
	if ( !ps )
	{
		SAFE_RELEASE( ps );
		SAFE_RELEASE( vs );
		SAFE_RELEASE( ds );
		SAFE_RELEASE( hs );
		return nullptr;
	}

	vs = CreateShader( entry, GpuApi::VertexShader, filename );
	if ( !vs )
	{
		SAFE_RELEASE( ps );
		SAFE_RELEASE( vs );
		SAFE_RELEASE( ds );
		SAFE_RELEASE( hs );
		return nullptr;
	}

	Bool hasDS = entry->GetShaderHash( GpuApi::DomainShader ) != 0;
	Bool hasHS = entry->GetShaderHash( GpuApi::HullShader ) != 0;

	if ( hasHS || hasDS || context.m_vertexFactory == MVF_TesselatedTerrain )
	{
		hs = CreateShader( entry, GpuApi::HullShader, filename );
		if ( !hs )
		{
			SAFE_RELEASE( ps );
			SAFE_RELEASE( vs );
			SAFE_RELEASE( ds );
			SAFE_RELEASE( hs );
			return nullptr;
		}

		ds = CreateShader( entry, GpuApi::DomainShader, filename );
		if ( !ds )
		{
			SAFE_RELEASE( ps );
			SAFE_RELEASE( vs );
			SAFE_RELEASE( ds );
			SAFE_RELEASE( hs );
			return nullptr;
		}
	}

	CRenderMaterial::TUsedParameterArray usedPixelParameters;
	MapUsedParameters( entry->m_psUsedParameters, m_pixelParameters, usedPixelParameters );

	CRenderMaterial::TUsedParameterArray usedVertexParameters;
	MapUsedParameters( entry->m_vsUsedParameters, m_vertexParameters, usedVertexParameters );

	TSamplerStateArray psSamplerStates;
	TSamplerStateArray vsSamplerStates;
	CRenderMaterial::CreateSamplerStates( entry, vsSamplerStates, psSamplerStates );

	// Create technique
	CompiledTechnique* newTechnique = new CompiledTechnique( ps, vs, usedPixelParameters, usedVertexParameters, psSamplerStates, vsSamplerStates, hs, ds );
	RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT( "Cached shader '%ls', took %1.3fms" ), filename.AsChar(), timer.GetTimePeriodMS() );

	return newTechnique;
}

CRenderMaterial::CompiledTechnique* CRenderMaterial::CompileTechnique( const MaterialRenderingContext& context )
{
	CRenderMaterial::CompiledTechnique* technique = GetTechniqueFromCache( context );
	if ( technique )
	{
		RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT("'%ls' Compiled from cache"), m_engineMaterialName.AsChar() );
		return technique;
	}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	// Try to compile fresh version if we have source material
	if ( m_engineMaterial.Get() )
	{
		CTimeCounter timer;

		// No graph, unable to compile technique
		IMaterialDefinition* engineMaterial = m_engineMaterial.Get();
		RED_ASSERT( engineMaterial );
		RED_ASSERT( engineMaterial->GetFile() );

		if ( !engineMaterial->SupportsContext( context ) )
		{
			WARN_RENDERER( TXT("Material '%ls' does not support current context '%ls', check log for more info."), m_engineMaterialName.AsChar(), context.ToString().AsChar() );
			return nullptr;
		}

		// THandle doesn't keep it alive for us, so make sure it's not going to get destroyed before we're done with it.
		engineMaterial->AddToRootSet();

		String filename = CFilePath( engineMaterial->GetFile()->GetFileName() ).GetFileName() + context.ToString();
		Uint64 includesCRC = GShaderCache->GetCachedIncludesCRC();

		Uint64 vsCRC;
		Uint64 psCRC;
		Uint64 materialCRC;

		// Create material compiler
		CHLSLMaterialCompiler compiler( context, PLATFORM_PC );
		compiler.GenerateDefaultCode();

		// Compile material
		engineMaterial->Compile( &compiler );

		// Sort interpolators (so they don't depend on the graph structure)
		compiler.SortAndOutputInterpolators();

		// Finalize data connections
		compiler.CreateDataConnections();

		engineMaterial->CacheCRCs( &compiler );

		vsCRC = engineMaterial->GetVSCRC( context.CalcID() );
		psCRC = engineMaterial->GetPSCRC( context.CalcID() );
		materialCRC = ACalcBufferHash64Merge( &psCRC, sizeof( psCRC ), vsCRC );

		// Don't need engineMaterial after this point, so remove it from root set. Any early returns from here on don't need to worry.
		engineMaterial->RemoveFromRootSet();

		// Get pixel shader code and compile it
		CStringPrinter psCode;
		compiler.m_pixelShader->GetFinalCode( psCode );

	//#define DUMP_AUTOGEN_SHADERS
	#ifdef DUMP_AUTOGEN_SHADERS
		String filePath = TXT("ShaderGen\\") + filename;

		IFile* file;
		file = GFileManager->CreateFileWriter( filePath + TXT(".ps.fx"), FOF_Buffered|FOF_AbsolutePath );
		if ( file )
		{
			String warnString = String::Printf( TXT("Pixel shader generated for %s!"), filePath.AsChar() );
			LOG_RENDERER( warnString.AsChar() );
			file->Serialize( psCode.m_text.Data(), psCode.m_text.Size() - 1 );
			delete file;
		}
	#endif

		// Prepare material defines
		CMaterialCompilerDefines defines;
		defines.InitFromMaterialStates( context );

		Uint64 psHash = 0;
		// Compile pixel shader
		CRenderShader* ps = CRenderShader::Create( RST_PixelShader, psCode.AsChar(), defines, filename + TXT(".ps.hlsl"), psHash );
		if ( !ps )
		{
#ifndef RED_PLATFORM_ORBIS // Logging this gives us nothing on Orbis as we can't recompile anyway.
			LOG_RENDERER( TXT("%") RED_PRIWas, psCode.AsChar() ); 
#endif
	#ifdef DUMP_AUTOGEN_SHADERS
			String filePath = TXT("ShaderGen\\") + filename;

			IFile* file;
			file = GFileManager->CreateFileWriter( filePath + TXT(".ps.error.fx"), FOF_Buffered|FOF_AbsolutePath );
			if ( file )
			{
				String warnString = String::Printf( TXT("Pixel shader generated for %s!"), filePath.AsChar() );
				LOG_RENDERER( warnString.AsChar() );
				file->Serialize( psCode.m_text.Data(), psCode.m_text.Size() - 1 );
				delete file;
			}
	#endif

			//WARN_RENDERER( TXT("Pixel shader compilation failed") );
			return nullptr;
		}

		// Get vertex shader code and compile it
		CStringPrinter vsCode;
		compiler.m_vertexShader->GetFinalCode( vsCode );

	#ifdef DUMP_AUTOGEN_SHADERS
		file = GFileManager->CreateFileWriter( filePath + TXT(".vs.fx"), FOF_Buffered|FOF_AbsolutePath );
		if ( file )
		{
			String warnString = String::Printf( TXT("Vertex shader generated for %s!"), filePath.AsChar() );
			LOG_RENDERER( warnString.AsChar() );
			file->Serialize( vsCode.m_text.Data(), vsCode.m_text.Size() - 1 );
			delete file;
		}
	#endif

		Uint64 vsHash = 0;
		// Compile vertex shader
		CRenderShader* vs = CRenderShader::Create( RST_VertexShader, vsCode.AsChar(), defines, filename + TXT(".vs.hlsl"), vsHash );
		if ( !vs )
		{
#ifndef RED_PLATFORM_ORBIS // Logging this gives us nothing on Orbis as we can't recompile anyway.
			LOG_RENDERER( TXT("%") RED_PRIWas, vsCode.AsChar() ); 
#endif

			ps->Release();

			WARN_RENDERER( TXT("Vertex shader compilation failed") );
			return nullptr;
		}

		CRenderShader* hs = nullptr;
		CRenderShader* ds = nullptr;
		Uint64 hsHash = 0;
		Uint64 dsHash = 0;

		if ( context.m_vertexFactory == MVF_TesselatedTerrain )
		{
			// Compile hull shader
			hs = CRenderShader::Create( RST_HullShader, vsCode.AsChar(), defines, filename + TXT(".hs.hlsl"), hsHash );
			if ( !hs )
			{
				ps->Release();
				vs->Release();

				WARN_RENDERER( TXT("Hull shader compilation failed") );
				return nullptr;
			}

			// Compile domain shader
			ds = CRenderShader::Create( RST_DomainShader, vsCode.AsChar(), defines, filename + TXT(".ds.hlsl"), dsHash );
			if ( !ds )
			{
				ps->Release();
				vs->Release();
				hs->Release();

				WARN_RENDERER( TXT("Domain shader compilation failed") );
				return nullptr;
			}
		}

		// Map parameters
		CRenderMaterial::TUsedParameterArray usedPixelParameters;
		MapUsedParameters( compiler.m_usedPixelParameters, m_pixelParameters, usedPixelParameters );

		CRenderMaterial::TUsedParameterArray usedVertexParameters;
		MapUsedParameters( compiler.m_usedVertexParameters, m_vertexParameters, usedVertexParameters );

		// Prepare a set of required sampler states
		TSamplerStateArray vsSamplerStates, psSamplerStates;
		CreateSamplerStates( compiler, vsSamplerStates, psSamplerStates );
		
		RED_LOG( RED_LOG_CHANNEL( Shaders ), TXT( "Compiled shader '%ls', context '%ls' took %1.3f ms" ), filename.AsChar(), context.ToString().AsChar(), timer.GetTimePeriodMS() );
		if ( GShaderCache && !GShaderCache->IsReadonly() )
		{
			timer.ResetTimer();

			MaterialEntry* entry = new MaterialEntry();
			entry->m_hash = CalculateMaterialHash( m_engineMaterialHash, context.CalcID() );

#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
			entry->m_crc = materialCRC;
			entry->m_includesCRC = includesCRC;
#endif // !RED_OPTIMIZED_MATERIAL_ENTRY

			RED_ASSERT( ps );
			if ( ps->GetShader() )
			{
				entry->SetShader( GpuApi::PixelShader, psHash );
			}

			RED_ASSERT( vs );
			if ( vs->GetShader() )
			{
				entry->SetShader( GpuApi::VertexShader, vsHash );
			}

			if ( hs && hs->GetShader() )
			{
				entry->SetShader( GpuApi::HullShader, hsHash );
			}

			if ( ds && ds->GetShader() )
			{
				entry->SetShader( GpuApi::DomainShader, dsHash );
			}
			
			CHLSLMaterialShaderCompiler* vsCompiler = static_cast< CHLSLMaterialShaderCompiler* >( compiler.GetVertexShaderCompiler() );
			RED_ASSERT( vsCompiler );	
			CHLSLMaterialShaderCompiler* psCompiler = static_cast< CHLSLMaterialShaderCompiler* >( compiler.GetPixelShaderCompiler() );
			RED_ASSERT( psCompiler );

			entry->m_vsSamplerStates = vsCompiler->m_samplerStates;
			entry->m_psSamplerStates = psCompiler->m_samplerStates;
			entry->m_vsUsedParameters = compiler.m_usedVertexParameters;
			entry->m_psUsedParameters = compiler.m_usedPixelParameters;

			GShaderCache->AddMaterial( entry->m_hash, entry );
			GShaderCache->Flush();
			RED_LOG_SPAM( RED_LOG_CHANNEL( Shaders ), TXT( "Saving compiled material took %1.3f" ), timer.GetTimePeriodMS() );
		}

		// Create technique
		return new CompiledTechnique( ps, vs, usedPixelParameters, usedVertexParameters, psSamplerStates, vsSamplerStates, hs, ds );
	}
#endif // NO_RUNTIME_MATERIAL_COMPILATION

	// Not compiled
	return nullptr;
}

void CRenderMaterial::CreateSamplerStates( const TDynArray< SamplerStateInfo, MC_MaterialSamplerStates>& samplerStateInfos, TSamplerStateArray& outSamplerStates )
{
	// Prepare table
	outSamplerStates.Resize( samplerStateInfos.Size() );

	// Setup samplers
	for ( Uint32 i = 0; i < samplerStateInfos.Size(); ++i )
	{
		const SamplerStateInfo& info = samplerStateInfos[ i ];

		GpuApi::SamplerStateDesc desc;
		desc.addressU = Map( info.m_addressU );
		desc.addressV = Map( info.m_addressV );
		desc.addressW = Map( info.m_addressW );

		desc.comparisonFunction = Map( info.m_comparisonFunc );

		desc.filterMin = Map( info.m_filteringMin );
		desc.filterMag = Map( info.m_filteringMag );
		desc.filterMip = Map( info.m_filteringMip );

		desc.mipLODBias = 0.0f;
		desc.pointZFilter = false;
		desc.allowSettingsBias = true;

		GpuApi::SamplerStateRef ref = GpuApi::RequestSamplerState( desc );
		outSamplerStates[ info.m_register ] = TPair< Uint32, GpuApi::SamplerStateRef >( info.m_register, ref );
#ifdef GPU_API_DEBUG_PATH
		if ( !ref.isNull() )
		{
			GpuApi::SetSamplerStateDebugPath( ref, "materialSamplerState" );
		}
#endif
	}
}

void CRenderMaterial::CreateSamplerStates( const MaterialEntry* materialEntry, TSamplerStateArray& outVSSamplerStates, TSamplerStateArray& outPSSamplerStates )
{
	// this function is not thread safe because of GpuApi::RequestSamplerState
	// RequestSamplerState is not use anywhere else and GpuApi has no thread safety anyway so I added this lock here. 
	// GpuApi can be redone to be more thread safe.
	static Red::Threads::CMutex st_lock;
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );

	RED_ASSERT( materialEntry );
	if ( materialEntry )
	{
		CreateSamplerStates( materialEntry->m_vsSamplerStates, outVSSamplerStates );
		CreateSamplerStates( materialEntry->m_psSamplerStates, outPSSamplerStates );
	}
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

void CRenderMaterial::CreateSamplerStates( CHLSLMaterialShaderCompiler* compiler, TSamplerStateArray& outSamplerStates )
{
	// Prepare table
	outSamplerStates.Resize( compiler->m_samplerStates.Size() );

	// Setup samplers
	for ( Uint32 i = 0; i < compiler->m_samplerStates.Size(); ++i )
	{
		SamplerStateInfo info = compiler->m_samplerStates[ i ];

		GpuApi::SamplerStateDesc desc;
		desc.addressU = Map( info.m_addressU );
		desc.addressV = Map( info.m_addressV );
		desc.addressW = Map( info.m_addressW );

		desc.comparisonFunction = Map( info.m_comparisonFunc );

		desc.filterMin = Map( info.m_filteringMin );
		desc.filterMag = Map( info.m_filteringMag );
		desc.filterMip = Map( info.m_filteringMip );

		desc.mipLODBias = 0.0f;
		desc.pointZFilter = false;
		desc.allowSettingsBias = true;

		GpuApi::SamplerStateRef ref = GpuApi::RequestSamplerState( desc );
		outSamplerStates[ info.m_register ] = TPair< Uint32, GpuApi::SamplerStateRef >( info.m_register, ref );
#ifdef GPU_API_DEBUG_PATH
		if ( !ref.isNull() )
		{
			GpuApi::SetSamplerStateDebugPath( ref, "materialSamplerState" );
		}
#endif
	}
}

void CRenderMaterial::CreateSamplerStates( CHLSLMaterialCompiler& compiler, TSamplerStateArray& outVSSamplerStates, TSamplerStateArray& outPSSamplerStates )
{
	// this function is not thread safe because of GpuApi::RequestSamplerState
	// RequestSamplerState is not use anywhere else and GpuApi has no thread safety anyway so I added this lock here. 
	// GpuApi can be redone to be more thread safe.
	static Red::Threads::CMutex st_lock;
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );

	if ( compiler.GetVertexShaderCompiler() )
	{
		CreateSamplerStates( static_cast< CHLSLMaterialShaderCompiler* >( compiler.GetVertexShaderCompiler() ), outVSSamplerStates );
	}
	if ( compiler.GetPixelShaderCompiler() )
	{
		CreateSamplerStates( static_cast< CHLSLMaterialShaderCompiler* >( compiler.GetPixelShaderCompiler() ), outPSSamplerStates );
	}
}

void CRenderMaterial::ClearTechniquesFromCache()
{
	if ( !GShaderCache )
	{
		return;
	}

	// No graph, unable to compile technique
	IMaterialDefinition* engineMaterial = m_engineMaterial.Get();
	RED_ASSERT( engineMaterial );

	if ( !engineMaterial->GetFile() )
	{
		return;
	}

	Uint64 filenameHash = Red::System::CalculateAnsiHash32LowerCase( engineMaterial->GetDepotPath().AsChar() );
	filenameHash = filenameHash << 32;

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_techniquesMutex );

	for ( auto it = m_techniques.Begin(); it != m_techniques.End(); ++it )
	{
		Uint64 materialHash = filenameHash | it->m_first;
		GShaderCache->RemoveMaterial( materialHash );
	}

	m_techniques.ClearPtr();
	m_techniquesValid.Clear();
}

void CRenderInterface::ForceMaterialRecompilation( const IMaterial* material )
{
	if ( !material )
	{
		return;
	}
	if ( !material->IsA< IMaterialDefinition >() )
	{
		return;
	}

	FlushRecompilingMaterials();
	if ( CRenderMaterial* renderMaterial = static_cast< CRenderMaterial* >( material->GetRenderResource() ) )
	{
		renderMaterial->ClearTechniquesFromCache();
	}
}

#endif // NO_RUNTIME_MATERIAL_COMPILATION

IRenderResource* CRenderInterface::UploadMaterial( const IMaterial* material )
{
	RED_ASSERT( !IsDeviceLost(), TXT("Unable to create new render resources when device is lost") );

	if ( IsDeviceLost() )
	{
		return nullptr;
	}

	// Use resource cache for any materials coming from a resource file.
	const Bool useResourceCache = CanUseResourceCache() && material->GetFile();

	CRenderMaterialParameters* renderMaterial = nullptr;

	Uint64 hash = 0;
	Bool shouldCreate = false;
	if ( useResourceCache )
	{			
		hash = CRenderMaterialParameters::CalcResourceHash( material );
		shouldCreate = CRenderMaterialParameters::ResourceCacheRequestPartialCreate( hash, renderMaterial );
	}
	else
	{
		hash = 0;
		shouldCreate = true;				
	}

	if ( shouldCreate )
	{
		RED_FATAL_ASSERT( !renderMaterial, "Why should we create when there is already one created" );

		if ( material->IsA< IMaterialDefinition >() )
		{
			renderMaterial = CRenderMaterial::Create( static_cast< const IMaterialDefinition* >( material ), hash );
		}
		else
		{
			renderMaterial = CRenderMaterialParameters::Create( material, hash );
		}
	}

	return renderMaterial;
}
