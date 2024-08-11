/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "steeringGraphEditor.h"

#include "../../common/game/moveSteeringBehavior.h"
#include "../../common/game/moveSteeringNode.h"
#include "../../common/game/moveSteeringTask.h"
#include "../../common/game/moveSteeringCondition.h"
#include "../../common/game/moveSteeringLocomotionSegment.h"

#include "itemSelectionDialogs/mappedClassSelectorDialog.h"
#include "steeringEditor.h"
#include "popupNotification.h"

enum ESteeringGraphEditorEvents
{
	ID_MENU_GAME_RUNNING = 1000,
	ID_MENU_CHECK_OUT,
	ID_CREATE_BLOCK,
	ID_CREATE_BLOCK_TASK,
	ID_CREATE_BLOCK_GROUP,
	ID_CREATE_BLOCK_CONDITION,
	ID_STEER_SET_TASK,
	ID_STEER_SET_CONDITION,
	ID_STEER_COPY_BRANCH,
	ID_STEER_CUT_BRANCH,
	ID_STEER_PASTE_BRANCH,
	ID_AUTO_LAYOUT
};


BEGIN_EVENT_TABLE( CEdSteeringGraphEditor, CEdTreeEditor )
END_EVENT_TABLE()

IMoveSteeringNode*	CEdSteeringGraphEditor::s_copiedBranch = NULL;
Int32				CEdSteeringGraphEditor::s_numEditors = 0;

CEdSteeringGraphEditor::CEdSteeringGraphEditor( wxWindow* parent, IHook* hook )
: CEdTreeEditor( parent, hook, true )
, m_steering( NULL )
{
	s_numEditors++;

	// Collect allowed component classes
	class CNamer : public CClassHierarchyMapper::CClassNaming
	{
		void GetClassName( CClass* classId, String& outName ) const override
		{
			IMoveSteeringNode* defaultNode = classId->IsAbstract() ? NULL : classId->GetDefaultObject< IMoveSteeringNode >();
			if ( defaultNode )
			{
				outName = defaultNode->GetNodeName();
			}
			else if ( classId == IMoveSteeringNode::GetStaticClass() )
			{
				outName = TXT("Base");
			}
			else
			{
				outName = classId->GetName().AsString();
			}
		}
	} cNamer;

	CClassHierarchyMapper::MapHierarchy( ClassID< IMoveSteeringNode >(), m_blockClasses, cNamer );

	SEvents::GetInstance().RegisterListener( CNAME( GameStarted ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnded ), this );
}

CEdSteeringGraphEditor::~CEdSteeringGraphEditor()
{
	SEvents::GetInstance().UnregisterListener( this );

	s_numEditors--;

	if( s_numEditors <= 0 )
	{
		if( s_copiedBranch )
		{
			s_copiedBranch->RemoveFromRootSet();
			s_copiedBranch->Discard();
			s_copiedBranch = NULL;
		}
	}
}

void CEdSteeringGraphEditor::SetSteering( CMoveSteeringBehavior* steering )
{
	if ( m_steering == steering )
		return;

	m_steering = steering;

	// Cancel selection
	DeselectAllObjects( false );
	SetActiveItem( NULL );

	// Update layout
	ForceLayoutUpdate();

	// Scale
	ZoomExtents();
}

String CEdSteeringGraphEditor::GetBlockName( IScriptable & block ) const
{
	IMoveSteeringNode* node = SafeCast< IMoveSteeringNode >( &block );
	return node->GetNodeCaption();
}

String CEdSteeringGraphEditor::GetBlockComment( IScriptable & block ) const
{
	IMoveSteeringNode* node = SafeCast< IMoveSteeringNode >( &block );
	return node->GetComment();
}

Bool CEdSteeringGraphEditor::IsLocked( IScriptable & block ) const
{
	IMoveSteeringNode* node = SafeCast< IMoveSteeringNode >( &block );
	return GGame->IsActive() || !m_steering->CanModify() || IsNodeInDisabledBranch( node );
}

