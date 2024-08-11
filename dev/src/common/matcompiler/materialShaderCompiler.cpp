/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CHLSLMaterialShaderCompiler::CHLSLMaterialShaderCompiler( CHLSLMaterialCompiler* compiler, Uint32 firstFreeConstReg, Uint32 firstFreeSamplerReg, Uint32 firstFreeSamplerStateReg, Uint32 firstFreeIntConstReg )
	: m_freeConstReg( firstFreeConstReg )
	, m_freeSamplerReg( firstFreeSamplerReg )
	, m_freeSamplerStateReg( firstFreeSamplerStateReg )
	, m_freeBoolConstReg ( firstFreeIntConstReg )
	, m_compiler( compiler )
{
	m_tessParams.m_vertexFactoryOverride = true; //HACK this way we don't generate tessellation code until necessary
}

CodeChunk CHLSLMaterialShaderCompiler::Var( EMaterialDataType type, const CodeChunk& value )
{
	// Local variable
	CodeChunk varName = AutomaticName();
	CodeChunk typeName = GetTypeName( type );

	// Emit code
	if ( value )
	{
		m_code.Print( "%s %s = %s;", typeName.AsChar(), varName.AsChar(), value.AsChar() );
	}
	else
	{
		m_code.Print( "%s %s;", typeName.AsChar(), varName.AsChar() );
	}

	// Use variable name as code chunk
	varName.SetConst( value.IsConst() );
	return varName;
}

CodeChunk CHLSLMaterialShaderCompiler::Var( EMaterialDataType type, Uint32 arrayCount, const CodeChunk& value )
{
	// Local variable
	CodeChunk varName = AutomaticName();
	CodeChunk typeName = GetTypeName( type );

	// Emit code
	if ( value )
	{
		m_code.Print( "%s %s[%i] = %s;", typeName.AsChar(), varName.AsChar(), arrayCount, value.AsChar() );
	}
	else
	{
		m_code.Print( "%s %s[%i];", typeName.AsChar(), varName.AsChar(), arrayCount );
	}

	// Use variable name as code chunk
	varName.SetConst( value.IsConst() );
	return varName;
}

CodeChunk CHLSLMaterialShaderCompiler::ConstReg( EMaterialDataType type, const AnsiChar* name )
{	
	return CodeChunk( name, false );
}

CodeChunk CHLSLMaterialShaderCompiler::Param( EMaterialDataType type, Int32 *allocatedRegister, const CodeChunk& givenName /*=String::EMPTY*/, Int32 arrayCount /* =0 */ )
{
	if ( m_freeConstReg == 0xFFFFFFFF )
	{
		ASSERT( !"Unable to allocate shader Param - it's disabled for this shader type" );
		return CodeChunk::EMPTY;
	}

	CodeChunk name = givenName;
	if ( name )
	{
		// Reuse if already allocated
		for ( Uint32 i=0; i<m_params.Size(); i++ )
		{
			const ParamInfo& param = m_params[i];
			if ( param.m_name == name )
			{
				ASSERT( param.m_type == type );
				if ( allocatedRegister )
				{
					*allocatedRegister = param.m_register;
				}
				return param.m_name;
			}
		}
	}
	else
	{
		// Generate name
		name = AutomaticName();
	}

	// Allocate register
	Int32 usedRegister = m_freeConstReg;
	m_freeConstReg += GetTypeRegisterCount( type ) * ::Max< Int32 >( 1, arrayCount );

	// Add info
	ParamInfo info;
	info.m_name = name;
	info.m_name.SetConst( false );
	info.m_numRegisters = GetTypeRegisterCount( type ) * arrayCount ? arrayCount : 1;
	info.m_register = usedRegister;
	info.m_arrayCount = arrayCount;
	info.m_type = type;
	m_params.PushBack( info );

	// Output allocated register index
	if ( allocatedRegister )
	{
		*allocatedRegister = usedRegister;
	}

	// Use param name as code chunk
	return info.m_name;
}

