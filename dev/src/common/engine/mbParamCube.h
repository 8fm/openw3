/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialParameter.h"


/// A block that defines cubemap parameter
class CMaterialParameterCube : public CMaterialParameter
{
	DECLARE_ENGINE_MATERIAL_PARAMETER( CMaterialParameterCube, CMaterialParameter, "Parameters", "Cube" );

public:
	THandle< CCubeTexture >		m_cube;

public:
	CMaterialParameterCube();	
	virtual IProperty* GetParameterProperty() const;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif
};

BEGIN_CLASS_RTTI( CMaterialParameterCube );
	PARENT_CLASS( CMaterialParameter );
	PROPERTY_EDIT( m_cube, TXT("Cube") );
END_CLASS_RTTI();
