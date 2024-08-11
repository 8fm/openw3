#include "build.h"
#include "undoDialogEditor.h"

#include "dialogEditorDialogsetPanel.h"
#include "dialogEditorLine.h"
#include "dialogEditor.h"
#include "dialogEditorPage.h"
#include "dialogEditorChoice.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneGraphBlock.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneChoiceLine.h"
#include "../../common/engine/graphContainer.h"

// ---------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoDialogSetExistance );

CUndoDialogSetExistance::CUndoDialogSetExistance( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet, Bool creation )
	: CUndoDialogSetStep( undoManager, panel, dialogSet )
	, m_creation( creation )
{
}

/*static*/ 
void CUndoDialogSetExistance::CreateCreationStep( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet )
{
	CUndoDialogSetExistance* step = new CUndoDialogSetExistance( undoManager, panel, dialogSet, true );
	step->PushStep();
}

void CUndoDialogSetExistance::CreateDeletionStep( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet )
{
	CUndoDialogSetExistance* step = new CUndoDialogSetExistance( undoManager, panel, dialogSet, false );
	step->PushStep();
}

void CUndoDialogSetExistance::DoStep( Bool creating )
{
	if ( creating )
	{
		m_editor->m_sceneEditor->HACK_GetStoryScene()->AddDialogsetInstance( m_dialogSet ); // sets the proper parent
		m_editor->UpdateDialogsetListNames();
	}
	else
	{
		m_dialogSet->SetParent( this );
		m_editor->m_sceneEditor->HACK_GetStoryScene()->RemoveDialogsetInstance( m_dialogSet->GetName() );
		m_editor->UpdateDialogsetList();
	}
}

/*virtual*/ 
void CUndoDialogSetExistance::DoUndo()
{
	DoStep( !m_creation );
}

/*virtual*/ 
void CUndoDialogSetExistance::DoRedo()
{
	DoStep( m_creation );
}

/*virtual*/ 
String CUndoDialogSetExistance::GetName()
{
	return m_creation ? TXT("Dialog set created") : TXT("Dialog set removed");
}

// ---------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoDialogSlotExistance );

CUndoDialogSlotExistance::CUndoDialogSlotExistance( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet, CStorySceneDialogsetSlot* slot, Bool creation )
	: CUndoDialogSetStep( undoManager, panel, dialogSet )
	, m_slot( slot )
	, m_creation( creation )
{
}

/*static*/ 
void CUndoDialogSlotExistance::CreateCreationStep( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet, CStorySceneDialogsetSlot* slot )
{
	CUndoDialogSlotExistance* step = new CUndoDialogSlotExistance( undoManager, panel, dialogSet, slot, true );
	step->PushStep();
}

void CUndoDialogSlotExistance::CreateDeletionStep( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet, CStorySceneDialogsetSlot* slot )
{
	CUndoDialogSlotExistance* step = new CUndoDialogSlotExistance( undoManager, panel, dialogSet, slot, false );
	step->PushStep();
}

void CUndoDialogSlotExistance::DoStep( Bool creating )
{
	if ( creating )
	{
		m_dialogSet->AddSlot( m_slot ); // sets the proper parent
		m_editor->UpdateSlotListNames( m_dialogSet );
	}
	else
	{
		m_slot->SetParent( this );
		m_dialogSet->RemoveSlot( m_slot->GetSlotName() );
		m_editor->UpdateSlotList( m_dialogSet );
	}
}

/*virtual*/ 
void CUndoDialogSlotExistance::DoUndo()
{
	DoStep( !m_creation );
}

/*virtual*/ 
void CUndoDialogSlotExistance::DoRedo()
{
	DoStep( m_creation );
}

/*virtual*/ 
String CUndoDialogSlotExistance::GetName()
{
	return m_creation ? TXT("Dialog set slot created") : TXT("Dialog set slot removed");
}

// --------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoDialogSectionMove );

CUndoDialogSectionMove::CUndoDialogSectionMove( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* panel, CEdSceneSectionPanel* sectionPanel, Bool up )
	: CUndoScreenplayStep( undoManager, panel )
	, m_sectionPanel( sectionPanel )
	, m_up( up )
{
}

/*static*/ 
void CUndoDialogSectionMove::CreateUpStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* panel, CEdSceneSectionPanel* sectionPanel )
{
	CUndoDialogSectionMove* step = new CUndoDialogSectionMove( undoManager, panel, sectionPanel, true );
	step->PushStep();
}

