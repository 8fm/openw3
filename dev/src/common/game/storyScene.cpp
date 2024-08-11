#include "build.h"
#include "storyScene.h"

#include "storySceneInput.h"
#include "storySceneOutput.h"
#include "storySceneSection.h"
#include "storySceneCutsceneSection.h"
#include "storySceneFlowCondition.h"
#include "storySceneItems.h"

#include "storySceneGraph.h"
#include "storySceneInputBlock.h"
#include "storySceneOutputBlock.h"
#include "storySceneScriptingBlock.h"
#include "storySceneSectionBlock.h"
#include "storySceneCutsceneBlock.h"
#include "storySceneRandomBlock.h"
#include "storySceneFlowConditionBlock.h"
#include "storySceneLine.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/factory.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/feedback.h"
#include "../engine/behaviorGraph.h"
#include "../engine/localizationManager.h"
#include "storySceneEventCustomCameraInstance.h"

IMPLEMENT_ENGINE_CLASS( StorySceneDefinition )
IMPLEMENT_ENGINE_CLASS( CStoryScene )

RED_DEFINE_NAME( SceneSectionRemoved );
RED_DEFINE_NAME( SceneSectionAdded );
RED_DEFINE_NAME( ScenePartBlockSocketsChanged );
RED_DEFINE_NAME( SceneSectionNameChanged );
RED_DEFINE_NAME( SceneSectionElementAdded );
RED_DEFINE_NAME( SceneSectionLinksChanged );
RED_DEFINE_NAME( SceneChoiceLineChanged );
RED_DEFINE_NAME( SceneChoiceLineLinkChanged );
RED_DEFINE_NAME( SceneSettingChanged );

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

CGatheredResource resSceneIds( TXT( "globals\\scenes\\scene_ids.csv" ), RGF_Startup );

CStoryScene::CStoryScene()
	: m_mayActorsStartWorking( true )
	, m_surpassWaterRendering( false )
	, m_blockMusicTriggers( false )
	, m_muteSpeechUnderWater( false )
#ifndef NO_EDITOR_GRAPH_SUPPORT
	, m_graph( nullptr )
#endif
#ifndef NO_EDITOR
	, m_modifier( nullptr )
#endif
{
}

Uint32 CStoryScene::GetNumberOfSections() const
{
	return m_sections.Size();
}

CStorySceneSection* CStoryScene::GetSection( Uint32 index )
{
	if ( index >= m_sections.Size() )
	{
		return NULL;
	}
	return m_sections[ index ];
}

const CStorySceneSection* CStoryScene::GetSection( Uint32 index ) const
{
	if ( index >= m_sections.Size() )
	{
		return NULL;
	}
	return m_sections[ index ];
}

