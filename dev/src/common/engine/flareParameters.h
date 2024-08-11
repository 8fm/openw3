/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

enum EEnvFlareColorGroup : CEnum::TValueType;

/// Lens flare group
enum ELensFlareGroup
{
	LFG_Default,
	LFG_Sun,
	LFG_Moon,
	LFG_Custom0,
	LFG_Custom1,
	LFG_Custom2,
	LFG_Custom3,
	LFG_Custom4,
	LFG_Custom5,

	LFG_MAX,
};

BEGIN_ENUM_RTTI( ELensFlareGroup );
ENUM_OPTION( LFG_Default );
ENUM_OPTION( LFG_Sun );
ENUM_OPTION( LFG_Moon );
ENUM_OPTION( LFG_Custom0 )
ENUM_OPTION( LFG_Custom1 )
ENUM_OPTION( LFG_Custom2 )
ENUM_OPTION( LFG_Custom3 )
ENUM_OPTION( LFG_Custom4 )
ENUM_OPTION( LFG_Custom5 )
END_ENUM_RTTI();


/// Flare category. This is also a reversed order of rendering (last category gets rendered first)
enum EFlareCategory
{
	FLARECAT_Default,
	FLARECAT_Sun,
	FLARECAT_Moon,

	FLARECAT_MAX,
};

BEGIN_ENUM_RTTI( EFlareCategory );
ENUM_OPTION( FLARECAT_Default );
ENUM_OPTION( FLARECAT_Sun );
ENUM_OPTION( FLARECAT_Moon );
END_ENUM_RTTI();


/// Flare parameters
struct SFlareParameters
{
	DECLARE_RTTI_STRUCT( SFlareParameters );

public:

	SFlareParameters ();
	SFlareParameters( const SFlareParameters& );

	void Reset();

	EFlareCategory		m_category;
	EEnvFlareColorGroup	m_colorGroup;
	ELensFlareGroup		m_lensFlareGroup;
	Float			m_occlusionExtent;
	Float			m_flareRadius;
	Float			m_fadeInMaxSpeed;
	Float			m_fadeOutMaxSpeed;
	Float			m_fadeInAccel;
	Float			m_fadeOutAccel;
	Float			m_visibilityFullDist;
	Float			m_visibilityFadeRange;
};

BEGIN_CLASS_RTTI( SFlareParameters );
	PROPERTY_EDIT( m_category,			TXT("Category") );
	PROPERTY_EDIT( m_colorGroup,		TXT("Flare color group") );
	PROPERTY_EDIT( m_lensFlareGroup,	TXT("Lens flare group") );
	PROPERTY_EDIT( m_occlusionExtent,	TXT("Occlusion tests extent") );
	PROPERTY_EDIT( m_flareRadius,		TXT("Flare radius") );
	PROPERTY_EDIT( m_fadeInMaxSpeed,	TXT("FadeIn max speed") );
	PROPERTY_EDIT( m_fadeOutMaxSpeed,	TXT("FadeOut max speed") );	
	PROPERTY_EDIT( m_fadeInAccel,		TXT("FadeIn speed acceleration") );
	PROPERTY_EDIT( m_fadeOutAccel,		TXT("FadeOut speed acceleration") );
	PROPERTY_EDIT( m_visibilityFullDist,	TXT("Visibility full dist") );
	PROPERTY_EDIT( m_visibilityFadeRange,	TXT("Visibility fade range") );
END_CLASS_RTTI();


/// Lens flare element parameters
struct SLensFlareElementParameters
{
	DECLARE_RTTI_STRUCT( SLensFlareElementParameters );

public:

	SLensFlareElementParameters();
	~SLensFlareElementParameters();

	THandle< CMaterialInstance >	m_material;
	Bool							m_isConstRadius;
	Bool							m_isAligned;
	Float							m_centerFadeStart;
	Float							m_centerFadeRange;
	Uint32							m_colorGroupParamsIndex;
	Float							m_alpha;
	Float							m_size;
	Float							m_aspect;
	Float							m_shift;
	Float							m_pivot;
	Color							m_color;
};

BEGIN_CLASS_RTTI( SLensFlareElementParameters );
	PROPERTY_INLINED( m_material,		TXT("Material") );
	PROPERTY_EDIT( m_isConstRadius,		TXT("Is constant radius") );
	PROPERTY_EDIT( m_isAligned,			TXT("Is aligned") );
	PROPERTY_EDIT( m_centerFadeStart,	TXT("Center fade start") );
	PROPERTY_EDIT( m_centerFadeRange,	TXT("Center fade range") );
	PROPERTY_EDIT( m_colorGroupParamsIndex,	TXT("Color group parameters index") );
	PROPERTY_EDIT( m_alpha,				TXT("Alpha") );
	PROPERTY_EDIT( m_size,				TXT("Size") );
	PROPERTY_EDIT( m_aspect,			TXT("Aspect") );
	PROPERTY_EDIT( m_shift,				TXT("Shift") );
	PROPERTY_EDIT( m_pivot,				TXT("Pivot") );
	PROPERTY_EDIT( m_color,				TXT("Color") );
END_CLASS_RTTI();


/// Lens flare parameters
struct SLensFlareParameters
{
	DECLARE_RTTI_STRUCT( SLensFlareParameters );

public:

	SLensFlareParameters();
	~SLensFlareParameters();

	Float m_nearDistance;
	Float m_nearRange;
	Float m_farDistance;
	Float m_farRange;
	TDynArray<SLensFlareElementParameters>	m_elements;
};

BEGIN_CLASS_RTTI( SLensFlareParameters );
PROPERTY_EDIT( m_nearDistance,	TXT("nearDistance") );
PROPERTY_EDIT( m_nearRange,		TXT("nearRange") );
PROPERTY_EDIT( m_farDistance,	TXT("farDistance") );
PROPERTY_EDIT( m_farRange,		TXT("farRange") );
PROPERTY_EDIT( m_elements,		TXT("Elements") );
END_CLASS_RTTI();