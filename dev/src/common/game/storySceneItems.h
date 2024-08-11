#pragma once

#include "../engine/entityTemplate.h"
#include "../engine/lightComponent.h"
#include "../engine/dimmerComponent.h"

//////////////////////////////////////////////////////////////////////////////////////////////

class IStorySceneItem : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IStorySceneItem, CObject );

	CName							m_id;
};

BEGIN_ABSTRACT_CLASS_RTTI( IStorySceneItem );
	PARENT_CLASS( CObject );	
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////

class CStorySceneActor : public IStorySceneItem
{
	DECLARE_ENGINE_CLASS( CStorySceneActor, IStorySceneItem, 0 );

	TagList							m_actorTags;
	TSoftHandle< CEntityTemplate >	m_entityTemplate;
	TDynArray< CName >				m_appearanceFilter;
	Bool							m_dontSearchByVoicetag;
	String							m_alias;
	Bool							m_useHiresShadows;
	Bool							m_forceSpawn;
	Bool							m_useMimic;

	CStorySceneActor()
		: m_dontSearchByVoicetag( false )
		, m_forceSpawn( false )
		, m_useHiresShadows( true )
		, m_useMimic( true )
	{}
};

BEGIN_CLASS_RTTI( CStorySceneActor )
	PARENT_CLASS( IStorySceneItem )
	PROPERTY_CUSTOM_EDIT( m_id, TXT( "Voicetag of actor" ), TXT( "DialogActorVoiceTagFromEntity" ) )	
	PROPERTY_EDIT( m_actorTags, TXT( "Tags used to choose prefered actor for scene" ) );
	PROPERTY_EDIT( m_entityTemplate, TXT( "Actor template" ) );
	PROPERTY_EDIT( m_appearanceFilter, TXT( "Actor appearances" ) );
	PROPERTY_EDIT( m_dontSearchByVoicetag, TXT( "Should actor be search by voicetag" ) );
	PROPERTY_EDIT( m_useHiresShadows, TXT( "Use hi-res shadows" ) );
	PROPERTY_EDIT( m_forceSpawn, TXT("Always spawn actor") )
	PROPERTY_EDIT( m_useMimic, TXT( "Use hi-res facial animation" ) );
	//PROPERTY_EDIT( m_alias, TXT( "Alias name which can be used to find this actor" ) );
	PROPERTY( m_alias );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////

class CStorySceneProp : public IStorySceneItem
{
	DECLARE_ENGINE_CLASS( CStorySceneProp, IStorySceneItem, 0 );

	TSoftHandle< CEntityTemplate >	m_entityTemplate;
	CName							m_forceBehaviorGraph;
	Bool							m_resetBehaviorGraph;
	Bool							m_useMimics;

	CStorySceneProp()
		: m_resetBehaviorGraph( true )
		, m_useMimics( true )
	{}

	const String& GetTemplatePath()
	{
		return m_entityTemplate.GetPath();
	}
};