Int32 CStoryScene::GetSectionIndex( CStorySceneSection* section ) const
{
	return static_cast< Int32 >( m_sections.GetIndex( section ) );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStoryScene::RemoveSection( Uint32 index )
{
	if ( index < m_sections.Size() )
	{
		RemoveSection( m_sections[ index ] );
	}	
}

void CStoryScene::RemoveSection( CStorySceneSection* section )
{
	EDITOR_DISPATCH_EVENT( CNAME( SceneSectionRemoved ), CreateEventData( section ) );

	m_sections.Remove( section );	
	RemoveControlPart( section );
}

void CStoryScene::RemoveControlPart( CStorySceneControlPart* controlPart )
{
	m_controlParts.Remove( controlPart );

	OnStorySceneModified();
}

void CStoryScene::AddControlPart( CStorySceneControlPart* controlPart )
{
	m_controlParts.PushBackUnique( controlPart );

	OnStorySceneModified();
}

CStorySceneGraphBlock* CStoryScene::CreateAndAddSceneBlock( CClass* blockClass, CClass* controlPartClass )
{
	if ( m_graph == NULL )
	{
		return NULL;
	}

	GraphBlockSpawnInfo info( blockClass );
	CStorySceneGraphBlock* createdBlock = Cast< CStorySceneGraphBlock >( m_graph->GraphCreateBlock( info ) );
	CStorySceneControlPart* controlPart = CreateObject< CStorySceneControlPart >( controlPartClass, this );

	createdBlock->SetControlPart( controlPart );
	createdBlock->OnRebuildSockets();
	
	CStorySceneSection* section = Cast< CStorySceneSection >( controlPart );	// Ugly and hacky, but still better than keeping separate method for each scene block class. Need to cleanup section adding to remove this
	if ( section != NULL )
	{
		const CStorySceneSectionVariantId variantId = section->CreateVariant( SLocalizationManager::GetInstance().GetCurrentLocaleId() );
		section->SetDefaultVariant( variantId );
		AddSectionAtPosition( section, -1 );
	}

	m_controlParts.PushBack( controlPart );

	return createdBlock;
}
#endif

template< class T >
RED_INLINE void RemoveEmptyPointers( TDynArray< T* >& arr )
{
	for ( Int32 i=(Int32)arr.Size()-1; i>=0; --i )
	{
		if ( !arr[i] )
		{			
			arr.Erase( arr.Begin() + i );
		}
	}
}

void CStoryScene::OnPostLoad()
{
	// Pass to base class
	TBaseClass::OnPostLoad();

	// Remove empty entries
	RemoveEmptyPointers< CStorySceneSection >( m_sections );

	RemoveEmptyPointers< CStorySceneDialogsetInstance >( m_dialogsetInstances );

#ifndef NO_RESOURCE_IMPORT
	if ( m_sceneId == 0 )
	{
		GenerateSceneId();
	}
#endif

#ifndef NO_EDITOR
	while ( GetDialogsetByName( CName::NONE ) )
	{
		RemoveDialogsetInstance( CName::NONE );
	}

	for ( Uint32 i=0; i<m_sections.Size(); ++i )
	{
		const CStorySceneSection* section = m_sections[i];
		const CStoryScene* scene = section->GetScene();
		SCENE_ASSERT( scene == this );
	}


	#ifdef RED_ASSERTS_ENABLED
	TDynArray< Uint64 > sectionIds;
	#endif

	// OMG!!! Please fix it ASAP
	for ( CStorySceneControlPart* cp : m_controlParts )
	{
		if ( CStorySceneSection* section = Cast< CStorySceneSection >( cp ) )
		{
			SCENE_ASSERT( m_sections.Exist( section ), TXT("CStoryScene::OnPostLoad - Section is not in m_section list") );
			if ( !m_sections.Exist( section ) )
			{
				m_sections.PushBack( section );
			}	
		}
	}

	RegenerateId();

#ifdef RED_ASSERTS_ENABLED
	for ( CStorySceneSection* section : m_sections )
	{
		const Uint64 id = section->GetSectionUniqueId();
		SCENE_ASSERT( m_sections.Exist( section ), TXT("CStoryScene::OnPostLoad - Section is not in m_section list") );
		sectionIds.PushBack( id );
	}
#endif
#endif

	// Check links in sections
	for ( Uint32 i=0; i<m_sections.Size(); ++i )
	{
		CStorySceneSection* section = m_sections[i];
		section->ValidateLinks();
	}

	#ifndef NO_EDITOR_GRAPH_SUPPORT
	if ( m_graph != NULL )
	{
		TDynArray< CGraphBlock* >& storyBlocks = m_graph->GraphGetBlocks();
		for ( TDynArray< CGraphBlock* >::iterator blockIter = storyBlocks.Begin(); 
			blockIter != storyBlocks.End(); ++blockIter )
		{
			CStorySceneGraphBlock* sceneBlock = Cast< CStorySceneGraphBlock >( *blockIter );
			if ( sceneBlock == NULL )
			{
				continue;
			}

			CStorySceneControlPart* controlPart = sceneBlock->GetControlPart();
			if ( controlPart != NULL )
			{
				controlPart->SetParent( this );
				m_controlParts.PushBackUnique( controlPart );
			}
		}
	}	
	#endif

	CompileDataLayout();
}

#ifndef NO_RESOURCE_COOKING

void CStoryScene::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	// When cooking the game, destroy all CStorySceneEventDialogLine and CStorySceneEventCsCamera events
	// as they don't need to be cooked (they are only needed in runtime) - cooking those would only make
	// patching the game harder because cooked scene file would change during each cooking process.
	for( CStorySceneSection* section : m_sections )
	{
		if( CStorySceneCutsceneSection* cutsceneSection = Cast< CStorySceneCutsceneSection >( section ) )
		{
			cutsceneSection->DestroyDialogEvents();
		}
	}
}

#endif // NO_RESOURCE_COOKING

void CStoryScene::CompileDataLayout()
{
	/*
	// Now only for section
	const Uint32 size = m_sections.Size();
	
	m_dataLayouts.Clear(); // Clear all struct (for destructors)
	m_dataLayouts.Resize( size );

	for ( Uint32 i=0; i<size; ++i )
	{
		SCENE_ASSERT( m_sections[ i ] );

		InstanceDataLayout& layout = m_dataLayouts[ i ];

		InstanceDataLayoutCompiler compiler( layout );

		m_sections[ i ]->OnBuildDataLayout( compiler );

		layout.ChangeLayout( compiler );
	}
	*/
}

void CStoryScene::OnStorySceneModified()
{
	CompileDataLayout();
}

/*
Creates instance buffer for specified section variant.
*/
InstanceBuffer* CStoryScene::CreateInstanceBuffer( CObject* parent, const CStorySceneSection* section, CStorySceneSectionVariantId variantId, InstanceDataLayout& temp ) const
{
	/*const Uint32 size = m_sections.Size();

	SCENE_ASSERT( m_dataLayouts.Size() == size );

	for ( Uint32 i=0; i<size; ++i )
	{
		SCENE_ASSERT( m_sections[ i ] );

		if ( section == m_sections[ i ] )
		{
			if ( i < m_dataLayouts.Size() )
			{
				const InstanceDataLayout& layout = m_dataLayouts[ i ];

				return layout.CreateBuffer( parent, TXT("StorySceneSectionInstance") );
			}

			break;
		}
	}

	SCENE_ASSERT( 0 );

	return nullptr;*/


	const Uint32 size = m_sections.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		SCENE_ASSERT( m_sections[ i ] );

		if ( section == m_sections[ i ] )
		{
			InstanceDataLayoutCompiler compiler( temp );

			m_sections[ i ]->OnBuildDataLayout( compiler, variantId );

			temp.ChangeLayout( compiler );

			return temp.CreateBuffer( parent, TXT("StorySceneSectionInstance") );
		}
	}

	SCENE_ASSERT( 0 );

	return nullptr;
}

