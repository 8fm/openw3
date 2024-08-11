/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/materialCompiler.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

class CHLSLMaterialCompiler;

/// DX9 Material shader compiler interface
class CHLSLMaterialShaderCompiler : public IMaterialShaderCompiler
{
public:
	/// Parameter info
	struct ParamInfo
	{
		CodeChunk			m_name;					// Parameter name
		EMaterialDataType	m_type;					// Parameter type
		Uint32				m_register;				// Allocated register
		Uint32				m_numRegisters;			// Number of allocated registers
		Uint32				m_arrayCount;			// Array count
	};

	/// Sampler info
	struct TextureInfo
	{
		CodeChunk				m_name;					// Sampler name
		EMaterialSamplerType	m_type;					// Sampler type
		Uint32					m_register;				// Allocated register
	};

	/// Input
	struct InputInfo
	{
		EMaterialDataType	m_type;					// Type
		CodeChunk			m_semantic;				// Input stream name
		CodeChunk			m_code;					// Access code chunk
	};

	/// Output
	struct OutputInfo
	{
		EMaterialDataType	m_type;					// Type
		CodeChunk			m_semantic;				// Input stream name
		CodeChunk			m_code;					// Access code chunk
	};

	/// Cached data
	struct DataInfo
	{
		CodeChunk			m_semantic;				// Data semantic name
		CodeChunk			m_code;					// Output code chunk
	};

	/// Include .fx
	struct IncludeInfo
	{
		CodeChunk			m_path;					// .fx file path
	};

	struct TessellationParamInfo
	{
		CodeChunk			m_triangleSize;
		CodeChunk			m_displacementMap;
		CodeChunk			m_displacementScale;
		ETessellationDomain m_domain;
		Bool				m_vertexFactoryOverride;
	};

public:
	CHLSLMaterialCompiler*									m_compiler;					// Top level compiler
	CStringPrinter											m_code;						// Generated function code
	Uint32													m_freeConstReg;				// Parameters allocation register
	Uint32													m_freeBoolConstReg;			// Parameters of type bool and int
	Uint32													m_freeSamplerReg;			// Sampler allocation register
	Uint32													m_freeSamplerStateReg;		// Sampler state allocation register
	
	TDynArray< ParamInfo >									m_params;					// Parameters
	TDynArray< TextureInfo >								m_textures;					// Textures
	TDynArray< SamplerStateInfo, MC_MaterialSamplerStates >	m_samplerStates;			// Sampler states
	TDynArray< ParamInfo >									m_boolParams;				// bool params

	TDynArray< InputInfo >									m_inputs;					// Inputs
	TDynArray< OutputInfo >									m_outputs;					// Outputs
	TDynArray< DataInfo >									m_data;						// Data

	TessellationParamInfo									m_tessParams;

	TDynArray< CodeChunk >									m_macros;					// Macros
	TDynArray< IncludeInfo >								m_includes;					// Included .fx code

public:
	// Constructor
	CHLSLMaterialShaderCompiler( CHLSLMaterialCompiler*	compiler, Uint32 firstFreeConstReg, Uint32 firstFreeSamplerReg, Uint32 firstFreeSamplerStateReg, Uint32 firstFreeIntConstReg );

	// Allocate shader local variable
	virtual CodeChunk Var( EMaterialDataType type, const CodeChunk& value );

	// Allocate shader local variable
	virtual CodeChunk Var( EMaterialDataType type, Uint32 arrayCount, const CodeChunk& value );

	// Allocate shader predefined const
	virtual CodeChunk ConstReg( EMaterialDataType type, const AnsiChar* name );

	// Allocate shader sampler
	virtual CodeChunk ConstSampler( EMaterialSamplerType type, const AnsiChar* name, Int32 constantRegister );

	// Allocate shader external dynamic parameter
	virtual CodeChunk Param( EMaterialDataType type, Int32* allocatedRegister, const CodeChunk& value = CodeChunk::EMPTY, Int32 arrayCount = 0 );

	// Allocate shader sampler
	virtual CodeChunk Texture( EMaterialSamplerType type, Int32* allocatedRegister, const CodeChunk& value = CodeChunk::EMPTY );

	// Allocate sampler state
	virtual CodeChunk SamplerState(	ETextureAddressing addressU,
								ETextureAddressing addressV,
								ETextureAddressing addressW,
								ETextureFilteringMin filteringMin,
								ETextureFilteringMag filteringMag,
								ETextureFilteringMip filteringMip,
								ETextureComparisonFunction comparisonFunction );

	// Allocate shader external bool parameter
	virtual CodeChunk BoolParam( EMaterialDataType type, Int32 *allocatedRegister, const CodeChunk& value = CodeChunk::EMPTY );

	// Macrodefinition
	virtual CodeChunk Macro( const CodeChunk& macro );

	// Include a .fx file
	virtual CodeChunk Include( const CodeChunk& path );

	// Allocate shader input
	virtual CodeChunk Input( EMaterialDataType type, const CodeChunk& semantic );

	// Declare shader output
	virtual CodeChunk Output( EMaterialDataType type, const CodeChunk& semantic, const CodeChunk& value );

	// Request shader internal data
	virtual CodeChunk Data( const CodeChunk& semantic );

	virtual void Tessellate( const CodeChunk& triangleSize, const CodeChunk& displacementMap, const CodeChunk& displacementScale, ETessellationDomain tessellationDomain, Bool vertexFactoryOverride );

	// Print statement
	virtual void Statement( const CodeChunk& value, Bool prepend = false );

	// Applies gamma into given value
	virtual CodeChunk ApplyGammaToLinearExponent( EMaterialDataType dataType, const CodeChunk &code, bool inverse, bool allowClampZero );
	
public:
	// Generate data
	virtual Bool GenerateData( const CodeChunk& semantic, CodeChunk& code )=0;

public:
	// Resolve type name
	CodeChunk GetTypeName( EMaterialDataType type ) const;

	// Resolve type size ( in floats )
	Int32 GetTypeFloatCount( EMaterialDataType type ) const;

	// Resolve type size ( in registers )
	Int32 GetTypeRegisterCount( EMaterialDataType type ) const;

	// Resolve texture type name
	CodeChunk GetTextureTypeName( EMaterialSamplerType type ) const;

	// Resolve sampler state type name
	CodeChunk GetSamplerStateName( SamplerStateInfo info ) const;

	// Get final shader code
	void GetFinalCode( CStringPrinter& code ) const;
};

#endif
