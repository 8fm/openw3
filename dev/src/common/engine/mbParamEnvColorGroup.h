/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialBlock.h"
#include "materialCompiler.h"
#include "environmentAreaParams.h"


class CMaterialParameterEnvColorGroup : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialParameterEnvColorGroup, CMaterialBlock, "Input", "Env Color Group" );

public:
	EEnvColorGroup		m_colorGroup;

public:
	CMaterialParameterEnvColorGroup();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Property was changed
	virtual void OnPropertyPostChange( IProperty *property );
	virtual String GetCaption() const;

#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	//! Compile code
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;	
#endif
};

BEGIN_CLASS_RTTI( CMaterialParameterEnvColorGroup );
	PARENT_CLASS( CMaterialBlock );
	PROPERTY_EDIT( m_colorGroup, TXT("Color group") );
END_CLASS_RTTI();