void CStoryScene::InitInstanceBuffer( InstanceBuffer* data, const CStorySceneSection* section, CStorySceneSectionVariantId variantId ) const
{
	section->OnInitInstance( *data, variantId );
}

void CStoryScene::ReleaseInstanceBuffer( InstanceBuffer* data, const CStorySceneSection* section, CStorySceneSectionVariantId variantId ) const
{
	section->OnReleaseInstance( *data, variantId );
}

void CStoryScene::DestroyInstanceBuffer( InstanceBuffer* data ) const
{
	data->Release();
}

void CStoryScene::OnPreSave()
{
#ifndef NO_EDITOR
	if ( !GIsCooker )
	{
		SLocalizationManager::GetInstance().UpdateStringDatabase( this, true );
		RefreshAllVoiceFileNames();
	}
#endif
}

void CStoryScene::OnSave()
{
	
}

void CStoryScene::OnPaste( Bool wasCopied )
{
	if ( wasCopied == true )
	{
		for ( Uint32 i = 0; i < m_sections.Size(); ++i )
		{
			m_sections[ i ]->MakeUniqueElementsCopies();
		}
	}
	SLocalizationManager::GetInstance().UpdateStringDatabase( this, true );

	OnStorySceneModified();
}

void CStoryScene::OnPreDependencyMap( IFile& mapper )
{
	// DIALOG_TOMSIN_TODO - to jakies hacki
#ifndef NO_RESOURCE_IMPORT
	if ( m_sceneId == 0 )
	{
		GenerateSceneId();
	}
#endif
}

void CStoryScene::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	//if ( file.IsReader() && file.GetVersion() < VER_APPERTURE_SCENE_DOF )
	if ( file.IsReader() )
	{
		for( TDynArray< StorySceneCameraDefinition >::iterator cameraIter = m_cameraDefinitions.Begin();
			cameraIter != m_cameraDefinitions.End(); ++cameraIter )
		{
			if ( cameraIter->m_dof.IsDefault() == true )
			{
				cameraIter->m_dof.FromEngineDofParams( cameraIter->m_dofFocusDistNear, cameraIter->m_dofFocusDistFar );
			}
		}
	}
}

Bool CStoryScene::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	RED_LOG( CNAME( StoryScene ), TXT( "Miss matching property: %s" ), propertyName.AsString().AsChar() );

	if( propertyName == CNAME( sceneTemplates ) )
	{
		TDynArray< StorySceneExpectedActor > actors;
		if( readValue.AsType( actors ) )
		{
			for( Uint32 i=0; i<actors.Size(); ++i )
			{
				// copy over data from old struct to new class
				StorySceneExpectedActor& oldDef = actors[ i ];
				CStorySceneActor* newDef = CreateObject< CStorySceneActor >( this );
				newDef->m_id = oldDef.m_voicetag;
				newDef->m_actorTags = oldDef.m_actorTags;
				newDef->m_entityTemplate = oldDef.m_entityTemplate;
				newDef->m_appearanceFilter = oldDef.m_appearanceFilter;
				newDef->m_dontSearchByVoicetag = oldDef.m_dontSearchByVoicetag;
				newDef->m_alias = oldDef.m_alias;

				m_sceneTemplates.PushBack( newDef );
			}
		}

		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}


#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStoryScene::InitializeSceneGraphs()
{
	// Create story graph
	if ( m_graph == NULL )
	{
		m_graph = CreateObject< CStorySceneGraph >( this );

		CStorySceneGraphBlock* input = CreateAndAddSceneBlock( ClassID< CStorySceneInputBlock >(), ClassID< CStorySceneInput >() );
		CStorySceneGraphBlock* section = CreateAndAddSceneBlock( ClassID< CStorySceneSectionBlock >(), ClassID< CStorySceneSection >() );
		CStorySceneGraphBlock* output = CreateAndAddSceneBlock( ClassID< CStorySceneOutputBlock >(), ClassID< CStorySceneOutput >() );

		input->SetPosition( Vector( 50, 50, 0 ) );
		section->SetPosition( Vector( 150, 50, 0 ) );
		output->SetPosition( Vector( 250, 50, 0 ) );

		OnStorySceneModified();
	}
}

#endif

const CStorySceneSection* CStoryScene::FindSection( const String& sectionName ) const
{
	// Linear search
	for ( Uint32 i=0; i<m_sections.Size(); i++ )
	{
		const CStorySceneSection* section = m_sections[i];
		if ( section->GetName() == sectionName )
		{
			return section;
		}
	}

	// Not found
	return NULL;
}

