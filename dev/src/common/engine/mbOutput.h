/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "materialBlock.h"

class CMaterialBlockCompiler;

/// Converts value in gamma space into linear space
class CMaterialBlockOutput : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockOutput, CMaterialBlock, "Deprecated", "Output" );

protected:
	CName			m_socketID;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual EGraphBlockShape GetBlockShape() const { return GBS_ArrowLeft; }

	virtual void OnPostLoad()
	{
		TBaseClass::OnPostLoad();
	}

	virtual void OnRebuildSockets();

	virtual void OnPropertyPostChange( IProperty *property )
	{
	}

#endif
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		return CodeChunk::EMPTY;
	}
#endif
};

BEGIN_CLASS_RTTI( CMaterialBlockOutput )
	PARENT_CLASS( CMaterialBlock )
END_CLASS_RTTI();