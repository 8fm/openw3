/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "editorExternalResources.h"

#include "../../common/game/wayPointComponent.h"
#include "../../common/game/hud.h"
#include "../../common/game/hudModule.h"
#include "../../common/game/menu.h"
#include "../../common/game/popup.h"
#include "../../common/game/storySceneAnimationList.h"
#include "../../common/game/encounter.h"

#include "../../games/r4/journalQuest.h"
#include "../../games/r4/journalCreature.h"



#include "propertyItemArray.h"
#include "propertyItemClass.h"
#include "propertyItemEnum.h"
#include "propertyItemPointer.h"


#include "behaviorAnimSelection.h"
#include "behaviorValueSelection.h"
#include "behaviorBoneSelection.h"
#include "behaviorBoneMultiSelection.h"
#include "tagPropertyEditor.h"
#include "gameTimePropertyEditor.h"
#include "selectionList.h"
#include "classList.h"
#include "componentListSelection.h"
#include "effectParameterListSelection.h"
#include "effectBehaviorEventListSelection.h"
#include "typeList.h"
#include "variantEditor.h"
#include "2daArrayValueSelection.h"
#include "2daArrayExclusiveValueEdition.h"
#include "spawnsetPhasesEditor.h"
#include "encounterPhasesEditor.h"
#include "creatureDefinitionsEditor.h"
#include "slider.h"
#include "2daTagListBoxEditor.h"
#include "2daTagListUpdater.h"
#include "stringPropertyEditor.h"
#include "voiceTagListEditor.h"
#include "curvePropertyEditor.h"
#include "soundPropertyEditors.h"
#include "appearanceList.h"
#include "textureGroupSelection.h"
#include "effectBoneListSelection.h"
#include "entityOnLayerReferenceEditor.h"
#include "itemListSelection.h"
#include "jobActionAnimSelection.h"
#include "layerGuidListSelection.h"
#include "jobTreeEditor.h"
#include "functionListSelection.h"
#include "layerGroupTreeSelection.h"
#include "layerGroupListSelection.h"
#include "entityAppearanceSelection.h"
#include "entityVoiceTagEditor.h"
#include "journalPropertySelector.h"
#include "huntingClueSelector.h"
#include "localizedStringEditor.h"
#include "gameInputSelection.h"
#include "questBehaviorGraphProperties.h"
#include "enumList.h"
#include "dialogEditorActorPropertyEditor.h"
#include "dialogEditorAnimationPropertyEditor.h"
#include "dialogEditorBehaviorEventPropertyEditor.h"
#include "dialogEditorEnterActorPropertyEditor.h"
#include "dialogEditorSettingPropertyEditor.h"
#include "dialogEditorChangePosePropertyEditor.h"
#include "dialogEditorChangeSlotPropertyEditor.h"
#include "dialogEditorCustomCameraSelector.h"
#include "dialogBoneSelectionEditor.h"
#include "cutsceneEffectsPropertyEditors.h"
#include "cameraShakeEnumList.h"
#include "sceneDialogsetsPropertyEditors.h"
#include "mimicsAnimationSelector.h"
#include "soundMaterialSelector.h"
#include "entityHandleEditor.h"
#include "scriptedEnumEditor.h"
#include "chooseItemCustomEditor.h"
#include "staticAnimationSelection.h"
#include "physicalCollisionSelector.h"
#include "worldSelectionCustomEditor.h"
#include "achievementSelection.h"
#include "chooseRewardCustomEditor.h"
#include "chooseRewardXMLCustomEditor.h"
#include "communityEmbeddedCustomEditor.h"
#include "traitEmbeddedCustomEditor.h"
#include "traitListSelection.h"
#include "enumVariantSelection.h"
#include "entityTagsSelector.h"
#include "behTreePropertyEditor.h"
#include "itemSelectionDialogs/simpleStringSelector.h"
#include "directorySelectionEditor.h"
#include "sceneEmotionStatesSelection.h"
#include "curveResourceEditor.h"
#include "multiCurvePropertyEditor.h"
#include "mimicPoseSelector.h"
#include "idInterlocutorIDEditor.h"
#include "materialValueMappingItem.h"
#include "enumRefreshableEditor.h"
#include "dialogLineSelector.h"
#include "characterDBPropertyEditor.h"
#include "hardAttachmentBonePicker.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/xmlFile.h"
#include "lootNameSelectionEditor.h"
#include "playGoChunkSelector.h"
#include "languageSelector.h"

namespace // anonymous
{
	CGatheredResource resGameplayEntitiesTags( TXT("gameplay\\globals\\gameplay_tags.csv"), 0 );
	CGatheredResource resReactionFieldsTags( TXT("gameplay\\globals\\reactionfields.csv"), 0 );
	CGatheredResource resStatistics( TXT("gameplay\\globals\\statistics.csv"), 0 );
}

CPropertyItem::CPropertyItem( CEdPropertiesPage* page, CBasePropItem* parent )
	: CBasePropItem( page, parent )
	, m_arrayIndex( -1 )
	, m_isDetermined( false )
    , m_isDefaultValue( true )
	, m_ctrlChoice( nullptr )
	, m_ctrlText( nullptr )
	, m_ctrlColorPicker( nullptr )
	, m_textEditor( nullptr )
	, m_customEditor( nullptr )
	, m_propertyType( nullptr )
	, m_property( nullptr )
	, m_mouseMove( false )
	, m_isFocused( false )
{
}

void CPropertyItem::Init( IProperty* prop, Int32 arrayIndex /*= -1*/ )
{
	m_property = prop;
	m_propertyType = prop->GetType();
	m_arrayIndex = arrayIndex;

	Init();
}

void CPropertyItem::Init( IRTTIType *type, Int32 arrayIndex /*= -1*/ )
{
	m_property = NULL;
	m_propertyType = type;
	m_arrayIndex = arrayIndex;

	Init();
}

void CPropertyItem::Init()
{
	ERTTITypeType propertyType = m_propertyType->GetType();
	// Array and struct properties are expandable
	if ( ( propertyType == RT_Class || m_propertyType->IsArrayType() ) &&
		 ( GetCustomEditorType().Empty() || ( m_property && m_property->HasArrayCustomEditor() ) ) )
	{
		m_isExpandable = true;
	}

	// Create custom editor
	CreateCustomEditor();

	// Expand
	if ( m_isExpandable && m_page->GetSettings().m_autoExpandGroups )
	{
		Expand();
	}
}

CPropertyItem::~CPropertyItem()
{
	// Close popup text editor, if any
	if ( m_textEditor )
	{
		m_textEditor->SetHook( NULL );
		m_textEditor->Close();
		m_textEditor = NULL;
	}

	// Close editors attached to this property item
	CloseEditors();

	// Delete custom editor
	if ( m_customEditor )
	{
		delete m_customEditor;
		m_customEditor = NULL;
	}
}

String CPropertyItem::GetCaption() const
{
	if ( m_arrayIndex == -1 )
	{
		if ( !m_caption.Empty() )
			return m_caption;
		else
			return GetName().AsChar();
	}
	else
	{
		return String::Printf( TXT("%i"), m_arrayIndex );
	}
}

Int32 CPropertyItem::GetIndent() const
{
	return 0;
}

Int32 CPropertyItem::GetLocalIndent() const
{
	return m_parent->GetLocalIndent() + 10;
}

Int32 CPropertyItem::GetHeight() const
{
	return 19;
}

void CPropertyItem::DrawLayout( wxDC& dc )
{
	wxColour text( 0, 0, 0 );
	const wxColour textDis( 128, 128, 128 );
	const Int32 indent = m_parent->GetLocalIndent();

	CBasePropItem::DrawLayout( dc );

	// Use system highlighted text color if the item is selected
	if ( IsSelected() )
	{
		text = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT );
	}

	// Calculate the left rect area and clip drawing to it
	wxRect leftRect = m_rect;
	leftRect.width = m_page->GetSplitterPos() - leftRect.x;
	dc.DestroyClippingRegion();
	dc.SetClippingRegion( leftRect );		

	// Draw property caption
	String caption = GetCaption();
	wxSize extents = dc.GetTextExtent( caption.AsChar() );
	INT yCenter = ( m_rect.height - extents.y ) / 2;
	if ( !caption.Empty() )
	{
		// Draw caption text
		dc.SetTextForeground( text );
		dc.DrawText( caption.AsChar(), m_rect.x + indent + 15, m_rect.y + yCenter );
	}

	// Done drawing, remove clipping rect
	dc.DestroyClippingRegion();

	// Draw value
	{
		// Draw within a clipping region
		wxRect valueRect = CalcValueRect();
		dc.SetClippingRegion( valueRect );
		if ( IsSelected() )
		{
			// do not use the selection text color for non-grayed value text
			DrawValue( dc, valueRect, IsReadOnly() ? textDis : wxColour( 0, 0, 0 ) );
		}
		else
		{
			DrawValue( dc, valueRect, IsReadOnly() ? textDis : text );
		}
		dc.DestroyClippingRegion();
	}

	// If the caption doesn't fit in the caption rect and the mouse is over the rect
	// draw the caption over the value rect
	if ( !caption.Empty() && m_rect.x + indent + 15 + extents.GetWidth() > leftRect.GetWidth() )
	{
		wxPoint p = m_page->CalcUnscrolledPosition( m_page->ScreenToClient( wxGetMousePosition() ) );
		if ( leftRect.Contains( p ) )
		{
			wxColor bgnd;
			if ( IsSelected() )
			{
				bgnd = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
			}
			else
			{
				bgnd = wxColour( 255, 255, 255 );
			}
			dc.SetBrush( wxBrush( bgnd ) );
			dc.SetPen( wxPen( bgnd ) );
			dc.DrawRectangle( m_rect.x + indent + 15, m_rect.y, extents.GetWidth() + 8, m_rect.GetHeight() );
			dc.SetPen( wxPen( wxColour( 192, 192, 192 ) ) );
			dc.DrawLine( m_rect.x + indent + 15 + extents.GetWidth() + 8, m_rect.y, m_rect.x + indent + 15 + extents.GetWidth() + 8, m_rect.y + m_rect.GetHeight() );
			dc.SetTextForeground( text );
			dc.DrawText( caption.AsChar(), m_rect.x + indent + 15, m_rect.y + yCenter );
		}
	}

	// Draw buttons
	DrawButtons( dc );

	// Draw children
	DrawChildren( dc );
}