const CStorySceneInput* CStoryScene::FindInput( const String& inputName ) const
{
	TDynArray< CStorySceneInput* > inputs;
	CollectControlParts< CStorySceneInput >( inputs );

	for ( Uint32 i = 0; i < inputs.Size(); ++i )
	{
		if ( inputs[ i ]->GetName() == inputName )
		{
			return inputs[ i ];
		}
	}
	return NULL;
}

const CStorySceneActor* CStoryScene::GetSceneActorDefinition( const CName& id ) const
{
	for( Uint32 i=0; i<m_sceneTemplates.Size(); ++i )
	{
		if( m_sceneTemplates[ i ] && m_sceneTemplates[ i ]->m_id == id )
		{
			return m_sceneTemplates[ i ];
		}
	}

	return nullptr;
}


const CStorySceneEffect* CStoryScene::GetSceneEffectDefinition( const CName& id ) const
{
	for( Uint32 i=0; i<m_sceneEffects.Size(); ++i )
	{
		if( m_sceneEffects[ i ] && m_sceneEffects[ i ]->m_id == id )
		{
			return m_sceneEffects[ i ];
		}
	}

	return nullptr;
}

const CStorySceneLight* CStoryScene::GetSceneLightDefinition( const CName& id ) const
{
	for( Uint32 i=0; i<m_sceneLights.Size(); ++i )
	{
		if( m_sceneLights[ i ] && m_sceneLights[ i ]->m_id == id )
		{
			return m_sceneLights[ i ];
		}
	}

	return nullptr;
}

const IStorySceneItem* CStoryScene::GetSceneItem( const CName& id ) const
{
	// TODO: these items should be stored in 1 map at some point instead of 4 arrays

	for( Uint32 i=0; i<m_sceneTemplates.Size(); ++i )
	{
		if( m_sceneTemplates[ i ] && m_sceneTemplates[ i ]->m_id == id )
		{
			return m_sceneTemplates[ i ];
		}
	}

	for( Uint32 i=0; i<m_sceneProps.Size(); ++i )
	{
		if( m_sceneProps[ i ] && m_sceneProps[ i ]->m_id == id )
		{
			return m_sceneProps[ i ];
		}
	}

	for( Uint32 i=0; i<m_sceneEffects.Size(); ++i )
	{
		if( m_sceneEffects[ i ] && m_sceneEffects[ i ]->m_id == id )
		{
			return m_sceneEffects[ i ];
		}
	}

	for( Uint32 i=0; i<m_sceneLights.Size(); ++i )
	{
		if( m_sceneLights[ i ] && m_sceneLights[ i ]->m_id == id )
		{
			return m_sceneLights[ i ];
		}
	}

	return nullptr;
}

CStorySceneActor* CStoryScene::GetSceneActorDefinition( const CName& id )
{
	return const_cast< CStorySceneActor* >( static_cast< const CStoryScene* >( this )->GetSceneActorDefinition( id ) );
}

CStorySceneEffect* CStoryScene::GetSceneEffectDefinition( const CName& id )
{
	return const_cast< CStorySceneEffect* >( static_cast< const CStoryScene* >( this )->GetSceneEffectDefinition( id ) );
}

CStorySceneLight* CStoryScene::GetSceneLightDefinition( const CName& id )
{
	return const_cast< CStorySceneLight* >( static_cast< const CStoryScene* >( this )->GetSceneLightDefinition( id ) );	
}

IStorySceneItem* CStoryScene::GetSceneItem( const CName& id )
{
	return const_cast< IStorySceneItem* >( static_cast< const CStoryScene* >( this )->GetSceneItem( id ) );
}

void CStoryScene::GetAppearancesForVoicetag( CName voicetag, TDynArray< CTemplateWithAppearance > & templates ) const
{
	for ( Uint32 i = 0; i < m_sceneTemplates.Size(); ++i )
	{
		if( !m_sceneTemplates[ i ] )
			continue;

		const THandle<CEntityTemplate> entityTemplate = m_sceneTemplates[ i ]->m_entityTemplate.Get();
		if ( !entityTemplate )
			continue;

		const TDynArray< CName > &            wantedAppearances = m_sceneTemplates[ i ]->m_appearanceFilter;
		TDynArray< const CEntityAppearance* > usedAppearances;
		entityTemplate->GetAllEnabledAppearances( usedAppearances );
			
		for ( Uint32 j = 0; j < usedAppearances.Size(); ++j )
		{
			const CEntityAppearance* appearance = usedAppearances[ j ];
			
			if ( wantedAppearances.Empty() == false && wantedAppearances.Exist( appearance->GetName() ) == false )
				continue;
			
			if ( entityTemplate->GetApperanceVoicetag( appearance->GetName() ) == voicetag )
			{
				CTemplateWithAppearance twa = { entityTemplate, appearance->GetName() };
				templates.PushBack( twa );
			}
		}

		m_sceneTemplates[ i ]->m_entityTemplate.Release();
	}
}