/*static*/ 
void CUndoDialogSectionMove::CreateDownStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* panel, CEdSceneSectionPanel* sectionPanel )
{
	CUndoDialogSectionMove* step = new CUndoDialogSectionMove( undoManager, panel, sectionPanel, false );
	step->PushStep();
}

void CUndoDialogSectionMove::DoStep( Bool up )
{
	m_editor->ShiftSectionPanel( m_sectionPanel, !up );
}

/*virtual*/ 
void CUndoDialogSectionMove::DoUndo()
{
	DoStep( !m_up );
}

/*virtual*/ 
void CUndoDialogSectionMove::DoRedo()
{
	DoStep( m_up );
}

/*virtual*/ 
String CUndoDialogSectionMove::GetName()
{
	return TXT("move section");
}

// ----------------------------

IMPLEMENT_ENGINE_CLASS( CUndoDialogTextChange );

CUndoDialogTextChange::CUndoDialogTextChange( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CObject* element, Type type )
	: CUndoScreenplayStep( undoManager, editor )
	, m_element( element )
	, m_type( type )
{
}

/*static*/ 
void CUndoDialogTextChange::PrepareStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CObject* element, Type type, int keyCode )
{
	CUndoDialogTextChange* step = undoManager.SafeGetStepToAdd< CUndoDialogTextChange >();

	Bool isSep = IsSeparator( keyCode );

	if ( !step )
	{
		IUndoStep* curStep = undoManager.GetCurrentStep();
		if ( isSep && curStep && curStep->IsA< CUndoDialogTextChange >() )
		{
			return;
		}

		step = new CUndoDialogTextChange( undoManager, editor, element, type );
		undoManager.SetStepToAdd( step );
		step->StoreValue();
	}
	else
	{
		ASSERT( step->m_element == element, L"Finalize the undo step for the previous line first" );
	}

	if ( isSep )
	{
		step->PushStep();
	}
}

/*static*/ 
void CUndoDialogTextChange::PrepareLineContentStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CAbstractStorySceneLine* element, int keyCode )
{
	PrepareStep( undoManager, editor, element, LINE_CONTENT, keyCode );
}

/*static*/ 
void CUndoDialogTextChange::PrepareLineCommentStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CAbstractStorySceneLine* element, int keyCode )
{
	PrepareStep( undoManager, editor, element, LINE_COMMENT, keyCode );
}

/*static*/ 
void CUndoDialogTextChange::PrepareLineVoiceTagStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CAbstractStorySceneLine* element, int keyCode )
{
	PrepareStep( undoManager, editor, element, LINE_VOICE_TAG, keyCode );
}

/*static*/ 
void CUndoDialogTextChange::PrepareCommentStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CStorySceneComment* element, int keyCode )
{
	PrepareStep( undoManager, editor, element, COMMENT, keyCode );
}

/*static*/ 
void CUndoDialogTextChange::PrepareScriptLineStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CStorySceneScriptLine* element, int keyCode )
{
	PrepareStep( undoManager, editor, element, SCRIPT_LINE, keyCode );
}

/*static*/ 
void CUndoDialogTextChange::PrepareChoiceLineStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CStorySceneChoiceLine* element, int keyCode )
{
	PrepareStep( undoManager, editor, element, CHOICE_LINE, keyCode );
}

/*static*/ 
void CUndoDialogTextChange::PrepareNameStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CStorySceneSection* section, int keyCode )
{
	PrepareStep( undoManager, editor, section, NAME, keyCode );
}

/*static*/ 
void CUndoDialogTextChange::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoDialogTextChange* step =  undoManager.SafeGetStepToAdd< CUndoDialogTextChange >() )
	{
		step->PushStep();
	}
}

/*static*/
Bool CUndoDialogTextChange::IsSeparator( int keyCode )
{
	return keyCode == ' ' || keyCode == '\n' || keyCode == '\t';
}

