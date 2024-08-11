/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockMathSplitAppendVector : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathSplitAppendVector, CMaterialBlock, "Math", "Split/Append Vector" );

public:
	bool	m_splitVector;

public:
	CMaterialBlockMathSplitAppendVector();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
	virtual void OnPropertyPostChange( IProperty *property );
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	
	
};

BEGIN_CLASS_RTTI( CMaterialBlockMathSplitAppendVector )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_splitVector, TXT("Set true to split the vector into components") );
END_CLASS_RTTI()

#endif