Bool CStoryScene::GetActorSpawnDefinition( const CName& voicetag, THandle<CEntityTemplate>& actorTemplate, CName& appearanceName, Bool& isSearchedByVoicetag ) const
{
	for ( Uint32 i = 0; i < m_sceneTemplates.Size(); ++i )
	{
		if( !m_sceneTemplates[ i ] )
			continue;

		if ( m_sceneTemplates[ i ]->m_id != CName::NONE && m_sceneTemplates[ i ]->m_id != voicetag )
		{
			continue;
		}

		THandle<CEntityTemplate> entityTemplate = m_sceneTemplates[ i ]->m_entityTemplate.Get();

		if ( !entityTemplate )
		{
			continue;
		}

		TDynArray< CEntityAppearance* > appearances; 
		
		entityTemplate->GetAllAppearances( appearances, &(m_sceneTemplates[ i ]->m_appearanceFilter) );

		for ( Uint32 j = 0; j < appearances.Size(); ++j )
		{
			CEntityAppearance* appearance = appearances[ j ];
			if (entityTemplate->GetEnabledAppearancesNames().Exist( appearance->GetName() ) == false )
			{
				continue;
			}

			if ( entityTemplate->GetApperanceVoicetag( appearance->GetName() ) == voicetag || m_sceneTemplates[ i ]->m_dontSearchByVoicetag == true )
			{
				actorTemplate = entityTemplate;
				appearanceName = appearance->GetName();
				isSearchedByVoicetag = m_sceneTemplates[ i ]->m_dontSearchByVoicetag == false;
				return true;
			}
		}

		m_sceneTemplates[ i ]->m_entityTemplate.Release();
	}

	actorTemplate = NULL;
	appearanceName = CName::NONE;
	isSearchedByVoicetag = false;

	return false;
}

const CStorySceneProp* CStoryScene::GetPropDefinition( CName id ) const
{
	for ( Uint32 i = 0; i < m_sceneProps.Size(); ++i )
	{
		if ( m_sceneProps[ i ] && m_sceneProps[ i ]->m_id == id )
		{
			return m_sceneProps[i];
		}		 
	}
	return nullptr;
}

Bool CStoryScene::GetTaglistForVoicetag( const CName& voicetag, TagList& tags ) const
{	
	for ( Uint32 i = 0; i < m_sceneTemplates.Size(); ++i )
	{
		if (  m_sceneTemplates[ i ] && m_sceneTemplates[ i ]->m_id == voicetag )
		{
			tags = m_sceneTemplates[ i ]->m_actorTags;
			return true;
		}
	}
	return false;
}

const CStorySceneActor* CStoryScene::GetActorDescriptionForVoicetag( const CName& voicetag ) const
{
	for ( Uint32 i = 0; i < m_sceneTemplates.Size(); ++i )
	{
		if ( m_sceneTemplates[ i ] && m_sceneTemplates[ i ]->m_id == voicetag )
		{
			return m_sceneTemplates[ i ];
		}
	}
	return NULL;
}

void CStoryScene::CollectVoicetags( TDynArray< CName >& voicetags, Bool includeNonSpeaking ) const
{
	for ( Uint32 i = 0; i < m_sections.Size(); ++i )
	{
		if ( m_sections[ i ]->IsA< CStorySceneCutsceneSection >() == true )
		{
			const CStorySceneCutsceneSection* cutsceneSection = Cast< CStorySceneCutsceneSection >( m_sections[ i ] );
			cutsceneSection->GetCutsceneVoicetags( voicetags, true );
		}
		else
		{
			m_sections[ i ]->GetVoiceTags( voicetags, true );
		}
	}

	if ( includeNonSpeaking == true )
	{
		for ( Uint32 j = 0; j < m_dialogsetInstances.Size(); ++j )
		{
			const CStorySceneDialogsetInstance* dialogset = m_dialogsetInstances[ j ];
			if ( dialogset == NULL )
			{
				continue;
			}
			const TDynArray< CStorySceneDialogsetSlot* > dialogsetSlots = dialogset->GetSlots();
			for ( Uint32 k = 0; k < dialogsetSlots.Size(); ++k )
			{
				if ( dialogsetSlots[ k ] == NULL )
				{
					continue;
				}
				voicetags.PushBackUnique( dialogsetSlots[ k ]->GetActorName() );
			}
		}
		for ( Uint32 k = 0; k < m_sceneTemplates.Size(); ++k )
		{
			if( m_sceneTemplates[ k ] )
			{
				voicetags.PushBackUnique( m_sceneTemplates[ k ]->m_id );
			}
		}
	}
}

#ifndef NO_EDITOR

CName CStoryScene::GetFirstDialogsetNameAtSection_Old( const CStorySceneSection* section ) const
{
	if ( section->GetDialogsetChange() != CName::NONE )
	{
		return section->GetDialogsetChange();
	}

	CName dialogsetName;

	TDynArray< CStorySceneInput* > inputs;
	CollectControlParts( inputs );

	CStorySceneInputSorterHelper sorterPred;
	Sort( inputs.Begin(), inputs.End(), sorterPred );

	for ( Uint32 i = 0; i < inputs.Size(); ++i )
	{
		CStorySceneInput* input = inputs[ i ];

		dialogsetName = input->GetDialogsetName();

		TDynArray< const CStorySceneLinkElement* > inputLinkElements;
		input->GetAllNextLinkedElements( inputLinkElements );
		for ( Uint32 j = 0; j < inputLinkElements.Size(); ++j )
		{
			const CStorySceneSection* linkedSection = Cast< const  CStorySceneSection >( inputLinkElements[ j ] );
			if ( linkedSection == NULL )
			{
				continue;
			}

			if ( linkedSection->GetDialogsetChange() != CName::NONE )
			{
				dialogsetName = linkedSection->GetDialogsetChange();
			}

			if ( linkedSection == section )
			{
				return dialogsetName;
			}
		}
	}

	return CName::NONE;
}

