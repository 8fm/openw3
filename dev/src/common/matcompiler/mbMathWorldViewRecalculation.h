/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockWorldViewRecalculation : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockWorldViewRecalculation, CMaterialBlock, "Image Processing", "World/View" );

public:
	bool	m_worldToView;

public:
	CMaterialBlockWorldViewRecalculation();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	virtual String GetCaption() const;
};

BEGIN_CLASS_RTTI( CMaterialBlockWorldViewRecalculation )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_worldToView, TXT("Set true to recalculate World to View basis") );
END_CLASS_RTTI()

#endif