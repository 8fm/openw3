/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "materialBlock.h"

/// Converts value in gamma space into linear space
class CMaterialBlockInterpolator : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockInterpolator, CMaterialBlock, "Misc", "Interpolator (VS to PS)" );

protected:
	CName			m_socketID;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnPostLoad()
	{
		TBaseClass::OnPostLoad();
	}

	virtual void OnRebuildSockets();

	virtual void OnPropertyPostChange( IProperty *property )
	{
	}

	virtual Int32 GetShaderTarget()
	{
		return MSH_VertexShader;
	}

#endif
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif
};

BEGIN_CLASS_RTTI( CMaterialBlockInterpolator )
	PARENT_CLASS( CMaterialBlock )
END_CLASS_RTTI();