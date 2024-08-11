/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/materialRootBlock.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that handles color and alpha output
class CMaterialBlockOutputColorDeferred : public CMaterialRootBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputColorDeferred, CMaterialBlock, "Output", "Output Deferred" );
	
public:	
	Bool					m_isTwoSided;
	Bool					m_rawOutput;
	Float					m_maskThreshold;
	Bool					m_terrain;
	
public:
	CMaterialBlockOutputColorDeferred();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();
#endif
	virtual Bool IsTwoSided() const;
	virtual ERenderingBlendMode GetBlendMode() const;
	virtual Bool IsEmissive() const;
	virtual Bool IsAccumulativelyRefracted() const;
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const;
	virtual Bool IsDeferred() const { return true; }
	virtual Bool IsForward() const { return false; }
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputColorDeferred )
	PARENT_CLASS( CMaterialRootBlock )	
	PROPERTY_EDIT( m_isTwoSided, TXT("Material is two sided") );
	PROPERTY_EDIT( m_rawOutput, TXT("Use raw output") ); 
	PROPERTY_EDIT( m_maskThreshold, TXT("Alpha mask threshold") );
	PROPERTY_EDIT( m_terrain, TXT("Terrain material") );
END_CLASS_RTTI()

#endif
