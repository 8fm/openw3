/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialParameter.h"

/// A block that defines color parameter
class CMaterialParameterColor : public CMaterialParameter
{
	DECLARE_ENGINE_MATERIAL_PARAMETER( CMaterialParameterColor, CMaterialParameter, "Parameters", "Color" );

public:
	Color		m_color;		//!< Raw color value

public:
	CMaterialParameterColor();
	virtual IProperty* GetParameterProperty() const;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif
};

BEGIN_CLASS_RTTI( CMaterialParameterColor );
PARENT_CLASS( CMaterialParameter );
PROPERTY_EDIT( m_color, TXT("Default color") );
END_CLASS_RTTI();
