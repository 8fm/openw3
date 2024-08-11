/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"

enum EGradientTypes
{
	GT_Linear,
	GT_Radial,
	GT_Spherical,
	GT_Angle,
	GT_Diamond
};

BEGIN_ENUM_RTTI( EGradientTypes );
ENUM_OPTION( GT_Linear );
ENUM_OPTION( GT_Radial );
ENUM_OPTION( GT_Spherical );
ENUM_OPTION( GT_Angle );
ENUM_OPTION( GT_Diamond );
END_ENUM_RTTI();

enum EGradientExtrapolationModes
{
	GEM_Clamp,
	GEM_Repeat,
	GEM_Mirror
};

BEGIN_ENUM_RTTI( EGradientExtrapolationModes );
ENUM_OPTION( GEM_Clamp );
ENUM_OPTION( GEM_Repeat );
ENUM_OPTION( GEM_Mirror );
END_ENUM_RTTI();

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that generates gradient texture
class CMaterialBlockGradient : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockGradient, CMaterialBlock, "System Samplers", "Gradient" );

public:
	EGradientTypes					m_gradientType;
	EGradientExtrapolationModes		m_gradientExtrapolationMode;
	Bool							m_reverse;
	Bool							m_loop;
	Float							m_offset;
	SSimpleCurve					m_gradient;

public:
	CMaterialBlockGradient();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	void OnPropertyPostChange( IProperty* property );
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	virtual String GetCaption() const;
};

BEGIN_CLASS_RTTI( CMaterialBlockGradient )
	PARENT_CLASS( CMaterialBlock )
	PROPERTY_EDIT( m_gradientType, TXT( "Choose the gradient type" ) );
	PROPERTY_EDIT( m_reverse, TXT( "Reverse the order of colors" ) );
	PROPERTY_EDIT( m_loop, TXT( "Blends between last and first color" ) );
	PROPERTY_EDIT( m_offset, TXT( "Offset to tweak color positions" ) );
	PROPERTY_EDIT( m_gradientExtrapolationMode, TXT( "Choose the gradient extrapolation mode" ) );
	PROPERTY_EDIT( m_gradient, TXT("Gradient colors") );
END_CLASS_RTTI()

#endif