void CPropertyItem::Expand()
{
	// Needed to redraw properties browser
	CBasePropItem::Expand();
}

Bool CPropertyItem::CreateCustomEditor()
{
	ASSERT( m_customEditor == NULL );
	
	String editorType = GetCustomEditorType();

	// dont create editors for arrays with array custom editors
	if ( m_property && m_property->HasArrayCustomEditor() )
	{
		return false;
	}

	// Hack for tag list
	if ( m_propertyType && m_propertyType->GetName() == GetTypeName< TagList >() )
	{
		CTagPropertyEditor* tagCustomEditor = new CTagPropertyEditor( this );
		m_customEditor = tagCustomEditor;
		return true;
	}

	// Hack for EntityHandle
	if ( m_propertyType && m_propertyType->GetName() == GetTypeName< EntityHandle >() )
	{
		m_customEditor = new CEntityHandleEditor( this );
		return true;
	}

	if ( !m_customEditor )
	{
		if ( editorType == TXT("TextLanguageSelection") )
		{
			m_customEditor = new CEdLanguageSelector( this, CEdLanguageSelector::eText );
		}
		else if ( editorType == TXT("SpeechLanguageSelection") )
		{
			m_customEditor = new CEdLanguageSelector( this, CEdLanguageSelector::eSpeech );
		}
		if ( editorType == TXT("BehaviorAnimSelection") )
		{
			m_customEditor = new CBehaviorAnimSelection( this );
		} 
		else if ( editorType == TXT("BehaviorParentInputSelection") )
		{
			m_customEditor = new CBehaviorParentInputSelection( this );
		}	
		else if ( editorType == TXT("BehaviorParentValueInputSelection") )
		{
			m_customEditor = new CBehaviorParentValueInputSelection( this );
		}
		else if ( editorType == TXT("BehaviorParentVectorValueInputSelection") )
		{
			m_customEditor = new CBehaviorParentVectorValueInputSelection( this );
		}
		else if ( editorType == TXT("BehaviorMimicParentInputSelection") )
		{
			m_customEditor = new CBehaviorMimicParentInputSelection( this );
		}
		else if ( editorType == TXT("BehaviorParentValueInputTransitionConditionSelection") )
		{
			m_customEditor = new CBehaviorParentValueInputTransitionConditionSelection( this );
		}	
		else if ( editorType == TXT("BehaviorVariableSelection") )
		{
			m_customEditor = new CBehaviorVariableSelection( this, false );
		}	
		else if ( editorType == TXT("BehaviorInternalVariableSelection") )
		{
			m_customEditor = new CBehaviorVariableSelection( this, true );
		}
		else if ( editorType == TXT("BehaviorVectorVariableSelection") )
		{
			m_customEditor = new CBehaviorVectorVariableSelection( this, false );
		}
		else if ( editorType == TXT("BehaviorInternalVectorVariableSelection") )
		{
			m_customEditor = new CBehaviorVectorVariableSelection( this, true );
		}
		else if ( editorType == TXT("BehaviorEventEdition") )
		{
			m_customEditor = new CBehaviorEventSelection( this, true );
		}
		else if ( editorType == TXT("BehaviorEventSelection") )
		{
			m_customEditor = new CBehaviorEventSelection( this, false );
		}
		else if ( editorType == TXT("BehaviorNotificationSelection") )
		{
			m_customEditor = new CBehaviorNotificationSelection( this );
		}
		else if ( editorType == TXT("BehaviorBoneSelection") )
		{
			m_customEditor = new CBehaviorBoneSelection( this );
		}
		else if ( editorType == TXT("BehaviorTrackSelection") )
		{
			m_customEditor = new CBehaviorTrackSelection( this );
		}
		else if ( editorType == TXT("BehaviorBoneMultiSelection") )
		{
			m_customEditor = new CBehaviorBoneMultiSelection( this );
		}	
		else if ( editorType == TXT("BehaviorBoneMultiSelectionWithWeight") )
		{
			m_customEditor = new CBehaviorBoneMultiSelection( this, true );
		}
		else if ( editorType == TXT("DialogBodyPartSelection_GroupName") )
		{
			m_customEditor = new CDialogBodyPartSelection_GroupName( this );
		}
		else if ( editorType == TXT("DialogBodyPartSelection_Bones") )
		{
			m_customEditor = new CDialogBodyPartSelection_Bones( this );
		}
		else if ( editorType == TXT("SkeletonBoneSelection") )
		{
			m_customEditor = new CSkeletonBoneSelection( this );
		}
		else if ( editorType == TXT("TreeEditorButton") )
		{
			m_customEditor = new COpenTreePropertyEditor( this );
		}
		else if ( editorType == TXT("QuestBehaviorGraphSelection") )
		{
			m_customEditor = new CQuestBehaviorGraphSelection( this );
		}
		else if ( editorType == TXT("QuestBehaviorTagsSelection") )
		{
			m_customEditor = new CQuestBehaviorTagsSelection( this );
		}
		else if ( editorType == TXT("TagListEditor") )
		{
			m_customEditor = new CTagPropertyEditor( this );
		}	
		else if ( editorType == TXT("TaggedEntitySelector") )
		{
			m_customEditor = new CEntityOnLayerReferenceEditor( this );
		}	
		else if ( editorType == TXT("EntityClassList") )
		{
			m_customEditor = new CClassListSelection( this, ClassID< CEntity >() );
		}
		else if ( editorType == TXT("EnumList") )
		{
			m_customEditor = new CEnumListSelection( this );
		}
		else if ( editorType == TXT("EnumVariantSelection") )
		{
			m_customEditor = new CEnumVariantSelection( this );
		}
		else if ( editorType == TXT("SceneFunctionList") )
		{
			m_customEditor = new CFunctionListSelection( this, CFunctionListSelection::FT_SCENE );
		}
		else if ( editorType == TXT("QuestFunctionList") )
		{
			m_customEditor = new CFunctionListSelection( this, CFunctionListSelection::FT_QUEST );
		}
		else if ( editorType == TXT("RewardFunctionList") )
		{
			m_customEditor = new CFunctionListSelection( this, CFunctionListSelection::FT_REWARD );
		}
		else if ( editorType == TXT("RewardSelection") )
		{
#ifdef REWARD_EDITOR
			m_customEditor = new CEdChooseRewardCustomEditor( this );
#else
			m_customEditor = new CEdChooseRewardXMLCustomEditor( this );
#endif
		}
		else if ( editorType == TXT("EntityComponentListEffectParameters") )
		{
			m_customEditor = new CComponentListSelectionEffectParameters( this, CLF_Components );
		}
		else if ( editorType == TXT("EntityComponentList") )
		{
			m_customEditor = new CComponentListSelectionAll( this, CLF_Components );
		}
		else if ( editorType == TXT("EntityComponentAndSlotsList") )
		{
			m_customEditor = new CComponentListSelectionAll( this, CLF_Components|CLF_Slots );
		}
		else if ( editorType == TXT("EntityAnimComponentList") )
		{
			m_customEditor = new CComponentListSelectionComponentType< CAnimatedComponent >( this, CLF_Components );
		}
		else if ( editorType == TXT("EntityAppearanceSelect") )
		{
			m_customEditor = new CEntityAppearanceSelection( this );
		}
		else if ( editorType == TXT("EntityVoiceTagSelect") )
		{
			m_customEditor = new CEntityVoiceTagEditor( this );
		}
		else if ( editorType == TXT("ActionCategorySelect") )
		{
			m_customEditor = new CEdActionCategoryPropEditor( this );
		}
		else if ( editorType == TXT("WayPointsComponentList") )
		{
			m_customEditor = new CComponentListSelectionComponentType< CWayPointComponent >( this, CLF_Components );
		}
		else if ( editorType == TXT("EntityBoneList") )
		{
			m_customEditor = new CEffectBoneListSelection( this );
		}
		else if ( editorType == TXT("EffectParameterFloatList") )
		{
			m_customEditor = new CEffectParameterListSelection( this );
		}
		else if ( editorType == TXT("EffectParameterColorList") )
		{
			m_customEditor = new CEffectParameterColorListSelection( this );
		}
		else if ( editorType == TXT("GameTimePropertyEditor") )
		{
			m_customEditor = new CGameTimePropertyEditor( this, false );
		}
		else if ( editorType == TXT("DayTimeEditor") )
		{
			m_customEditor = new CGameTimePropertyEditor( this, true );
		}
		else if ( editorType == TXT("AudioEventBrowser") )
		{
			m_customEditor = new CAudioEventBrowserPropertyEditor( this );
		}
		else if ( editorType == TXT("AudioSwitchBrowser") )
		{
			m_customEditor = new CAudioSwitchBrowserPropertyEditor( this );
		}
		else if ( editorType == TXT("TypeList") )
		{
			m_customEditor = new CTypeListSelection( this );
		}
		else if ( editorType == TXT("VariantEditor") )
		{
			m_customEditor = new CVariantEditor( this );
		}
		else if ( editorType == TXT("VariantEnumEditor") )
		{
			m_customEditor = new CVariantEnumEditor( this );
		}
		else if ( editorType == TXT("2daValueSelection") )
		{
			m_customEditor = new C2dArrayValueSelection( this );
		}
		else if ( editorType == TXT("GameplayTagEditor") )
		{
			m_customEditor = new C2dArrayExclusiveValueEdition( this, true, resGameplayEntitiesTags );
		}
		else if ( editorType == TXT("ReactionFieldEditor") )
		{
			m_customEditor = new C2dArrayExclusiveValueEdition( this, false, resReactionFieldsTags );
		}
		else if ( editorType == TXT("StatisticsNamesEditor") )
		{
			m_customEditor = new C2dArrayExclusiveValueEdition( this, false, resStatistics );
		}
		else if (editorType == TXT("SpawnsetPhasesEditor") )
		{
			m_customEditor = new CSpawnsetPhasesEditor( this );
		}
		else if (editorType == TXT("EncounterEntitySelector") )
		{
			m_customEditor = new CEntityTagsSelector( CEncounter::GetStaticClass(), this );
		}
		else if (editorType == TXT("EncounterPhasesEditor") )
		{
			m_customEditor = new CEncounterPhasesEditor( this );
		}
		else if ( editorType == TXT("CreatureDefinitionsEditor") )
		{
			m_customEditor = new CCreatureDefinitionsEditor( this );
		}
		else if ( editorType == TXT("Slider") )
		{
			m_customEditor = new CSlider( this );
		}
		else if ( editorType == TXT("StickerPropertyEditor") )
		{
			m_customEditor = new CStringPropertyEditor( this, TXT("Sticker message"), TXT("Write sticker message") );
		}
		else if ( editorType == TXT("VoiceTagListEditor") )
		{
			m_customEditor = new CVoiceTagListSelection( this );
		}
		else if ( editorType == TXT("CurveSelection") )
		{
			m_customEditor = new CCurvePropertyEditor( this );
		}
		else if ( editorType == TXT("BaseCurveDataEditor") )
		{
			m_customEditor = new CBaseCurveDataPropertyEditor( this );
		}
		else if ( editorType == TXT("VoiceCurveDataEditor") )
		{
			m_customEditor = new CVoiceCurveDataPropertyEditor( this );
		}
		else if ( editorType == TXT("AppearanceEditor") )
		{
			m_customEditor = new CAppearanceListSelection( this );
		}
		else if ( editorType == TXT("TextureGroupList") )
		{
			m_customEditor = new CTextureGroupSelectionList( this );
		}
		else if ( editorType == TXT( "LayerGUIDSelection" ) )
		{
			m_customEditor = new CLayerGUIDListSelection( this );
		}
		else if ( editorType == TXT( "ItemSelection" ) )
		{
			m_customEditor = new CItemListSelection( this );
		}
		else if ( editorType == TXT( "SuggestedListSelection" ) )
		{
			m_customEditor = new CSuggestedListSelection( this );
		}
		else if ( editorType == TXT( "SelfSuggestedListSelection" ) )
		{
			m_customEditor = new CAnimEvent_SuggestedListSelection( this );
		}
		else if ( editorType == TXT( "AbilitySelection" ) )
		{
			m_customEditor = new CAbilityListSelection( this );
		}
		else if ( editorType == TXT( "EffectSelection" ) )
		{
			m_customEditor = new CEffectsListSelection( this );
		}
		else if ( editorType == TXT( "jobActionAnimSelection" ) )
		{
			m_customEditor = new CJobActionAnimSelection( this );
		}
		else if ( editorType == TXT("LayerGroupTree") )
		{
			m_customEditor = new CEdLayerGroupTreeSelection( this );
		}
		else if ( editorType == TXT("LayerGroupList") )
		{
			m_customEditor = new CEdLayerGroupListSelection( this );
		}
		else if ( editorType == TXT( "JournalPropertyBrowserQuest" ) )
		{
			m_customEditor = new CEdJournalPropertySelector( this, JOURNAL_SELECTOR_QUESTS );
		}
		else if ( editorType == TXT( "JournalPropertyBrowserCreatureVitalSpot" ) )
		{
			m_customEditor = new CEdJournalPropertySelector( this, JOURNAL_SELECTOR_CREATURES, ClassID< CJournalCreatureVitalSpotEntry >() );
		}
		else if ( editorType == TXT( "JournalPropertyBrowserCreatureVitalSpotGroup" ) )
		{
			m_customEditor = new CEdJournalPropertySelector( this, JOURNAL_SELECTOR_CREATURES, ClassID< CJournalCreatureVitalSpotGroup >() );
		}
		else if ( editorType == TXT( "JournalPropertyBrowserQuest_Quest" ) )
		{
			m_customEditor = new CEdJournalPropertySelector( this, JOURNAL_SELECTOR_QUESTS, ClassID< CJournalQuest >() );
		}
		else if ( editorType == TXT( "JournalPropertyBrowserQuest_Objective" ) )
		{
			m_customEditor = new CEdJournalPropertySelector( this, JOURNAL_SELECTOR_QUESTS, ClassID< CJournalQuestObjective >() );
		}
		else if ( editorType == TXT( "JournalPropertyBrowserQuest_Mappin" ) )
		{
			m_customEditor = new CEdJournalPropertySelector( this, JOURNAL_SELECTOR_QUESTS, ClassID< CJournalQuestMapPin >() );
		}
		else if ( editorType == TXT( "JournalPropertyBrowserPlace" ) )
		{
			m_customEditor = new CEdJournalPropertySelector( this, JOURNAL_SELECTOR_PLACES );
		}
		else if ( editorType == TXT( "JournalPropertyBrowserNonQuest" ) )
		{
			m_customEditor = new CEdJournalPropertySelector( this, JOURNAL_SELECTOR_ALL_BUT_QUESTS );
		}
		else if ( editorType == TXT( "JournalPropertyBrowserAll" ) )
		{
			m_customEditor = new CEdJournalPropertySelector( this, JOURNAL_SELECTOR_ALL );
		}
		else if ( editorType == TXT( "HuntingClue" ) )
		{
			m_customEditor = new CEdJournalHuntingClueSelector( this );
		}
		else if ( editorType == TXT( "LocalizedStringEditor" ) )
		{
			m_customEditor = new CLocalizedStringEditor( this, false );
		}
		else if ( editorType == TXT( "LocalizedStringEditorIdReset" ) )
		{
			m_customEditor = new CLocalizedStringEditor( this, true );
		}
		else if ( editorType == TXT( "EntityDisplayNameSelector" ) )
		{
			m_customEditor = new CEdLocalizedStringPropertyEditorReadOnly( this, TXT( "Entity display name" ) );
		}
		else if ( editorType.BeginsWith( TXT("Dialog") ) && editorType.EndsWith( TXT("Tag") ) )
		{
			Int32 prop = 0;			
			if( editorType.ContainsSubstring( TXT("Voice") ) )
				prop = prop | AT_ACTOR;
			if( editorType.ContainsSubstring( TXT("Prop") ) )
				prop = prop | AT_PROP;
			if( editorType.ContainsSubstring( TXT("Effect") ) )
				prop = prop | AT_EFFECT;
			if( editorType.ContainsSubstring( TXT("Light") ) )
				prop = prop | AT_LIGHT;
			m_customEditor = new CEdDialogEditorActorPropertyEditor( this, prop );
		}
		else if ( editorType == TXT( "DialogActorVoiceTagFromEntity" ) )
		{
			m_customEditor = new CEdDialogEditorActorVoiceTagEditor( this );
		}
		else if ( editorType == TXT( "CDialogLineSelector" ) )
		{
			m_customEditor = new CDialogLineSelector( this );
		}
		else if ( editorType == TXT( "DialogEnterActor" ) )
		{
			m_customEditor = new CEdDialogEnterAnimationSelection( this );
		}
		else if ( editorType == TXT( "DialogExitActor" ) )
		{
			m_customEditor = new CEdDialogExitAnimationSelection( this );
		}
		else if ( editorType == TXT( "DialogSetting" ) )
		{
			m_customEditor = new CEdDialogEditorSettingPropertyEditor( this );
		}
		else if ( editorType == TXT( "ItemCategorySelection" ) )
		{
			m_customEditor = new CItemCategoriesListSelection( this );
		}
		else if ( editorType == TXT( "CutsceneActorEffect" ) )
		{
			m_customEditor = new CEdCutsceneActorEffectPropertyEditor( this );
		}
		else if ( editorType == TXT( "CutsceneEffect" ) )
		{
			m_customEditor = new CEdCutsceneEffectPropertyEditor( this );
		}
		else if ( editorType == TXT( "CameraShakeEnumList" ) )
		{
			m_customEditor = new CEdCameraShakeEnumPropertyEditor( this );
		}
		else if ( editorType == TXT( "SoundReverbEditor" ) )
		{
			m_customEditor = new CSoundReverbPropertyEditor( this );
		}
		else if ( editorType == TXT( "SoundBankEditor" ) )
		{
			m_customEditor = new CSoundBankBrowserPropertyEditor( this );
		}
		else if ( editorType == TXT( "SoundGameParamterEditor" ) )
		{
			m_customEditor = new CSoundGameParamterEditor( this );
		}
		
		else if ( editorType == TXT( "DialogsetCameraShotName" ) )
		{
			m_customEditor = new CEdDialogsetCameraShotNamePropertyEditor( this );
		}
		else if ( editorType == TXT( "DialogsetCameraShotAnimation" ) )
		{
			m_customEditor = new CEdDialogsetCameraShotAnimationPropertyEditor( this );
		}
		else if ( editorType == TXT( "DialogsetCameraNumber" ) )
		{
			m_customEditor = new CEdDialogsetCameraNumberPropertyEditor( this );
		}
		else if ( editorType == TXT( "DialogsetCharacterNumber" ) )
		{
			m_customEditor = new CEdDialogsetCharacterNumberPropertyEditor( this );
		}
		else if ( editorType == TXT( "DialogAnimationSelection" ) )
		{
			m_customEditor = new CEdDialogAnimationSelection( this );
		}
		else if ( editorType == TXT( "DialogMimicAnimationSelection" ) )
		{
			m_customEditor = new CEdDialogMimicAnimationSelection( this );
		}
		else if ( editorType == TXT( "DialogCameraAnimationSelection" ) )
		{
			m_customEditor = new CEdDialogCameraAnimationSelection( this );
		}
		else if ( editorType == TXT( "CameraDefinitionDialogsetSlot" ) )
		{
			m_customEditor = new CEdDialogEditorChangeSlotPropertyEditor( this );
		}
		else if ( editorType == TXT( "SoundMaterialSelector" ) )
		{
			m_customEditor = new CEdSoundMaterialSelector( this );
		}
		else if ( editorType == TXT( "SceneInputSelector" ) )
		{
			m_customEditor = new CEdSceneInputSelector( this );
		}
		else if ( editorType.BeginsWith( TXT( "ScriptedEnum_" ) ) )
		{
			m_customEditor = new CEdScriptedEnumPropertyEditor( this );
		}
		else if ( editorType == TXT( "SlotBoneList" ) )
		{
			m_customEditor = new CSlotBoneListSelection( this );
		}
		//else if ( editorType == TXT( "InterlocutorIDList" ) )
		//{
		//	m_customEditor = new CIDInterlocutorIDEditor( this );
		//}
	}

	// NOTE: This break is here because the if/else if branching is too deep, compiler complains if these are all together.
	if ( !m_customEditor )
	{
		if ( editorType == TXT("AnimatedPropertyName") )
		{
			m_customEditor = new CAnimatedPropertyEditor( this );
		}
		else if ( editorType == TXT("MultiCurveEditor") )
		{
			m_customEditor = new CMultiCurvePropertyEditor( this );
			m_isExpandable = true;
		}
		else if ( editorType == TXT("MultiCurveEditor2D") )
		{
			m_customEditor = new CMultiCurvePropertyEditor( this, CMultiCurvePropertyEditor::EditMode_2D );
			m_isExpandable = true;
		}
		else if ( editorType == TXT("MultiCurveEditor3D") )
		{
			m_customEditor = new CMultiCurvePropertyEditor( this, CMultiCurvePropertyEditor::EditMode_3D );
			m_isExpandable = true;
		}
		else if ( editorType == TXT( "DialogBodyAnimation_Status" ) )
		{
			m_customEditor = new CEdDialogBodyAnimationFilter( this, CStorySceneAnimationList::LEVEL_BODY_STATUS );
		}
		else if ( editorType == TXT( "DialogBodyAnimation_EmotionalState" ) )
		{
			m_customEditor = new CEdDialogBodyAnimationFilter( this, CStorySceneAnimationList::LEVEL_BODY_EMOTIONAL_STATE );
		}
		else if ( editorType == TXT( "DialogBodyAnimation_Pose" ) )
		{
			m_customEditor = new CEdDialogBodyAnimationFilter( this, CStorySceneAnimationList::LEVEL_BODY_POSE );
		}
		else if ( editorType == TXT( "DialogBodyAnimation_Type" ) )
		{
			m_customEditor = new CEdDialogBodyAnimationFilter( this, CStorySceneAnimationList::LEVEL_BODY_TYPE );
		}
		else if ( editorType == TXT( "DialogBodyAnimation_FriendlyName" ) )
		{
			m_customEditor = new CEdDialogBodyAnimationFilter( this, CStorySceneAnimationList::LEVEL_BODY_ANIMATIONS );
		}
		else if ( editorType == TXT( "DialogBodyAnimation_Transition" ) )
		{
			m_customEditor = new CEdDialogBodyAnimationFilter( this, CStorySceneAnimationList::LEVEL_BODY_TRANSITION );
		}
		else if ( editorType == TXT( "DialogMimicsAnimation_ActionType" ) )
		{
			m_customEditor = new CEdDialogMimicsAnimationFilter( this, CStorySceneAnimationList::LEVEL_MIMICS_ACTION_TYPE );
		}
		else if ( editorType == TXT( "DialogMimicsAnimation_FriendlyName" ) )
		{
			m_customEditor = new CEdDialogMimicsAnimationFilter( this, CStorySceneAnimationList::LEVEL_MIMICS_ANIMATIONS );
		}
		else if ( editorType == TXT( "DialogMimicsAnimation_EmotionalState" ) )
		{
			m_customEditor = new CEdDialogMimicsAnimationFilter( this, CStorySceneAnimationList::LEVEL_MIMICS_EMOTIONAL_STATE );
		}
		else if ( editorType == TXT( "DialogMimicsAnimation_LayerEyes" ) )
		{
			m_customEditor = new CEdDialogMimicsAnimationFilter( this, CStorySceneAnimationList::LEVEL_MIMICS_LAYER_EYES );
		}
		else if ( editorType == TXT( "DialogMimicsAnimation_LayerPose" ) )
		{
			m_customEditor = new CEdDialogMimicsAnimationFilter( this, CStorySceneAnimationList::LEVEL_MIMICS_LAYER_POSE );
		}
		else if ( editorType == TXT( "DialogMimicsAnimation_LayerAnimation" ) )
		{
			m_customEditor = new CEdDialogMimicsAnimationFilter( this, CStorySceneAnimationList::LEVEL_MIMICS_LAYER_ANIMATION );
		}
		else if ( editorType == TXT("HudClassList") )
		{
			m_customEditor = new CClassListSelection( this, ClassID< CHud >() );
		}
		else if ( editorType == TXT("MenuClassList") )
		{
			m_customEditor = new CClassListSelection( this, ClassID< CMenu >() );
		}
		else if ( editorType == TXT("HudModuleClassList") )
		{
			m_customEditor = new CClassListSelection( this, ClassID< CHudModule >() );
		}
		else if ( editorType == TXT("PopupClassList") )
		{
			m_customEditor = new CClassListSelection( this, ClassID< CPopup >() );
		}
		else if ( editorType == TXT("WorldClassList") )
		{
			m_customEditor = new CClassListSelection( this, ClassID< CWorld >() );
		}
		else if ( editorType == TXT( "ChooseItem" ) )
		{
			m_customEditor = new CEdChooseItemCustomEditor( this );
		}
		else if ( editorType == TXT("GameplayMimicSelection") )
		{
			m_customEditor = new CEdGameplayMimicSelection( this );
		}
		else if ( editorType == TXT("GameInputSelection") )
		{
			m_customEditor = new CGameInputSelection( this );
		}
		else if( editorType == TXT( "CustomCameraSelector") )
		{
			m_customEditor = new CEdCustomCameraSelector(this);
		}
		else if ( editorType == TXT("PhysicalCollisionTypeSelector") )
		{
			m_customEditor = new CEdPhysicalCollisionTypeSelector( this );
		}
		else if ( editorType == TXT("PhysicalCollisionGroupSelector") )
		{
			m_customEditor = new CEdPhysicalCollisionGroupSelector( this );
		}
		else if ( editorType == TXT( "WorldSelection" ) )
		{
			m_customEditor = new CEdWorldSelectionEditor( this );
		}
		else if ( editorType == TXT( "WorldSelectionQuestBinding" ) )
		{
			m_customEditor = new CEdWorldSelectionQuestBindingEditor( this );
		}
		else if ( editorType == TXT( "CSVWorldSelection" ) )
		{
			m_customEditor = new CEdCSVWorldSelectionEditor( this );
		}
		else if ( editorType == TXT("AchievementSelection") )
		{
			m_customEditor = new CEdAchievementSelection( this );
		}
		else if ( editorType == TXT( "EmbeddedCommunity" ) )
		{
			m_customEditor = new CEdEmbeddedCommunityCustomEditor( this );
		}
		else if ( editorType == TXT( "DirectorySelectionEditor" ) )
		{
			m_customEditor = new CEdDirectorySelectionEditor( this );
		}
		else if ( editorType == TXT( "SceneMimicPoseSelection" ) )
		{
			m_customEditor = new CEdSceneMimicPoseSelector( this, false );
		}
		else if ( editorType == TXT( "SceneMimicFilterSelection" ) )
		{
			m_customEditor = new CEdSceneMimicPoseSelector( this, true );
		}
		else if ( editorType == TXT( "MaterialValueMapping" ) )
		{
			m_customEditor = new CEdMaterialValueMappingPropertyItem( this, false );
		}
		else if ( editorType == TXT( "MaterialValueMappingInlined" ) )
		{
			m_customEditor = new CEdMaterialValueMappingPropertyItem( this, true );
		}
		else if ( editorType == TXT("EdEnumRefreshableEditor") )
		{
			m_customEditor = new CEdEnumRefreshableEditor( this );
		}
		//else if ( editorType == TXT( "EmbeddedTrait" ) )
		//{
		//	m_customEditor = new CEdTraitEmbeddedCustomEditor( this );
		//}
		else  if ( editorType == TXT( "TraitNameEditor" ) )
		{
			m_customEditor = new CTraitListSelection( this );
		}
		else  if ( editorType == TXT( "SkillNameEditor" ) )
		{
			m_customEditor = new CSkillListSelection( this );
		}
		else if ( editorType == TXT( "CharacterEditor" ) )
		{
			m_customEditor = new CCharacterDBPropertyEditor( this );
		}
		else if ( editorType == TXT( "EdHardAttachmentBonePicker" ) )
		{
			m_customEditor = new CEdHardAttachmentBonePicker( this );
		}
		else if ( editorType == TXT("LootNameSelectionEditor") )
		{
			m_customEditor = new CLootNameSelectionEditor( this );
		}
		else if ( editorType == TXT("PlayGoChunkSelector") )
		{
			m_customEditor = new CEdPlayGoChunkSelector( this );
		}
	}

    return ( m_customEditor != NULL );
}

