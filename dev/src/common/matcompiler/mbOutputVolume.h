/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialRootBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that handles color and alpha output
class CMaterialBlockOutputVolume : public CMaterialRootBlock
{	
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputVolume, CMaterialRootBlock, "Output", "Output Volume" );

protected:
	CMaterialBlock* FindDebugMipMapsSampler( const CName& parameterName ) const;

public:
	Bool					m_isTwoSided;
	Bool					m_noDepthWrite;
	Bool					m_inputColorLinear;
	Bool					m_checkRefractionDepth;
	Float					m_maskThreshold;
	Bool					m_isWaterBlended;
	ERenderingBlendMode		m_blendMode;

public:
	CMaterialBlockOutputVolume();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();
#endif	

	virtual Bool IsTwoSided() const;
	virtual Bool IsTwoSideLighted() const;
	virtual ERenderingBlendMode GetBlendMode() const;
	virtual Bool IsEmissive() const;
	virtual Bool IsAccumulativelyRefracted() const;	
	
	Bool IsWaterBlended() const { return m_isWaterBlended; }
	
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const;
	virtual Bool IsDeferred() const { return true; }
	virtual Bool IsForward() const { return true; }
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputVolume )
	PARENT_CLASS(CMaterialRootBlock)	
	PROPERTY_EDIT( m_isWaterBlended, TXT("m_isWaterBlended") );
END_CLASS_RTTI()

#endif