void CUndoDialogTextChange::StoreValue()
{
	switch ( m_type )
	{
	case LINE_CONTENT:
		m_value = static_cast< CAbstractStorySceneLine* >( m_element )->GetContent();
		break;

	case LINE_COMMENT:
		m_value = static_cast< CAbstractStorySceneLine* >( m_element )->GetComment();
		break;

	case LINE_VOICE_TAG:
		m_value = static_cast< CAbstractStorySceneLine* >( m_element )->GetVoiceTag().AsString();
		break;

	case COMMENT:
		m_value = static_cast< CStorySceneComment* >( m_element )->GetCommentText();
		break;

	case CHOICE_LINE:
		m_value = static_cast< CStorySceneChoiceLine* >( m_element )->GetChoiceLine();
		break;

	case NAME:
		m_value = static_cast< CStorySceneSection* >( m_element )->GetName();
		break;
	}
}

void CUndoDialogTextChange::RefreshPanel( CStorySceneElement* element )
{
	CEdSceneSectionPanel* sectionPanel = m_editor->FindSectionPanel( element->GetSection() );
	ASSERT ( sectionPanel != NULL );
	CEdStorySceneElementPanel* elementPanel = sectionPanel->GetPanelByElement( element );
	ASSERT ( elementPanel );
	elementPanel->RefreshData();
}

void CUndoDialogTextChange::DoStep()
{
	String valToRestore = m_value;
	StoreValue();

	switch ( m_type )
	{
	case LINE_CONTENT:
		{
			CAbstractStorySceneLine* element = static_cast< CAbstractStorySceneLine* >( m_element );
			element->SetContent( valToRestore );
			RefreshPanel( element );
		}
		break;

	case LINE_COMMENT:
		{
			CAbstractStorySceneLine* element = static_cast< CAbstractStorySceneLine* >( m_element );
			element->SetComment( valToRestore );
			RefreshPanel( element );
		}
		break;

	case LINE_VOICE_TAG:
		{
			CAbstractStorySceneLine* element = static_cast< CAbstractStorySceneLine* >( m_element );
			element->SetVoiceTag( CName( valToRestore ) );
			RefreshPanel( element );
		}
		break;

	case COMMENT:
		{
			CStorySceneComment* element = static_cast< CStorySceneComment* >( m_element );
			element->SetCommentText( valToRestore );
			RefreshPanel( element );
		}
		break;

	case CHOICE_LINE:
		{
			CStorySceneChoiceLine* element = static_cast< CStorySceneChoiceLine* >( m_element );
			element->SetChoiceLine( valToRestore );
			RefreshPanel( element->GetChoice() );
		}
		break;

	case NAME:
		{
			CStorySceneSection* section = static_cast< CStorySceneSection* >( m_element );
			section->SetName( valToRestore );
			m_editor->FindSectionPanel( section )->RefreshData();
		}
		break;
	}

}

/*virtual*/ 
void CUndoDialogTextChange::DoUndo()
{
	DoStep();
}

void CUndoDialogTextChange::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoDialogTextChange::GetName()
{
	String msg;

	switch ( m_type )
	{
	case LINE_CONTENT:
		msg = TXT("Script line");
		break;

	case LINE_COMMENT:
		msg = TXT("Script line comment");
		break;

	case LINE_VOICE_TAG:
		msg = TXT("Voice tag");
		break;

	case COMMENT:
		msg = TXT("Comment");
		break;

	case CHOICE_LINE:
		msg = TXT("Choice line");
		break;

	case NAME:
		msg = TXT("Section name");
		break;
	}

	return msg + TXT(" change");
}

// --------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoDialogGraphBlockExistance );

CUndoDialogGraphBlockExistance::CUndoDialogGraphBlockExistance( CEdUndoManager& undoManager, CEdSceneGraphEditor* graphEditor )
	: CUndoGraphBlockExistance( undoManager, graphEditor )
	, m_scene( nullptr )
{
}

/*static*/ 
CUndoDialogGraphBlockExistance* CUndoDialogGraphBlockExistance::PrepareStep( CEdUndoManager& undoManager, CEdSceneGraphEditor* graphEditor, CGraphBlock* block )
{
	CUndoDialogGraphBlockExistance* step =  undoManager.SafeGetStepToAdd< CUndoDialogGraphBlockExistance >();

	if ( !step )
	{
		step = new CUndoDialogGraphBlockExistance( undoManager, graphEditor );
		undoManager.SetStepToAdd( step );
	}

 	if ( CStorySceneGraphBlock* storyBlock = Cast< CStorySceneGraphBlock >( block ) )
	{
		CStoryScene* scene = storyBlock->GetControlPart()->GetScene(); 

		if ( step->m_scene.Get() == NULL )
		{
			step->m_scene = scene;
		}
		else
		{
			ASSERT ( step->m_scene == scene );
		}
	}

	return step;
}