void CPropertyItem::CreateControls()
{
	if ( m_isFocused )
	{
		return;
	}

	m_isFocused = true;	

	// Array element
	if ( m_arrayIndex != -1 )
	{
		CPropertyItemArray* parentArray = (CPropertyItemArray*)m_parent;
		if ( parentArray->GetPropertyType() && parentArray->GetPropertyType()->IsArrayType() )
		{
			const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( parentArray->GetPropertyType() );
			if ( arrayType->ArrayIsResizable() )
			{
				// Add array element buttons
				AddButton( m_page->GetStyle().m_iconDelete, wxCommandEventHandler( CPropertyItem::OnArrayDeleteItem ) );
				AddButton( m_page->GetStyle().m_iconInsert, wxCommandEventHandler( CPropertyItem::OnArrayInsertItem ) );
			}
		}
	}

	if ( m_page->AllowGrabbing() && m_property && GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetSelectionManager() )
	{
		CSelectionManager* selectionManager = GGame->GetActiveWorld()->GetSelectionManager();
		TDynArray< CNode* > nodes;
		selectionManager->GetSelectedNodes( nodes );
		String propName = m_property->GetName().AsString();
		m_grabProperty = NULL;
		for ( Int32 i=nodes.SizeInt() - 1; m_grabProperty == NULL && i >= 0; --i ) // hack to check entity first, will show a popup eventually
		{
			CNode* node = nodes[i];
			TDynArray< CProperty* > properties;
			node->GetClass()->GetProperties( properties );
			for ( auto it=properties.Begin(); it != properties.End(); ++it )
			{
				CProperty* property = *it;
				if ( property->GetType() == m_propertyType )
				{
					m_grabFrom = THandle< CObject >( node );
					m_grabProperty = property;
					AddButton( m_page->GetStyle().m_iconGrab, wxCommandEventHandler( CPropertyItem::OnGrabProperty ) );
					break;
				}
			}
		}
	}

	ASSERT ( m_ctrlText == NULL && m_ctrlChoice == NULL );

	// Use custom editor if given
	if ( !m_customEditor )
	{
		CreateMainControl();

		// Grab property value and propagate it to edit controls
		GrabPropertyValue();

		// Connect KillFocus handlers to spawned control
		if ( m_ctrlText )
		{
			m_ctrlText->Bind( wxEVT_KILL_FOCUS, &CPropertyItem::OnEditorKillFocus, this );
		}

		if ( m_ctrlChoice )
		{
			m_ctrlChoice->Bind( wxEVT_KILL_FOCUS, &CPropertyItem::OnEditorKillFocus, this );
		}
	}
	else if ( !IsReadOnly() )
	{
		// Calculate placement, hacked !
		wxRect valueRect = CalcValueRect();
		wxPoint pos = m_page->CalcScrolledPosition( wxPoint( valueRect.x, valueRect.y ) );
		valueRect.y = pos.y;
		valueRect.x = pos.x;

		// Let the custom editor spawn controls
		TDynArray< wxControl* > spawnedControls;
		m_customEditor->CreateControls( valueRect, spawnedControls );

		for ( wxControl* ctrl : spawnedControls )
		{
			ctrl->Bind( wxEVT_KILL_FOCUS, &CPropertyItem::OnEditorKillFocus, this );
		}

		// Grab property value and propagate it to edit controls
		GrabPropertyValue();
	}

	// Do not create the transaction if there is no editor created, because then there will be no OnEditorKillFocus attached,
	// and the transaction will be left open after switching to another widow, etc.
	if ( m_customEditor || m_ctrlText || m_ctrlChoice || m_ctrlColorPicker )
	{
		ASSERT( !m_transaction );
		// bEEf no global transaction on bool properties, the event should be sent just on clicking, not on the focus loose
		if ( m_transaction == NULL && m_propertyType->GetName() != RED_NAME( Bool ) )
		{
			m_transaction.Reset( new CPropertyTransaction( *m_page ) );
		}
	}
}