void CEdSteeringGraphEditor::DrawBlockLinks( LayoutInfo & layout )
{
	CEdTreeEditor::DrawBlockLinks( layout );

	static wxColour shadow( 40, 40, 40 );
	static wxColour textCol( 0, 150, 255 );
	static String trueStr = TXT("TRUE");
	static String falseStr = TXT("FALSE");

	IMoveSteeringNode* ownerNode = Cast< IMoveSteeringNode >( layout.m_owner );
	if ( !ownerNode )
	{
		return;
	}

	IMoveSNComposite* parentComposite = Cast< IMoveSNComposite >( ownerNode->GetParentNode() );
	if ( !parentComposite )
	{
		return;
	}

	Int32 childIdx = parentComposite->GetChildIdx( ownerNode );

	String childDesc;
	CMoveSNCondition* parentCond = Cast< CMoveSNCondition >( ownerNode->GetParentNode() );
	if ( parentCond )
	{
		String* condTxt;
		if ( parentCond->GetChildCount() >= 1 && childIdx == 0 )
		{
			condTxt = &trueStr;
		}
		else if ( parentCond->GetChildCount() == 2 && childIdx == 1 )
		{
			condTxt = &falseStr;
		}
		childDesc = String::Printf( TXT( "(%d)%s" ), childIdx, condTxt->AsChar() );
	}
	else
	{
		childDesc = String::Printf( TXT( "(%d)" ), childIdx );
	}

	wxPoint size = TextExtents( GetGdiBoldFont(), childDesc );
	wxPoint pt( layout.m_windowPos.x + layout.m_windowSize.x / 2 - size.x / 2, layout.m_windowPos.y - size.y - 1 );
	if ( !GetBlockComment( *ownerNode ).Empty() )
	{ // shift up to not overlap the comment
		pt.y -= size.y;
	}

	DrawText( pt, GetGdiBoldFont(), childDesc, shadow );
	DrawText( wxPoint( pt.x - 1, pt.y - 1 ), GetGdiBoldFont(), childDesc, textCol );
}