const CStorySceneDialogsetInstance*	CStoryScene::GetFirstDialogsetAtSection_Old( const CStorySceneSection* section ) const
{
	CName dialogsetName = GetFirstDialogsetNameAtSection_Old( section );
	return GetDialogsetByName( dialogsetName );
}

#endif

const CStorySceneDialogsetInstance* CStoryScene::GetDialogsetByName( const CName& dialogsetName ) const
{
	for ( Uint32 i = 0; i < m_dialogsetInstances.Size(); ++i )
	{
		if ( m_dialogsetInstances[ i ] != NULL && m_dialogsetInstances[ i ]->GetName() == dialogsetName )
		{
			return m_dialogsetInstances[ i ];
		}
	}
	return NULL;
}

void CStoryScene::GetDialogsetInstancesNames( TDynArray< CName >& names ) const
{
	for ( Uint32 i = 0; i < m_dialogsetInstances.Size(); ++i )
	{
		if ( m_dialogsetInstances[ i ] != NULL && m_dialogsetInstances[ i ]->GetName() != CName::NONE )
		{
			names.PushBackUnique( m_dialogsetInstances[ i ]->GetName() );
		}
	}
}

Bool CStoryScene::CheckSectionsIdsCollision( const CStoryScene* s ) const
{
	TDynArray< Uint64 > sectionIds;

	for ( const CStorySceneSection* section : m_sections )
	{
		const Uint64 id = section->GetSectionUniqueId();

		if ( sectionIds.Exist( id ) )
		{
			return false;
		}
		sectionIds.PushBack( id );
	}

	for ( const CStorySceneSection* section : s->m_sections )
	{
		const Uint64 id = section->GetSectionUniqueId();

		if ( sectionIds.Exist( id ) )
		{
			return false;
		}
		sectionIds.PushBack( id );
	}

	return true;
}

#ifndef NO_EDITOR
CStorySceneDialogsetInstance* CStoryScene::GetFirstDialogsetAtSection( const CStorySceneSection* section )
{
	CName dialogsetName = GetFirstDialogsetNameAtSection_Old( section );
	return GetDialogsetByName( dialogsetName );
}

const CStorySceneDialogsetInstance* CStoryScene::GetFirstDialogsetAtSection( const CStorySceneSection* section ) const
{
	CName dialogsetName = GetFirstDialogsetNameAtSection_Old( section );
	return GetDialogsetByName( dialogsetName );
}

CStorySceneDialogsetInstance* CStoryScene::GetDialogsetByName( const CName& dialogsetName )
{
	for ( Uint32 i = 0; i < m_dialogsetInstances.Size(); ++i )
	{
		if ( m_dialogsetInstances[ i ] != NULL && m_dialogsetInstances[ i ]->GetName() == dialogsetName )
		{
			return m_dialogsetInstances[ i ];
		}
	}
	return NULL;
}

void CStoryScene::AddDialogsetInstance( CStorySceneDialogsetInstance* dialogsetInstance )
{
	dialogsetInstance->SetParent( this );
	m_dialogsetInstances.PushBack( dialogsetInstance );
}

void CStoryScene::RemoveDialogsetInstance( const CName& dialogsetName )
{
	m_dialogsetInstances.Remove( GetDialogsetByName( dialogsetName ) );
}

void CStoryScene::AddActorDefinition( CStorySceneActor* actorDef )
{
	if( actorDef )
	{
		m_sceneTemplates.PushBack( actorDef );
	}
}

void CStoryScene::AddLightDefinition( CStorySceneLight* lightDef )
{
	if( lightDef )
	{
		m_sceneLights.PushBack( lightDef );
	}
}

#endif

void CStoryScene::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const*/
{
	for ( Uint32 i = 0; i < m_sections.Size(); ++i )
	{
		CStorySceneSection* section = m_sections[ i ];
		section->GetLocalizedStrings( localizedStrings );
	}

	// At this point all LocalizedStringEntry::m_parentResource ptrs are nullptr.
	// Make all of them point to this CStoryScene as parent resource.
	for ( Uint32 j = 0; j < localizedStrings.Size(); ++j )
	{
		localizedStrings[ j ].m_parentResource = this;
	}
}

String CStoryScene::GenerateUniqueElementID()
{
	Uint32 id = ++m_elementIDCounter;
	return String::Printf( TXT("_%d"), id );
}

Uint32 CStoryScene::GenerateUniqueSectionID()
{
	return ++m_sectionIDCounter;
}

void CStoryScene::CleanupSceneGraph()
{		 
	#ifndef NO_EDITOR_GRAPH_SUPPORT
	if ( m_graph != NULL )
	{
		m_graph->Discard();
		m_graph = NULL;
	}	
	#endif
}

