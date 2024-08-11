/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialCompiler.h"
#include "../core/dependencyMapper.h"
#include "../engine/baseEngine.h"
#include "../engine/renderFragment.h"

String MaterialRenderingContext::ToString() const
{
	String mode;

	// Debug material mode
	switch ( m_materialDebugMode )
	{
	case MDM_None:										break;
	case MDM_UVDensity:									mode += TXT("_UVDensity"); break;
	case MDM_Holes:										mode += TXT("_Holes"); break;
	case MDM_Mask:										mode += TXT("_Mask"); break;
	case MDM_Overlay:									mode += TXT("_Overlay"); break;
	case MDM_Heightmap:									mode += TXT("_Heightmap"); break;
	case MDM_WaterMode:									mode += TXT("_WaterMode"); break;
	case MDM_FullGBuffer:								mode += TXT("_FullGBuffer"); break;
	default:											RED_HALT( "invalid material debug mode" );
	}
	static_assert( MDM_Max == 8, "EMaterialDebugMode probably changed. Don't forget to update here!" );

	// Vertex factory
	switch ( m_vertexFactory )
	{
	case MVF_Terrain:									mode += TXT("_Terrain"); break;
	case MVF_TerrainSkirt:								mode += TXT("_TerrainSkirt"); break;
	case MVF_MeshStatic:								mode += TXT("_MeshStatic"); break;
	case MVF_MeshDestruction:						mode += TXT("_DestructionSkinned"); break;
	case MVF_MeshSkinned:								mode += TXT("_MeshSkinned"); break;
	case MVF_ParticleBilboard:							mode += TXT("_ParticleBillboard"); break;
	case MVF_ParticleBilboardRain:						mode += TXT("_ParticleBillboardRain"); break;
	case MVF_ParticleParallel:							mode += TXT("_ParticleParallell"); break;
	case MVF_ParticleMotionBlur:						mode += TXT("_ParticleMotionBlur"); break;
	case MVF_ParticleSphereAligned:						mode += TXT("_ParticleSphereAligned"); break;
	case MVF_ParticleVerticalFixed:						mode += TXT("_ParticleVerticalFixed"); break;
	case MVF_ParticleTrail:								mode += TXT("_ParticleTrail"); break;
	case MVF_ParticleFacingTrail:						mode += TXT("_ParticleFacingTrail"); break;
	case MVF_ParticleBeam:								mode += TXT("_ParticleBeam"); break;
	case MVF_ParticleScreen:							mode += TXT("_ParticleScreen"); break;
	case MVF_ApexWithoutBones:							mode += TXT("_ApexWithoutBones"); break;
	case MVF_ApexWithBones:								mode += TXT("_ApexWithBones"); break;
	case MVF_TesselatedTerrain:							mode += TXT("_TesselatedTerrain"); break;
	default:											RED_HALT( "invalid vertex factory" );
	}
	static_assert( MVF_Max == 19, "EMaterialVertexFactory probably changed. Don't forget to update here!" );

	// Render context info
	switch ( m_renderingContext->m_pass )
	{
	case RP_NoLighting:									mode += TXT("_NoLighting"); break;
	case RP_HitProxies:									mode += TXT("_HitProxies"); break;
	case RP_ShadowDepthSolid:							mode += TXT("_ShadowDepthSolid"); break;
	case RP_ShadowDepthMasked:							mode += TXT("_ShadowDepthMasked"); break;
	case RP_Emissive:									mode += TXT("_Emissive"); break;
	case RP_GBuffer:									mode += TXT("_GBuffer"); break;
	case RP_RefractionDelta:							mode += TXT("_RefractionDelta"); break;
	case RP_ReflectionMask:								mode += TXT("_ReflectionMask"); break;
	case RP_ForwardLightingSolid:						mode += TXT("_ForwardLightingSolid"); break;
	case RP_ForwardLightingTransparent:					mode += TXT("_ForwardLightingTransparent"); break;
	case RP_HiResShadowMask:							mode += TXT("_HiResShadowMask"); break;
	default:											RED_HALT( "invalid rendering context pass" );
	}
	static_assert( RP_Max == 11, "ERenderingPass probably changed. Don't forget to update here!" );

	// Flags
	if ( m_selected )									mode += TXT("_Select");
	if ( m_useInstancing )								mode += TXT("_UseInstancing");
	if ( m_useParticleInstancing )						mode += TXT("_UseParticleInstancing");
	if ( m_hasExtraStreams )							mode += TXT("_HasExtraStreams");
	if ( m_hasVertexCollapse )							mode += TXT("_HasVertexCollapse");
	if ( m_pregeneratedMaps )							mode += TXT("_PregenMaps" );
	if ( m_discardingPass )								mode += TXT("_Discard");
	if ( m_lowQuality )									mode += TXT("_LowQ");
	if ( m_discardingPass && m_uvDissolveSeparateUV )	mode += TXT("_UVDissolvedSeparate");

	return mode;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CHLSLMaterialCompiler::CHLSLMaterialCompiler( const MaterialRenderingContext& context, ECookingPlatform platform )
	: IMaterialCompiler( context, platform )
{
	// Create vertex shader compiler
	m_vertexShader = new CHLSLMaterialVertexShaderCompiler( this );

	// Create pixel shader compiler
	m_pixelShader = new CHLSLMaterialPixelShaderCompiler( this, m_vertexShader );
}

CHLSLMaterialCompiler::~CHLSLMaterialCompiler()
{
	// Delete shader compilers
	delete m_vertexShader;
	delete m_pixelShader;
}

IMaterialShaderCompiler* CHLSLMaterialCompiler::GetVertexShaderCompiler()
{
	return m_vertexShader;
}

IMaterialShaderCompiler* CHLSLMaterialCompiler::GetPixelShaderCompiler()
{
	return m_pixelShader;
}

void CHLSLMaterialCompiler::Param( const CName& name, Uint8 reg, EMaterialShaderTarget shaderTarget )
{
	if (shaderTarget == MSH_PixelShader)
	{
		m_usedPixelParameters.PushBack( MaterialUsedParameter( name, reg ) );
	}
	else
	{
		m_usedVertexParameters.PushBack( MaterialUsedParameter( name, reg ) );
	}
}

Bool CHLSLMaterialCompiler::Function( const String& name, const String& code, const THashMap<String, EMaterialParamType>& params, EMaterialShaderTarget shaderTarget )
{
	// Do not add twice
	for ( Uint32 i=0; i<m_customFunctions.Size(); i++ )
	{
		CustomFunctionInfo& param = m_customFunctions[i];
		if ( param.m_functionName == name )
		{
			param.m_shaderTarget |= (Int32)shaderTarget;
			return true;
		}
	}

	String function = String::Printf( TXT("float4 %ls("), name.AsChar(), code.AsChar() );
	for (THashMap< String, EMaterialParamType >::const_iterator it=params.Begin(); it!=params.End(); ++it)
	{
		function += String::Printf( TXT("%ls %ls, "), GetMaterialParamTypeName(it->m_second).AsChar(), it->m_first.AsChar());
	}
	if (params.Size())
	{
		size_t SubStringIndex;
		if(function.FindSubstring(TXT(","), SubStringIndex, true))
		{
			function = function.MidString(0, SubStringIndex);
		}
	}
	function += String::Printf( TXT(") \n\r { \n\r %ls \n\r } \n\r"), code.AsChar());


	CustomFunctionInfo info;
	info.m_codeChunk = CodeChunk::Printf( false, UNICODE_TO_ANSI( function.AsChar() ) );
	info.m_shaderTarget = shaderTarget;
	info.m_functionName = name;

	m_customFunctions.PushBack(info);
	return false;
}

void CHLSLMaterialCompiler::GenerateDefaultCode()
{
	// empty
}

void CHLSLMaterialCompiler::Connect( EMaterialDataType type, const CodeChunk& vsSource, const CodeChunk& psTarget, const CodeChunk& semantic )
{
	Interpolator output;
	output.m_type = type;
	output.m_semantic = semantic;
	output.m_psTarget = psTarget;
	output.m_vsSource = vsSource;
	output.m_index = m_interpolators.Size();
	m_interpolators.PushBack( output );

	// Use temporary connection variable to write it into the pixel shader
	m_pixelShader->m_code.Print( "%s = %s;", psTarget.AsChar(), ConnectionVarName( output.m_index ).AsChar() );
}

Uint64 CHLSLMaterialCompiler::GetVSCodeCRC() const
{
	CStringPrinter printer;
	m_vertexShader->GetFinalCode( printer );	
	return ACalcBufferHash64Merge( printer.AsChar(), 0 );
}

Uint64 CHLSLMaterialCompiler::GetPSCodeCRC() const
{
	CStringPrinter printer;
	m_pixelShader->GetFinalCode( printer );
	return ACalcBufferHash64Merge( printer.AsChar(), 0 );
}

void CHLSLMaterialCompiler::CreateDataConnections()
{
	IMaterialShaderCompiler& vs = *GetVertexShaderCompiler();
	// Output screen position
	CodeChunk screenPosition = vs.Data( "ScreenPosition" );
	vs.Output( MDT_Float4, "SYS_POSITION", screenPosition );

	/// Create matched output and inputs
	Uint32 i = 0;
	for ( Uint32 allIndex=0; allIndex<m_connections.Size(); allIndex++ )
	{
		const Connection& info = m_connections[allIndex];
		EMaterialDataType dataTypes[][5] = { { MDT_Float, MDT_Float, MDT_Float2, MDT_Float3, MDT_Float4 },
											{ MDT_Int, MDT_Int, MDT_Int2, MDT_Int3, MDT_Int4 } };
		CodeChunk connectionSemantic;
		if (info.m_semantic.IsEmpty())
		{
			connectionSemantic = CodeChunk::Printf( true, "TEXCOORD%i", i );
			++i;
		}
		else
		{
			connectionSemantic = info.m_semantic;
		}
		if (GetContext().m_renderingContext->m_pass != RP_ShadowDepthSolid || GCookingPlatform == PLATFORM_PC)
		{
			m_vertexShader->Output( dataTypes[info.m_componentType][ info.m_usedComponents ], connectionSemantic, CodeChunk::EMPTY );
		}
		m_pixelShader->Input( dataTypes[info.m_componentType][ info.m_usedComponents ], connectionSemantic );
	}
}

void CHLSLMaterialCompiler::SortAndOutputInterpolators()
{
	// Sor interpolators based on the Semantic item
	Sort( m_interpolators.Begin(), m_interpolators.End(), SortBySemantic::Sort );

	// Allocate interpolators (evaluate semantic indices)
	for( auto i : m_interpolators )
	{
		AllocateInterpolator( i );
	}
}

void CHLSLMaterialCompiler::AllocateInterpolator( Interpolator& interpolator )
{
	const AnsiChar swizzle[4] = { 'x', 'y', 'z', 'w' };

	// Get number of components needed
	const Uint32 numComponents = m_pixelShader->GetTypeFloatCount( interpolator.m_type );
	ASSERT( numComponents >= 1 && numComponents <= 4 );

	// Qualify component type (float/int/...)
	EComponentType componentType = CT_Float;
	if ( interpolator.m_type >= MDT_Uint )
	{
		componentType = CT_Int;
	}

	CodeChunk connectionVar = ConnectionVarName( interpolator.m_index );
	CStringPrinter declaration;
	declaration.Print( "%s %s = ( 0.000000 );", m_pixelShader->GetTypeName( interpolator.m_type ).AsChar(), connectionVar.AsChar() );

	// Connection data
	Int32 conStart = - 1;
	Int32 conRegister=-1;
	Int32 conIndex=-1;

	// Try to allocate inside existing block
	Uint32 numFreeComponents = 0;
	Uint32 i=0;
	Uint32 allIndex=0;
	for ( ; allIndex<m_connections.Size(); allIndex++ )
	{
		Connection &c = m_connections[allIndex];

		// Is the data type compliant
		if (c.m_semantic.IsEmpty())
		{
			if (c.m_componentType == componentType)
			{
				// Calculate how many components are still free
				const Uint32 freeComponents = 4 - c.m_usedComponents;
				numFreeComponents += freeComponents;

				// Will fit ?
				if ( freeComponents >= numComponents )
				{
					conRegister = i;
					conIndex = allIndex;
					conStart = c.m_usedComponents;
					break;
				}
			}
			++i;
		}
	}

	// If continuous space was not found and there is no possibility for packing then create new connection
	if ( conRegister == -1 && numFreeComponents < numComponents )
	{
		// Add new connection
		Connection info;
		info.m_usedComponents = 0;
		info.m_componentType = componentType;
		m_connections.PushBack( info );

		// Use it
		conRegister = i;
		conIndex = allIndex;
		conStart = 0;
	}

	// If we have valid register then place connection there now
	if ( conRegister != -1 )
	{   
		AnsiChar dataMask[5];
		AnsiChar conMask[5];

		// Generate swizzle
		conMask[ numComponents ] = 0;
		dataMask[ numComponents ] = 0;
		for ( Uint32 i=0; i<numComponents; i++ )
		{
			conMask[ i ] = swizzle[ conStart + i ];
			dataMask[ i ] = swizzle[ i ];
		}

		// Generate link
		if ( GetContext().m_renderingContext->m_pass != RP_ShadowDepthSolid || GCookingPlatform == PLATFORM_PC )
		{
			m_vertexShader->m_code.Print( "Output.outTEXCOORD%i.%s = %s.%s;", conRegister, conMask, interpolator.m_vsSource.AsChar(), dataMask );
		}
		declaration.Print( "%s.%s = Input.inTEXCOORD%i.%s;", connectionVar.AsChar(), dataMask, conRegister, conMask );

		// Eat free components from connection register
		m_connections[ conIndex ].m_usedComponents += numComponents;
	}

	// Try packing
	else
	{
		Uint32 start=0;
		Uint32 i = 0;
		for ( Uint32 allIndex=0; allIndex<m_connections.Size(); allIndex++ )
		{
			Connection &c = m_connections[allIndex];

			if (!c.m_semantic.IsEmpty())
			{
				// Skip registers with predefined semantics
				continue; 
			}

			if ( c.m_componentType != componentType || c.m_usedComponents == 4 )
			{
				// Skip other types or used up connections
				++i;
				continue;
			}

			// How many we can pack
			Uint32 capacity = 4 - c.m_usedComponents;
			Uint32 pack = Min( capacity, numComponents - start );
			AnsiChar dataMask[5];
			AnsiChar conMask[5];

			// Generate swizzle
			conMask[ pack ] = 0;
			dataMask[ pack ] = 0;
			for ( Uint32 j=0; j<pack; j++ )
			{
				conMask[j] = swizzle[ c.m_usedComponents + j ];
				dataMask[j] = swizzle[ start + j ];
			}

			// Generate link
			if (GetContext().m_renderingContext->m_pass != RP_ShadowDepthSolid || GCookingPlatform == PLATFORM_PC)
			{
				m_vertexShader->m_code.Print( "Output.outTEXCOORD%i.%s = %s.%s;", i, conMask, interpolator.m_vsSource.AsChar(), dataMask );
			}
			declaration.Print( "%s.%s = Input.inTEXCOORD%i.%s;", connectionVar.AsChar(), dataMask, i, conMask );			

			// Advance
			start += pack;
			c.m_usedComponents += pack;

			++i;

			// All components packet
			if ( start == numComponents ) break;
		}
	}  
	m_pixelShader->m_code.Prepend( declaration.AsChar() );
}

#endif

