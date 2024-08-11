
#pragma once

#include "storySceneEvent.h"
#include "..\engine\lightComponent.h"
#include "..\engine\dimmerComponent.h"

class CStorySceneLight;

//////////////////////////////////////////////////////////////////////////

struct SStorySceneSpotLightProperties
{
	DECLARE_RTTI_STRUCT( SStorySceneSpotLightProperties );	

	Float						m_innerAngle;				// Inner cone angle
	Float						m_outerAngle;				// Outer cone angle
	Float						m_softness;					// Cone softness

	SStorySceneSpotLightProperties()
		: m_innerAngle( -1.0f )
		, m_outerAngle( -1.0f )
		, m_softness( -1.0f )
	{}
};

BEGIN_CLASS_RTTI( SStorySceneSpotLightProperties )			
	PROPERTY_EDIT_RANGE( m_innerAngle, TXT( "" ), 1.0f, 170.0f )
	PROPERTY_EDIT_RANGE( m_outerAngle, TXT( "" ), 1.0f, 170.0f )
	PROPERTY_EDIT_RANGE( m_softness, TXT( "" ), 0.0f, 20.0f )
END_CLASS_RTTI();



struct SStorySceneLightDimmerProperties
{
	DECLARE_RTTI_STRUCT( SStorySceneLightDimmerProperties );	

	SSimpleCurve				m_ambientLevel;
	SSimpleCurve				m_marginFactor;

	SStorySceneLightDimmerProperties();

	Float GetAmbientLevel( CWorld* onWorld ) const;
	Float GetMarginFactor( CWorld* onWorld ) const;
};

BEGIN_CLASS_RTTI( SStorySceneLightDimmerProperties )		
	PROPERTY_EDIT( m_ambientLevel, TXT( "" ) )
	PROPERTY_EDIT( m_marginFactor, TXT( "" ) )
END_CLASS_RTTI();


namespace StoryScene { template < typename T > class InterpolationTraits; }

//////////////////////////////////////////////////////////////////////////


enum ESceneEventLightColorSource
{	
	ELCS_EnvLightColor1 = 0,
	ELCS_EnvLightColor2 = 1,
	ELCS_EnvLightColor3 = 2,
	ELCS_CustomLightColor = 3,
};

BEGIN_ENUM_RTTI( ESceneEventLightColorSource )
	ENUM_OPTION( ELCS_CustomLightColor )
	ENUM_OPTION( ELCS_EnvLightColor1 )
	ENUM_OPTION( ELCS_EnvLightColor2 )
	ENUM_OPTION( ELCS_EnvLightColor3 )
END_ENUM_RTTI()

class CStorySceneEventLightProperties : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventLightProperties, CStorySceneEvent );

	friend class StoryScene::InterpolationTraits< CStorySceneEventLightProperties >;

protected:
	CName							m_lightId;

	Bool							m_enabled;
	Bool							m_additiveChanges;
	Bool							m_useGlobalCoords;
	// general light properties
	Color							m_color;
	ESceneEventLightColorSource		m_lightColorSource;
	SSimpleCurve					m_radius;
	SSimpleCurve					m_brightness;
	SSimpleCurve					m_attenuation;
	EngineTransform					m_placement;

	SStorySceneAttachmentInfo		m_attachment;
	SStorySceneLightTrackingInfo    m_lightTracker;

	SLightFlickering				m_flickering;
	// spot light properties
	SStorySceneSpotLightProperties		m_spotLightProperties;
	SStorySceneLightDimmerProperties	m_dimmerProperties;

public:
	CStorySceneEventLightProperties();
	CStorySceneEventLightProperties( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventLightProperties* Clone() const override;

	Bool UseGlobalCoords() {return m_useGlobalCoords;}
	void SetUseCustomColor() { m_lightColorSource = ELCS_CustomLightColor; }
	Bool GetUseEnvColor() { return m_lightColorSource != ELCS_CustomLightColor; }

	Color GetColor( CStoryScenePlayer* scenePlayer ) const;

	RED_INLINE Bool GetEnabled() const													{ return m_enabled; }
	RED_INLINE const EngineTransform& GetTransform() const								{ return m_placement; }
	RED_INLINE EngineTransform& GetTransformRef()										{ return m_placement; }

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	
	virtual CName GetSubject() const override { return m_lightId; }

	void SetAttachmentInfo( const SStorySceneAttachmentInfo& attachmentInfo ) { m_attachment = attachmentInfo; }

	CName GetAttachmentActor() const { return m_attachment.m_attachTo; }
	CName GetAttachmentBone() const { return m_attachment.m_parentSlotName; }
	Bool  IsAttached() const { return !m_attachment.Empty(); }
	Bool  IsTracking() const { return m_lightTracker.m_enable; }
	Uint32 GetAttachmentFlags() const { return m_attachment.GetAttachmentFlags(); } 
	void ResetCurves();
	Float GetBrightness( CWorld* onWorld ) const;
	Float GetRadius( CWorld* onWorld ) const;
	Float GetAttenuation( CWorld* onWorld ) const;

#ifndef NO_EDITOR
public:
	void LoadDataFromOtherEvent( const CStorySceneEventLightProperties* other );
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventLightProperties )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_lightId, TXT( "Light ID" ), TXT( "DialogLightTag" ) )
	PROPERTY_EDIT( m_enabled, TXT("") )
	PROPERTY_EDIT( m_additiveChanges, TXT("") )
	PROPERTY_EDIT( m_color, TXT("") )
	PROPERTY_EDIT( m_lightColorSource, TXT("") )
	PROPERTY_EDIT( m_radius, TXT( "" ) )
	PROPERTY_EDIT( m_brightness, TXT( "" ) )
	PROPERTY_EDIT( m_attenuation, TXT( "" ) )
	PROPERTY_EDIT( m_placement, TXT( "Placement" ) );
	PROPERTY_EDIT( m_flickering, TXT("Light flickering") );
	PROPERTY_EDIT( m_useGlobalCoords, TXT("") )
	PROPERTY_INLINED( m_spotLightProperties, TXT( "" ) )
	PROPERTY_INLINED( m_dimmerProperties, TXT( "" ) )		
	PROPERTY_INLINED( m_attachment, TXT("Attachment info") );
	PROPERTY_INLINED( m_lightTracker, TXT("Sun tracking info") );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