#ifndef NO_RESOURCE_IMPORT

void CStoryScene::GenerateSceneId()
{
#ifdef RED_FINAL_BUILD
	if ( GGame->IsActive() == true )
	{
		return;
	}
#endif
	
	m_sceneId =  GetDepotPath().CalcQHash();
}

#endif

#ifndef NO_RESOURCE_IMPORT

void CStoryScene::RefreshAllVoiceFileNames()
{
	for ( Uint32 j = 0; j < m_sections.Size(); ++j )
	{
		for ( Uint32 k = 0; k < m_sections[ j ]->GetNumberOfElements(); ++k )
		{
			CStorySceneLine* sceneLine = Cast< CStorySceneLine >( m_sections[ j ]->GetElement( k ) );
			if ( sceneLine != NULL )
			{
				sceneLine->RefreshVoiceFileName();
			}
		}
	}
}

#endif

void CStoryScene::GetLines( TDynArray< CAbstractStorySceneLine* >& lines ) const
{
	for ( Uint32 i=0; i<m_sections.Size(); i++ )
	{
		CStorySceneSection* section = m_sections[i];
		section->GetLines( lines );
	}
}

void CStoryScene::AddSectionAtPosition( CStorySceneSection* section, Int32 index )
{
	RED_FATAL_ASSERT( section->GetNumVariants() > 0, "CStoryScene::AddSectionAtPosition(): section has no variants." );
	RED_FATAL_ASSERT( section->GetDefaultVariant() != -1, "CStoryScene::AddSectionAtPosition(): section has no default variant set." );

	if ( index == -1 || static_cast<Uint32>( index ) >= m_sections.Size() )
	{
		m_sections.PushBack( section );
	}
	else
	{
		m_sections.Insert( index, section );
	}

	section->SetParent( this );

	section->GenerateSectionId();

	section->OnSectionCreate();

	EDITOR_DISPATCH_EVENT( CNAME( SceneSectionAdded ), CreateEventData( section ) );

	OnStorySceneModified();
}

#ifndef NO_RESOURCE_IMPORT

void CStoryScene::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT( "sceneId" ) )
	{
		TDynArray< CAbstractStorySceneLine* > sceneLines;
		GetLines( sceneLines );
		for ( Uint32 i = 0; i < sceneLines.Size(); ++i )
		{
			CStorySceneLine* sceneLine = Cast< CStorySceneLine >( sceneLines[ i ] );
			if ( sceneLine != NULL )
			{
				//sceneLine->GenerateVoiceFileName();
			}
		}
	}
}

#endif

#ifndef NO_EDITOR

TDynArray< String > CStoryScene::GetInputNames() const
{
	TDynArray< String > inputNames;
	
	TDynArray< CStorySceneInput* > inputs;
	CollectControlParts< CStorySceneInput >( inputs );

	for ( Uint32 i = 0; i < inputs.Size(); ++i )
	{
		inputNames.PushBack( inputs[ i ]->GetName() );
	}

	return inputNames;
}

TDynArray< String > CStoryScene::GetOutputNames() const
{
	TDynArray< String > outputNames;

	TDynArray< CStorySceneOutput* > outputs;
	CollectControlParts< CStorySceneOutput >( outputs );

	for ( Uint32 i = 0; i < outputs.Size(); ++i )
	{
		outputNames.PushBack( outputs[ i ]->GetName() );
	}

	return outputNames;
}

#endif

const StorySceneCameraDefinition* CStoryScene::GetCameraDefinition( const CName& cameraName ) const
{
	for ( TDynArray< StorySceneCameraDefinition >::const_iterator cameraIter = m_cameraDefinitions.Begin();
		cameraIter != m_cameraDefinitions.End(); ++cameraIter )
	{
		if ( cameraIter->m_cameraName == cameraName )
		{
			return &(*cameraIter);
		}
	}
	return NULL;
}

StorySceneCameraDefinition* CStoryScene::GetCameraDefinition( const CName& cameraName )
{
	for ( TDynArray< StorySceneCameraDefinition >::iterator cameraIter = m_cameraDefinitions.Begin();
		cameraIter != m_cameraDefinitions.End(); ++cameraIter )
	{
		if ( cameraIter->m_cameraName == cameraName )
		{
			return &(*cameraIter);
		}
	}
	return NULL;
}

void CStoryScene::AddCameraDefinition( const StorySceneCameraDefinition& cameraDefinition )
{
	//If camera with this name exists override its values 
	StorySceneCameraDefinition * camDef = GetCameraDefinition(cameraDefinition.m_cameraName);
	if( camDef )
	{
		*camDef = cameraDefinition;
	}
	else
	{
		m_cameraDefinitions.PushBack( cameraDefinition );
	}
}

Bool CStoryScene::DeleteCameraDefinition( const CName& cameraName )
{
	TDynArray< StorySceneCameraDefinition >::iterator cameraIter;
	for ( cameraIter = m_cameraDefinitions.Begin(); cameraIter != m_cameraDefinitions.End(); ++cameraIter )
	{
		if ( cameraIter->m_cameraName == cameraName )
		{
			break;
		}
	}
	
	if ( cameraIter != m_cameraDefinitions.End() )
	{
		m_cameraDefinitions.Erase( cameraIter );
		return true;
	}

	return false;
}