CodeChunk CHLSLMaterialShaderCompiler::BoolParam( EMaterialDataType type, Int32 *allocatedRegister, const CodeChunk& value /*= CodeChunk::EMPTY*/ )
{
	CodeChunk name = value;
	if ( name )
	{
		// Reuse if already allocated
		for ( Uint32 i=0; i<m_boolParams.Size(); i++ )
		{
			const ParamInfo& param = m_boolParams[i];
			if ( param.m_name == name )
			{
				ASSERT( param.m_type == type );
				if ( allocatedRegister )
				{
					*allocatedRegister = param.m_register;
				}
				return param.m_name;
			}
		}
	}
	else
	{
		// Generate name
		name = AutomaticName();
	}

	// Allocate new register
	Int32 arrayCount = 0;
	Int32 usedRegister = m_freeBoolConstReg;
	m_freeBoolConstReg += GetTypeRegisterCount( type ) * ::Max< Int32 >( 1, arrayCount );

	// Add info
	ParamInfo info;
	info.m_name = name;
	info.m_name.SetConst( false );
	info.m_numRegisters = GetTypeRegisterCount( type ) * arrayCount ? arrayCount : 1;
	info.m_register = usedRegister;
	info.m_arrayCount = arrayCount;
	info.m_type = type;
	m_boolParams.PushBack( info );

	// Output allocated register index
	if ( allocatedRegister )
	{
		*allocatedRegister = usedRegister;
	}

	// Use param name as code chunk
	return info.m_name;
}

CodeChunk CHLSLMaterialShaderCompiler::ConstSampler( EMaterialSamplerType type, const AnsiChar* name, Int32 constantRegister )
{
	// All const samplers are being defined anyway.
	// We don't want any sampler 'allocated' here (added explicitly to the final shader) because in case original 
	// definition from globalConstantsPS will get used anywhere, the "overlapping register semantic not implemented" 
	// compiler error would pop out.
	return CodeChunk( name, false );

/*
	// Reuse if already allocated
	for ( Uint32 i=0; i<m_samplers.Size(); i++ )
	{
		const SamplerInfo& param = m_samplers[i];
		if ( param.m_name == name )
		{
			ASSERT( param.m_register == (Uint32)constantRegister );
			return param.m_name;
		}
	}

	// Add info
	SamplerInfo info;
	info.m_name = CodeChunk( name, false );
	info.m_register = constantRegister;
	info.m_type = type;
	m_samplers.PushBack( info );

	// Use sampler name as code chunk
	return info.m_name;
*/
}

CodeChunk CHLSLMaterialShaderCompiler::Texture( EMaterialSamplerType type, Int32 *allocatedRegister, const CodeChunk& givenName /*=String::EMPTY*/ )
{
	CodeChunk name = givenName;
	if ( name )
	{
		// Reuse if already allocated
		for ( Uint32 i=0; i<m_textures.Size(); i++ )
		{
			const TextureInfo& texInfo = m_textures[i];
			if ( texInfo.m_name == name )
			{
				ASSERT( texInfo.m_type == type );
				if ( allocatedRegister )
				{
					*allocatedRegister = texInfo.m_register;
				}
				return texInfo.m_name;
			}
		}
	}
	else
	{
		// Generate name
		name = AutomaticName();
	}

	// Allocate new register
	Int32 usedRegister = m_freeSamplerReg;
	m_freeSamplerReg += 1;

	// Add info
	TextureInfo info;
	info.m_name = name;
	info.m_name.SetConst( false );
	info.m_register = usedRegister;
	info.m_type = type;
	m_textures.PushBack( info );

	// Output allocated register index
	if ( allocatedRegister )
	{
		*allocatedRegister = usedRegister;
	}

	// Use sampler name as code chunk
	return info.m_name;
}

