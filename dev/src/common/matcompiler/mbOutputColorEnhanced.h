/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialRootBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that handles color and alpha output
class CMaterialBlockOutputColorEnhanced : public CMaterialRootBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputColorEnhanced, CMaterialBlock, "Deprecated", "Output Enhanced" );

public:	
	Float					m_maskThreshold;
	Float					m_alphaToCoverageScale;

public:
	CMaterialBlockOutputColorEnhanced();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();
#endif
	virtual Bool IsTwoSided() const;
	virtual Bool IsTwoSideLighted() const;
	virtual ERenderingBlendMode GetBlendMode() const;
	virtual Bool IsEmissive() const;
	virtual Bool IsAccumulativelyRefracted() const;
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const;
	virtual Bool IsDeferred() const { return false; }
	virtual Bool IsForward() const { return true; }
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputColorEnhanced )
	PARENT_CLASS( CMaterialRootBlock )	
	PROPERTY_EDIT( m_maskThreshold, TXT("Alpha mask threshold") );
	PROPERTY_EDIT( m_alphaToCoverageScale, TXT("Alpha To Coverage scale") );
END_CLASS_RTTI()

#endif


