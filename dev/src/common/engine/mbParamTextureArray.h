/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialParameter.h"


/// A block that defines texture parameter
class CMaterialParameterTextureArray : public CMaterialParameter
{
	DECLARE_ENGINE_MATERIAL_PARAMETER( CMaterialParameterTextureArray, CMaterialParameter, "Parameters", "Texture Array" );

public:
	THandle< CTextureArray >	m_texture;			//!< A texture to sample

public:
	CMaterialParameterTextureArray();

	virtual IProperty* GetParameterProperty() const;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif
};

BEGIN_CLASS_RTTI( CMaterialParameterTextureArray )
	PARENT_CLASS( CMaterialParameter )
	PROPERTY_EDIT( m_texture, TXT("Texture array") );
END_CLASS_RTTI();
