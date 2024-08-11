#include "build.h"
#include "controlRigPresetsPanel.h"
#include "..\..\common\game\storySceneEventPoseKey.h"
#include "controlRigPanel.h"
#include "..\..\common\engine\pathlibSimpleBuffers.h"
#include "..\..\common\core\depot.h"
#include "editorDataPresets.h"
#include "..\..\common\core\factory.h"


//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdControlRigPresetPanel, wxPanel )
END_EVENT_TABLE()

CEdControlRigPresetPanel::CEdControlRigPresetPanel( wxPanel* window, CEdControlRigPanel* parent )
	: m_parent( parent )
{
	wxXmlResource::Get()->LoadPanel( this, window, wxT("CRPresetsPanel") );

	m_saveButton =  XRCCTRL( *this, "PresetSaveBut", wxButton );
	m_saveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdControlRigPresetPanel::OnSave), NULL, this );

	m_loadButton = XRCCTRL( *this, "PresetLoadBut", wxButton );
	m_loadButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdControlRigPresetPanel::OnLoad), NULL, this );

	m_deleteButton = XRCCTRL( *this, "PresetDeleteBut", wxButton );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CEdControlRigPresetPanel::OnDelete), NULL, this );

	m_itemsList = XRCCTRL( *this, "CRPresetsList", wxListBox );

	LoadPresetsFromFile();
	RefreshList();
}



void CEdControlRigPresetPanel::OnLoad( wxCommandEvent& event )
{
	String selectedItem = m_itemsList->GetStringSelection().wc_str();
	if ( selectedItem.Empty() || selectedItem == NEW_ITEM.AsChar() || !m_parent->GetEvent() )
	{
		return;
	}
	Int32 index = PresetIndexByName( selectedItem );
	if ( index >= 0 )
	{
		m_presets[index].LoadEvent( m_parent->GetEvent() );
		m_parent->Reload();
		//////////////////////////////////////////////////
	}
	else
	{
		ASSERT( false );
	}
}

void CEdControlRigPresetPanel::OnSave( wxCommandEvent& event )
{
	String selectedItem = m_itemsList->GetStringSelection().wc_str();
	if ( selectedItem.Empty() || !m_parent->GetEvent() )
	{
		return;
	}
	Int32 saveAtIndex = -1;
	if( selectedItem == NEW_ITEM.AsChar() )
	{
		wxString dialogsetNameString = wxGetTextFromUser( TXT( "New preset name:" ), TXT( "New preset" ), TXT("") );
		if ( !dialogsetNameString.IsEmpty() )
		{
			saveAtIndex = m_presets.Size();
			m_presets.Grow( 1 );
			m_presets.Last().m_presetName = dialogsetNameString;
			m_presets.Last().m_presetCName = CName( dialogsetNameString.wc_str() );
			RefreshList();
		}		
	}
	else
	{
		saveAtIndex = PresetIndexByName( selectedItem );	
	}
	if( saveAtIndex >= 0 )
	{
		m_presets[saveAtIndex].m_presetVersion++;
		m_presets[saveAtIndex].SaveEvent( m_parent->GetEvent() );
		SavePresetsToFile();
	}
	else
	{
		ASSERT( false );
	}
}

Int32 CEdControlRigPresetPanel::PresetIndexByName( String& name )
{
	for( Int32 i = 0; i < m_presets.SizeInt(); i++ )
	{
		if ( m_presets[i].m_presetName == name )
		{
			return i;
		}
	}
	return -1;
}

void CEdControlRigPresetPanel::OnDelete( wxCommandEvent& event )
{
	String selectedItem = m_itemsList->GetStringSelection().wc_str();
	if ( selectedItem.Empty() || selectedItem == NEW_ITEM.AsChar() )
	{
		return;
	}
	Int32 index = PresetIndexByName( selectedItem );
	if ( index >= 0 )
	{
		m_presets.RemoveAt( index );
	}
	else
	{
		ASSERT( false );
	}
	SavePresetsToFile();
	RefreshList();
}

void CEdControlRigPresetPanel::RefreshList()
{
	m_itemsList->Freeze();
	m_itemsList->Clear();
	m_itemsList->Append( NEW_ITEM.AsChar() );
	for( const CStorySceneEventPoseKeyPresetData& preset : m_presets )
	{
		m_itemsList->Append( preset.m_presetName.AsChar() );
	}
	m_itemsList->Thaw();
}

