/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "storySceneControlPart.h"
#include "storySceneVoicetagMapping.h"
#include "storyScene.h"
#include "../engine/cameraComponent.h"

class IStorySceneInputStructureListener;

enum ESoundStateDuringScene
{
	SOUND_DEFAULT = 0,
	SOUND_NO_CHANGE,
	SOUND_SILENCE,
	SOUND_REDUCED
};

BEGIN_ENUM_RTTI( ESoundStateDuringScene )
	ENUM_OPTION( SOUND_DEFAULT )
	ENUM_OPTION( SOUND_NO_CHANGE )
	ENUM_OPTION( SOUND_SILENCE )
	ENUM_OPTION( SOUND_REDUCED )
END_ENUM_RTTI()

class CStorySceneSection;

//! Input point to the story scene - describes mapping and reaction conditions that must be fulfilled to run the scene
class CStorySceneInput : public CStorySceneControlPart, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CStorySceneInput, CStorySceneControlPart, 0 )

private:
	TDynArray< IStorySceneInputStructureListener* >	m_listeners;	//!< Editor side listeners of some events related to the edition

	String									m_inputName;			//!< Name of the input
	
	// DIALOG_TOMSIN_TODO - to wywalenia
	TDynArray< CStorySceneVoicetagMapping >	m_voicetagMappings;		//!< Mappings of Voice Tags to entities from community and their positioning
	
	Bool		m_isImportant;
	Bool		m_isGameplay;
	Bool		m_dontStopByExternalSystems;	//!< Prevents stopping scene by other systems, eg. combat or reactions
												//!< This should be done by proper use isImportant flag when it will be safe to verify its priorities

	ESoundStateDuringScene	m_musicState;
	ESoundStateDuringScene	m_ambientsState;

	ENearPlaneDistance	m_sceneNearPlane;
	EFarPlaneDistance	m_sceneFarPlane;

	Float				m_maxActorsStaryingDistanceFromPlacement;
	Float				m_maxActorsStartingDistanceFormPlayer;

	Bool				m_enableIntroVehicleDismount;
	Bool				m_enableIntroLookAts;
	Float				m_introTotalTime;
	Bool				m_enableIntroFadeOut;
	Float				m_introFadeOutStartTime;

	Bool				m_blockSceneArea;
	Bool				m_enableDestroyDeadActorsAround;

	CName				m_dialogsetPlacementTag;
	CName				m_dialogsetInstanceName;

public:
	CStorySceneInput();

	//! Property was changed in editor
	void OnPropertyPostChange( IProperty* property );

	//! Get the name of the section
	virtual String GetName() const;
	
	//! Get more friendly name of the section
	virtual String GetFriendlyName() const;

	//! Set the name of the section
	void SetName( String name );

	//! Fill Voice Tag Mappings array
	void FillVoicetagMappings();

	void CollectVoiceTags( TDynArray< CName >& voiceTags ) const;

	virtual void GetRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates ) const override;

	//! Callback called when some other object is linked to this element
	virtual void OnConnected( CStorySceneLinkElement* linkedToElement );

	//! Callback called when other object is unlinked from this element
	virtual void OnDisconnected( CStorySceneLinkElement* linkedToElement );

	// Editor
	void AddListener( IStorySceneInputStructureListener* listener );
	void RemoveListener( IStorySceneInputStructureListener* listener );
	void NotifyAboutNameChanged();

	Bool IsImportant()		const	{ return m_isImportant; }
	Bool IsGameplay()		const	{ return m_isGameplay; }

	const TDynArray< CStorySceneVoicetagMapping >& GetVoicetagMappings() const { return m_voicetagMappings; }

	RED_INLINE ESoundStateDuringScene GetMusicState() const { return m_musicState; }
	RED_INLINE ESoundStateDuringScene GetAmbientsState() const { return m_ambientsState; }

	RED_INLINE ENearPlaneDistance GetNearPlane() const { return m_sceneNearPlane; }
	RED_INLINE EFarPlaneDistance GetFarPlane() const { return m_sceneFarPlane; }

	RED_INLINE Bool CanStopByExternalSystem() const { return m_dontStopByExternalSystems == false; }

	RED_INLINE Float	GetMaxActorsStaryingDistanceFromPlacement() const { return m_maxActorsStaryingDistanceFromPlacement; }
	RED_INLINE Float	GetMaxActorsStartingDistanceFormPlayer() const { return m_maxActorsStartingDistanceFormPlayer; }

	RED_INLINE Bool GetEnableIntroStage() const { return m_enableIntroVehicleDismount || m_enableIntroLookAts; }
	RED_INLINE Bool GetEnableIntroVehicleDismount() const { return m_enableIntroVehicleDismount; }
	RED_INLINE Bool GetEnableIntroLookAts() const { return m_enableIntroLookAts; }
	RED_INLINE Float GetIntroTotalTime() const { return m_introTotalTime; }
	RED_INLINE Bool GetEnableIntroFadeOut() const { return m_enableIntroFadeOut; }
	RED_INLINE Float GetIntroFadeOutStartTime() const { return m_introFadeOutStartTime; }

	RED_INLINE Bool GetBlockSceneArea() const { return m_blockSceneArea; }
	RED_INLINE Bool GetEnableDestroyDeadActorsAround() const { return m_enableDestroyDeadActorsAround; }

