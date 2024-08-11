/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// Type
typedef TDynArray< TPair< StringAnsi, StringAnsi > > TMaterialCompilerDefinesList;

class RenderingContext;
class MaterialRenderingContext;
enum ECookingPlatform : Int32;

/// Custom shader defines
class CMaterialCompilerDefines
{
protected:
	TMaterialCompilerDefinesList	m_defines;		//! Defines

public:
	//! Get list of defines
	RED_INLINE const TMaterialCompilerDefinesList& GetDefines() const { return m_defines; }

public:
	CMaterialCompilerDefines();

	//! Single define constructor
	CMaterialCompilerDefines( const StringAnsi& defineName, const StringAnsi& value );

	//! Add define
	CMaterialCompilerDefines& Add( const StringAnsi& defineName, const StringAnsi& value );

	//! Add defines
	CMaterialCompilerDefines& Add( const CMaterialCompilerDefines& other );

	//! Add define, numerical
	CMaterialCompilerDefines& AddNumerical( const StringAnsi& defineName, Int32 value );

	//! Add platform define
	CMaterialCompilerDefines& AddPlatformFlag( ECookingPlatform platform );

	//! Clear definition list
	void Clear();

	//! Remove define
	void RemoveDefine( const StringAnsi& defineName );

	//! Initialize from rendering states
	void InitFromRenderingStates( const RenderingContext& context );

	//! Initialize from material states
	void InitFromMaterialStates( const MaterialRenderingContext& context );

public:
	//! Empty material definitions
	static const CMaterialCompilerDefines& EMPTY();
};