/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "dialogEventGenerator.h"

//enum ESceneEventTypeFilter;


struct SStorySceneActorAnimationGenData : public IDialogBodyAnimationFilterInterface
{
	DECLARE_RTTI_STRUCT( SStorySceneActorAnimationGenData );

	virtual const CName& GetBodyFilterStatus() const { return m_status; }
	virtual const CName& GetBodyFilterEmotionalState() const { return m_emotionalState; }
	virtual const CName& GetBodyFilterPoseName() const { return m_poseType; }
	virtual const CName& GetBodyFilterTypeName() const { return CName::NONE; }
	virtual const CName& GetBodyFilterActor() const { return m_actorName; }


	SStorySceneActorAnimationGenData() : m_overrideDialogsetData( false ) , m_generateEventsForActor( true )
	{}

	CName   m_actorName;

	Bool	m_overrideDialogsetData;
	Bool	m_generateEventsForActor;

	CName	m_status;
	CName	m_emotionalState;
	CName	m_poseType;
};

BEGIN_CLASS_RTTI( SStorySceneActorAnimationGenData );
	PROPERTY_RO( m_actorName, TXT( "Name of actor" ) );
	PROPERTY_EDIT( m_generateEventsForActor, TXT("Generate events for this actor" ) );
	PROPERTY_EDIT( m_overrideDialogsetData, TXT("Override actor status from dialogset slot" ) );
	PROPERTY_CUSTOM_EDIT( m_status, TXT( "Status" ), TXT( "DialogBodyAnimation_Status" ) );
	PROPERTY_CUSTOM_EDIT( m_emotionalState, TXT( "Emotional state" ), TXT( "DialogBodyAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT( m_poseType, TXT( "Pose name" ), TXT( "DialogBodyAnimation_Pose" ) );
END_CLASS_RTTI();

struct CDialogEventGeneratorConfig
{
	DECLARE_RTTI_STRUCT( CDialogEventGeneratorConfig );

	CDialogEventGeneratorConfig() : m_blendIn( 0.14f ), m_overlap( 0.25f ), m_localScope( true ), m_playerInScene(  CNAME( GERALT ) )
	{}

	SStorySceneActorAnimationState GetActorState( CName actor, const CStorySceneSection* atSection ) const
	{
		SStorySceneActorAnimationState state;
		for ( Uint32 i = 0; i < m_actorState.Size(); i++)
		{
			if ( m_actorState[i].m_actorName == actor )
			{
				if ( m_actorState[i].m_overrideDialogsetData )
				{
					state.m_emotionalState = m_actorState[i].m_emotionalState;
					state.m_status = m_actorState[i].m_status;
					state.m_poseType = m_actorState[i].m_poseType;	
				}
				else
				{
					const CStorySceneDialogsetInstance* dialogset = atSection->GetScene()->GetFirstDialogsetAtSection( atSection );
					if ( dialogset )
					{
						const CStorySceneDialogsetSlot* slot = dialogset->GetSlotByActorName( actor );
						if ( slot )
						{
							state.m_emotionalState = slot->GetBodyFilterEmotionalState();
							state.m_status = slot->GetBodyFilterStatus();
							state.m_poseType = slot->GetBodyFilterPoseName();
						}						
					}
				}				
			}			
		}
		return state;
	}

	TDynArray<CName> GetActorsToProcess() const 
	{
		TDynArray<CName> actors;
		for ( Uint32 j = 0; j < m_actorState.Size(); j++ )
		{
			if( m_actorState[j].m_generateEventsForActor )
			{
				actors.PushBack( m_actorState[j].m_actorName );
			}			
		}
		return actors;
	}


	CName	m_playerInScene;

	Bool	m_localScope;	
	Bool	m_generateCam;
	Bool	m_generateShake;
	Bool	m_generateLookats;
	Bool	m_generateAnim;
	Bool	m_generateMimic;
	Bool    m_preserveExisting;

	Float	m_overlap;
	Float	m_blendIn;
	TDynArray<SStorySceneActorAnimationGenData>  m_actorState;
};

BEGIN_CLASS_RTTI( CDialogEventGeneratorConfig );
	PROPERTY( m_blendIn )
	PROPERTY_EDIT( m_playerInScene, TXT("") )
	PROPERTY_INLINED( m_actorState, TXT("") )
END_CLASS_RTTI();


class CEdStorySceneEventGeneratorSetupDialog : public wxDialog, public ISavableToConfig
{
	DECLARE_EVENT_TABLE();

protected:
	wxCheckBox*		m_cameraEventsCheckBox;
	wxCheckBox*		m_cameraShakeCheckBox;	
	wxCheckBox*		m_lookatEventsCheckBox;
	wxCheckBox*		m_animEventsCheckBox;
	wxCheckBox*		m_mimicEventsCheckBox;
	wxCheckBox*		m_preserveExistingCheckBox;
	wxRadioBox*		m_scopeRadioBox;

	CEdPropertiesPage*	m_propertiesBrowser;
	CEdSceneEditor*		m_mediator;


	Uint32	m_eventTypes;
	CDialogEventGeneratorConfig m_config;
public:
	CEdStorySceneEventGeneratorSetupDialog( CEdSceneEditor* parent );
	CDialogEventGeneratorConfig & GetConfig() {return m_config;}
	void ReloadActorStatus();


protected:
	void OnGenerateClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );
	void OnPropertiesChanged( wxCommandEvent& event );


	virtual void SaveSession( CConfigurationManager &config );
	virtual void LoadOptionsFromConfig();

	virtual void RestoreSession( CConfigurationManager &config );
	virtual void SaveOptionsToConfig();
};
