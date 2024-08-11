/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialBlock.h"

/// A block that defines vector parameter
class CMaterialBlockSampler : public CMaterialBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMaterialBlockSampler, CMaterialBlock );

public:
	ETextureAddressing				m_addressU;
	ETextureAddressing				m_addressV;
	ETextureAddressing				m_addressW;
	ETextureFilteringMin			m_filterMin;
	ETextureFilteringMag			m_filterMag;
	ETextureFilteringMip			m_filterMip;
	ETextureComparisonFunction		m_comparisonFunction;

public:
	CMaterialBlockSampler();
	virtual ~CMaterialBlockSampler() {}

public:

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

	// Compile block, non root blocks should return value
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;

	CodeChunk BindSamplerState( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget ) const;

#endif // NO_RUNTIME_MATERIAL_COMPILATION

};

BEGIN_ABSTRACT_CLASS_RTTI( CMaterialBlockSampler );
	PARENT_CLASS( CMaterialBlock );
	PROPERTY_EDIT( m_addressU, TXT("AddressU") );
	PROPERTY_EDIT( m_addressV, TXT("AddressV") );
	PROPERTY_EDIT( m_addressW, TXT("AddressW") );
	PROPERTY_EDIT( m_filterMin, TXT("FilterMin") );
	PROPERTY_EDIT( m_filterMag, TXT("FilterMag") );
	PROPERTY_EDIT( m_filterMip, TXT("FilterMip") );
	PROPERTY_EDIT( m_comparisonFunction, TXT("Comparison function") );
END_CLASS_RTTI();