void CPropertyItem::CreateMainControl()
{
	if ( m_propertyType->GetName() == RED_NAME( Color ) )
	{
		// == COLOR ==
		//AddButton( m_page->GetStyle().m_iconPick,   wxCommandEventHandler( CPropertyItemClass::OnColorPick ) );
		AddButton( m_page->GetStyle().m_iconBrowse, wxCommandEventHandler( CPropertyItem::OnColorOpenBrowser )  );

		m_ctrlColorPicker = new CEdColorPicker( m_page );
 		m_ctrlColorPicker->Bind( wxEVT_COMMAND_SCROLLBAR_UPDATED, &CPropertyItem::OnColorPicked, this );
	}
	else if ( !IsInlined() && ( m_propertyType->GetName() != RED_NAME( Bool ) ) )
	{
		// Add popup text editor button
		AddButton( m_page->GetStyle().m_iconDots, wxCommandEventHandler( CPropertyItem::OnShowTextEditor ) );

		// Calculate placement, hacked !
		wxRect valueRect = CalcValueRect();
		wxPoint pos = m_page->CalcScrolledPosition( wxPoint( valueRect.x, valueRect.y ) );
		valueRect.y = pos.y + 3;
		valueRect.height -= 3;
		valueRect.x = pos.x + 2;
		valueRect.width -= 2;

		// Create text editor
	    m_ctrlText = new wxTextCtrlEx( m_page, wxID_ANY, m_displayValue.AsChar(), valueRect.GetTopLeft(), valueRect.GetSize(), wxNO_BORDER | wxTE_PROCESS_ENTER | ( IsReadOnly() ? wxTE_READONLY : 0 ));
		m_ctrlText->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CPropertyItem::OnEditKeyDown ), NULL, this );
		if ( !IsReadOnly() )
		{
			m_ctrlText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CPropertyItem::OnEditTextEnter ), NULL, this );
			m_ctrlText->Connect( wxEVT_MOTION, wxCommandEventHandler( CPropertyItem::OnEditTextMotion ), NULL, this );
		}
		m_ctrlText->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
		m_ctrlText->SetFont( m_page->GetStyle().m_drawFont );
		m_ctrlText->SetSelection( -1, -1 );
		m_ctrlText->SetFocus();
	}
	


	wxCommandEvent event( wxEVT_COMMAND_PROPERTY_SELECTED );
	event.SetClientData( GetProperty() );
	wxPostEvent( GetPage(), event );
}