BEGIN_CLASS_RTTI( CStorySceneProp )
	PARENT_CLASS( IStorySceneItem )
	PROPERTY_EDIT( m_id, TXT( "ID of Prop" ) )
	PROPERTY_EDIT( m_entityTemplate, TXT( "Entity template" ) );
	PROPERTY_EDIT( m_forceBehaviorGraph, TXT( "Force beh graph" ) );
	PROPERTY_EDIT( m_resetBehaviorGraph, TXT( "Reset beh graph" ) );
	PROPERTY_EDIT( m_useMimics, TXT( "Load mimic" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////

class CStorySceneEffect : public IStorySceneItem
{
	DECLARE_ENGINE_CLASS( CStorySceneEffect, IStorySceneItem, 0 );

	TSoftHandle< CParticleSystem >	m_particleSystem;

	CStorySceneEffect()
	{}
};

BEGIN_CLASS_RTTI( CStorySceneEffect )
	PARENT_CLASS( IStorySceneItem )
	PROPERTY_EDIT( m_id, TXT( "ID of Effect" ) )
	PROPERTY_EDIT( m_particleSystem, TXT( "Particle System" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////

enum ELightType
{
	LT_SpotLight,
	LT_PointLight,
	LT_Dimmer,
};

BEGIN_ENUM_RTTI( ELightType );
	ENUM_OPTION( LT_SpotLight );
	ENUM_OPTION( LT_PointLight );
	ENUM_OPTION( LT_Dimmer );
END_ENUM_RTTI();

class CStorySceneLight : public IStorySceneItem
{
	DECLARE_ENGINE_CLASS( CStorySceneLight, IStorySceneItem, 0 );

	ELightType						m_type;

	Float							m_innerAngle;
	Float							m_outerAngle;
	Float							m_softness;

	ELightShadowCastingMode			m_shadowCastingMode;
	Float							m_shadowFadeDistance;
	Float							m_shadowFadeRange;
	EDimmerType						m_dimmerType;
	Bool							m_dimmerAreaMarker;


	CStorySceneLight()
		: m_type( LT_PointLight )
		, m_innerAngle( 30.0f )
		, m_outerAngle( 45.0f )
		, m_softness( 2.0f )
		, m_shadowCastingMode( LSCM_None )
		, m_shadowFadeRange( 5.0f )
		, m_dimmerAreaMarker( false )
		, m_dimmerType( DIMMERTYPE_Default )
	{
		
	}

	void CopyFrom( const CStorySceneLight& rhs )
	{
		m_type = rhs.m_type;

		m_innerAngle = rhs.m_innerAngle;
		m_outerAngle = rhs.m_outerAngle;
		m_softness = rhs.m_softness;

		m_shadowCastingMode = rhs.m_shadowCastingMode;
		m_shadowFadeDistance = rhs.m_shadowFadeDistance;
		m_shadowFadeRange = rhs.m_shadowFadeRange;
	}
};

BEGIN_CLASS_RTTI( CStorySceneLight )
	PARENT_CLASS( IStorySceneItem )
	PROPERTY_EDIT( m_id, TXT( "ID of Light" ) )
	PROPERTY_EDIT( m_type, TXT( "Light Type" ) )
	PROPERTY_EDIT( m_innerAngle, TXT( "SpotLight: Inner Angle" ) )
	PROPERTY_EDIT( m_outerAngle, TXT( "SpotLight: Outer Angle" ) )
	PROPERTY_EDIT( m_softness, TXT( "SpotLight: Softness" ) )
	PROPERTY_EDIT( m_shadowCastingMode, TXT("Shadow casting mode") );
	PROPERTY_EDIT( m_shadowFadeDistance, TXT("Distance from light at which the shadow should start to fade away (if 0 then disabled)") );
	PROPERTY_EDIT( m_shadowFadeRange, TXT("Shadow fading range") );
	PROPERTY_EDIT( m_dimmerType, TXT("Dimmer type") );
	PROPERTY_EDIT( m_dimmerAreaMarker, TXT("Dimmer area marker") );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////

// TODO: We keep this old struct to load older data. Clean away once all story scenes have been updated to new format
struct StorySceneExpectedActor
{
	DECLARE_RTTI_STRUCT( StorySceneExpectedActor )

	CName							m_voicetag;
	TagList							m_actorTags;
	TSoftHandle< CEntityTemplate >	m_entityTemplate;
	TDynArray< CName >				m_appearanceFilter;
	Bool							m_dontSearchByVoicetag;
	String							m_alias;

	StorySceneExpectedActor()
		: m_dontSearchByVoicetag( false )	
	{}
};

BEGIN_CLASS_RTTI( StorySceneExpectedActor )
	PROPERTY_EDIT( m_voicetag, TXT( "Voicetag of actor" ) )
	PROPERTY_EDIT( m_actorTags, TXT( "Tags used to choose prefered actor for scene" ) );
	PROPERTY_EDIT( m_entityTemplate, TXT( "Actor template" ) );
	PROPERTY_EDIT( m_appearanceFilter, TXT( "Actor appearances" ) );
	PROPERTY_EDIT( m_dontSearchByVoicetag, TXT( "Should actor be search by voicetag" ) );
	PROPERTY( m_alias );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////////////