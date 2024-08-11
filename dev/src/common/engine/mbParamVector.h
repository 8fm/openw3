/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialParameter.h"


/// A block that defines vector parameter
class CMaterialParameterVector : public CMaterialParameter
{
	DECLARE_ENGINE_MATERIAL_PARAMETER( CMaterialParameterVector, CMaterialParameter, "Parameters", "Vector" );

public:
	Vector		m_vector;				//!< Value
	Bool	m_useInVertexShader;

public:
	CMaterialParameterVector();

	virtual IProperty* GetParameterProperty() const;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif
};

BEGIN_CLASS_RTTI( CMaterialParameterVector );
	PARENT_CLASS( CMaterialParameter );
	PROPERTY_EDIT( m_vector, TXT("Vector data") );
	PROPERTY_EDIT( m_useInVertexShader, TXT("Variable used in vertex shader") );
END_CLASS_RTTI();