void CEdSteeringGraphEditor::PaintCanvas( Int32 width, Int32 height )
{
	CEdTreeEditor::PaintCanvas( width, height );

	const SMoveLocomotionGoal& goal = m_editor->GetActiveGoal();

	Float scale = GetScale();
	wxPoint offset = GetOffset();
	SetScale( 1.0f, false );
	SetOffset( wxPoint( 0, 0 ), false );

	Int32 y = 10;
	static Int32 fontHeight = 10;
	static Int32 boxWidth = 350;
	static Int32 borderWidth = 1;

	// movement stats
	const CMovingAgentComponent* agent = m_editor->GetActiveAgent();
	if ( agent )
	{
		wxRect rect( 5, y - 5, boxWidth, 7 * fontHeight + 5 );
		wxRect canvasRect = ClientToCanvas( rect );
		FillRect( canvasRect, wxColour(128, 120, 255, 40) );
		DrawRect( canvasRect, wxColour(255, 0, 0, 140), borderWidth );

		Vector worldPos = agent->GetWorldPosition();
		Float orientation = agent->GetWorldYaw();
		Vector velocity = agent->GetVelocity();
		Float speed = agent->GetAbsoluteMoveSpeed();
		Float maxSpeed = agent->GetMaxSpeed();

		Vector2 headingOutput( 0, 0 );
		Float headingImportance = -1.f;
		Float speedOutput = 0.f;
		Float speedImportance = -1.f;
		Float rotationOutput = 0.f;
		Float rotationImportance = -1.f;

		IMoveLocomotionSegment* segment = agent->GetLocomotion()->GetCurrentSegment();
		CMoveLSSteering* steeringSegment = segment ? segment->AsCMoveLSSteering() : NULL;
		if ( steeringSegment )
		{
			headingOutput = steeringSegment->GetHeading();
			headingImportance = steeringSegment->GetHeadingImportance();
			speedOutput = steeringSegment->GetSpeed();
			speedImportance = steeringSegment->GetSpeedImportance();
			rotationOutput = steeringSegment->GetRotation();
			rotationOutput = steeringSegment->GetRotationImportance();
		}
		


		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Position:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("[%.3f, %.3f, %3.f]"), worldPos.X, worldPos.Y, worldPos.Z ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Orientation:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), orientation ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Velocity:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("[%.3f, %.3f, %3.f]"), velocity.X, velocity.Y, velocity.Z ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Speed:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), speed ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Max speed:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), maxSpeed ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Heading output:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("[%.3f, %.3f]"), headingOutput.X, headingOutput.Y ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Heading importance:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), headingImportance ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Speed output:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), speedOutput ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Speed importance:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), speedImportance ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Rotation output:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), rotationOutput ).AsChar(), *wxWHITE );
		y += fontHeight;

		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Rotation importance:"), *wxGREEN );
		DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), rotationImportance ).AsChar(), *wxWHITE );
		y += fontHeight;

		y += 2*fontHeight;
	}

	// debug frames
	const TDynArray< TDynArray< String > >& debugFrames = m_editor->GetDebugFrames();
	for ( TDynArray< TDynArray< String > >::const_iterator framesIt = debugFrames.Begin(); framesIt != debugFrames.End(); ++framesIt )
	{
		wxRect rect( 5, y - 5, boxWidth, 7 * fontHeight + 5 );
		wxRect canvasRect = ClientToCanvas( rect );
		FillRect( canvasRect, wxColour(128, 120, 255, 40) );
		DrawRect( canvasRect, wxColour(255, 0, 0, 140), borderWidth );

		const TDynArray< String >& contents = *framesIt;
		Uint32 count = contents.Size();
		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), contents[0].AsChar(), *wxBLUE );
		y += fontHeight;
		y += fontHeight;

		for ( Uint32 i = 1; i < count; ++i )
		{
			DrawText( wxPoint( 10, y ), GetGdiBoldFont(), contents[i].AsChar(), *wxWHITE );
			y += fontHeight;
		}

		y += 2*fontHeight;
	}

	if ( !goal.IsSet() )
	{
		DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("NO GOAL SET"), *wxGREEN );

		y += 2*fontHeight;
	}
	else
	{
		if ( goal.IsHeadingGoalSet() )
		{
			wxRect rect( 5, y - 5, boxWidth, 7 * fontHeight + 5 );
			wxRect canvasRect = ClientToCanvas( rect );
			FillRect( canvasRect, wxColour(255, 255, 0, 40) );
			DrawRect( canvasRect, wxColour(255, 0, 0, 140), borderWidth );

			DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Heading to goal:"), *wxGREEN );
			DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("[%.3f, %.3f]"), goal.GetHeadingToGoal().X, goal.GetHeadingToGoal().Y ).AsChar(), *wxWHITE );
			y += fontHeight;

			DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Rotation to goal:"), *wxGREEN );
			DrawText( wxPoint( 150, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), goal.GetRotationToGoal() ).AsChar(), *wxWHITE );
			y += fontHeight;

			y += 2*fontHeight;
		}

		if ( goal.IsOrientationGoalSet() )
		{
			wxRect rect( 5, y - 5, boxWidth, 2 * fontHeight + 5 );
			wxRect canvasRect = ClientToCanvas( rect );
			FillRect( canvasRect, wxColour(255, 255, 0, 40) );
			DrawRect( canvasRect, wxColour(255, 0, 0, 140), borderWidth );

			DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Orientation diff:"), *wxGREEN );
			DrawText( wxPoint( 100, y ), GetGdiBoldFont(), String::Printf( TXT("%.3f"), goal.GetOrientationDiff() ).AsChar(), *wxWHITE );
			y += fontHeight;

			y += 2*fontHeight;
		}

		if ( !goal.m_expectedBehNotification.Empty() )
		{
			wxRect rect( 5, y - 5, boxWidth, 2 * fontHeight + 5 );
			wxRect canvasRect = ClientToCanvas( rect );
			FillRect( canvasRect, wxColour(255, 255, 0, 40) );
			DrawRect( canvasRect, wxColour(255, 0, 0, 140), borderWidth );

			DrawText( wxPoint( 10, y ), GetGdiBoldFont(), TXT("Awaiting an event notification"), *wxBLUE );
			y += fontHeight;

			y += 2*fontHeight;
		}
	}

	SetScale( scale, false );
	SetOffset( offset, false );
}

void CEdSteeringGraphEditor::ForceLayoutUpdate()
{
	ClearLayout();

	if ( !m_steering )
		return;

	FillNodeLayout( m_steering->GetRootNode(), NULL );
} 

void CEdSteeringGraphEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( GameStarted ) || name == CNAME( GameEnded ) )
	{
		ForceLayoutUpdate();
		Repaint();
	}
}

void CEdSteeringGraphEditor::FillNodeLayout( IMoveSteeringNode * node, LayoutInfo * parentLayout )
{
	if ( ! node )
	{
		return;
	}

	LayoutInfo * layout = UpdateBlockLayout( node, parentLayout );

	// block background color
	if ( node->IsA< CMoveSNComposite >() )
	{
		layout->m_bgColor = wxColour( 128, 128, 128 );
	}
	else if ( node->IsA< CMoveSNCondition >() )
	{
		layout->m_bgColor = wxColour( 255, 101, 63 );
	}
	else if ( node->IsA< CMoveSNTask >() )
	{
		layout->m_bgColor = wxColour( 68, 205, 255 );
	}

	wxColour activationColour = m_editor->GetActivationColour( *node );
	layout->m_bgColor = wxColour( 
		::Min( layout->m_bgColor.Red() + activationColour.Red(), 255 ),
		::Min( layout->m_bgColor.Green() + activationColour.Green(), 255 ),
		::Min( layout->m_bgColor.Blue() + activationColour.Blue(), 255 )
		);

	// fill the layout of children nodes
	IMoveSNComposite* compNode = Cast< IMoveSNComposite >( node );
	if( compNode )
	{
		for ( Uint32 i = 0; i < compNode->GetChildCount(); ++i )
		{
			FillNodeLayout( const_cast< IMoveSteeringNode* >( compNode->GetChild( i ) ), layout );
		}
	}
}

