/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/materialRootBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that handles color and alpha output
class CMaterialBlockOutputColor : public CMaterialRootBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputColor, CMaterialRootBlock, "Output", "Output Color" );

protected:
	CMaterialBlock* FindDebugMipMapsSampler( const CName& parameterName ) const;

public:
	Bool					m_isTwoSided;
	Bool					m_noDepthWrite;
	Bool					m_inputColorLinear;
	Bool					m_checkRefractionDepth;
	Bool					m_implicitTransparencyColor;
	Bool					m_implicitTransparencyAlpha;
	Bool					m_implicitGlobalFogVertexBased;
	Bool					m_implicitGlobalFog;
	Float					m_maskThreshold;
	ERenderingBlendMode		m_blendMode;

public:
	CMaterialBlockOutputColor();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();
#endif
	virtual Bool IsTwoSided() const;
	virtual Bool IsTwoSideLighted() const;
	virtual Bool HasBlending() const;
	virtual ERenderingBlendMode GetBlendMode() const;
	virtual Bool IsEmissive() const;
	virtual Bool IsAccumulativelyRefracted() const;
	virtual Bool IsReflectiveMasked() const;
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const;
	virtual Bool IsDeferred() const { return false; }
	virtual Bool IsForward() const { return true; }
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputColor )
	PARENT_CLASS(CMaterialRootBlock)
	PROPERTY_EDIT( m_isTwoSided, TXT("Material is two sided") );
	PROPERTY_EDIT( m_noDepthWrite, TXT("Force depth write disable") );
	PROPERTY_EDIT( m_inputColorLinear, TXT("Treat input color as encoded in linear space") );
	PROPERTY_EDIT( m_maskThreshold, TXT("Alpha mask threshold") );
	PROPERTY_EDIT( m_blendMode, TXT("Blend mode") );
	PROPERTY_EDIT( m_checkRefractionDepth, TXT("Check refraction depth") );
	PROPERTY_EDIT( m_implicitTransparencyColor, TXT("Implicit transparency color") );
	PROPERTY_EDIT( m_implicitTransparencyAlpha, TXT("Implicit transparency alpha") );
	PROPERTY_EDIT( m_implicitGlobalFogVertexBased, TXT("Implicit global fog per vertex") );
	PROPERTY_EDIT( m_implicitGlobalFog, TXT("Implicit global fog") );
END_CLASS_RTTI()

#endif