void CStoryScene::DeleteAllCameraDefinitions()
{
	m_cameraDefinitions.ClearFast();
}

void CStoryScene::CollectAllRequiredPositionTags( TDynArray< CName >& positionTags )
{
	positionTags.Clear();
	for ( TDynArray< CStorySceneDialogsetInstance* >::const_iterator settingIter = m_dialogsetInstances.Begin();
		settingIter != m_dialogsetInstances.End(); ++settingIter )
	{
		if ( *settingIter == NULL )
		{
			return;
		}

		if ( (*settingIter)->GetPlacementTag().Empty() == false )
		{
			positionTags.PushBackUnique( (*settingIter)->GetPlacementTag().GetTags() );
		}
		
	}

	for ( TDynArray< CStorySceneControlPart* >::iterator partIter = m_controlParts.Begin();
		partIter != m_controlParts.End(); ++partIter )
	{
		CStorySceneCutsceneSection* cutsceneSection = Cast< CStorySceneCutsceneSection >( *partIter );
		if ( cutsceneSection != NULL )
		{
			if ( cutsceneSection->GetTagPoint().Empty() == false )
			{
				positionTags.PushBackUnique( cutsceneSection->GetTagPoint().GetTags() );
			}
			if ( cutsceneSection->GetCsTemplate() != NULL )
			{
				if ( cutsceneSection->GetCsTemplate()->GetPointTag().Empty() == false )
				{
					positionTags.PushBackUnique( cutsceneSection->GetCsTemplate()->GetPointTag().GetTags() );
				}
			}
			continue;
		}
	}
}

#ifndef NO_EDITOR
void CStoryScene::ConnectSceneModifier( IStorySceneModifier* m )
{
	m_modifier = m;
}

void CStoryScene::RegenerateId()
{
	GenerateSceneId();

	m_sectionIDCounter = 0;
	for ( CStorySceneSection* section : m_sections )
	{
		section->GenerateSectionId();
	}
}

Bool CStoryScene::IsDialogsetUsed( CName dialogset ) const
{
	for( const CStorySceneSection* section : m_sections )
	{
		if( section->GetDialogsetChange() == dialogset )
		{
			return true;
		}
	}

	for ( const CStorySceneControlPart*  controlPartIter : m_controlParts )
	{
		if ( const CStorySceneInput* input = Cast< CStorySceneInput >( controlPartIter ) )
		{
			if( input->GetDialogsetName() == dialogset )
			{
				return true;
			}
		}
	}

	return false;
}
#endif

Bool CStoryScene::OnLinkElementMarkModifiedPre( const CStorySceneLinkElement* element )
{
#ifndef NO_EDITOR
	return true;
#else
	SCENE_ASSERT__FIXME_LATER( 0 );
	return true;
#endif
}

void CStoryScene::OnLinkElementMarkModifiedPost( const CStorySceneLinkElement* element )
{
#ifndef NO_EDITOR
	OnStorySceneModified();
#endif
}

//////////////////////////////////////////////////////////////////////////

void CStoryScene::funcGetCustomBehaviorForVoicetag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, voicetag, CName::NONE );
	FINISH_PARAMETERS;

	CBehaviorGraph* behaviorGraph = NULL;

	RETURN_OBJECT( behaviorGraph );
}

void CStoryScene::funcGetCustomAnimsetForVoicetag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, voicetag, CName::NONE );
	FINISH_PARAMETERS;

	CSkeletalAnimationSet* animset = NULL;

	RETURN_OBJECT( animset );
}

void CStoryScene::funcGetRequiredPositionTags( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	TDynArray< CName > positionTags;
	CollectAllRequiredPositionTags( positionTags );

	RETURN_STRUCT( TDynArray< CName >, positionTags );
}

//////////////////////////////////////////////////////////////////////////

CStoryScene::ControlPartIterator::ControlPartIterator( CStoryScene* scene, const CClass* c )
	: m_list( scene->m_controlParts )
	, m_class( c )
	, m_index( -1 )
{
	Next();
}

Bool CStoryScene::ControlPartIterator::IsValid() const
{
	return ( m_index >= 0 ) && ( m_index < m_list.SizeInt() );
}

void CStoryScene::ControlPartIterator::Next()
{
	while ( ++m_index < m_list.SizeInt() )
	{
		if ( m_list[ m_index ] && m_list[ m_index ]->IsA( m_class ) )
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

class StorySceneFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( StorySceneFactory, IFactory, 0 );

public:
	StorySceneFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS( StorySceneFactory, IFactory );
IMPLEMENT_ENGINE_CLASS( StorySceneFactory);

StorySceneFactory::StorySceneFactory()
{
	m_resourceClass = ClassID< CStoryScene >();
}

CResource* StorySceneFactory::DoCreate( const FactoryOptions& options )
{
	CStoryScene* scene = ::CreateObject< CStoryScene >( options.m_parentObject );
	return scene;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