Bool CEdSteeringGraphEditor::IsNodeInDisabledBranch( IMoveSteeringNode * node ) const
{
	const IMoveSteeringNode* n = node;
	while( n )
	{
		if( !n->IsEnabled() )
			return true;

		n = n->GetParentNode();
	}

	return false;
}

Bool CEdSteeringGraphEditor::LoadBlockPosition( IScriptable * block, wxPoint & pos )
{
	if ( IMoveSteeringNode * node = Cast< IMoveSteeringNode >( block ) )
	{
		pos.x = node->GetGraphPosX();
		pos.y = node->GetGraphPosY();
		return true;
	}

	return false;
}

Bool CEdSteeringGraphEditor::SaveBlockPosition( IScriptable * block, const wxPoint & pos )
{
	if ( IMoveSteeringNode * node = Cast< IMoveSteeringNode >( block ) )
	{
		node->SetGraphPosition( pos.x, pos.y );
		return true;
	}

	return false;
}

// void CEdSteeringGraphEditor::MoveSelectedBlocks( wxPoint totalOffset, Bool alternate )
// {
// 	if( GGame->IsActive() )
// 	{
// 		return;
// 	}
// 
// 	if( !m_steering->CanModify() )
// 	{
// 		return;
// 	}
// 
// 	CEdTreeEditor::MoveSelectedBlocks( totalOffset, alternate );
// 
// 	for ( Uint32 i = 0; i < m_selected.Size(); ++i )
// 	{
// 		LayoutInfo * layout;
// 		if ( ! m_layout.Find( m_selected[i], layout ) )
// 			continue;
// 
// 		IMoveSteeringNode * node = Cast< IMoveSteeringNode >( layout->m_owner );
// 		if ( ! node )
// 			continue;
// 
// 		node->SetGraphPosition( layout->m_windowPos.x + layout->m_windowSize.x / 2, layout->m_windowPos.y  + layout->m_windowSize.y / 2 );
// 	}
// }

//! Called when move ended
void CEdSteeringGraphEditor::OnMoveEnded()
{
	if( GGame->IsActive() )
	{
		return;
	}

	if( !m_steering->CanModify() )
	{
		return;
	}

	if( GetSelectedObjects().Size() > 0 )
	{
		if( m_steering->GetRootNode()->CorrectChildrenOrder() )
		{
			MarkStructureModified();
		}

		// Update layout
		ForceLayoutUpdate();
		Repaint();
	}
}

