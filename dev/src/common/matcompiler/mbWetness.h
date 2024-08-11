/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

enum EWetnessOverrideType
{
	EM_Diffuse = 0,
	EM_Specular = 1,
	EM_Roughness = 2
};

BEGIN_ENUM_RTTI( EWetnessOverrideType );
ENUM_OPTION( EM_Diffuse );
ENUM_OPTION( EM_Specular );
ENUM_OPTION( EM_Roughness );
END_ENUM_RTTI();

/// A material block that emits texture coordinates
class CMaterialBlockWetness : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockWetness, CMaterialBlock, "System Samplers", "Wetness" );
public:
	CMaterialBlockWetness();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	

	EWetnessOverrideType		m_overrideType;	
};

BEGIN_CLASS_RTTI( CMaterialBlockWetness )
	PARENT_CLASS( CMaterialBlock )	
	PROPERTY_EDIT(m_overrideType, TXT("Calculate Wetness influence by input type"))
END_CLASS_RTTI()

#endif