/*static*/ 
void CUndoDialogGraphBlockExistance::PrepareCreationStep( CEdUndoManager& undoManager, CEdSceneGraphEditor* graphEditor, CGraphBlock* block )
{
	CUndoDialogGraphBlockExistance* step = PrepareStep( undoManager, graphEditor, block );
	step->DoPrepareCreationStep( block );
	step->StoreSectionIndexOn( step->m_createdBlocksSections, block );
}

/*static*/ 
void CUndoDialogGraphBlockExistance::PrepareDeletionStep( CEdUndoManager& undoManager, CEdSceneGraphEditor* graphEditor, CGraphBlock* block, DeleteStrategy strategy )
{
	CUndoDialogGraphBlockExistance* step = PrepareStep( undoManager, graphEditor, block );
	step->DoPrepareDeletionStep( block );
	step->StoreSectionIndexOn( step->m_deletedBlocksSections, block );

	if ( CStorySceneGraphBlock* storyBlock = Cast< CStorySceneGraphBlock >( block ) )
 	{
		switch ( strategy )
		{
		case EDITOR_DELETE_BLOCK:
			graphEditor->DeleteBlock( block );
			break;
		case GRAPH_REMOVE_BLOCK:
			graphEditor->GetGraph()->GraphRemoveBlock( block );
			break;
		case DO_NOT_DELETE:
			// nothing to do
			break;
		}
		// Changing the parent must go after DeleteBlock, the code in CStorySceneGraphBlock::OnDestroyed assumes it's a Scene
 		storyBlock->GetControlPart()->SetParent( step );
 	}
}

/*static*/ 
void CUndoDialogGraphBlockExistance::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoDialogGraphBlockExistance* step =  undoManager.SafeGetStepToAdd< CUndoDialogGraphBlockExistance >() )
	{
		ASSERT ( step->m_createdBlocks.Size() == step->m_createdBlocksSections.Size() );
		ASSERT ( step->m_deletedBlocks.Size() == step->m_deletedBlocksSections.Size() );
		step->PushStep();
	}
}

void CUndoDialogGraphBlockExistance::StoreSectionIndexOn( TDynArray< Int32 >& sections, CGraphBlock* block )
{
	Uint32 sectionIdx = -1;

	if ( CStorySceneGraphBlock* storyBlock = Cast< CStorySceneGraphBlock >( block ) )
	{
		CStorySceneControlPart* part = storyBlock->GetControlPart();

		if ( CStorySceneSection* section = Cast< CStorySceneSection >( part ) )
		{
			ASSERT ( m_scene.Get() != NULL );
			if ( m_scene.Get() )
			{
				sectionIdx = m_scene->GetSectionIndex( section );
			}
			ASSERT ( sectionIdx != -1 );
		}
	}

	// There should be an entry for every block in the step, non-StorySection ones will be filled with -1
	sections.PushBack( sectionIdx );
}

void CUndoDialogGraphBlockExistance::DoCreationOn( const TDynArray< Info >& infos, const TDynArray< Int32 >& sections )
{
	ASSERT ( infos.Size() == sections.Size() );

	for ( Uint32 i=0; i<infos.Size(); ++i )
	{
		if ( CStorySceneGraphBlock* storyBlock = Cast< CStorySceneGraphBlock >( infos[i].m_block ) )
		{
 			CStorySceneControlPart* part = storyBlock->GetControlPart();
			if ( CStoryScene* s = m_scene.Get() )
			{
				part->SetParent( s );
				s->AddControlPart( part );

				if ( CStorySceneSection* section = Cast< CStorySceneSection >( part ) )
				{
					s->AddSectionAtPosition( section, sections[i] );
				}
			}
		}
	}
}

void CUndoDialogGraphBlockExistance::DoDeletionOn( const TDynArray< Info >& infos )
{
	// Currently nothing has to be done, CStorySceneGraphBlock::OnDestroyed takes care of the rest (removing sections etc).
}

/*virtual*/ 
void CUndoDialogGraphBlockExistance::DoUndo()
{
	DoDeletionOn( m_createdBlocks );
	DoCreationOn( m_deletedBlocks, m_deletedBlocksSections );
	__super::DoUndo();
}

/*virtual*/ 
void CUndoDialogGraphBlockExistance::DoRedo()
{
	DoCreationOn( m_createdBlocks, m_createdBlocksSections );
	DoDeletionOn( m_deletedBlocks );
	__super::DoRedo();
}

