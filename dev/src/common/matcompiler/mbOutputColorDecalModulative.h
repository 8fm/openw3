/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialRootDecalBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that handles modulative decals
class CMaterialBlockOutputColorDecalModulative : public CMaterialRootDecalBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputColorDecalModulative, CMaterialRootDecalBlock, "Output", "Output Decal Modulative" );
	
public:	
	Float					m_maskThreshold;
	
public:
	CMaterialBlockOutputColorDecalModulative();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();
#endif
	virtual Bool IsTwoSided() const;
	virtual Bool IsTwoSideLighted() const;
	virtual ERenderingBlendMode GetBlendMode() const;
	virtual Bool IsEmissive() const;
	virtual Bool CanOverrideMasked() const { return false; }
	virtual ERenderingSortGroup GetSortGroup() const;
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const;
	virtual Bool IsDeferred() const { return false; }
	virtual Bool IsForward() const { return true; } 
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputColorDecalModulative )
	PARENT_CLASS( CMaterialRootDecalBlock )	
	PROPERTY_EDIT( m_maskThreshold, TXT("Alpha mask threshold") );
END_CLASS_RTTI()

#endif