void CEdSteeringGraphEditor::DeleteSelectedObjects( bool askUser /* = true */ )
{
	if( GGame->IsActive() )
	{
		return;
	}

	// Ask the question
	if ( askUser && GetSelectedObjects().Size() )
	{
		if ( !YesNo( TXT("Sure to delete selection ?") ) )
		{
			return;
		}
	}

	if ( ! m_steering->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	CleanUpSelection( true, true );

	for ( Uint32 i = 0; i < GetSelectedObjects().Size(); ++i )
	{
		IMoveSteeringNode * node = Cast< IMoveSteeringNode >( GetSelectedObjects()[i] );
		if ( ! node )
			continue;

		CObject * parent = node->GetParent();
		if ( parent == m_steering )
		{
			m_steering->SetRootNode( NULL );
		}
		else
		{
			IMoveSteeringNode * parentNode = Cast< IMoveSteeringNode >( parent );
			ASSERT( parentNode );
			if ( parentNode )
			{
				CMoveSNComposite* compNode = Cast< CMoveSNComposite >( parentNode );
				if( compNode )
				{
					compNode->RemoveChild( node );					
				}
			}
		}		
	}

	// Mark modified
	MarkStructureModified();

	// selection is actually reset after destroying the component, which prevents deleted objects from being included in the properties pages
	DeselectAllObjects();

	ForceLayoutUpdate();
	// Redraw
	Repaint();
}

void CEdSteeringGraphEditor::FillTaskClasses()
{
	// Collect allowed component classes
	class CNamer : public CClassHierarchyMapper::CClassNaming
	{
		void GetClassName( CClass* classId, String& outName ) const override
		{
			IMoveSteeringTask* defaultNode = classId->IsAbstract() ? NULL : classId->GetDefaultObject< IMoveSteeringTask >();
			if ( defaultNode )
			{
				outName = defaultNode->GetTaskName();
			}
			else if ( classId == IMoveSteeringTask::GetStaticClass() )
			{
				outName = TXT("Base");
			}
			else
			{
				outName = classId->GetName().AsString();
			}
		}
	} cNamer;

	CClassHierarchyMapper::MapHierarchy( ClassID< IMoveSteeringTask >(), m_taskClasses, cNamer );
}

void CEdSteeringGraphEditor::FillConditionClasses()
{
	// Collect allowed component classes
	class CNamer : public CClassHierarchyMapper::CClassNaming
	{
		void GetClassName( CClass* classId, String& outName ) const override
		{
			IMoveSteeringCondition* defaultNode = classId->IsAbstract() ? NULL : classId->GetDefaultObject< IMoveSteeringCondition >();
			if ( defaultNode )
			{
				outName = defaultNode->GetConditionName();
			}
			else if ( classId == IMoveSteeringCondition::GetStaticClass() )
			{
				outName = TXT("Base");
			}
			else
			{
				outName = classId->GetName().AsString();
			}
		}
	} cNamer;

	CClassHierarchyMapper::MapHierarchy( ClassID< IMoveSteeringCondition >(), m_conditionClasses, cNamer );
}

void CEdSteeringGraphEditor::OnOpenContextMenu()
{
	wxMenu menu;

	Bool cannotModify = false;
	if( GGame->IsActive() )
	{
		menu.Append( ID_MENU_GAME_RUNNING, TXT("Game is running") );
		cannotModify = true;
	}

	if ( !m_steering->CanModify() )
	{
		menu.Append( ID_MENU_CHECK_OUT, TXT("Check out for editing") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSteeringGraphEditor::OnCheckOut, this, ID_MENU_CHECK_OUT );
		
		cannotModify = true;
	}

	if ( cannotModify )
	{
		PopupMenu( &menu );
		return;
	}

	IMoveSteeringNode* node = GetActiveItem() ? Cast< IMoveSteeringNode >( GetActiveItem()->m_owner ) : NULL;
	m_contextMenuItem = node;
	IMoveSNComposite* compNode = Cast< IMoveSNComposite >( node );
	Bool canHaveChildren = m_steering->GetRootNode() == NULL || ( compNode && compNode->CanAddChildren() );

	// Default menu
	if ( canHaveChildren )
	{
		menu.Append( ID_CREATE_BLOCK_TASK, TXT( "Add child: Task" ) );
		menu.Connect( ID_CREATE_BLOCK_TASK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnSpawnBlock ), NULL, this );
		menu.Append( ID_CREATE_BLOCK_GROUP, TXT( "Add child: Group" ) );
		menu.Connect( ID_CREATE_BLOCK_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnSpawnBlock ), NULL, this );
		menu.Append( ID_CREATE_BLOCK_CONDITION, TXT( "Add child: Condition" ) );
		menu.Connect( ID_CREATE_BLOCK_CONDITION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnSpawnBlock ), NULL, this );
		// Add composite classes
		//menu.Append( ID_CREATE_BLOCK, TXT( "Add child Block" ) );
		//menu.Connect( ID_CREATE_BLOCK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnSpawnBlock ), NULL, this );
		menu.AppendSeparator();
	}

	if ( node )
	{
		if( node->IsA< CMoveSNTask >() )
		{
			menu.Append( ID_STEER_SET_TASK, TXT( "Set Task" ) );
			menu.Connect( ID_STEER_SET_TASK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnSetTask ), NULL, this );
		}

		if( node->IsA< CMoveSNCondition >() )
		{
			menu.Append( ID_STEER_SET_CONDITION, TXT( "Set Condition" ) );
			menu.Connect( ID_STEER_SET_CONDITION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnSetCondition ), NULL, this );
		}

		menu.AppendSeparator();

		menu.Append( ID_STEER_COPY_BRANCH, TXT("Copy branch") );
		menu.Connect( ID_STEER_COPY_BRANCH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnCopyBranch ), NULL, this );
		menu.Append( ID_STEER_CUT_BRANCH, TXT("Cut branch") );
		menu.Connect( ID_STEER_CUT_BRANCH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnCutBranch ), NULL, this );	
	}

	if( s_copiedBranch && canHaveChildren )
	{
		menu.Append( ID_STEER_PASTE_BRANCH, TXT("Paste branch") );
		menu.Connect( ID_STEER_PASTE_BRANCH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnPasteBranch ), NULL, this );
	}

	menu.AppendSeparator();

	if (
		( compNode && compNode->GetChildCount() > 0 ) || 
		( !node && m_steering->GetRootNode() )
		)
	{
		menu.Append( ID_AUTO_LAYOUT, TXT("Auto layout") );
		menu.Connect( ID_AUTO_LAYOUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSteeringGraphEditor::OnApplyDefaultLayout ), NULL, this );
	}

	wxMenuUtils::CleanUpSeparators( &menu );

	// Show menu
	PopupMenu( &menu );
}

