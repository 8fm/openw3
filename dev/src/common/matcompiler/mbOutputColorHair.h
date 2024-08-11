/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialRootBlock.h"

/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that handles color and alpha output
class CMaterialBlockOutputColorHair : public CMaterialRootBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputColorHair, CMaterialBlock, "Output", "Output Hair" );

public:	
	Bool					m_isTwoSided;
	Bool					m_rawOutput;
	Float					m_maskThreshold;
	Bool					m_implicitGlobalFogVertexBased;
	Bool					m_shadowingSolidVertexBased;
	Bool					m_shadowingTransparentVertexBased;
	Bool					m_shadowingCascadesVertexBased;
	Bool					m_envProbesSolidVertexBased;
	Bool					m_envProbesTransparentVertexBased;

public:
	CMaterialBlockOutputColorHair();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();
#endif
	virtual Bool IsTwoSided() const;
	virtual ERenderingBlendMode GetBlendMode() const;
	virtual Bool IsEmissive() const;
	virtual Bool IsAccumulativelyRefracted() const;
	virtual Bool CanOverrideMasked() const { return false; }
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const;
	virtual Bool IsDeferred() const { return true; }
	virtual Bool IsForward() const { return true; }
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputColorHair )
	PARENT_CLASS( CMaterialRootBlock )	
	PROPERTY_EDIT( m_isTwoSided, TXT("Material is two sided") );
	PROPERTY_EDIT( m_rawOutput, TXT("Use raw output") ); 
	PROPERTY_EDIT( m_maskThreshold, TXT("Alpha mask threshold") );
	PROPERTY_EDIT( m_implicitGlobalFogVertexBased, TXT("Use vertex shader for fog calculation") );	
	PROPERTY_EDIT( m_shadowingSolidVertexBased, TXT("Use vertex shader for solid shadowing") );	
	PROPERTY_EDIT( m_shadowingTransparentVertexBased, TXT("Use vertex shader for transparency shadowing") );	
	PROPERTY_EDIT( m_shadowingCascadesVertexBased, TXT("Include cascades shadow in vertex based data") );	
	PROPERTY_EDIT( m_envProbesSolidVertexBased, TXT("Use vertex shader for solid envprobes - allowed only with vertex based shadowing") );
	PROPERTY_EDIT( m_envProbesTransparentVertexBased, TXT("Use vertex shader for transparent envprobes - allowed only with vertex based shadowing") );
END_CLASS_RTTI()

#endif