// Allocate sampler state
CodeChunk CHLSLMaterialShaderCompiler::SamplerState( ETextureAddressing addressU,
	ETextureAddressing addressV,
	ETextureAddressing addressW,
	ETextureFilteringMin filteringMin,
	ETextureFilteringMag filteringMag,
	ETextureFilteringMip filteringMip,
	ETextureComparisonFunction comparisonFunction )
{
	SamplerStateInfo info;
	info.m_addressU = addressU;
	info.m_addressV = addressV;
	info.m_addressW = addressW;
	info.m_filteringMin = filteringMin;
	info.m_filteringMag = filteringMag;
	info.m_filteringMip = filteringMip;
	info.m_comparisonFunc = comparisonFunction;

	CodeChunk infoName = GetSamplerStateName( info );

	// Reuse if already allocated
	for ( Uint32 i=0; i<m_samplerStates.Size(); i++ )
	{
		const SamplerStateInfo& param = m_samplerStates[i];
		if ( param == info )
		{
			return infoName;
		}
	}

	// Allocate new register
	Int32 usedRegister = m_freeSamplerStateReg;
	++m_freeSamplerStateReg;

	info.m_register = usedRegister;

	m_samplerStates.PushBack( info );

	return infoName;
}

CodeChunk CHLSLMaterialShaderCompiler::Macro( const CodeChunk& macro )
{
	m_macros.PushBack(macro);

	// No code chunk is generated by this call
	return CodeChunk::EMPTY;
}

CodeChunk CHLSLMaterialShaderCompiler::Include( const CodeChunk& path )
{
	// Check if already included
	bool alreadyIncluded = false;
	for (Uint32 i=0; i<m_includes.Size(); ++i)
	{
		if (m_includes[i].m_path == path)
		{
			alreadyIncluded = true;
			break;
		}
	}

	// Add new path
	if ( !alreadyIncluded )
	{
		IncludeInfo info;
		info.m_path = path;
		m_includes.PushBack(info);
	}

	// No code chunk is generated by this call
	return CodeChunk::EMPTY;
}

CodeChunk CHLSLMaterialShaderCompiler::Input( EMaterialDataType type, const CodeChunk& semantic )
{
	// Do not add twice
	for ( Uint32 i=0; i<m_inputs.Size(); i++ )
	{
		const InputInfo& param = m_inputs[i];
		if ( param.m_semantic == semantic )
		{
			ASSERT( type == param.m_type );
			return param.m_code;
		}
	}

	// Add
	InputInfo info;
	info.m_semantic = semantic;
	info.m_code = CodeChunk::Printf( false, "Input.in%s", semantic.AsChar() );
	info.m_type = type;
	m_inputs.PushBack( info );
	return info.m_code;
}

CodeChunk CHLSLMaterialShaderCompiler::Output( EMaterialDataType type, const CodeChunk& semantic, const CodeChunk& value )
{
	// Do not add twice
	for ( Uint32 i=0; i<m_outputs.Size(); i++ )
	{
		OutputInfo& param = m_outputs[i];
		if ( param.m_semantic == semantic )
		{
			ASSERT( type == param.m_type );

			// Output data
			if ( value )
			{
				//HACK TESS
				if ( Red::System::StringCompare(value.AsChar(), "FatVertex.WorldPosition") == 0 )
				{
					m_code.Print( "Output.out%s = %s;", param.m_semantic.AsChar(), "float4(FatVertex.WorldPosition, 1)" );
				}
				else
				{
					m_code.Print( "Output.out%s = %s;", param.m_semantic.AsChar(), value.AsChar() );
				}
			}

			return param.m_code;
		}
	}

	// Add
	OutputInfo info;
	info.m_semantic = semantic;
	info.m_code = CodeChunk::Printf( value.IsConst(), "Output.out%s", semantic.AsChar() );
	info.m_type = type;
	m_outputs.PushBack( info );

	// Output data
	if ( value )
	{
		m_code.Print( "Output.out%s = %s;", semantic.AsChar(), value.AsChar() );
	}

	// Identification
	return info.m_code;
}