void CEdSteeringGraphEditor::ShowGameActiveInfo()
{
	wxMessageBox( TXT("Cannot modify. Game is running!"), TXT("Steering Editor") );
}

void CEdSteeringGraphEditor::ShowMustCheckoutInfo()
{
	wxMessageBox( TXT("Cannot modify. Checkout file!"), TXT("Steering Editor") );	
}

void CEdSteeringGraphEditor::OnCheckOut( wxCommandEvent& event )
{
	CResource* res = m_editor->GetEditedSteeringBehavior();
	if ( res )
	{
		res->GetFile()->CheckOut();

		ForceLayoutUpdate();
		Repaint();
	}
}

void CEdSteeringGraphEditor::SpawnBlock( CClass* blockClass )
{
	if( GGame->IsActive() )
	{
		ShowGameActiveInfo();
		return;
	}

	if ( ! m_steering->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	IMoveSteeringNode* selectedNode = m_contextMenuItem.Get();

	if ( m_steering->GetRootNode() == NULL )
	{
		IMoveSteeringNode* node = ::CreateObject< IMoveSteeringNode >( blockClass, m_steering );
		m_steering->SetRootNode( node );
		UpdateBlockLayout( node, NULL );
		ZoomExtents();
	}
	else if ( selectedNode )
	{
		IMoveSNComposite* compNode = Cast< IMoveSNComposite >( selectedNode );
		if( compNode )
		{
			LayoutInfo* compNodeLayout = GetLayout( compNode );
			ASSERT( compNodeLayout );

			IMoveSteeringNode* node = ::CreateObject< IMoveSteeringNode >( blockClass, compNode );
			if( compNode->GetChildCount() > 0 )
			{
				const IMoveSteeringNode* lastChild = compNode->GetChild( compNode->GetChildCount() - 1 );
				Int32 x = lastChild->GetGraphPosX();
				Int32 y = lastChild->GetGraphPosY();
				node->SetGraphPosition( x + OFFSET_X_MANY, y+OFFSET_Y_MANY );
			}
			else
			{
				Int32 x = compNode->GetGraphPosX();
				Int32 y = compNode->GetGraphPosY();
				node->SetGraphPosition( OFFSET_X_SINGLE, OFFSET_Y_SINGLE );
			}

			compNode->AddChild( node );				
			UpdateBlockLayout( node, compNodeLayout );
		}
	}

	// Mark modified
	MarkStructureModified();

	Repaint();
}

void CEdSteeringGraphEditor::OnSpawnBlock( wxCommandEvent& event )
{
	if( GGame->IsActive() )
	{
		ShowGameActiveInfo();
		return;
	}

	if ( ! m_steering->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	Bool doSpawn = false;

	IMoveSteeringNode* selectedNode = m_contextMenuItem.Get();

	if ( m_steering->GetRootNode() == NULL )
	{
		doSpawn = true;
	}
	else if ( selectedNode )
	{
		IMoveSNComposite* node = Cast< IMoveSNComposite >( selectedNode );
		if( node && node->CanAddChildren() )
		{
			doSpawn = true;
		}
	}

	if ( doSpawn )
	{
		switch ( event.GetId() )
		{
		case ID_CREATE_BLOCK_TASK:
			SpawnBlock( CMoveSNTask::GetStaticClass() );
			break;
		case ID_CREATE_BLOCK_GROUP:
			SpawnBlock( CMoveSNComposite::GetStaticClass() );
			break;
		case ID_CREATE_BLOCK_CONDITION:
			SpawnBlock( CMoveSNCondition::GetStaticClass() );
			break;
		case ID_CREATE_BLOCK:
			{
				CEdMappedClassSelectorDialog selector( this, m_blockClasses, TXT( "/Frames/SteeringGraphBlockSelectorDialog" ), IMoveSteeringNode::GetStaticClass() );

				if ( CClass* blockClass = selector.Execute() )
				{
					SpawnBlock( blockClass );
				}
			}
			break;
		}
	}
}

void CEdSteeringGraphEditor::OnSetCondition( wxCommandEvent& event )
{
	if( GGame->IsActive() )
	{
		ShowGameActiveInfo();
		return;
	}

	if ( ! m_steering->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	IMoveSteeringNode* selectedNode = m_contextMenuItem.Get();

	if( selectedNode && selectedNode->IsA< CMoveSNCondition >() )
	{
		CMoveSNCondition* state = SafeCast< CMoveSNCondition >( selectedNode );
		CClass* selectedClass = NULL;
		if( state->GetCondition() )
		{
			if( !YesNo( TXT("Sure to change condition?") ) )
			{
				return;
			}
			selectedClass = state->GetCondition()->GetClass();
		}

		FillConditionClasses();

		CEdMappedClassSelectorDialog dialog( this, m_conditionClasses, TXT( "/Frames/SteeringGraphConditionSelectorDialog" ), IMoveSteeringCondition::GetStaticClass(), selectedClass );

		if ( CClass* conditionClass = dialog.Execute() )
		{
			if( GGame->IsActive() )
			{
				ShowGameActiveInfo();
				return;
			}

			if ( ! m_steering->CanModify() )
			{
				ShowMustCheckoutInfo();
				return;
			}

			IMoveSteeringNode* selectedNode = m_contextMenuItem.Get();

			if( selectedNode && selectedNode->IsA< CMoveSNCondition >() )
			{
				CMoveSNCondition* state = SafeCast< CMoveSNCondition >( selectedNode );
				IMoveSteeringCondition* condition = ::CreateObject< IMoveSteeringCondition >( conditionClass, state );
				state->SetCondition( condition );

				m_editor->OnGraphSelectionChanged();		
				ForceLayoutUpdate();		
				Repaint();
			}
		}
	}
}

void CEdSteeringGraphEditor::OnSetTask( wxCommandEvent& event )
{
	if( GGame->IsActive() )
	{
		ShowGameActiveInfo();
		return;
	}

	if ( ! m_steering->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	IMoveSteeringNode* selectedNode = m_contextMenuItem.Get();

	if( selectedNode && selectedNode->IsA< CMoveSNTask >() )
	{
		CMoveSNTask* state = SafeCast< CMoveSNTask >( selectedNode );
		CClass* selectedClass = NULL;
		if( state->GetTask() )
		{
			if( !YesNo( TXT("Sure to change task?") ) )
			{
				return;
			}
			selectedClass = state->GetTask()->GetClass();
		}

		FillTaskClasses();

		CEdMappedClassSelectorDialog selector( this, m_taskClasses, TXT( "/Frames/SteeringGraphTaskSelectorDialog" ), IMoveSteeringTask::GetStaticClass(), selectedClass );

		if ( CClass* taskClass = selector.Execute() )
		{
			if( GGame->IsActive() )
			{
				ShowGameActiveInfo();
				return;
			}

			if ( ! m_steering->CanModify() )
			{
				ShowMustCheckoutInfo();
				return;
			}

			IMoveSteeringNode* selectedNode = m_contextMenuItem.Get();

			if( selectedNode && selectedNode->IsA< CMoveSNTask >() )
			{
				CMoveSNTask* state = SafeCast< CMoveSNTask >( selectedNode );
				IMoveSteeringTask* task = ::CreateObject< IMoveSteeringTask >( taskClass, state );
				state->SetTask( task );

				m_editor->OnGraphSelectionChanged();		
				ForceLayoutUpdate();		
				Repaint();
			}
		}
	}
}

void CEdSteeringGraphEditor::CopyActiveItem()
{
	IMoveSteeringNode* node = m_contextMenuItem.Get();
	if ( !node )
	{
		return;
	}

	if( s_copiedBranch )
	{
		s_copiedBranch->RemoveFromRootSet();
		s_copiedBranch->Discard();
		s_copiedBranch = NULL;
	}

	s_copiedBranch = SafeCast< IMoveSteeringNode >( node->Clone( NULL ) );
	s_copiedBranch->AddToRootSet();
}

void CEdSteeringGraphEditor::OnCopyBranch( wxCommandEvent& event )
{
	if( GGame->IsActive() )
		return;

	CopyActiveItem();

	String friendlyName = s_copiedBranch->GetFriendlyName();
	SEdPopupNotification::GetInstance().Show( this, TXT("COPY"), friendlyName );
}

void CEdSteeringGraphEditor::OnCutBranch( wxCommandEvent& event )
{
	if( GGame->IsActive() )
		return;

	if ( ! m_steering->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	CopyActiveItem();

	IMoveSteeringNode* node = m_contextMenuItem.Get();
	ASSERT( node );
	const IMoveSteeringNode* parent = node->GetParentNode();
	if( parent )
	{
		if( parent->IsA< IMoveSNComposite >() )
		{
			IMoveSteeringNode* nonConstParent = const_cast< IMoveSteeringNode* >( parent );
			SafeCast< IMoveSNComposite >( nonConstParent )->RemoveChild( node );
		}
	}
	else
	{
		m_steering->SetRootNode( NULL );
	}

	String friendlyName = s_copiedBranch->GetFriendlyName();
	SEdPopupNotification::GetInstance().Show( this, TXT("CUT"), friendlyName );

	DeselectAllObjects();
	ForceLayoutUpdate();
	Repaint();

	// Mark modified
	MarkStructureModified();
}

void CEdSteeringGraphEditor::OnPasteBranch( wxCommandEvent& event )
{	
	if( GGame->IsActive() )
		return;

	if ( ! m_steering->CanModify() )
	{
		ShowMustCheckoutInfo();
		return;
	}

	if( !s_copiedBranch )
		return;

	IMoveSteeringNode* selectedNode = m_contextMenuItem.Get();
	IMoveSteeringNode * node = NULL;
	Bool zoom = false;

	if( m_steering->GetRootNode() == NULL )
	{
		node = SafeCast< IMoveSteeringNode >( s_copiedBranch->Clone( m_steering ) );
		m_steering->SetRootNode( node );

		Int32 oldX = node->GetGraphPosX();
		Int32 oldY = node->GetGraphPosY();

		// Root in [0,0]
		node->OffsetNodesPosition( -oldX, -oldY );
		zoom = true;
	}
	else if( selectedNode )
	{	
		IMoveSNComposite * parent = Cast< IMoveSNComposite >( selectedNode );
		node = SafeCast< IMoveSteeringNode >( s_copiedBranch->Clone( parent ) );

		if( parent->GetChildCount() > 0 )
		{
			const IMoveSteeringNode* lastChild = parent->GetChild( parent->GetChildCount()-1 );
			Int32 x = lastChild->GetGraphPosX();
			Int32 y = lastChild->GetGraphPosY();
			Int32 oldX = node->GetGraphPosX();
			Int32 oldY = node->GetGraphPosY();
			Int32 offsetX = x + OFFSET_X_MANY - oldX;
			Int32 offsetY = y + OFFSET_Y_MANY - oldY;
			node->OffsetNodesPosition( offsetX, offsetY );
		}
		else
		{
			Int32 oldX = node->GetGraphPosX();
			Int32 oldY = node->GetGraphPosY();
			Int32 offsetX = OFFSET_X_SINGLE - oldX;
			Int32 offsetY = OFFSET_Y_SINGLE - oldY;
			node->OffsetNodesPosition( offsetX, offsetY );			
		}

		parent->AddChild( node );
	}

	if( node )
	{
		// Select cloned nodes
		TDynArray< IMoveSteeringNode* > nodes;
		node->CollectNodes( nodes );

		DeselectAllObjects();

		for( Uint32 i=0; i<nodes.Size(); i++ )
		{
			SelectObject( nodes[i], true, false );
		}

		String friendlyName = node->GetFriendlyName();
		SEdPopupNotification::GetInstance().Show( this, TXT("PASTE"), friendlyName );

		ForceLayoutUpdate();		
		Repaint();

		if( zoom )
		{
			ZoomExtents();
		}
	}

	// Mark modified
	MarkStructureModified();
}

void CEdSteeringGraphEditor::OnApplyDefaultLayout( wxCommandEvent& event )
{
	IMoveSteeringNode* selectedNode = m_contextMenuItem.Get();

	IMoveSteeringNode * node = selectedNode ? Cast< IMoveSteeringNode >( selectedNode ) : m_steering->GetRootNode();	

	if ( !node )
	{
		return;
	}

	AutoLayout( node );
}

void CEdSteeringGraphEditor::MarkStructureModified()
{
	if ( m_steering )
	{
		m_steering->OnStructureModified();
	}
}