void CPropertyItem::CloseControls()
{
	if ( !m_isFocused )
	{
		return;
	}

	m_isFocused = false;

	//LOG_EDITOR( TXT("CPropertyItem::LostFocus") );

	if ( !IsReadOnly() )
	{
		SavePropertyValue();
	}

	DestroyButtons();

	CloseEditors();

	// Close transaction
	if ( m_transaction )
	{
		GrabPropertyValue();
		m_transaction.Reset();
    }
}

void CPropertyItem::DrawValue( wxDC &dc, const wxRect& valueRect, const wxColour& textColor )
{
	INT labelShift = 0;
	wxColour textColorInternal(textColor);

	// Custom property
	if ( m_customEditor )
	{
		if ( m_customEditor->DrawValue( dc, valueRect, textColorInternal ) )
		{
			return;
		}
	}

	if ( m_propertyType->GetName() == RED_NAME( Color ) )
	{
		Color value;
		if ( Read( &value ) )
		{
			wxRect colorRect = valueRect;
			colorRect.Inflate( -2, -3 );

			// Draw color rect
			dc.SetPen( wxPen( wxColour(0,0,0) ) );
			dc.SetBrush( wxBrush( wxColour( value.R, value.G, value.B ) ) );
			dc.DrawRectangle( colorRect );
		}
	}
	else
	{
		// Boolean value, special case
		if ( m_propertyType->GetName() == RED_NAME( Bool ) )
		{
			wxRect rect = CalcCheckBoxRect();

			// Draw icon, a little hacked
			if ( m_displayValue == TEXT("true") && m_isDetermined )
			{
				dc.DrawBitmap( m_page->GetStyle().m_iconCheckOn, rect.x, rect.y, true );
			}
			else if ( m_displayValue == TEXT("false") && m_isDetermined )
			{
				dc.DrawBitmap( m_page->GetStyle().m_iconCheckOff, rect.x, rect.y, true );
			}
			else
			{
				dc.DrawBitmap( m_page->GetStyle().m_iconCheckGray, rect.x, rect.y, true );
			}

			// Shift text a little
			labelShift += rect.width + 5;
		}

		// Draw property value 
		if ( !m_ctrlText )
		{
			// Calculate placement
			String value = m_isDetermined ? m_displayValue : TXT("Many values");
			wxSize extents = dc.GetTextExtent( value.AsChar() );
			const INT yCenter = ( valueRect.height - extents.y ) / 2;
			int textPos = valueRect.x + 5 + labelShift;

			// If the value text doesn't fit in the value rect area and the mouse is inside
			// the rect area, then push the value in the caption rect area and fill a rectangle
			// to cover the caption
			if ( m_page->GetActiveItem() != this && extents.GetWidth() + textPos > m_rect.width )
			{
				wxPoint p = m_page->CalcUnscrolledPosition( m_page->ScreenToClient( wxGetMousePosition() ) );
				if ( valueRect.Contains( p ) )
				{
					dc.DestroyClippingRegion();
					textPos = m_rect.width - extents.GetWidth();
					dc.SetBrush( wxBrush( wxColour( 255, 255, 255 ) ) );
					if ( textPos - 5 >= 0 )
					{
						dc.DrawRectangle( textPos - 5, m_rect.y - 1, m_page->GetSize().GetWidth(), m_rect.height + 2);
					}
					else
					{
						dc.DrawRectangle( -1, m_rect.y - 1, m_page->GetSize().GetWidth() + 1, m_rect.height + 2);
					}
				}
			}

			// trim the text to avoid Windows bug where very large lines generate errors
			// when ui themes are enabled
			value = value.LeftString( 200 );

			// Draw caption text
			if ( m_isDefaultValue )
			{
				dc.SetTextForeground( textColorInternal );
				dc.DrawText( value.AsChar(), textPos, valueRect.y + yCenter );
			}
			else
			{
				wxFont oldFont = dc.GetFont();
				const CEdPropertiesDrawingStyle &drawingStyle = GetPage()->GetStyle();
				dc.SetFont( drawingStyle.m_boldFont );
				dc.SetTextForeground( textColorInternal );
				dc.DrawText( value.AsChar(), textPos, valueRect.y + yCenter );
				dc.SetFont( oldFont );
			}
		}
	}
}