CodeChunk CHLSLMaterialShaderCompiler::Data( const CodeChunk& semantic )
{
	// Do not add twice
	for ( Uint32 i=0; i<m_data.Size(); i++ )
	{
		const DataInfo& param = m_data[i];
		if ( param.m_semantic == semantic )
		{
			return param.m_code;
		}
	}

	// Request data generation
	CodeChunk chunk;
	if ( !GenerateData( semantic, chunk ))
	{
		ASSERT( TXT("Unrecognized data semantic") );
		return CodeChunk::EMPTY;
	}	

	// Add
	DataInfo info;
	info.m_semantic = semantic;
	info.m_code = chunk;
	info.m_code.SetConst( false );
	m_data.PushBack( info );	
	return info.m_code;
}

void CHLSLMaterialShaderCompiler::Tessellate( const CodeChunk& triangleSize, const CodeChunk& displacementMap, const CodeChunk& displacementScale, ETessellationDomain tessellationDomain, Bool vertexFactoryOverride )
{
	m_tessParams.m_triangleSize = triangleSize;
	m_tessParams.m_displacementMap = displacementMap;
	m_tessParams.m_displacementScale = displacementScale;
	m_tessParams.m_domain = tessellationDomain;
	m_tessParams.m_vertexFactoryOverride = vertexFactoryOverride;
}

void CHLSLMaterialShaderCompiler::Statement( const CodeChunk& value, Bool prepend )
{
	if (prepend)
	{
		m_code.Prepend( "\r\n" );
		m_code.Prepend( value.AsChar() );
	}
	else
	{
		m_code.Append( value.AsChar() );
		m_code.Append( "\r\n" );
	}
}

CodeChunk CHLSLMaterialShaderCompiler::ApplyGammaToLinearExponent( EMaterialDataType dataType, const CodeChunk &code, bool inverse, bool allowClampZero )
{
	Float exp = GAMMA_TO_LINEAR_EXPONENT;
	ASSERT ( exp > 0 && 1 != exp );

	if ( inverse )
	{
		exp = 1.f / exp;
	}
	
	switch ( dataType )
	{
	case MDT_Float:		// falldown
	case MDT_Float2:	// falldown
	case MDT_Float3:
		{
			CodeChunk result = Var( dataType, code );
			
			if ( allowClampZero )
			{
				result = Var( dataType, CodeChunkOperators::Max( CodeChunk(0.f), result ) );
			}

			return CodeChunkOperators::Pow( result, exp );		
		}

	case MDT_Float4:
		{
			CodeChunk result = Var( dataType, code );

			if ( allowClampZero )
			{
				result = Var( dataType, CodeChunkOperators::Float4( CodeChunkOperators::Max( CodeChunk(0.f), result.xyz() ), result.w() ) );
			}

			return CodeChunkOperators::Float4( CodeChunkOperators::Pow( result.xyz(), exp ), result.w() ); // apply to rgb only
		}
		break;

	default:
		RED_LOG_WARNING( Material, TXT("Shader compilation ApplyGammaToLinearExponent() failed because of unsupported data type.") );
	}

	return code;
}

CodeChunk CHLSLMaterialShaderCompiler::GetTypeName( EMaterialDataType type ) const
{
	switch ( type )
	{
		case MDT_Float: return "float";
		case MDT_Float2: return "float2";
		case MDT_Float3: return "float3";
		case MDT_Float4: return "float4";
		case MDT_Float4x4: return "float4x4";
		case MDT_Float4x3: return "float4x3";
		case MDT_Float3x3: return "float3x3";
		case MDT_Bool:	   return "bool";
		case MDT_Uint: return "uint";
		case MDT_Int: return "int";
		case MDT_Int2: return "int2";
		case MDT_Int3: return "int3";
		case MDT_Int4: return "int4";
	};

	ASSERT( !"Unknown shader data type" );
	return "float";
}

Int32 CHLSLMaterialShaderCompiler::GetTypeFloatCount( EMaterialDataType type ) const
{
	switch ( type )
	{
		case MDT_Float: return 1;
		case MDT_Float2: return 2;
		case MDT_Float3: return 3;
		case MDT_Float4: return 4;
		case MDT_Float4x4: return 16;
		case MDT_Float4x3: return 12;
		case MDT_Float3x3: return 9;
		case MDT_Bool: return 1;	//??
		case MDT_Uint: return 1;
		case MDT_Int: return 1;
		case MDT_Int2: return 2;
		case MDT_Int3: return 3;
		case MDT_Int4: return 4;
	};

	ASSERT( !"Unknown shader data type" );
	return 1;
}