void CEdControlRigPresetPanel::LoadPresetsFromFile()
{
	THandle< CResource > handle =  GDepot->LoadResource( FILE_PATH );
	CDataPresets* preset = Cast<CDataPresets>( handle.Get() );
	if ( preset )
	{
		if( CStoryScenePresets* scenePresets = Cast<CStoryScenePresets> ( preset->m_data ) )
		{
			m_presets = scenePresets->m_posePresets;
		}		
	}
}

void CEdControlRigPresetPanel::SavePresetsToFile()
{
	CResource* res = nullptr;  
	if ( !GDepot->FileExist( FILE_PATH ) )
	{	
		IFactory* factory = IFactory::FindFactory( CDataPresets::GetStaticClass() );
		IFactory::FactoryOptions options;
		options.m_parentObject = NULL;
		res = factory->DoCreate( options );		
		if ( res )
		{
			CDirectory* directory = GDepot->CreatePath( FILE_DIR.AsChar() );
			res->SaveAs( directory, FILE_NAME );	
		}		
	}
	else
	{
		res =  GDepot->LoadResource( FILE_PATH ).Get();
	}
 
	CDataPresets* preset = Cast<CDataPresets>( res );
	if ( preset )
	{
		CStoryScenePresets* scenePresets = Cast<CStoryScenePresets> ( preset->m_data );	
		if ( !scenePresets )
		{
			scenePresets = CreateObject<CStoryScenePresets>( CStoryScenePresets::GetStaticClass(), preset );
			preset->m_data = scenePresets;
		}
		scenePresets->m_posePresets	= m_presets;
		preset->Save();
	}
	else
	{
		ASSERT( false );
	}
}

String CEdControlRigPresetPanel::FILE_DIR = TXT("gameplay\\resources\\");
String CEdControlRigPresetPanel::FILE_NAME = TXT("scene.redPresets");
String CEdControlRigPresetPanel::FILE_PATH = FILE_DIR + FILE_NAME;

String CEdControlRigPresetPanel::NEW_ITEM = TXT("< New preset >" );

IMPLEMENT_ENGINE_CLASS( CStorySceneEventPoseKeyPresetData );

void CStorySceneEventPoseKeyPresetData::LoadEvent( CStorySceneEventPoseKey* data ) const
{
	data->m_presetName		= m_presetCName;
	data->m_presetVersion	= m_presetVersion;

	data->m_bones = m_bones;
	data->m_cachedBonesIK = m_cachedBonesIK;
	data->m_cachedTransformsIK = m_cachedTransformsIK;
	data->m_bonesHands = m_bonesHands;
	data->m_tracks = m_tracks;
	data->m_cachedBones = m_cachedBones;
	data->m_cachedTransforms = m_cachedTransforms;
	data->m_cachedTracks = m_cachedTracks;
	data->m_cachedTracksValues = m_cachedTracksValues;

	data->m_editorCachedHandTracks = m_editorCachedHandTracks;
	data->m_editorCachedIkEffectorsID = m_editorCachedIkEffectorsID;
	data->m_editorCachedIkEffectorsPos = m_editorCachedIkEffectorsPos;
	data->m_editorCachedIkEffectorsWeight = m_editorCachedIkEffectorsWeight;
	data->m_editorCachedMimicSliders = m_editorCachedMimicSliders;
}

void CStorySceneEventPoseKeyPresetData::SaveEvent( const CStorySceneEventPoseKey* data )
{
	m_bones = data->m_bones;
	m_cachedBonesIK = data->m_cachedBonesIK;
	m_cachedTransformsIK = data->m_cachedTransformsIK;
	m_bonesHands = data->m_bonesHands;
	m_tracks = data->m_tracks;
	m_cachedBones = data->m_cachedBones;
	m_cachedTransforms = data->m_cachedTransforms;
	m_cachedTracks = data->m_cachedTracks;
	m_cachedTracksValues = data->m_cachedTracksValues;

	m_editorCachedHandTracks = data->m_editorCachedHandTracks;
	m_editorCachedIkEffectorsID = data->m_editorCachedIkEffectorsID;
	m_editorCachedIkEffectorsPos = data->m_editorCachedIkEffectorsPos;
	m_editorCachedIkEffectorsWeight = data->m_editorCachedIkEffectorsWeight;
	m_editorCachedMimicSliders = data->m_editorCachedMimicSliders;
}