// --------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoDialogElementExistance );

CUndoDialogElementExistance::Info::Info( CStorySceneElement* element, Uint32 index )
	: m_element( element ), m_index( index )
	, m_parent( element->GetParent() )
{
}

CUndoDialogElementExistance::CUndoDialogElementExistance( CEdUndoManager& undoManager, CStorySceneSection* section, CEdSceneEditorScreenplayPanel* editor )
	: CUndoScreenplayStep( undoManager, editor )
	, m_section( section )
{
}

/*static*/ 
CUndoDialogElementExistance* CUndoDialogElementExistance::PrepareStep( CEdUndoManager& undoManager, CEdSceneSectionPanel* panel )
{
	CUndoDialogElementExistance* step =  undoManager.SafeGetStepToAdd< CUndoDialogElementExistance >();

	CStorySceneSection* section = panel->GetSection();
	CEdSceneEditorScreenplayPanel* editor = panel->GetStorySceneEditor();

	if ( !step )
	{
		step = new CUndoDialogElementExistance( undoManager, section, editor );
		undoManager.SetStepToAdd( step );
	}
	else
	{
		ASSERT ( step->m_section == section );
	}

	return step;
}

/*static*/ 
void CUndoDialogElementExistance::PrepareCreationStep( CEdUndoManager& undoManager, CEdSceneSectionPanel* panel, CStorySceneElement* element, Uint32 index )
{
	CUndoDialogElementExistance* step = PrepareStep( undoManager, panel );
	step->m_createdElements.PushBack( Info( element, index ) );
}

/*static*/ 
void CUndoDialogElementExistance::PrepareDeletionStep( CEdUndoManager& undoManager, CEdSceneSectionPanel* panel, CStorySceneElement* element, Uint32 index )
{
	CUndoDialogElementExistance* step = PrepareStep( undoManager, panel );
	step->m_deletedElements.PushBack( Info( element, index ) );
	element->SetParent( step );
}

/*static*/ 
void CUndoDialogElementExistance::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoDialogElementExistance* step =  undoManager.SafeGetStepToAdd< CUndoDialogElementExistance >() )
	{
		step->PushStep();
	}
}


void CUndoDialogElementExistance::DoCreationOn( const TDynArray< Info >& infos )
{
	CEdSceneSectionPanel* panel = m_editor->FindSectionPanel( m_section );
	ASSERT ( panel != NULL );

	for ( auto infoIt = infos.Begin(); infoIt != infos.End(); ++infoIt )
	{
		m_editor->PreChangeSceneElements();
		{
			infoIt->m_element->SetParent( infoIt->m_parent );
			panel->CreateAndAddStorySceneElementPanel( infoIt->m_element, infoIt->m_index );
		}
		m_editor->PostChangeSceneElements( panel );
	}
}

void CUndoDialogElementExistance::DoDeletionOn( const TDynArray< Info >& infos )
{
	CEdSceneSectionPanel* panel = m_editor->FindSectionPanel( m_section );
	ASSERT ( panel != NULL );

	for ( auto infoIt = infos.Begin(); infoIt != infos.End(); ++infoIt )
	{
		infoIt->m_element->SetParent( this );
		panel->RemoveDialogElement( infoIt->m_index );
	}
}

/*virtual*/ 
void CUndoDialogElementExistance::DoUndo()
{
	DoDeletionOn( m_createdElements );
	DoCreationOn( m_deletedElements );
}

/*virtual*/ 
void CUndoDialogElementExistance::DoRedo()
{
	DoDeletionOn( m_deletedElements );
	DoCreationOn( m_createdElements );
}

/*virtual*/ 
String CUndoDialogElementExistance::GetName()
{
	if ( !m_createdElements.Empty() )
	{
		if ( !m_deletedElements.Empty() )
		{
			return TXT("creating and removing story scene elements");
		}
		else
		{
			return TXT("creating story scene element");
		}
	}
	else
	{
		return TXT("removing story scene element");
	}
}

// ------------------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoDialogChoiceLineExistance );

CUndoDialogChoiceLineExistance::Info::Info( CStorySceneChoiceLine* element, Uint32 index )
	: m_element( element ), m_index( index )
	, m_parent( element->GetParent() )
{
}