Int32 CHLSLMaterialShaderCompiler::GetTypeRegisterCount( EMaterialDataType type ) const
{
	switch ( type )
	{
		case MDT_Float: return 1;
		case MDT_Float2: return 1;
		case MDT_Float3: return 1;
		case MDT_Float4: return 1;
		case MDT_Float4x4: return 4;
		case MDT_Float4x3: return 3;
		case MDT_Float3x3: return 3;
		case MDT_Bool:	return 1;
		case MDT_Uint:	return 1;
		case MDT_Int: return 1;
		case MDT_Int2: return 1;
		case MDT_Int3: return 1;
		case MDT_Int4: return 1;
	};

	ASSERT( !"Unknown shader data type" );
	return 1;
}

CodeChunk CHLSLMaterialShaderCompiler::GetTextureTypeName( EMaterialSamplerType type ) const
{
	switch ( type )
	{
		case MST_Texture:		return "Texture2D";
		case MST_Cubemap:		return "TextureCube";
		case MST_Volume:		return "Texture3D";
		case MST_TextureArray:	return "TEXTURE2D_ARRAY";
	};

	ASSERT( !"Unknown shader sampler type" );
	return "Texture2D";
}

CodeChunk CHLSLMaterialShaderCompiler::GetSamplerStateName( SamplerStateInfo info ) const
{
	static const AnsiChar* MinFilterChar[]	= { "MinP", "MinL", "MinA", "MinAL" };
	static const AnsiChar* MagFilterChar[]	= { "MagP", "MagL" };
	static const AnsiChar* MipFilterChar[]	= { "NoMip", "MipP", "MipL" };
	static const AnsiChar* AddressingChar[]	= { "Wrap", "Mirr", "Clamp", "MirrOnce" };
	static const AnsiChar* CmpFunc[]		= { "", "Less", "Equal", "LE", "Great", "NE", "GE", "Always" };

	return CodeChunk::Printf( false, "s_%s%s%s%s%s%s%s", 
		MinFilterChar[info.m_filteringMin], 
		MagFilterChar[info.m_filteringMag],
		MipFilterChar[info.m_filteringMip],
		AddressingChar[info.m_addressU],
		AddressingChar[info.m_addressV],
		AddressingChar[info.m_addressW],
		CmpFunc[info.m_comparisonFunc]
	);
}