void CPropertyItem::CloseEditors()
{
	// Destroy text editor control
	if ( m_ctrlText )
	{
		wxPendingDelete.Append( m_ctrlText );
		m_ctrlText = nullptr;
	}

	// Destroy choice list
	if ( m_ctrlChoice )
	{
		wxPendingDelete.Append( m_ctrlChoice );
		m_ctrlChoice = nullptr;
	}

	// Destroy color control
	if ( m_ctrlColorPicker )
	{
		wxPendingDelete.Append( m_ctrlColorPicker );
		m_ctrlColorPicker = nullptr;
	}

	// Destroy custom editor
	if ( m_customEditor )
	{
		m_customEditor->CloseControls();
	}
}

void CPropertyItem::OnTextEditorModified( CEdTextEditor* editor )
{
	ASSERT( editor == m_textEditor, TXT("The OnTextEditorModified event should come from the same editor as m_textEditor in the property item - somehow the m_textEditor is not up to date or an existing text editor was not destroyed. Now i will probably crash") );

	if ( m_ctrlText )
	{
		m_ctrlText->SetValue( editor->GetText().AsChar() );
	}
	SavePropertyValue();
}

void CPropertyItem::OnTextEditorClosed( CEdTextEditor* editor )
{
	ASSERT( editor == m_textEditor, TXT("The OnTextEditorClosed event should come from the same editor as m_textEditor in the property item - somehow the m_textEditor is not up to date or an existing text editor was not destroyed") );
	m_textEditor = NULL;
}

Bool CPropertyItem::Read( void *buffer, Int32 objectIndex /*= 0*/ )
{
	return CBasePropItem::ReadImp( this, buffer, objectIndex );
}

Bool CPropertyItem::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0*/ )
{
	// if you're hitting this asset there isn't proper subclass implementing read operation
	ASSERT( false );	
	return false;
}

Bool CPropertyItem::Write( void *buffer, Int32 objectIndex /*= 0*/ )
{
	return CBasePropItem::WriteImp( this, buffer, objectIndex );
}

Bool CPropertyItem::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0 */ )
{
	// if you're hitting this asset there isn't proper subclass implementing write operation
	ASSERT( m_property );	
	return false;
}

Bool CPropertyItem::SerializeXML( IXMLFile& file )
{
    if ( file.IsWriter() )
    {
        CPropertyDataBuffer buffer( m_propertyType );
        if ( !Read( buffer.Data(), 0 ) )
        {
            return false;
        }

        String value;
        if ( !m_propertyType->ToString( buffer.Data(), value ) )
        {
            return false;
        }

        String name = GetCaption();
        file.BeginNode( TXT("property") );        
        file.Attribute( TXT("name"), name );
        file.Attribute( TXT("value"), value );

        const TDynArray< CBasePropItem * > &children = GetChildren();
        for ( Uint32 i = 0; i < children.Size(); ++i )
        {
            if ( !children[i]->SerializeXML( file ) )
            {
                return false;
            }
        }

        file.EndNode();

        return true;
    }
    else
    {
        if ( !file.BeginNode( TXT("property") ) )
        {
            return false;
        }

        String name;
        file.Attribute( TXT("name"), name );
        if ( name != GetCaption() )
        {
            return false;
        }
        
        String value;
        file.Attribute( TXT("value"), value );

        if ( m_isExpandable && file.GetChildCount() > 0 )
        {
            if ( ! IsExpanded() )
            {
                Expand();
            }

            const TDynArray< CBasePropItem * > &children = GetChildren();
            ASSERT( !children.Empty() );
            for ( Uint32 i = 0; i < children.Size(); ++i )
            {
                if ( !children[i]->SerializeXML( file ) )
                {
                    return false;
                }
            }
        }
        else
        {
            CPropertyDataBuffer buffer( m_propertyType );
            if ( !m_propertyType->FromString( buffer.Data(), value ) )
            {
                return false;
            }

            if ( !Write( buffer.Data(), 0 ) )
            {
                return false;
            }
        }

        file.EndNode();
        return true;
    }

    return false;
}

STypedObject CPropertyItem::GetParentObject( Int32 objectIndex ) const
{
	if ( m_arrayIndex >= 0 )
	{
		CPropertyItemArray* parentItemArray = static_cast< CPropertyItemArray* >( m_parent );
		void* obj = parentItemArray->GetArrayElement( m_arrayIndex , objectIndex );		

		CRTTIArrayType* parentArrayType = static_cast< CRTTIArrayType* >( parentItemArray->GetPropertyType() );
		IRTTIType* type = parentArrayType->GetInnerType();

		return STypedObject( obj, type );
	}

	return m_parent->GetParentObject( objectIndex );
}

