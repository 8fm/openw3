/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialRootBlock.h"
#include "../engine/renderFrame.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material output block that controls tessellation
class CMaterialBlockOutputVertexModifiers : public CMaterialRootBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutputVertexModifiers, CMaterialBlock, "Output", "Vertex Modifiers" );

public:
	CMaterialBlockOutputVertexModifiers();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual Bool IsTwoSided() const {return false;}
	virtual Bool IsTwoSideLighted() const {return false;}
	virtual ERenderingBlendMode GetBlendMode() const {return RBM_None;}
	virtual Bool IsEmissive() const {return false;}
	virtual Bool IsAccumulativelyRefracted() const {return false;}
	virtual void Compile( CMaterialBlockCompiler& compiler ) const;
	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const {}
	virtual Bool IsDeferred() const { return false; }
	virtual Bool IsForward() const { return true; }

	virtual Int32 GetShaderTarget() const
	{
		return MSH_VertexShader;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockOutputVertexModifiers )
	PARENT_CLASS( CMaterialRootBlock )
END_CLASS_RTTI()

#endif