void CHLSLMaterialShaderCompiler::GetFinalCode( CStringPrinter& code ) const
{
	// Macros
	if ( m_macros.Size() )
	{
		code.Print( "// Custom macros" );
		for (Uint32 i=0; i<m_macros.Size(); ++i)
		{
			code.Print( "%s", m_macros[i].AsChar() );
		}
		code.Print( "" );
	}

	if ( GetShaderTarget() == MSH_VertexShader && (m_compiler->GetContext().m_vertexFactory == MVF_TesselatedTerrain || m_compiler->GetContext().m_vertexFactory == MVF_TerrainSkirt ) )
	{
		StringAnsi code2;

		// Read shader from file
		String shaderPath = GFileManager->GetBaseDirectory();
		if( m_compiler->GetContext().m_vertexFactory == MVF_TesselatedTerrain )
		{
			shaderPath += TXT("/shaders/tesselatedVertexFactoryTerrain.fx");
		}
		else
		{
			 shaderPath += TXT("/shaders/vertexFactoryTerrainSkirt.fx");
		}
		IFile* shaderFile = GFileManager->CreateFileReader( shaderPath, FOF_AbsolutePath );
		Uint32 fileSize = static_cast< Uint32 >( shaderFile->GetSize() );
		char* externalCode = new char[fileSize+1];
		shaderFile->Serialize( externalCode, fileSize );

		code2.Append( externalCode, fileSize );

		// Add generated outputs structure declaration
		{
			ASSERT( !m_outputs.Empty() );

			CStringPrinter generatedCode;
			generatedCode.Print( "// Outputs" );
			generatedCode.Print( "struct OUTPUTS {" );

			Bool hasPos = false;
			for ( Uint32 i=0; i<m_outputs.Size(); i++ )
			{
				const OutputInfo& info = m_outputs[i];
				if( Red::System::StringSearch( info.m_semantic.AsChar(), "SYS_POSITION" ) != NULL )
				{
					hasPos = true;
					continue;
				}
				generatedCode.Print( "  %s out%s : %s;", GetTypeName( info.m_type ).AsChar(), info.m_semantic.AsChar(), info.m_semantic.AsChar() );
			}

			if (hasPos)
			{
				generatedCode.Print( "  %s out%s : %s;", "float4", "SYS_POSITION", "SYS_POSITION" );
			}

			generatedCode.Print( "};" ); 
			generatedCode.Print( "" );
			StringAnsi outputsStruct( generatedCode.AsChar() );

			code2.Replace( "$GENERATE_OUTPUTS_STRUCTURE", outputsStruct );
		}

		// Add generated outputs assignment
		{
			StringAnsi outputsAssignment( "OUTPUTS Output = (OUTPUTS)0;\n" );
			outputsAssignment += m_code.AsChar();

			code2.Replace( "$GENERATE_OUTPUTS_FROM_FAT_VERTEX", outputsAssignment );
		}

		// Add return line
		{
			StringAnsi returnOutput( "return Output;\n" );

			code2.Replace( "$END_OF_DOMAIN", returnOutput );
		}
		
		//LOG( ANSI_TO_UNICODE( code2.AsChar() ) );

		delete [] externalCode;
		delete shaderFile;
		code.Append( code2.AsChar() );

		return;
	}

	// Common includes
	if ( GetShaderTarget() == MSH_VertexShader )
	{
		code.Print( "#include \"commonVS.fx\"" );
	}
	else
	{
		code.Print( "#include \"commonPS.fx\"" );
	}

	// Vertex factory
	if ( GetShaderTarget() == MSH_VertexShader )
	{
		// Include a header depending on the type of vertex factory
		const EMaterialVertexFactory factoryType = m_compiler->GetContext().m_vertexFactory;
		switch ( factoryType )
		{
			case MVF_Terrain:							code.Print( "#include \"vertexFactoryTerrain.fx\"" ); break;
			case MVF_MeshStatic:						code.Print( "#include \"vertexFactoryMeshStatic.fx\"" ); break;
			case MVF_MeshSkinned:						code.Print( "#include \"vertexFactoryMeshSkinned.fx\"" ); break;			
			case MVF_ParticleBilboard:					code.Print( "#include \"vertexFactoryParticleBillboards.fx\"" ); break;
			case MVF_ParticleBilboardRain:				code.Print( "#include \"vertexFactoryParticleBillboardsRain.fx\"" ); break;
			case MVF_ParticleParallel: 					code.Print( "#include \"vertexFactoryParticleEmitterOrientation.fx\"" ); break;
			case MVF_ParticleMotionBlur:				code.Print( "#include \"vertexFactoryParticleMotionBlur.fx\"" ); break;
			case MVF_ParticleSphereAligned:				code.Print( "#include \"vertexFactoryParticleSphereAligned.fx\"" ); break;
			case MVF_ParticleVerticalFixed:				code.Print( "#include \"vertexFactoryParticleVerticalFixed.fx\"" ); break;
			case MVF_ParticleTrail:						code.Print( "#include \"vertexFactoryParticleTrail.fx\"" ); break;
			case MVF_ParticleFacingTrail:				code.Print( "#include \"vertexFactoryParticleFacingTrail.fx\"" ); break;
			case MVF_ParticleBeam:						code.Print( "#include \"vertexFactoryParticleBeam.fx\"" ); break;
			case MVF_ParticleScreen:					code.Print( "#include \"vertexFactoryParticleScreen.fx\"" ); break;
			case MVF_ApexWithoutBones:					code.Print( "#include \"vertexFactoryApexStatic.fx\"" ); break;
			case MVF_ApexWithBones:						code.Print( "#include \"vertexFactoryApexSkinned.fx\"" ); break;
			case MVF_MeshDestruction:					code.Print( "#include \"vertexFactoryDestructionSkinned.fx\"" ); break;
			default:									ASSERT( !"invalid vertex factory" );
		}
	}
	
	// Custom functions
	if ( m_compiler->m_customFunctions.Size() )
	{
		code.Print( "// Custom functions" );

		for ( Uint32 i=0; i<m_compiler->m_customFunctions.Size(); i++ )
		{
			if (m_compiler->m_customFunctions[i].m_shaderTarget & GetShaderTarget())
			{
				const CodeChunk& chunk = m_compiler->m_customFunctions[i].m_codeChunk;
				code.Print( "\n\r %s \n\r", chunk.AsChar() );
			}
		}
	}

	// Parameters
	if ( m_params.Size() )
	{
		code.Print( "// Parameters" );
		Uint32 startRegister = 0;
		if (GetShaderTarget() == MSH_PixelShader)
		{
			code.Print( "START_CB( ShaderSpecificConstants, 4 )" );	// Goes after CustomPixelConsts
		}
		else
		{
			code.Print( "START_CB( ShaderSpecificVertexConstants, 4 )" ); // Goes after CustomVertexConsts
		}

		for ( Uint32 i=0; i<m_params.Size(); i++ )
		{
			const ParamInfo& info = m_params[i];
			if ( info.m_arrayCount )
			{
				code.Print( "%s %s[%i] : packoffset ( c%i );", GetTypeName( info.m_type ).AsChar(), info.m_name.AsChar(), info.m_arrayCount, info.m_register - startRegister );
			}
			else
			{
				code.Print( "%s %s : packoffset ( c%i );", GetTypeName( info.m_type ).AsChar(), info.m_name.AsChar(), info.m_register - startRegister );
			}
		}

		code.Print( "END_CB" );
		code.Print( "" );
	}

	// Textures
	if ( m_textures.Size() )
	{
		code.Print( "// Textures" );
		for ( Uint32 i=0; i<m_textures.Size(); i++ )
		{
			const TextureInfo& info = m_textures[i];

			code.Print( "%s %s : register ( t%i );", GetTextureTypeName( info.m_type ).AsChar(), info.m_name.AsChar(), info.m_register );
		}
		code.Print( "" );
	}

	// Samplers
	if ( m_textures.Size() )
	{
		code.Print( "// Samplers" );
		for ( Uint32 i=0; i<m_samplerStates.Size(); i++ )
		{
			const SamplerStateInfo& info = m_samplerStates[i];

			code.Print( "SamplerState %s : register ( s%i );", GetSamplerStateName( info ).AsChar(), info.m_register );
		}
		code.Print( "" );
	}

	// Inputs to shader ( interpolators )
	if ( GetShaderTarget() == MSH_PixelShader )
	{
		// HACK DX10 POSITION ÜBERHACK
		bool hasVPos = false;
		bool hasVFace = false;

		code.Print( "// Inputs" );
		code.Print( "struct INPUTS {" );
		if( m_inputs.Size() > 0 )
		{
			for ( Uint32 i=0; i<m_inputs.Size(); i++ )
			{
				const InputInfo& info = m_inputs[i];
				if( Red::System::StringSearch( info.m_semantic.AsChar(), "SYS_POSITION" ) != NULL )
				{
					hasVPos = true;
					continue;
				}

				if( Red::System::StringSearch( info.m_semantic.AsChar(), "SYS_FRONT_FACE" ) != NULL )
				{
					hasVFace = true;
					continue;
				}
				code.Print( "  %s in%s : %s;", GetTypeName( info.m_type ).AsChar(), info.m_semantic.AsChar(), info.m_semantic.AsChar() );
			}
			if (hasVPos)
			{
				code.Print( "  %s in%s : %s;", "float4", "SYS_POSITION", "SYS_POSITION" );
			}
			if (hasVFace)
			{
				code.Print( "  %s in%s : %s;", "uint", "SYS_FRONT_FACE", "SYS_FRONT_FACE" );
			}
		}
		else // don't emit empty structs
		{
			code.Print( "  float4 inPosition : SYS_POSITION;" );
		}

		code.Print( "};" );
		code.Print( "" );
	}

	// Outputs
	Bool hasPos = false; // HACK DX10 POSITION ÜBERHACK		
	Bool hasNormal = false; // HACK TESS normal is needed for displacement
	if ( !m_outputs.Empty() )
	{
		code.Print( "// Outputs" );
		code.Print( "struct OUTPUTS {" );
				
		for ( Uint32 i=0; i<m_outputs.Size(); i++ )
		{
			const OutputInfo& info = m_outputs[i];
			if( Red::System::StringSearch( info.m_semantic.AsChar(), "SYS_POSITION" ) != NULL )
			{
				hasPos = true;
				continue;
			}
			if( Red::System::StringSearch( info.m_semantic.AsChar(), "NORMAL" ) != NULL )
			{
				hasNormal = true;
			}
			code.Print( "  %s out%s : %s;", GetTypeName( info.m_type ).AsChar(), info.m_semantic.AsChar(), info.m_semantic.AsChar() );
		}

		if (hasPos)
		{
			code.Print( "  %s out%s : %s;", "float4", "SYS_POSITION", "SYS_POSITION" );
		}

		code.Print( "};" );
		code.Print( "" );
	}

	// Custom included .fx headers
	if ( m_includes.Size() )
	{
		code.Print( "// Custom includes" );
		for (Uint32 i=0; i<m_includes.Size(); ++i)
		{
			code.Print( "#include \"%s\"", m_includes[i].m_path.AsChar() );
		}
		code.Print( "" );
	}

	// Pixel shader extra shit
	if ( GetShaderTarget() == MSH_PixelShader )
	{
		// Include a header depending on the type of vertex factory
		const EMaterialVertexFactory factoryType = m_compiler->GetContext().m_vertexFactory;
		if ( factoryType == MVF_TesselatedTerrain || factoryType == MVF_TerrainSkirt )
		{
			code.Print( "#include \"terrainMaterialDX11.fx\"" );
		}
	}

	// Headers
	const AnsiChar *outputsDefCode = m_outputs.Empty() ? "" : ", out OUTPUTS Output";
	if ( GetShaderTarget() == MSH_PixelShader )
	{
		// PS header
		code.Print( "void ps_main( INPUTS Input %s ) {", outputsDefCode );
	}
	else
	{
		if( m_compiler->GetContext().m_proxyMaterialCast )
		{
			// VS header
			code.Print( "#include \"materialCastStaticMesh.fx\"" );
			code.Print( "void vs_main( uint vId : SV_VertexID %s ) {", outputsDefCode );
			code.Print( "VS_INPUT Input = ExtractVertexStream( vId );" );
			code.Print( "float4 pos4 = float4( Input.Position, 1 );" );

			// Vertex generation
			code.Print( "// Generate vertex from vertex factory" );
			code.Print( "VS_FAT_VERTEX FatVertex = GenerateVertex( Input );");
			code.Print( "FatVertex.ScreenPosition = pos4;" );
			code.Print( "" );
		}
		else
		{
			// VS header
			code.Print( "void vs_main( VS_INPUT Input %s ) {", outputsDefCode );

			// Vertex generation
			code.Print( "// Generate vertex from vertex factory" );
			code.Print( "VS_FAT_VERTEX FatVertex = GenerateVertex( Input );");
			code.Print( "" );
		}
	}
	
	// Function code
	code.Print( "// Auto generated code START" );
	code.Append( m_code.AsChar() );
	code.Print( "// Auto generated code END" );

	// End of function
	code.Print( "}" );
}

#endif