public:
	RED_INLINE const CName& GetDialogsetName() const { return m_dialogsetInstanceName; }
	RED_INLINE const CName& GetDialogsetPlacementTag() const { return m_dialogsetPlacementTag; }

public:
	virtual void OnPostLoad();

	virtual void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties );
};

BEGIN_CLASS_RTTI( CStorySceneInput );
	PARENT_CLASS( CStorySceneControlPart );
	PROPERTY_EDIT(	m_inputName,			TXT( "Input name" ) );
	PROPERTY_RO(	m_voicetagMappings,		TXT( "Mappings of Voice Tags to entities from community and their positioning" ) );
	PROPERTY_EDIT( m_musicState, TXT( "Music state during this scene path" ) );
	PROPERTY_EDIT( m_ambientsState, TXT( "Ambient sounds state during this scene path" ) );
	PROPERTY_EDIT( m_sceneNearPlane, TXT( "Camera near plane during this scene" ) );
	PROPERTY_EDIT( m_sceneFarPlane, TXT( "Camera near plane during this scene" ) );
	PROPERTY_EDIT( m_dontStopByExternalSystems, TXT( "Can scene be stopped by external systems" ) );
	PROPERTY_EDIT_RANGE( m_maxActorsStaryingDistanceFromPlacement, TXT( "Maximum distance of actors from scene placement node; used as a condition for starting scene; used only for gameplay scenes" ), 0.f, 100.f );
	PROPERTY_EDIT_RANGE( m_maxActorsStartingDistanceFormPlayer, TXT( "Maximum distance of actors from player; used as a condition for starting scene; used only for gameplay scenes; used when no scene placement is defined" ), 0.f, 100.f );
	PROPERTY_EDIT( m_dialogsetPlacementTag, TXT( "" ) );
	PROPERTY_CUSTOM_EDIT( m_dialogsetInstanceName, TXT( "Name of initial dialogset instance" ), TXT( "DialogSetting" ) );
	PROPERTY_EDIT( m_enableIntroVehicleDismount, TXT( "Enables vehicle (e.g. horse) dismount on dialog startup" ) );
	PROPERTY_EDIT( m_enableIntroLookAts, TXT( "Enables look ats on dialog startup" ) );
	PROPERTY_EDIT( m_introTotalTime, TXT( "Intro stage total time; only used with lookats" ) );
	PROPERTY_EDIT( m_enableIntroFadeOut, TXT( "Enables fade out to black during intro stage" ) );
	PROPERTY_EDIT( m_introFadeOutStartTime, TXT( "Time at which intro stage fade out starts; mustn't be higher than 'introTotalTime'; only used with look ats" ) );
	PROPERTY_EDIT( m_blockSceneArea, TXT( "Should non-scene actors be teleported away and scene area blocked from entering?" ) );
	PROPERTY_EDIT( m_enableDestroyDeadActorsAround, TXT( "Should non-scene dead actors be destroyed around the dialog?" ) );
	PROPERTY( m_isImportant );
	PROPERTY( m_isGameplay );
END_CLASS_RTTI();


class CStorySceneInputSorterHelper
{
public:
	Bool operator()( const CStorySceneInput* a, const CStorySceneInput* b ) const
	{
		ASSERT( a && b );
		const String &commentA = a->GetComment();
		const String &commentB = b->GetComment();
		if ( commentA != commentB )
		{
			return commentA < commentB;
		}
		else
		{
			return a->GetName() < b->GetName();
		}
	}
};
