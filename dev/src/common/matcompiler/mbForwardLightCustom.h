/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"
#include "../engine/lightComponent.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

class CMaterialBlockForwardLightCustom : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockForwardLightCustom, CMaterialBlock, "System Samplers", "Forward Light Custom" );

public:
	Uint32						m_lightUsageMask;			//!< Light flags determining in which situations light is used
	bool						m_globalLightDiffuse;
	bool						m_globalLightSpecular;
	bool						m_deferredDiffuse;
	bool						m_deferredSpecular;
	bool						m_envProbes;
	bool						m_ambientOcclusion;
	bool						m_excludeFlags;
	bool						m_fog;

public:
	CMaterialBlockForwardLightCustom();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual void OnPropertyPostChange( IProperty *property );
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
};

BEGIN_CLASS_RTTI( CMaterialBlockForwardLightCustom )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_globalLightDiffuse, TXT("Set true to add effect of Global lighting on diffuse shading") );
	PROPERTY_EDIT( m_globalLightSpecular, TXT("Set true to add effect of Global lighting on specular shading") );
	PROPERTY_EDIT( m_deferredDiffuse, TXT("Set true to add effect of point and spotlights on diffuse shading") );
	PROPERTY_EDIT( m_deferredSpecular, TXT("Set true to add effect of point and spotlights on specular shading") );
	PROPERTY_EDIT( m_envProbes, TXT("Set true to add effect of envProbes on shading") );
	PROPERTY_EDIT( m_ambientOcclusion, TXT("Set true to add effect of AO on shading") );
	PROPERTY_EDIT( m_fog, TXT("Set true to add effect of fog on shading") );
	PROPERTY_EDIT( m_excludeFlags, TXT("set to true to exclude flags. If in include mode light is used in calculations if any of its flag matches. In exclude mode if any of the flag matches light is not included") );
	PROPERTY_BITFIELD_EDIT( m_lightUsageMask, ELightUsageMask, TXT("Flags to use light in special situations") );
END_CLASS_RTTI()

#endif