void CPropertyItem::GrabPropertyValue()
{
	// Reset display value
	m_initialValue = m_displayValue = TEXT("");

	// If we have custom editor use it
	if ( m_customEditor )
	{
		if ( m_customEditor->GrabValue( m_displayValue ) )
		{
			m_isDetermined = true;
            m_initialValue = m_displayValue;
			return;		
		}
	}

	// Read property for first object
	CPropertyDataBuffer buffer( m_propertyType );
	if ( !Read( buffer.Data(), 0 ) )
	{
		m_isDetermined = false;
		return;		
	}

	// Compare with other objects to see if property value is determined
	m_isDetermined = true;
	const Int32 numObjects = GetNumObjects();
	for ( Int32 i=1; i<numObjects; i++ )
	{
		CPropertyDataBuffer temp( m_propertyType );
		if ( !Read( temp.Data(), i ) )
		{
			m_isDetermined = false;
			break;
		}

		if ( !m_propertyType->Compare( temp.Data(), buffer.Data(), 0 ) )
		{
			// Value of this property differs between objects, property is not determined
			m_isDetermined = false;
			break;
		}
	}

    /* Uncomment the section below to enable bolding properties
    // Check if current propertyItem has a default value
    m_isDefaultValue = false;
    if ( m_isDetermined )
    {
        // Get the default value of property
        CPropertyDataBuffer temp( m_propertyType );
        if ( Read( temp.Data(), -1 ) )
        {
            m_isDefaultValue = m_propertyType->Compare( buffer.Data(), temp.Data(), 0 );
        }
        else
        {
            //m_isDefaultValue = true;
        }
    }*/

    // If property is determined, get the value
	if ( m_isDetermined )
	{
		// Just use the string representation of data
		m_propertyType->ToString( buffer.Data(), m_displayValue );
        m_initialValue = m_displayValue;
	}

	if ( m_ctrlText )
	{
		m_ctrlText->SetValue( m_displayValue.AsChar() );
		m_ctrlText->SetSelection( -1, -1 );
	}

	// We have a choice list, copy the value
	if ( m_ctrlChoice )
	{
		if ( m_isDetermined )
		{
			// Select active item
			INT itemIndex = m_ctrlChoice->FindString( m_displayValue.AsChar(), false );
			m_ctrlChoice->SetSelection( itemIndex );
		}
		else
		{
			// Select nothing
			m_ctrlChoice->SetSelection( -1 );
		}
	}
}

void CPropertyItem::SavePropertyValue( Bool readFromCtrls /*=true*/ )
{
	// Use current value as default
	String value = m_displayValue;

	// Use custom editor to save the value
	if ( m_customEditor )
	{
		if ( m_customEditor->SaveValue() )
		{
			GrabPropertyValue();
			return;
		}
	}

	// Read the true value from controls
	if ( readFromCtrls )
	{
		// Grab value from editor
		if ( m_ctrlText )
		{
			value = m_ctrlText->GetValue().wc_str();
		}
		else if ( m_ctrlChoice )
		{
			// Get text value
			value = m_ctrlChoice->GetValue().wc_str();
		}
	}

    if ( m_isDetermined && m_initialValue == value )
        return; // do not save if value hasn't been changed

	// Import value
	CPropertyDataBuffer buffer( m_propertyType );
	// get rid of unsupported chars
	CStringSanitizer::SanitizeString( value );

	// apply range limit
	if ( m_propertyType->GetType() == RT_Fundamental && m_propertyType->GetName() != RED_NAME( Bool ) )
	{
		Float numValue;
		if ( FromString< Float >( value, numValue ) )
		{
			if( m_property )
			{
				numValue = Clamp< Float >( numValue, m_property->GetRangeMin(), m_property->GetRangeMax() );
			}
		}
		value = ToString( numValue );
	}

	if ( m_propertyType->FromString( buffer.Data(), value ))
	{
        CPropertyTransaction transaction( *m_page );
		// For each object, write the property value
        const Int32 numObjects = GetNumObjects();
        for ( Int32 i = 0; i < numObjects; i++ )
        {
		    Write( buffer.Data(), i );
        }

		GrabPropertyValue();
	}
	else
	{
		WARN_EDITOR( TXT("Invalid value for property '%s'"), GetName().AsChar() );
		MessageBeep( MB_ICONERROR );
	}
}

void CPropertyItem::CyclePropertyValue()
{
	// Boolean, easy case
	if ( m_propertyType->GetName() == RED_NAME( Bool ) )
	{
		// Cycle between true and false
		if ( m_displayValue == TXT("true") )
		{
			m_displayValue = TXT("false");
		}
		else
		{
			m_displayValue = TXT("true");
		}

		// Save and regrab
		SavePropertyValue();
		GrabPropertyValue();
	}

	// Redraw
	m_page->Refresh( false );
}

wxRect CPropertyItem::CalcValueRect() const
{
	// Calculate value drawing rect
	wxRect valueRect = m_rect;
	valueRect.x = m_page->GetSplitterPos() + 1;
	valueRect.width = ( m_rect.x + m_rect.width ) - m_buttonsWidth - (valueRect.x+1);
	return valueRect;
}

wxRect CPropertyItem::CalcCheckBoxRect() const
{
	const INT size = 14;

	// Setup defaults
	wxRect checkRect;
	checkRect.x = m_page->GetSplitterPos() + 1 + 5;
	checkRect.width = size;

	// Center image vertically
	INT space = ( m_rect.height - size ) / 2;
	checkRect.y = m_rect.y + space + 1;
	checkRect.height = size;

	return checkRect;
}

void CPropertyItem::OnEditKeyDown( wxKeyEvent& event )
{
	// Navigation, hacked but code is safer that way because
	// we wont call lostFocus() when in the middle of message processing
	if ( event.GetKeyCode() == WXK_UP )
	{
		PostMessage( (HWND) m_page->GetHandle(), WM_KEYDOWN, VK_UP, 0 );
		return;
	}
	else if ( event.GetKeyCode() == WXK_DOWN )
	{
		PostMessage( (HWND) m_page->GetHandle(), WM_KEYDOWN, VK_DOWN, 0 );
		return;
	}

	// Escape, restore original value
	if ( event.GetKeyCode() == WXK_ESCAPE )
	{
		GrabPropertyValue();
		return;
	}

	// Allow text ctrl to process key
	event.Skip();
}

void CPropertyItem::OnEditTextEnter( wxCommandEvent& event )
{
	SavePropertyValue();
	FinishTransaction();
}

void CPropertyItem::OnChoiceSelected( wxCommandEvent& event )
{
	SavePropertyValue();
}

void CPropertyItem::OnEditTextMotion( wxCommandEvent& event )
{
	// Make sure we do not receive this event after we set m_ctrlText to NULL
	if ( !m_ctrlText )
	{
		return;
	}

	if( !wxGetMouseState().LeftIsDown() )
	{
		m_mouseMove = false;
	}
	else
	{
		if( !m_mouseMove )
		{
			m_mouseMove = true;
			m_mouseStart = wxGetMousePosition();
			m_mouseValue = m_displayValue;
		}
		else
		{
			OnMouseMoveValueChange();
		}
	}
}

void CPropertyItem::OnBrowserMouseEvent( wxMouseEvent& event )
{
	// Double click !
	if ( event.LeftDClick() )
	{
		// If we are not an expandable property use the double click to cycle property value, great for booleans
		if ( !m_isExpandable && !IsReadOnly() )
		{
			CyclePropertyValue();
			return;
		}
	}

	// Single left click
	if ( event.LeftDown() )
	{
		// Special hack for boolean properties
		if ( m_propertyType->GetName() == RED_NAME( Bool ) )
		{
			// If we clicked inside the check box rect then cycle boolean value
			if ( CalcCheckBoxRect().Contains( event.GetPosition() ) && !IsReadOnly() )
			{
				CyclePropertyValue();
			}
		}
		if( m_ctrlText )
		{
			m_mouseMove = true;
			m_mouseStart = wxGetMousePosition();
			m_mouseValue = m_displayValue;
			//m_ctrlText->CaptureMouse();
		}
	}

	if( !event.LeftIsDown() )
	{
		//if( m_mouseMove && m_ctrlText )
			//m_ctrlText->ReleaseMouse();
		m_mouseMove = false;
	}

	if( m_mouseMove && !IsReadOnly() && m_ctrlText )
	{
		OnMouseMoveValueChange();
	}

	// Process default events
	CBasePropItem::OnBrowserMouseEvent( event );
}

Float Snap( Float value, Float grid )
{
	if ( grid > 0.0f )
	{
		Float val = ( value > 0.0f ) ? ( value + grid * 0.5f ) : ( value - grid * 0.5f );
		return (Int32)( val / grid ) * grid;
	}
	else
	{
		return value;
	}
}

