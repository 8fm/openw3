/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialParameter.h"


/// A block that defines texture parameter
class CMaterialParameterTexture : public CMaterialParameter
{
	DECLARE_ENGINE_MATERIAL_PARAMETER( CMaterialParameterTexture, CMaterialParameter, "Parameters", "Texture" );

public:
	THandle< ITexture >		m_texture;			//!< A texture to sample
	Bool					m_isAtlas;			//!< This is an atlas

public:
	CMaterialParameterTexture();

	virtual IProperty* GetParameterProperty() const;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif
};

BEGIN_CLASS_RTTI( CMaterialParameterTexture )
	PARENT_CLASS( CMaterialParameter )
	PROPERTY_EDIT( m_texture, TXT("Texture") );
	PROPERTY_EDIT( m_isAtlas, TXT("Atlas") );
END_CLASS_RTTI();
