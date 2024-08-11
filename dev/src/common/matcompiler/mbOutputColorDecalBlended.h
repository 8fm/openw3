/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialRootDecalBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that handles blended decals
class CMaterialBlockOutputColorDecalBlended : public CMaterialRootDecalBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputColorDecalBlended, CMaterialRootDecalBlock, "Deprecated", "Output Decal Blended" );
	
public:	
	Float					m_maskThreshold;
	Bool					m_isMimicMaterial;
	
public:
	CMaterialBlockOutputColorDecalBlended();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();
#endif
	virtual Bool IsTwoSided() const;
	virtual Bool IsTwoSideLighted() const;
	virtual ERenderingBlendMode GetBlendMode() const;	
	virtual Bool IsEmissive() const;
	virtual Bool IsMimicMaterial() const { return m_isMimicMaterial; }
	virtual Bool CanOverrideMasked() const { return false; }
	virtual ERenderingSortGroup GetSortGroup() const;
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const;
	virtual Bool IsDeferred() const { return true; }
	virtual Bool IsForward() const { return false; }
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputColorDecalBlended )
	PARENT_CLASS( CMaterialRootDecalBlock )	
	PROPERTY_EDIT( m_maskThreshold,		TXT("Alpha mask threshold") );
	PROPERTY_EDIT( m_isMimicMaterial,	TXT("Is mimic material") );
END_CLASS_RTTI()

#endif