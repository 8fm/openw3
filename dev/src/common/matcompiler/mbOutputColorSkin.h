/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialRootBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that handles color and alpha output
class CMaterialBlockOutputColorSkin : public CMaterialRootBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputColorSkin, CMaterialBlock, "Output", "Output Skin" );

public:	
	Float					m_maskThreshold;
	Bool					m_isMimicMaterial;

public:
	CMaterialBlockOutputColorSkin();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();
#endif
	virtual Bool IsTwoSided() const;
	virtual Bool IsTwoSideLighted() const;
	virtual Bool IsAccumulativelyRefracted() const;
	virtual ERenderingBlendMode GetBlendMode() const;	
	virtual Bool IsEmissive() const;
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const;
	virtual Bool IsDeferred() const { return true; }
	virtual Bool IsForward() const { return true; }
	virtual Bool IsMimicMaterial() const { return m_isMimicMaterial; }
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputColorSkin )
PARENT_CLASS( CMaterialRootBlock )	
PROPERTY_EDIT( m_maskThreshold, TXT("Alpha mask threshold") );
PROPERTY_EDIT( m_isMimicMaterial,	TXT("Is mimic material") );
END_CLASS_RTTI()

#endif