void CPropertyItem::OnMouseMoveValueChange()
{
	wxPoint p = wxGetMousePosition();

	Double diff = m_mouseStart.y - p.y;

	if( fabs( diff ) < 5 )
	{
		if( m_mouseValue != m_displayValue )
		{
			m_ctrlText->SetValue( m_mouseValue.AsChar() );
			SavePropertyValue();
			GrabPropertyValue();
			// Redraw
			m_page->Refresh( false );
		}

		m_ctrlText->SetSelection( 0, 999 );
		return;
	}

	if( diff < 0 )
		diff += 4;
	else
		diff -= 4;

	String newValue = m_displayValue;

	if
	(
		m_propertyType->GetName() == RED_NAME( Uint8 ) ||
		m_propertyType->GetName() == RED_NAME( Uint16 ) ||
		m_propertyType->GetName() == RED_NAME( Uint32 ) ||
		m_propertyType->GetName() == RED_NAME( Int8 ) ||
		m_propertyType->GetName() == RED_NAME( Int16 ) ||
		m_propertyType->GetName() == RED_NAME( Int32 )
	)
	{
		Int32 v;
		if( FromString( m_mouseValue, v ) )
		{
			Int32 fd = fabs( diff );
			Int32 fsign = diff < 0 ? -1 : 1;
			if( fd > 100 )
			{
				if( fd < 200 )
					diff = fsign * ( 100 + 3 * ( fd - 100 ) / 2 );
				else
					diff = fsign * ( 250 + 2 * ( fd - 200 ) );
			}
			v += diff;
			newValue = ToString( v );
		}
	}
	else if( m_propertyType->GetName() == RED_NAME( Float ) ||
			 m_propertyType->GetName() == RED_NAME( Double ) )
	{
		Double v;
		if( FromString( m_mouseValue, v ) )
		{
			Double fd = fabs( diff );
			Double fsign = diff < 0 ? -1.0f : 1.0f;
			Double snap = 0.01f;

			v = Snap( v, snap );

			if( fd < 10.0f )
				diff /= 100.0f; //0.1
			else
			{
				if( fd < 25.0f )
				{
					diff = fsign * ( 0.1f + ( fd - 10.0f ) / 20.0f ); //0.85
					snap = 0.05f;
				}
				else
				{
					if( fd < 50.0f )
					{
						diff = fsign * ( 0.85f + ( fd - 25.0f ) / 10.0f ); //3.35
						snap = 0.1f;
					}
					else
					{
						if( fd < 100.0f )
						{
							diff = fsign * ( 3.35f + ( fd - 50.0f ) / 5.0f );
							snap = 0.2f;
						}
						else
						if( fd < 150.0f )
						{
							diff = fsign * ( 13.35f + ( fd - 100.0f ) / 2.0f );
							snap = 0.5f;
						}
						else
						{
							diff = fsign * ( 38.35f + ( fd - 150.0f ) );
							snap = 1.0f;
						}
					}
				}
			}
			v += diff;
			v = Snap( v, snap );
			newValue = ToString( v );
		}
	}

	if( newValue != m_displayValue )
	{
		m_ctrlText->SetValue( newValue.AsChar() );
		SavePropertyValue();
		GrabPropertyValue();
		// Redraw
		m_page->Refresh( false );
	}
}

void CPropertyItem::OnShowTextEditor( wxCommandEvent& event )
{
	if ( m_textEditor )
	{
		if ( !m_textEditor->IsVisible() )
		{
			m_textEditor->Show();
		}
		m_textEditor->Raise();
		m_textEditor->SetFocus();
	}
	else
	{
		m_textEditor = new CEdTextEditor( wxTheFrame, TXT("Edit ") + GetCaption() );
		m_textEditor->SetText( String( m_ctrlText->GetValue().c_str() ) );
		m_textEditor->SetHook( this );
		m_textEditor->Show();
	}
}

void CPropertyItem::OnColorOpenBrowser( wxCommandEvent& event )
{
	Color value;
	if ( Read( &value ) )
	{
		ASSERT( m_ctrlColorPicker );
		m_ctrlColorPicker->Show( value );
	}
}

void CPropertyItem::OnColorPicked( wxCommandEvent& event )
{
	ASSERT( m_ctrlColorPicker );
	if ( m_ctrlColorPicker )
	{
		// Get color from picker
		Color value = m_ctrlColorPicker->GetColor();

		// Copy it to properties
		Write( &value );

		// Regrab values
		GrabPropertyValue();

		// Refresh color components editors if expanded
		if ( !m_children.Empty() )
		{
			for ( CBasePropItem* child : m_children[0]->GetChildren() )
			{
				if ( CPropertyItem* item = dynamic_cast< CPropertyItem* >( child ) )
				{
					item->GrabPropertyValue();
				}
			}
		}
	}
}

void CPropertyItem::OnBrowserKeyDown( wxKeyEvent& event )
{
	// Enter
	if ( event.GetKeyCode() == WXK_RETURN )
	{
		// Expand or cycle value
		if ( !m_isExpandable )
		{
			if ( !IsReadOnly() )
			{
				CyclePropertyValue();
			}
		}
		else
		{
			ToggleExpand();
		}
	}

	// Escape, restore original value
	if ( event.GetKeyCode() == WXK_ESCAPE )
	{
		GrabPropertyValue();
	}
}

void CPropertyItem::OnEditorKillFocus( wxFocusEvent& event )
{
	if ( wxWindow* newFocused = event.GetWindow() )
	{
		if ( m_textEditor )
		{
			if ( newFocused->GetParent() == m_textEditor )
			{
				// focus moved to the text editor owned by this item, keep text control open
				return;
			}
			else
			{
				// focus moved to "something else" - close the text editor (it would not work anyway)
				m_textEditor->SetHook( nullptr );
				m_textEditor->Close();
				m_textEditor = nullptr;
			}
		}

		if ( GetPage()->IsDescendant( newFocused ) )
		{
			// The window focus went into the descendant of the page, so it's either another property (handled by the manual call
			// to the LostFocus() from the page) or some sub-window of the editor (which should be ignored).
			return;
		}
	}

	// NOTE: Calling LostFocus directly won't clear the m_activeItem in the page object
	GetPage()->SelectItem( nullptr );
}

void CPropertyItem::OnArrayDeleteItem( wxCommandEvent& event )
{
	CPropertyItemArray* parentArray = (CPropertyItemArray*)m_parent;
	ASSERT( parentArray->GetPropertyType()->IsArrayType() );

	parentArray->OnArrayDeleteItem( m_arrayIndex );
}

void CPropertyItem::OnArrayInsertItem( wxCommandEvent& event )
{
	CPropertyItemArray* parentArray = (CPropertyItemArray*)m_parent;
	ASSERT( parentArray->GetPropertyType()->IsArrayType() );

	parentArray->OnArrayInsertItem( m_arrayIndex );
}

void CPropertyItem::OnGrabProperty( wxCommandEvent& event )
{
	// Get the object to grab from (and make sure we still have one)
	CObject* object = m_grabFrom.Get();
	if ( !object )
	{
		return;
	}

	// Grab the data from the source object
	void* buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, m_propertyType->GetSize() );
	m_grabProperty->Get( object, buffer );

	// Write the data to the object(s) we're editing
	CPropertyTransaction transaction( *m_page );
	const Int32 numObjects = GetNumObjects();
	for ( Int32 i=0; i < numObjects; ++i )
	{
		Write( buffer, i );
	}

	// Refresh editors
	GrabPropertyValue();

	// kthxbye
	RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );
}

Bool CPropertyItem::CanUseSelectedResource() const
{
    if ( ( m_property->GetType()->GetType() != RT_Pointer ) ||
		 ( ((CRTTIPointerType*)m_property->GetType())->GetPointedType()->GetName() != TXT("CObject") ) )
        return false;

    // Get object property and determine object class from that
    CRTTIPointerType* type = (CRTTIPointerType* )m_property->GetType();
	ASSERT( type->GetPointedType()->GetType() == RT_Class );
    CClass* objectClass = (CClass*)( type->GetPointedType() );

    // Is this a resource class
    if ( objectClass->IsBasedOn( ClassID< CResource >() ) )
    {
        String selectedResource;
        if ( GetActiveResource( selectedResource ) )
        {
            // Import value
            CPropertyDataBuffer buffer( m_property );
            if ( m_property->GetType()->FromString( buffer.Data(), selectedResource ))
                return true;
        }
    }

    // It's an entity, get it from active selection
    if ( objectClass->IsBasedOn( ClassID< CEntity >() ) )
    {
        CEntity* selectedEntity = GetSelectedEntity();
        if ( selectedEntity && selectedEntity->IsA( objectClass ) )
        {
			// Entity should be in the same layer as each edited object
            Bool sameLayer = true;
            for ( Int32 i=0; i<GetNumObjects(); i++ )
            {
				if ( CObject* root = GetRootObject( i ).AsObject() )
				{
					if ( selectedEntity->GetRoot() != root )
					{
						// Fuckup
						sameLayer = false;
						break;
					}
				}
			}

			// Valid selected entity, set it
			if ( sameLayer )
			{
				return true;
			}
		}
	}

    return false;
}


Bool CPropertyItem::IsReadOnly() const
{
	if ( m_property )
		return m_page->IsReadOnly( m_property ) || m_property->IsReadOnly();
	else
		return m_parent->IsReadOnly();
}

Bool CPropertyItem::IsInlined() const
{
	if ( m_property )
		return m_property->IsInlined();
	else
		return m_parent->IsInlined();
}

String CPropertyItem::GetName() const
{
	if ( m_property )
		return m_property->GetName().AsString();
	else
		return m_parent->GetName();
}

String CPropertyItem::GetCustomEditorType() const
{
	if ( m_property )
	{
		return m_property->GetCustomEditorType();
	}
	else
	{
		return m_parent->GetCustomEditorType();
	}
}

const SEdPropertiesPagePropertyStyle* CPropertyItem::GetPropertyStyle() const
{
	return m_property ? m_page->GetPropertyStyle( m_property->GetName() ) : NULL;
}

void CPropertyItem::FinishTransaction()
{
	if( m_transaction )
	{
		GrabPropertyValue();
		m_transaction.Reset();		// Reset without new transaction, so the page can update the property before new transaction is constructed
		m_transaction.Reset( new CPropertyTransaction( *m_page ) );		// Begin new transaction
	}
}
