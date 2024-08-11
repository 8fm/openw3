/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialRootBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that handles color and alpha output
class CMaterialBlockOutputColorEye : public CMaterialRootBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputColorEye, CMaterialBlock, "Output", "Output Eye" );

public:	
	Bool					m_rawOutput;
	Float					m_maskThreshold;

public:
	CMaterialBlockOutputColorEye();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();
#endif
	virtual Bool IsTwoSided() const;
	virtual Bool IsTwoSideLighted() const;
	virtual ERenderingBlendMode GetBlendMode() const;
	virtual Bool IsEmissive() const;
	virtual Bool IsAccumulativelyRefracted() const;
	virtual Bool CanOverrideMasked() const { return false; }
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const;
	virtual Bool IsDeferred() const { return true; }
	virtual Bool IsForward() const { return true; }
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputColorEye )
PARENT_CLASS( CMaterialRootBlock )	
PROPERTY_EDIT( m_rawOutput, TXT("Use raw output") ); 
PROPERTY_EDIT( m_maskThreshold, TXT("Alpha mask threshold") );
END_CLASS_RTTI()

#endif