CUndoDialogChoiceLineExistance::CUndoDialogChoiceLineExistance( CEdUndoManager& undoManager, CStorySceneChoice* choice, CEdSceneEditorScreenplayPanel* editor )
	: CUndoScreenplayStep( undoManager, editor )
	, m_choice( choice )
{
}

/*static*/ 
CUndoDialogChoiceLineExistance* CUndoDialogChoiceLineExistance::PrepareStep( CEdUndoManager& undoManager, CEdStorySceneChoicePanel* panel )
{
	CUndoDialogChoiceLineExistance* step =  undoManager.SafeGetStepToAdd< CUndoDialogChoiceLineExistance >();

	CStorySceneChoice* choice = SafeCast< CStorySceneChoice >( panel->GetDialogElement() );
	CEdSceneEditorScreenplayPanel* editor = panel->GetSectionPanel()->GetStorySceneEditor();

	if ( !step )
	{
		step = new CUndoDialogChoiceLineExistance( undoManager, choice, editor );
		undoManager.SetStepToAdd( step );
	}
	else
	{
		ASSERT ( step->m_choice == choice );
	}

	return step;
}

/*static*/ 
void CUndoDialogChoiceLineExistance::PrepareCreationStep( CEdUndoManager& undoManager, CEdStorySceneChoicePanel* panel, CStorySceneChoiceLine* element, Uint32 index )
{
	CUndoDialogChoiceLineExistance* step = PrepareStep( undoManager, panel );
	step->m_createdElements.PushBack( Info( element, index ) );
}

/*static*/ 
void CUndoDialogChoiceLineExistance::PrepareDeletionStep( CEdUndoManager& undoManager, CEdStorySceneChoicePanel* panel, CStorySceneChoiceLine* element, Uint32 index )
{
	CUndoDialogChoiceLineExistance* step = PrepareStep( undoManager, panel );
	step->m_deletedElements.PushBack( Info( element, index ) );
	element->SetParent( step );
}

/*static*/ 
void CUndoDialogChoiceLineExistance::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoDialogChoiceLineExistance* step =  undoManager.SafeGetStepToAdd< CUndoDialogChoiceLineExistance >() )
	{
		step->PushStep();
	}
}

CEdStorySceneChoicePanel* CUndoDialogChoiceLineExistance::GetPanel() const
{
	CEdSceneSectionPanel* sectionPanel = m_editor->FindSectionPanel( m_choice->GetSection() );
	ASSERT ( sectionPanel != NULL );
	CEdStorySceneElementPanel* elementPanel = sectionPanel->GetPanelByElement( m_choice );
	CEdStorySceneChoicePanel* result = dynamic_cast< CEdStorySceneChoicePanel* >( elementPanel );
	ASSERT ( result );
	return result;
}

void CUndoDialogChoiceLineExistance::DoCreationOn( const TDynArray< Info >& infos )
{
	CEdStorySceneChoicePanel* panel = GetPanel();

	for ( auto infoIt = infos.Begin(); infoIt != infos.End(); ++infoIt )
	{
		infoIt->m_element->SetParent( infoIt->m_parent );
		m_choice->AddChoiceLine( infoIt->m_element, infoIt->m_index );
		panel->CreateAndAddChoiceLine( infoIt->m_index, infoIt->m_element );
	}
}

void CUndoDialogChoiceLineExistance::DoDeletionOn( const TDynArray< Info >& infos )
{
	CEdStorySceneChoicePanel* panel = GetPanel();

	for ( auto infoIt = infos.Begin(); infoIt != infos.End(); ++infoIt )
	{
		infoIt->m_element->SetParent( this );
		panel->RemoveChoiceLine( infoIt->m_index );
	}
}

/*virtual*/ 
void CUndoDialogChoiceLineExistance::DoUndo()
{
	DoDeletionOn( m_createdElements );
	DoCreationOn( m_deletedElements );
}

/*virtual*/ 
void CUndoDialogChoiceLineExistance::DoRedo()
{
	DoDeletionOn( m_deletedElements );
	DoCreationOn( m_createdElements );
}

/*virtual*/ 
String CUndoDialogChoiceLineExistance::GetName()
{
	if ( !m_createdElements.Empty() )
	{
		if ( !m_deletedElements.Empty() )
		{
			return TXT("creating and removing choice lines");
		}
		else
		{
			return TXT("creating choice line");
		}
	}
	else
	{
		return TXT("removing choice line");
	}
}
