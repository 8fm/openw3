#pragma once
#include "..\..\common\game\storySceneEventPoseKey.h"

class CEdControlRigPanel;
class CStorySceneEventPoseKey;


class CStorySceneEventPoseKeyPresetData
{
	DECLARE_RTTI_SIMPLE_CLASS( CStorySceneEventPoseKeyPresetData )

	TDynArray< SSSBoneTransform >	m_bones;
	TDynArray< Int32 >				m_cachedBonesIK;
	TEngineQsTransformArray			m_cachedTransformsIK;
	TDynArray< SSSBoneTransform >	m_bonesHands;
	TDynArray< SSSTrackTransform >	m_tracks;

	TDynArray< Int32 >				m_cachedBones;
	TEngineQsTransformArray			m_cachedTransforms;
	TDynArray< Int32 >				m_cachedTracks;
	TDynArray< Float >				m_cachedTracksValues;

	TDynArray< Float >				m_editorCachedHandTracks;
	TDynArray< Int32 >				m_editorCachedIkEffectorsID;
	TDynArray< Vector >				m_editorCachedIkEffectorsPos;
	TDynArray< Float >				m_editorCachedIkEffectorsWeight;
	TDynArray< Float >				m_editorCachedMimicSliders;

public:
	String							m_presetName;
	CName							m_presetCName;
	Int32							m_presetVersion;

	void LoadEvent( CStorySceneEventPoseKey* data ) const;
	void SaveEvent( const CStorySceneEventPoseKey* data );
	CStorySceneEventPoseKeyPresetData() : m_presetVersion( 0 )
	{}
};

BEGIN_CLASS_RTTI( CStorySceneEventPoseKeyPresetData )
	PROPERTY_EDIT( m_presetName, TXT("") )
	PROPERTY_EDIT( m_presetCName, TXT("") )
	PROPERTY_EDIT( m_presetVersion, TXT("") )
	PROPERTY_EDIT( m_bones, TXT("") )
	PROPERTY_EDIT( m_cachedBonesIK, TXT("") )
	PROPERTY_EDIT( m_cachedTransformsIK, TXT("") )
	PROPERTY_EDIT( m_bonesHands, TXT("") )
	PROPERTY_EDIT( m_tracks, TXT("") )
	PROPERTY_EDIT( m_cachedBones, TXT("") )
	PROPERTY_EDIT( m_cachedTransforms, TXT("") )
	PROPERTY_EDIT( m_cachedTracks, TXT("") )
	PROPERTY_EDIT( m_cachedTracksValues, TXT("") )
	PROPERTY_EDIT( m_editorCachedHandTracks, TXT("") )
	PROPERTY_EDIT( m_editorCachedIkEffectorsID, TXT("") )
	PROPERTY_EDIT( m_editorCachedIkEffectorsPos, TXT("") )
	PROPERTY_EDIT( m_editorCachedIkEffectorsWeight, TXT("") )
	PROPERTY_EDIT( m_editorCachedMimicSliders, TXT("") )
END_CLASS_RTTI()




class CEdControlRigPresetPanel : public wxPanel
{
	DECLARE_EVENT_TABLE();

	static String NEW_ITEM;
	static String FILE_DIR;
	static String FILE_NAME;
	static String FILE_PATH;

	CEdControlRigPanel* m_parent;

	wxButton* m_saveButton;
	wxButton* m_loadButton;
	wxButton* m_deleteButton;

	wxListBox* m_itemsList;
	
	TDynArray< CStorySceneEventPoseKeyPresetData >	m_presets;

public:
	CEdControlRigPresetPanel(  wxPanel* window, CEdControlRigPanel* parent );

protected:
	Int32 PresetIndexByName( String& name );

	void SavePresetsToFile();
	void LoadPresetsFromFile();
	void RefreshList();

	void OnLoad( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnDelete( wxCommandEvent& event );
};
