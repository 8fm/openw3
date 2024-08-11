/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorDebugVisualizer.h"
#include "behaviorEditor.h"
#include "behaviorGraphEditor.h"
#include "behaviorProperties.h"
#include "editorExternalResources.h"
#include "../../common/engine/behaviorGraphAnimationNode.h"
#include "../../common/engine/behaviorGraphBlendMultipleNode.h"
#include "../../common/engine/behaviorGraphComboNode.h"
#include "../../common/engine/behaviorGraphContainerNode.h"
#include "../../common/engine/behaviorGraphEngineValueNode.h"
#include "../../common/engine/behaviorGraphOutputNode.h"
#include "../../common/engine/behaviorGraphParentInputNode.h"
#include "../../common/engine/behaviorGraphParentValueInputNode.h"
#include "../../common/engine/behaviorGraphParentVectorValueInputNode.h"
#include "../../common/engine/behaviorGraphPointerStateNode.h"
#include "../../common/engine/behaviorGraphRandomNode.h"
#include "../../common/engine/behaviorGraphRandomSelectNode.h"
#include "../../common/engine/behaviorGraphSelfActStateMachine.h"
#include "../../common/engine/behaviorGraphStage.h"
#include "../../common/engine/behaviorGraphStartingStateNode.h"
#include "../../common/engine/behaviorGraphStateMachine.h"
#include "../../common/engine/behaviorGraphStateNode.h"
#include "../../common/engine/behaviorGraphTransitionForce.h"
#include "../../common/engine/behaviorGraphTransitionNode.h"
#include "../../common/engine/behaviorGraphValueNode.h"
#include "../../common/engine/behaviorGraphMimicConverter.h"
#include "../../common/engine/behaviorGraphMimicPoseNodes.h"
#include "../../common/engine/behaviorGraph2DVariableNode.h"
#include "../../common/engine/behaviorGraphAnimationManualSlot.h"
#include "../../common/engine/behaviorGraphAnimationBlendSlotNode.h"
#include "../../common/engine/behaviorGraphSocket.h"
#include "../../common/engine/behaviorGraphRecentlyUsedAnimsStateNode.h"

#include "../../common/game/behaviorGraphGameplayAdditiveNode.h"
#include "../../common/game/behaviorGraphScriptStateNode.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/engine/mimicFac.h"
#include "../../common/engine/mimicComponent.h"
#include "../../common/engine/graphConnection.h"
#include "../../common/engine/graphBlock.h"
#include "../../common/engine/engineTypeRegistry.h"

#define ID_SET_START_STATE				3000
#define ID_ADD_INPUT					3001
#define ID_REMOVE_INPUT					3002
#define ID_ADD_ANIMATION_INPUT			3003
#define ID_ADD_VALUE_INPUT				3004
#define ID_ADD_VISUALIZER				3005
#define ID_ADD_VISUALIZER_SB			3006
#define ID_CHANGE_VISUALIZER			3007
#define ID_VISUALIZER_TOGGLE_AXIS		3008
#define ID_VISUALIZER_TOGGLE_NAMES		3009
#define ID_VISUALIZER_CHOOSE_HELPERS	3010
#define ID_VISUALIZER_BONE_STYLE_LINE	3011
#define ID_VISUALIZER_BONE_STYLE_3D		3012
#define ID_EDIT_TRACK_NAMES				3013
#define ID_REMOVE_VISUALIZER			3020
#define ID_EDIT_COPY					3021
#define ID_EDIT_DELETE					3022
#define ID_EDIT_CUT						3023
#define ID_EDIT_PASTE					3024
#define ID_COLLAPSE_TO_STAGE			3025
#define ID_CANCEL_TRANSITION			3099
#define ID_CREATE_TRANSITION			3100
#define	ID_PASTE_TRANSITION_NODE		3200
#define	ID_CONVERT_TRANSITION			3201
#define	ID_COPY_TRANSITION_DATA			3301
#define	ID_PASTE_TRANSITION_DATA		3302
#define ID_ADD_GLOBAL_TRANS_BLOCK		3303
#define ID_SET_START_STATE_MACHINE		3304
#define ID_ADD_MIMIC_INPUT				3305
#define ID_VISUALIZER_TOGGLE_BOX		3306
#define ID_SET_STATE					3307
#define ID_REFRESH_ENUM					3308
#define ID_ADD_COMBO_BLOCK				3309
#define ID_ADD_GLOBAL_COMBO_TRANS_BLOCK 3310
#define ID_CREATE_DEFAULT_TRANS_FL		3311
#define ID_CREATE_DEFAULT_TRANS_FR		3312
#define ID_CREATE_DEFAULT_TRANS_END_AUX	3313
#define ID_CONVERT_GWS_TRANSITION		3313
#define ID_GOTO_TRANS_DEST				3314
#define ID_GOTO_TRANS_SRC				3315
#define ID_GOTO_POINTED_STEATE			3316
#define ID_COPY_ALL_TRANSITIONS			3317
#define ID_PASTE_ALL_TRANSITIONS		3318
#define ID_ADD_VECTOR_INPUT				3319

#define	ID_GOTO_SNAPSHOT				3400

BEGIN_EVENT_TABLE( CEdBehaviorGraphEditor, CEdGraphEditor )
	EVT_LEFT_DCLICK( CEdBehaviorGraphEditor::OnMouseLeftDblClick )
	EVT_KEY_DOWN( CEdBehaviorGraphEditor::OnKeyDown )
	EVT_KEY_UP( CEdBehaviorGraphEditor::OnKeyUp )
END_EVENT_TABLE()

CEdBehaviorGraphEditor::CEdBehaviorGraphEditor( CEdBehaviorEditor* editor )
	: CEdGraphEditor( editor, false )	
	, CEdBehaviorEditorPanel( editor )		
	, m_currentRoot( NULL )
	, m_offset( 0.0f, 0.0f, 0.0f )
	, m_debugDisplayBlocksActivation( false )
	, m_displayConditions( false )
	, m_displayConditionTests( true )
	, m_handleToCopyAllTransitionsGraphState( )
{	
	// Set hook
	SetHook( editor );

	// Reset graph
	ResetGraph();

	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );
}

wxAuiPaneInfo CEdBehaviorGraphEditor::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.CloseButton( false ).Center().CentrePane().MinSize(100,100).BestSize( 600, 600 );

	return info;
}

void CEdBehaviorGraphEditor::SetReadOnly( Bool flag )
{
	m_canBeModify = !flag;
}

wxDragResult CEdBehaviorGraphEditor::OnDragOver( wxCoord x, wxCoord y, wxDragResult def )
{
	if ( !PrepareForGraphStructureModified() )
	{
		return wxDragCancel;
	}

	wxTextDataObject* textData = GetDraggedDataObject();
	if ( !textData )
	{
		return wxDragNone;
	}

	String text = textData->GetText().c_str();

	size_t cutPlace;
	if ( !text.FindCharacter( ';', cutPlace ) )
	{
		return wxDragNone;
	}

	String path = text.LeftString( cutPlace );
	CName animationName = CName( text.RightString( text.GetLength() - cutPlace - 1 ) );

	ResourceLoadingContext context;
	CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( path, context );
	if ( set )
	{
		CSkeletalAnimationSetEntry* entry = set->FindAnimation( animationName );
		if ( entry )
		{
			return wxDragCopy;
		}
	}

	return wxDragNone;
}

Bool CEdBehaviorGraphEditor::OnDropText( wxCoord x, wxCoord y, String &text )
{
	if ( !PrepareForGraphStructureModified() )
	{
		return false;
	}


	size_t cutPlace;
	if ( !text.FindCharacter( ';', cutPlace ) )
	{
		return false;
	}

	String path = text.LeftString( cutPlace );
	CName animationName = CName( text.RightString( text.GetLength() - cutPlace - 1 ) );

	ResourceLoadingContext context;
	CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( path, context );
	if ( set )
	{
		CSkeletalAnimationSetEntry* entry = set->FindAnimation( animationName );
		if ( entry )
		{
			wxPoint point = ClientToCanvas( wxPoint( x, y ) );

			if ( m_currentRoot && m_currentRoot->IsA< CBehaviorGraphStateMachineNode >() )
			{
				CBehaviorGraphStateMachineNode* machine = static_cast< CBehaviorGraphStateMachineNode* >( m_currentRoot );

				CBehaviorGraphStateNode* state = Cast< CBehaviorGraphStateNode >( SpawnBlock( CBehaviorGraphStateNode::GetStaticClass(), wxPoint( x, y ) ) );
				if ( state )
				{
					GraphBlockSpawnInfo info( CBehaviorGraphAnimationNode::GetStaticClass() );
					info.m_position.X += 200.f;

					CBehaviorGraphAnimationNode* animNode = Cast< CBehaviorGraphAnimationNode >( state->CreateChildNode( info ) );
					animNode->SetAnimationName( animationName );

					const TDynArray< CGraphBlock* >& nodes = state->GetConnectedChildren();
					for ( Uint32 i=0; i<nodes.Size(); ++i )
					{
						CGraphBlock* n = nodes[ i ];

						if ( n->IsA< CBehaviorGraphOutputNode >() )
						{
							CBehaviorGraphOutputNode* outNode = static_cast< CBehaviorGraphOutputNode* >( n );

							CBehaviorGraphAnimationInputSocket* socketA = Cast< CBehaviorGraphAnimationInputSocket >( outNode->CGraphBlock::FindSocket( TXT("Input") ) );
							CBehaviorGraphAnimationOutputSocket* socketB = Cast< CBehaviorGraphAnimationOutputSocket >( animNode->CGraphBlock::FindSocket( TXT("Animation") ) );

							if ( socketA && socketB )
							{
								socketA->ConnectTo( socketB );
							}
						}
					}

					GraphStructureModified();

					GetEditor()->RecreateBehaviorGraphInstance();
				}

				return true;
			}

			ISerializable* item = GetActiveItem( point ).Get();

			if ( item && item->IsA< CBehaviorGraphAnimationNode >() )
			{
				CBehaviorGraphAnimationNode* node = static_cast< CBehaviorGraphAnimationNode* >( item );

				node->SetAnimationName( animationName );

				GetEditor()->RecreateBehaviorGraphInstance();

				GraphStructureModified();

				return true;
			}

			if ( m_currentRoot && m_currentRoot->IsA< CBehaviorGraphContainerNode >() )
			{
				CBehaviorGraphAnimationNode* node = Cast< CBehaviorGraphAnimationNode >( SpawnBlock( CBehaviorGraphAnimationNode::GetStaticClass(), wxPoint( x, y ) ) );
				node->SetAnimationName( animationName );

				GraphStructureModified();

				GetEditor()->RecreateBehaviorGraphInstance();

				return true;
			}
		}
	}

	return false;
}

void CEdBehaviorGraphEditor::SaveSession( CConfigurationManager &config, const String& _path )
{
	if ( m_currentRoot )
	{
		String path = _path + TXT("CEdBehaviorGraphEditor/RootID");
		Uint32 rootId = m_currentRoot->GetId();
		config.Write( path, (Int32)rootId );
	}

	{
		String pathScale = _path + TXT("CEdBehaviorGraphEditor/scale");

		config.Write( pathScale, GetScale() );

		String pathX = _path + TXT("CEdBehaviorGraphEditor/offsetX");
		String pathY = _path + TXT("CEdBehaviorGraphEditor/offsetY");
		String pathZ = _path + TXT("CEdBehaviorGraphEditor/offsetZ");

		config.Write( pathX, m_offset.X );
		config.Write( pathY, m_offset.Y );
		config.Write( pathZ, m_offset.Z );
	}
}

void CEdBehaviorGraphEditor::RestoreSession( CConfigurationManager &config, const String& _path )
{
	{
		String path = _path + TXT("CEdBehaviorGraphEditor/RootID");

		Int32 rootId = config.Read( path, -1 );
		if ( rootId >= 0 )
		{
			CBehaviorGraphContainerNode* node = Cast< CBehaviorGraphContainerNode >( GetBehaviorGraph()->FindNodeById( (Uint32)rootId ) );
			if ( node )
			{
				SetRootNode( node );
			}
		}
	}

	{
		String pathScale = _path + TXT("CEdBehaviorGraphEditor/scale");

		Float scale = config.Read( pathScale, 1.f );

		String pathX = _path + TXT("CEdBehaviorGraphEditor/offsetX");
		String pathY = _path + TXT("CEdBehaviorGraphEditor/offsetY");
		String pathZ = _path + TXT("CEdBehaviorGraphEditor/offsetZ");

		Vector offset;
		offset.X = config.Read( pathX, 0.f );
		offset.Y = config.Read( pathY, 0.f );
		offset.Z = config.Read( pathZ, 0.f );

		SetScale( scale );
		m_desiredScale = scale;
		SetBackgroundOffset( wxPoint( offset.X, offset.Y ) );

		m_shouldZoom = false;

		CheckBlocksVisibility();
	}
}

/*Bool CEdBehaviorGraphEditor::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
	ASSERT( 0 );

	wxString msg;
	for( Uint32 i=0; i<resources.Size(); i++ )
	{
		CSkeletalAnimation *anim = Cast< CSkeletalAnimation>( resources[ i ] );

		if( anim )
		{
			String name = anim->GetName();

			CEntity *entity = m_behaviorEditor->GetEntity();

			TDynArray< CAnimatedComponent* > animComponents;
			entity->GetComponentsOfClass( animComponents );

			Bool notOnList = true;
			if( animComponents.Size() )
			{
				for( TDynArray< CAnimatedComponent* >::iterator it=animComponents.Begin(); it!=animComponents.End() && notOnList; it++ )
				{
					TDynArray< CSkeletalAnimationSet* > animSets;
					( *it )->GetAnimationSets( animSets );
					for( TDynArray< CSkeletalAnimationSet* >::iterator it2=animSets.Begin(); it2!=animSets.End(); it2++ )
					{
						if( ( *it2 )->FindAnimation( name ) )
						{
							CBehaviorGraphAnimationNode *node = ( CBehaviorGraphAnimationNode* ) CEdGraphEditor::SpawnBlock( SRTTI::GetInstance().FindClass( CNAME( CBehaviorGraphAnimationNode ) ), wxPoint( x, y ) );

							node->SetAnimationName( name );

							notOnList = false;

							y += GetScale() * 120;

							break;
						}
					}
				}
			}

			if( notOnList )
			{
				msg += TXT( "  " );
				msg += name.AsChar();
				msg += TXT( "\n" );
			}
		}
	}
	if( !msg.IsEmpty() )
	{
		wxString m;
		m.sprintf( TXT("Cannot find animation(s):\n%swithin entity animation set(s)"), msg );
		wxMessageDialog dialog( 0, m, wxT("Error"), wxOK | wxICON_ERROR );
		dialog.ShowModal();
	}

	return true;
}*/

void CEdBehaviorGraphEditor::OnTick( Float dt )
{
	if ( !GetBehaviorGraphInstance() )
	{
		m_background.OnTick( dt );

		Refresh();
	}
	TDynArray< CBehaviorEditorEffect* > toRemove;
	for ( TSet< CBehaviorEditorEffect* >::iterator it = m_effects.Begin(); it != m_effects.End(); ++it )
	{
		(*it)->OnTick( dt, this );
		if ( (*it)->IsToRemove() ) toRemove.PushBack( (*it) );
	}
	while ( toRemove.Size() )
	{
		m_effects.Erase( toRemove[ toRemove.Size() - 1 ] );
		toRemove.PopBack();
	}
	Repaint();
}
CEdBehaviorGraphEditor::~CEdBehaviorGraphEditor()
{
	while ( m_effects.Size() )
	{
		CBehaviorEditorEffect* toRemove = *m_effects.Begin();
		m_effects.Erase( toRemove );
		delete toRemove;

	}
}

void CEdBehaviorGraphEditor::AddEffect( CBehaviorEditorEffect* effect )
{
	m_effects.Insert( effect );
}

void CEdBehaviorGraphEditor::RemoveEffect( CBehaviorEditorEffect* effect )
{
	(*m_effects.Find( effect ))->Remove();
}

void CEdBehaviorGraphEditor::ResetGraph()
{
	if ( GetBehaviorGraph() )
	{		
		SetRootNode( GetBehaviorGraph()->GetRootNode() );
	}
	else
	{
		SetRootNode( NULL );
	}
}

void CEdBehaviorGraphEditor::SetRootNode( CBehaviorGraphContainerNode* rootNode, Bool zoomExtents /* = true */)
{
	if ( m_currentRoot == rootNode )
	{
		return;
	}

	if ( m_currentRoot )
	{
		TDynArray< CGraphBlock* >& blocks = GraphGetBlocks();
		ResetBlocksVisibility( blocks );
	}

	m_currentRoot = rootNode;

	if ( m_currentRoot )
	{
		CEdGraphEditor::SetGraph( static_cast< IGraphContainer* >( this ) );
		m_shouldZoom = zoomExtents; // CEdGraphEditor::SetGraph sets this to true, which couses to zoom out graph in next few frames.

		if ( zoomExtents )
		{
			ZoomExtents(); 
		}
	}
}

CBehaviorGraphContainerNode* CEdBehaviorGraphEditor::GetRootNode() const
{
	return m_currentRoot;
}

void CEdBehaviorGraphEditor::SetDisplayConditions( Bool flag )
{
	m_displayConditions = flag;
}

void CEdBehaviorGraphEditor::SetDisplayConditionTests( Bool flag )
{
	m_displayConditionTests = flag;
}

Bool CEdBehaviorGraphEditor::IsBlockActivated( CGraphBlock* block ) const 
{ 
	CBehaviorGraphNode* node = Cast< CBehaviorGraphNode >( block );
	if ( !node )
	{
		return false;
	}
	CBehaviorGraphInstance* instance = GetEditor()->GetBehaviorGraphInstance();
	return instance ? node->IsActive( *instance ) : false; 
}

Float CEdBehaviorGraphEditor::GetBlockActivationAlpha( CGraphBlock* block ) const 
{ 
	CBehaviorGraphNode* node = Cast< CBehaviorGraphNode >( block );
	if ( !node )
	{
		return false;
	}
	CBehaviorGraphInstance* instance = GetEditor()->GetBehaviorGraphInstance();
	return instance ? node->GetActivationAlpha( *instance ) : 0.f;  
}

SGraphLayer* CEdBehaviorGraphEditor::GetLayer( Uint32 num )
{
	return NULL;
}

Uint32 CEdBehaviorGraphEditor::GetLayerNum() const
{
	return 0;
}

void CEdBehaviorGraphEditor::FocusOnBehaviorNode( CBehaviorGraphNode* node )
{
	m_LastFocusedBlock = node;

	// Get block layout info
	BlockLayoutInfo* layout = m_layout.FindPtr( node );
	if ( layout )
	{
		// Reset scale
		m_desiredScale = 1.0f;
		SetScale( 1.0f );

		// Center on block
		wxSize windowSize = GetSize();
		const Vector &currBlockPos = node->GetPosition();
		wxPoint blockCenter( currBlockPos.X + layout->m_windowSize.x - windowSize.x / 2, currBlockPos.Y + layout->m_windowSize.y - windowSize.y / 2 );
		SetBackgroundOffset( -blockCenter );

		CheckBlocksVisibility();
	}
}

void CEdBehaviorGraphEditor::CalcBlockInnerArea( CGraphBlock* block, wxSize& innerArea )
{	
	innerArea = wxSize( 64, 32 );
}

void CEdBehaviorGraphEditor::DrawBlockSocketsCaptions( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data )
{
	if ( ( m_displayConditions ^ m_displayConditionsAlternate ) &&
		 block->IsA( CBehaviorGraphStateTransitionNode::GetStaticClass() ) )
	{
		return;
	}

	CEdGraphEditor::DrawBlockSocketsCaptions( block, layout, data );
}

void CEdBehaviorGraphEditor::DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect )
{
	CBehaviorGraphNode *node = Cast< CBehaviorGraphNode >( block );
	if ( !node )
	{
		return;
	}

	if ( GetScale() > 0.2f && node->IsA< CBehaviorGraphStateTransitionNode >() &&
		 ( ( m_displayConditions ^ m_displayConditionsAlternate ) || GetEditor()->GetGraphEditor()->IsBlockSelected( node ) ) )
	{
		CBehaviorGraphStateTransitionNode* trans = Cast< CBehaviorGraphStateTransitionNode >( node );
		TDynArray< String > captions;
		TDynArray< String > setInternalVariables;

		wxColour frontColor(255, 255, 255);
		
		if( trans->IsEnabled() )
		{
			const IBehaviorStateTransitionCondition* cond = trans->GetTransitionCondition();
			if( cond )
			{
				cond->GetCaption( captions, m_displayConditionTests, GetEditor()->GetBehaviorGraphInstance() );
				if( cond->IsA< CCompositeTransitionCondition >() )
				{					
					frontColor = wxColour( 128, 255, 255 );
				}
				else if( cond->IsA< CCompositeSimultaneousTransitionCondition >() )
				{
					frontColor = wxColour( 255, 255, 0 );
				}
			}
			trans->GetSetInternalVariableCaptions( setInternalVariables );
		}
		else
		{
			captions.PushBack( TXT("Disabled") );
		}

		Bool allowColorPerCondition = m_displayConditionTests;

		if ( GetEditor()->GetDebugger() && GetEditor()->GetBehaviorGraphInstance() )
		{
			if ( trans->TestTransitionCondition( *(GetEditor()->GetBehaviorGraphInstance()) ) )
			{
				allowColorPerCondition = false;
				frontColor = wxColour( 0, 255, 0 );
			}
			else
			{
				// not active
				if ( GetEditor()->GetGraphEditor()->IsBlockSelected( node ) )
				{
					frontColor = wxColour( 230, 230, 255 );
				}
			}
			if ( CBehaviorGraphStateMachineNode const * stateMachine = Cast< CBehaviorGraphStateMachineNode >( trans->GetParentNode() ) )
			{
				if ( stateMachine->GetCurrentState( *(GetEditor()->GetBehaviorGraphInstance()) ) == trans )
				{
					frontColor = wxColour( 50, 220, 255 );
					allowColorPerCondition = false;
				}
			}
		}


		// build colored captions
		struct SColoredCaption
		{
			String m_caption;
			wxColour m_color;

			SColoredCaption(String const & caption, wxColour color)
				:	m_caption( caption )
				,	m_color( color )
			{
			}
		};
		TDynArray< SColoredCaption > coloredCaptions;
		if ( trans->GetPriority() )
		{
			coloredCaptions.PushBack( SColoredCaption( String::Printf(TXT("prio: %.3f"), trans->GetPriority()), trans->GetPriority() < 0.0f? wxColour( 80, 255, 80 ) : wxColour( 80, 80, 255 ) ) );
		}
		for ( TDynArray< String >::iterator iCaption = captions.Begin(); iCaption != captions.End(); ++ iCaption )
		{
			wxColour color = frontColor;
			if ( allowColorPerCondition )
			{
				if (iCaption->BeginsWith(TXT("[v]")))
				{
					if ( GetEditor()->GetGraphEditor()->IsBlockSelected( node ) )
					{
						color = wxColour( 150, 250, 70 );
					}
					else
					{
						color = wxColour( 100, 170, 40 );
					}
				}
				else if (iCaption->BeginsWith(TXT("[ ]")))
				{
					if ( GetEditor()->GetGraphEditor()->IsBlockSelected( node ) )
					{
						color = wxColour( 150, 150, 150 );
					}
					else
					{
						color = wxColour( 90, 90, 90 );
					}
				}
			}
			coloredCaptions.PushBack( SColoredCaption( *iCaption, color ) );
		}
		for ( TDynArray< String >::iterator iCaption = setInternalVariables.Begin(); iCaption != setInternalVariables.End(); ++ iCaption )
		{
			coloredCaptions.PushBack( SColoredCaption( *iCaption, wxColour( 255, 100, 255 ) ) );
		}

		if( captions.Size() > 0 )
		{
			// Reset clip rect :)
			ResetClip();

			// get size
			int paddingY = -2;
			wxPoint wholeSize = wxPoint( 0, 0 );

			for ( TDynArray< SColoredCaption >::iterator iCaption = coloredCaptions.Begin(); iCaption != coloredCaptions.End(); ++ iCaption )
			{
				wxPoint size = TextExtents( GetGdiDrawFont(), iCaption->m_caption.AsChar() );
				size.y += paddingY;
				wholeSize.x = Max( wholeSize.x, size.x );
				wholeSize.y += size.y;
			}

			// render
			wholeSize.y = Max( 0, wholeSize.y - paddingY );
			wxPoint sizeSoFar = wxPoint( 0, 0 );

			for ( TDynArray< SColoredCaption >::iterator iCaption = coloredCaptions.Begin(); iCaption != coloredCaptions.End(); ++ iCaption )
			{
				// Draw text
				wxColour black(0, 0, 0);		
				wxColour frontColor = iCaption->m_color;

				wxPoint size = TextExtents( GetGdiDrawFont(), iCaption->m_caption.AsChar() );
				wxPoint offset( rect.x + rect.width/2 - size.x/2, rect.y + rect.height/2 - wholeSize.y/2 + sizeSoFar.y );

				BlockLayoutInfo* layout = m_layout.FindPtr( block );
				ASSERT( layout );
				if ( layout->m_freeze )
				{
					black = ConvertBlockColorToFreezeMode( black );
					frontColor = ConvertBlockColorToFreezeMode( frontColor );
				}

				DrawText( offset + wxPoint(1,1), GetGdiDrawFont(), iCaption->m_caption.AsChar(), black );
				DrawText( offset, GetGdiDrawFont(), iCaption->m_caption.AsChar(), frontColor );

				sizeSoFar.y += size.y + paddingY;
			}

		}
	}
	else if ( node->IsA< CBehaviorGraphStateNode >() && !node->IsA< CBehaviorGraphComboStateNode >() )
	{
		ResetClip();

		// draw state socket
		CGraphSocket *socket = node->CGraphBlock::FindSocket( TXT("Out") );
		ASSERT( socket );

		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		ASSERT( layout );

		SocketLayoutInfo* socketLayout = layout->m_sockets.FindPtr( socket );
		if ( socketLayout )
		{
			wxRect socketRect = socketLayout->m_socketRect;
			socketRect.Offset( node->GetPosition().X, node->GetPosition().Y );

			wxColour color( 25, 220, 25 );
			wxColour black( 0, 0, 0 );

			if ( layout->m_freeze )
			{
				black = ConvertBlockColorToFreezeMode( black );
				color = ConvertBlockColorToFreezeMode( color );
			}

			FillRect( socketRect, color );

			DrawRect( socketRect, black, 2.0f );
		
			/*
			String caption = node->GetName();

			ResetClip();

			wxPoint size = TextExtents( GetGdiDrawFont(), caption.AsChar() );
			wxPoint offset( rect.x + rect.width/2 - size.x/2, rect.y + rect.height/2 - size.y/2 );
			DrawText( offset + wxPoint(1,1), GetGdiDrawFont(), caption.AsChar(), wxColour(0,0,0) );
			DrawText( offset, GetGdiDrawFont(), caption.AsChar(), wxColour(255,255,255) );
			*/
		}
	}

	// Display node's activation num - for debug 
	if ( m_debugDisplayBlocksActivation && GetEditor()->GetBehaviorGraphInstance() && block->IsA< CBehaviorGraphNode >() )
	{
		CBehaviorGraphNode *node = SafeCast< CBehaviorGraphNode >( block );

		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen )
		{
			Int32 acitveNum = node->GetActiveNum( *GetEditor()->GetBehaviorGraphInstance() );
			String text = String::Printf( TXT("%d"), acitveNum );

			ResetClip();

			// Draw text
			wxPoint size = TextExtents( GetGdiDrawFont(), text.AsChar() );
			wxPoint offset( rect.x + rect.width/2 - size.x/2, rect.y + rect.height/2 - size.y/2 );

			BlockLayoutInfo* layout = m_layout.FindPtr( block );
			ASSERT( layout );

			DrawText( offset + wxPoint(1,1), GetGdiDrawFont(), text.AsChar(), wxColour(0, 0, 0) );
			DrawText( offset, GetGdiDrawFont(), text.AsChar(), wxColour(255, 0, 0) );

 			if ( block->IsA< CBehaviorGraphStateTransitionNode >() )
 			{
 				CBehaviorGraphStateTransitionNode* transition = Cast< CBehaviorGraphStateTransitionNode >( block );
				
				String conditions;
				transition->HierarchicalConditionsTest( *GetEditor()->GetBehaviorGraphInstance(), conditions );

 				wxColour color = transition->TestTransitionCondition( *GetEditor()->GetBehaviorGraphInstance() ) ? wxColour( 255, 0, 0 ) : wxColour( 255, 255, 255 );

				// Draw text
				offset.x += 20;

				DrawText( offset + wxPoint(1,1), GetGdiDrawFont(), conditions.AsChar(), wxColour( 0, 0, 0 ) );
				DrawText( offset, GetGdiDrawFont(), conditions.AsChar(), color );
 			}
		}
	}
}

CObject* CEdBehaviorGraphEditor::GraphGetOwner()
{
	// used to set parent of newly created objects, return currently set stage
	return m_currentRoot;
}

CGraphBlock* CEdBehaviorGraphEditor::GraphCreateBlock( const GraphBlockSpawnInfo& info )
{
	ASSERT( info.GetClass() );
	ASSERT( GraphSupportsBlockClass( info.GetClass() ) );

	if ( !PrepareForGraphStructureModified() )
	{
		return NULL;
	}

	// Create block
	CGraphBlock *block = m_currentRoot->CreateChildNode( info );

	CEdGraphEditor::GraphStructureModified();

	return block;
}

Bool CEdBehaviorGraphEditor::GraphCanRemoveBlock( CGraphBlock *block ) const
{
	if ( !CanBeModify() || block->IsA< CBehaviorGraphOutputNode >() )
		return false;

	return true;
}

Bool CEdBehaviorGraphEditor::GraphRemoveBlock( CGraphBlock *block )
{
	// don't delete output nodes
	if ( !CanBeModify() || block->IsA< CBehaviorGraphOutputNode >() )
		return false;

	TDynArray< CGraphBlock* >& blocks = GraphGetBlocks();
	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		if ( blocks[i] == block )
		{
			if ( !PrepareForGraphStructureModified() )
			{
				return false;
			}

			CBehaviorGraph* graph = GetEditor()->GetBehaviorGraph();
			if ( graph->GetDefaultStateMachine() == block )
			{
				graph->SetDefaultStateMachine( NULL );
			}

			// Notify
			block->OnDestroyed();
			block->BreakAllLinks();

			// Remove from tree
			m_currentRoot->RemoveChildNode( block );

			// Notify graph
			CEdGraphEditor::GraphStructureModified();
			return true;
		}
	}

	return false;
}

Bool CEdBehaviorGraphEditor::GraphPasteBlocks( const TDynArray< Uint8 >& data, TDynArray< CGraphBlock* >& pastedBlocks, Bool relativeSpawn, const Vector& spawnPosition )
{
	return GraphPasteBlocks( data, pastedBlocks, relativeSpawn, spawnPosition, Cast< CBehaviorGraphContainerNode >( GraphGetOwner() ) );
}

Bool CEdBehaviorGraphEditor::GraphPasteBlocks( const TDynArray< Uint8 >& data, TDynArray< CGraphBlock* >& pastedBlocks, Bool relativeSpawn, const Vector& spawnPosition, CBehaviorGraphContainerNode *parent )
{
	if ( !CanBeModify() )
	{
		return false;
	}

	// Deserialize
	CMemoryFileReader reader( data, 0 );
	CDependencyLoader loader( reader, NULL );
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = parent;
	if ( !loader.LoadObjects( loadingContext ) )
	{
		// No objects loaded
		return false;
	}
	TDynArray< CObject* > &objects = loadingContext.m_loadedRootObjects;

	if ( !PrepareForGraphStructureModified() )
	{
		return false;
	}

	// Get spawned entities
	TDynArray< CGraphBlock* > suppBlocks;
	TDynArray< CGraphBlock* > notSuppBlocks;

	for ( Uint32 i=0; i<objects.Size(); i++ )
	{
		CGraphBlock* block = Cast< CGraphBlock >( loadingContext.m_loadedRootObjects[i] );
		if ( block )
		{
			// CBehaviorGraphStateTransitionNode can be copy/paste but not visible in context menu (witch use ChildNodeClassSupported function)
			if ( parent->ChildNodeClassSupported( block->GetClass() ) || block->IsA<CBehaviorGraphStateTransitionNode>())
			{
				suppBlocks.PushBack( block );
			}
			else
			{
				notSuppBlocks.PushBack( block );
			}
		}
	}

	// Call post load of spawned objects
	loader.PostLoad();

	// Break links for not supported blocks
	for ( Uint32 i=0; i<notSuppBlocks.Size(); i++ )
	{
		notSuppBlocks[i]->BreakAllLinks();
	}

	// Offset root components by specified spawn point
	if ( relativeSpawn && !suppBlocks.Empty() )
	{
		Vector rootPosition = suppBlocks[0]->GetPosition();
		for ( Uint32 i=0; i<suppBlocks.Size(); i++ )
		{
			CGraphBlock* block = suppBlocks[i];
			Vector delta = block->GetPosition() - rootPosition;
			block->SetPosition( spawnPosition + delta );
		}
	}

	// Add to list of blocks
	for ( Uint32 i=0; i<suppBlocks.Size(); i++ )
	{
		CGraphBlock* block = suppBlocks[i];
		block->InvalidateLayout();	

		parent->OnChildNodeAdded( block );
	}

	// Done
	pastedBlocks = suppBlocks;
	CEdGraphEditor::GraphStructureModified();

	return true;
}

Bool CEdBehaviorGraphEditor::PrepareForGraphStructureModified()
{
	if ( !CanBeModify() )
	{
		return false;
	}

	GetEditor()->UseBehaviorInstance( false );

	return true;
}

void CEdBehaviorGraphEditor::GraphStructureModified()
{
	ASSERT( CanBeModify() );

	// Check if all transition block are valid
	TDynArray< CGraphBlock* > blocksToRemove;

	TDynArray< CGraphBlock* >& blocks = GraphGetBlocks();
	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		// All blocks
		if ( blocks[i]->IsA< CBehaviorGraphStateTransitionGlobalBlendNode >() )
		{
			const TDynArray<CGraphSocket*>& sockets = blocks[i]->GetSockets();

			// All sockets
			for ( Uint32 j=0; j<sockets.Size(); j++ )
			{
				if ( sockets[j]->IsA<CBehaviorGraphTransitionSocket>())
				{
					const CGraphSocket* socket = sockets[j];
					Uint32 connSize = socket->GetConnections().Size();

					if (( socket->GetDirection() == LSD_Input && connSize != 0 ) ||
						( socket->GetDirection() == LSD_Output && connSize != 1 ))
					{
						blocksToRemove.PushBack( blocks[i] );
						break;
					}
				}
			}
		}
		else if ( blocks[i]->IsA<CBehaviorGraphStateTransitionNode>() )
		{			
			const TDynArray<CGraphSocket*>& sockets = blocks[i]->GetSockets();

			// All sockets
			for ( Uint32 j=0; j<sockets.Size(); j++ )
			{
				if ( sockets[j]->IsA<CBehaviorGraphTransitionSocket>())
				{
					const TDynArray< CGraphConnection* >& connections = sockets[j]->GetConnections();

					if (connections.Size()<1)
					{
						blocksToRemove.PushBack(blocks[i]);
						break;
					}
				}
			}
		}
	}

	if ( blocksToRemove.Size() > 0 )
	{
		for ( Uint32 i=0; i<blocksToRemove.Size(); i++ )
		{
			GraphRemoveBlock(blocksToRemove[i]);
		}

		DeselectAllBlocks();
	}
}

Bool CEdBehaviorGraphEditor::GraphSupportsBlockClass( CClass *blockClass ) const
{
	ASSERT( blockClass );
	return m_currentRoot->ChildNodeClassSupported( blockClass );
}

Vector CEdBehaviorGraphEditor::GraphGetBackgroundOffset() const
{
	return m_offset;
}

void CEdBehaviorGraphEditor::GraphSetBackgroundOffset( const Vector& offset )
{
	m_offset = offset;
}

void CEdBehaviorGraphEditor::PaintCanvas( Int32 width, Int32 height )
{
#ifdef BEH_DEBUG_PRINT
	CTimeCounter timeCounter;
#endif

	if ( GetBehaviorGraphInstance() )
	{
		CEdGraphEditor::PaintCanvas( width, height );

		GetEditor()->OnPrintNodes( this );
	}
	else
	{
		SetOffset( wxPoint( 0, 0 ) );
		SetScale( 1.f );

		Clear( wxColour(255, 0, 255) );
// 		Gdiplus::Bitmap* img = m_background.Draw( width, height );
// 		DrawImage( img, 0, 0 );
		
		String text = TXT("You should load entity");

		wxPoint size = TextExtents( GetGdiDrawFont(), text );
		wxPoint offset(  width/2 - size.x/2, height/2 - size.y/2 );		

		DrawRect( offset.x, offset.y , size.x, size.y, wxColour( 50,50,50 ) );

		DrawText( offset + wxPoint(1,1), GetGdiDrawFont(), text.AsChar(), wxColour( 0,0,0 ) );
		DrawText( offset, GetGdiDrawFont(), text.AsChar(), wxColour( 200, 200, 200 ) );
	}

#ifdef BEH_DEBUG_PRINT
	const Float timeUsed = timeCounter.GetTimePeriod();
	BEH_LOG( TXT("Graph print canvas took %f"), timeUsed );
#endif
}

void CEdBehaviorGraphEditor::DisplayUpperLevel()
{
	// in case we were recently in some node that was removed by someone else, get to main root
	if ( ! m_currentRoot )
	{
		m_currentRoot = GetBehaviorGraph()->GetRootNode();
	}

	CBehaviorGraphNode* prevNode = m_currentRoot;

	CObject* parent = m_currentRoot->GetParent();
	if ( parent )
	{
		CBehaviorGraphContainerNode* contNode = Cast< CBehaviorGraphContainerNode >( parent );
		if ( contNode )
		{
			SetRootNode( contNode );

			FocusOnBehaviorNode( prevNode );
		}
	}
}

void CEdBehaviorGraphEditor::DisplayBlockContent( CBehaviorGraphNode *node )
{
	CBehaviorGraphContainerNode* contNode = Cast< CBehaviorGraphContainerNode >( node );
	if ( !contNode )
	{
		return;
	}

	if ( contNode->CanBeExpanded() )
	{
		SetRootNode( contNode );
	}
}


Bool CEdBehaviorGraphEditor::IsDraggedByClickOnInnerArea( wxMouseEvent& event, CGraphBlock* block )
{
	wxPoint mousePoint = event.GetPosition();
	wxPoint graphPoint = ClientToCanvas( mousePoint );

	if ( CBehaviorGraph2DVariableNode* varBlock = Cast< CBehaviorGraph2DVariableNode >( block ) )
	{
		const BlockLayoutInfo *info = m_layout.FindPtr( varBlock );

		// Select only if layout data is valid
		if ( info && info->m_visible && !info->m_freeze )
		{
			CBehaviorGraphInstance& bgi = *GetEditor()->GetBehaviorGraphInstance();
			CBehaviorGraph2DVariableNode* vec2D = Cast< CBehaviorGraph2DVariableNode >( varBlock );
			if ( vec2D )
			{
				wxRect rect;
				rect.x = varBlock->GetPosition().X + BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
				rect.y = varBlock->GetPosition().Y + info->m_titleRect.height + BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
				rect.width = info->m_windowSize.x - 2 * BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
				rect.height = info->m_windowSize.y - info->m_titleRect.height - 2 * BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
				if ( rect.Contains( graphPoint ) )
				{
					return false;
				}
			}
		}
		return true;
	}

	if ( CBehaviorGraphVariableBaseNode* varBlock = Cast< CBehaviorGraphVariableBaseNode >( m_activeItem.Get() ) )
	{
		const BlockLayoutInfo *info = m_layout.FindPtr( varBlock );

		// Select only if layout data is valid
		if ( info && info->m_visible && !info->m_freeze )
		{
			wxRect r(
				varBlock->GetPosition().X,
				varBlock->GetPosition().Y + info->m_windowSize.y - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE,
				info->m_windowSize.x,// - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE,
				BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE );
			if ( r.Contains( graphPoint ) )
			{
				return false;
			}
		}
		return true;
	}

	return CEdGraphEditor::IsDraggedByClickOnInnerArea( event, block );
}


Bool CEdBehaviorGraphEditor::MouseClickAndMoveActions( wxMouseEvent& event )
{
	wxPoint mousePoint = event.GetPosition();
	wxPoint graphPoint = ClientToCanvas( mousePoint );
	if ( m_action == MA_None && event.LeftIsDown() )
	{
		if ( m_activeItem.Get() && m_activeItem.Get()->IsA< CBehaviorGraph2DVariableNode >() )
		{
			CBehaviorGraph2DVariableNode* block = static_cast< CBehaviorGraph2DVariableNode* >( m_activeItem.Get() );

			const BlockLayoutInfo *info = m_layout.FindPtr( block );

			// Select only if layout data is valid
			if ( info && info->m_visible && !info->m_freeze )
			{
				CBehaviorGraphInstance& bgi = *GetEditor()->GetBehaviorGraphInstance();
				CBehaviorGraph2DVariableNode* vec2D = Cast< CBehaviorGraph2DVariableNode >( block );
				if ( vec2D )
				{
					wxRect rect;
					rect.x = block->GetPosition().X + BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
					rect.y = block->GetPosition().Y + info->m_titleRect.height + BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
					rect.width = info->m_windowSize.x - 2 * BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
					rect.height = info->m_windowSize.y - info->m_titleRect.height - 2 * BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
					if ( rect.Contains( graphPoint ) )
					{
						float x = graphPoint.x - rect.x;
						x /= rect.width;
						float y = rect.y + rect.height - graphPoint.y;
						y /= rect.height;
						Vector d = vec2D->GetMaxVal( bgi ) - vec2D->GetMinVal( bgi );
						vec2D->SetVectorValue( bgi, vec2D->GetMinVal( bgi ) + d * Vector( x, y, 0, 0 ) );
						Repaint();
						return true;
					}
				}
			}
		}

		if ( m_activeItem.Get() && m_activeItem.Get()->IsA< CBehaviorGraphVariableBaseNode >() )
		{
			CBehaviorGraphVariableBaseNode* block = static_cast< CBehaviorGraphVariableBaseNode* >( m_activeItem.Get() );

			const BlockLayoutInfo *info = m_layout.FindPtr( block );

			// Select only if layout data is valid
			if ( info && info->m_visible && !info->m_freeze )
			{
				wxRect r(
				block->GetPosition().X,
				block->GetPosition().Y + info->m_windowSize.y - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE,
				info->m_windowSize.x - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE,
				BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE );
				if ( r.Contains( graphPoint ) )
				{
					Float f = graphPoint.x - block->GetPosition().X - ( BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE >> 1 );
					f /= info->m_windowSize.x - ( BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE << 1 );
					f = Clamp( f, 0.0f, 1.0f );
					CBehaviorGraphInstance& bgi = *GetEditor()->GetBehaviorGraphInstance();
					block->SetValue( bgi, block->GetMin( bgi ) + f * ( block->GetMax( bgi ) - block->GetMin( bgi ) ) );
					Repaint();
					return true;
				}
			}
		}
	}

	if ( m_action == MA_None && event.LeftDown() )
	{
		if ( m_activeItem.Get() && m_activeItem.Get()->IsA< CBehaviorGraphVariableBaseNode >() )
		{
			CBehaviorGraphVariableBaseNode* block = static_cast< CBehaviorGraphVariableBaseNode* >( m_activeItem.Get() );

			const BlockLayoutInfo *info = m_layout.FindPtr( block );

			// Select only if layout data is valid
			if ( info && info->m_visible && !info->m_freeze )
			{
				wxRect r(
					block->GetPosition().X + info->m_windowSize.x - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE,
					block->GetPosition().Y + info->m_windowSize.y - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE,
					BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE,
					BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE );
				if ( r.Contains( graphPoint ) )
				{
					CName name( block->GetVariableName() );
					Bool found = false;
					for ( TSet< CBehaviorEditorEffect* >::iterator it = m_effects.Begin(); it != m_effects.End(); ++it )
					{
						if ( (*it)->GetType() == BEET_INTERPOLATE_FLOAT )
						{
							CBehaviorEditorEffectInterpolateFloat* b = (CBehaviorEditorEffectInterpolateFloat*)(*it);
							if ( name == b->GetVariableName() )
							{
								b->SetAccumulationSpeed( - b->GetAccumulationSpeed() );
								found = true;
								break;
							}
						}
					}
					if ( !found ) AddEffect( new CBehaviorEditorEffectInterpolateFloat( name, this ) );
				}
			}
		}
	}
	return false;
}
void CEdBehaviorGraphEditor::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	MouseClickAndMoveActions( event );
	CEdGraphEditor::MouseMove( event, delta );
}
void CEdBehaviorGraphEditor::MouseClick( wxMouseEvent& event )
{
	if ( !GetBehaviorGraphInstance() )
	{
		m_background.OnClick();
	}
	
	MouseClickAndMoveActions( event );

	CEdGraphEditor::MouseClick( event );
}

void CEdBehaviorGraphEditor::OnMouseLeftDblClick( wxMouseEvent &event )
{
	if ( m_action == MA_None )
	{
		if ( !m_activeItem.Get() )
		{
			CUndoBehaviorGraphSetRoot::CreateStep( *m_undoManager, this );
			DisplayUpperLevel();
		}
		else
		{
			if ( m_activeItem.Get()->IsA< CBehaviorGraphContainerNode >() )
			{
				CUndoBehaviorGraphSetRoot::CreateStep( *m_undoManager, this );
				DisplayBlockContent( static_cast< CBehaviorGraphContainerNode* >( m_activeItem.Get() ) );
			}
			else if ( CanBeModify() && m_activeItem.Get()->IsA< CBehaviorGraphAnimationNode >() && GetEditor()->GetStaticPropertyBrowser() )
			{
				// quite ugly way to find proper property and activate editor...
				TQueue< CBasePropItem * > propsToTest;

				propsToTest.Push( GetEditor()->GetStaticPropertyBrowser()->GetRootItem() );

				while( !propsToTest.Empty() )
				{
					CBasePropItem *currProp = propsToTest.Front();
					propsToTest.Pop();

					if ( currProp->GetCaption() == TXT("animationName") )
					{
						// pretend that user clicked on the button
						GetEditor()->GetStaticPropertyBrowser()->SelectItem( currProp );

						const TDynArray< CPropertyButton* >& buttons = currProp->GetButtons();

						ASSERT( buttons.Size() >= 1 );

						buttons[0]->Clicked();

						break;
					}
				
					const TDynArray< CBasePropItem *> &blockProperties = currProp->GetChildren();
					for( Uint32 i=0; i<blockProperties.Size(); ++i )
					{
						propsToTest.Push( blockProperties[i] );
					}
				}
			}
		}
	}
}

void CEdBehaviorGraphEditor::OnKeyDown( wxKeyEvent &event )
{
	if ( event.GetKeyCode() == WXK_DELETE )
		DeleteSelection();

	if (event.GetKeyCode() == WXK_F4)
		SGarbageCollector::GetInstance().CollectNow();

	if ( event.GetKeyCode() == WXK_SHIFT )
	{
		if ( ! m_displayConditionsAlternate )
		{
			m_displayConditionsAlternate = true;
			Repaint();
		}
	}

	CEdGraphEditor::OnKeyDown( event );
}

void CEdBehaviorGraphEditor::OnKeyUp( wxKeyEvent &event )
{
	if ( event.GetKeyCode() == WXK_SHIFT )
	{
		if ( m_displayConditionsAlternate )
		{
			m_displayConditionsAlternate = false;
			Repaint();
		}
	}
}

void CEdBehaviorGraphEditor::InitClipboardMenu( wxMenu& menu )
{
	if ( !CanBeModify() )
	{
		return;
	}

	menu.Append( ID_EDIT_COPY, TXT("Copy") );
	menu.Connect( ID_EDIT_COPY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorEditor::OnEditCopy ), NULL, GetEditor() );
	menu.Append( ID_EDIT_CUT, TXT("Cut") );
	menu.Connect( ID_EDIT_CUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorEditor::OnEditCut ), NULL, GetEditor() );
	menu.Append( ID_EDIT_DELETE, TXT("Delete") );
	menu.Connect( ID_EDIT_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorEditor::OnEditDelete ), NULL, GetEditor() );

	// Paste menu
	if ( wxTheClipboard->Open() )
	{
		CClipboardData data( String(TXT("Blocks")) + ClipboardChannelName() );
		if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
		{			
			menu.Append( ID_EDIT_PASTE, TXT("Paste") );
			menu.Connect( ID_EDIT_PASTE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnPasteHere ), NULL, this );
		} 
		wxTheClipboard->Close();
	}

	TDynArray<String> snaphots;
	GetGraphSnaphots(snaphots);
	if(snaphots.Size() >0)
	{
		menu.AppendSeparator();
		wxMenu* subMenu = new wxMenu();

		for (Uint32 i=0; i<snaphots.Size(); i++)
		{
			subMenu->Append( ID_GOTO_SNAPSHOT+i, snaphots[i].AsChar(), wxEmptyString );
			menu.Connect( ID_GOTO_SNAPSHOT+i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnGoToSnapshot ), NULL, this );
		}

		menu.AppendSubMenu(subMenu, wxT("Pose"));
	}

	menu.AppendSeparator();
	menu.Append( ID_EDIT_TRACK_NAMES, TXT("Edit track names") );
	menu.Connect( ID_EDIT_TRACK_NAMES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnEditTrackNames ), NULL, this );
}

void CEdBehaviorGraphEditor::OnPasteHere( wxCommandEvent &event )
{	
	if ( !CanBeModify() )
	{
		return;
	}

	Vector pastePos( m_lastMousePos.x, m_lastMousePos.y, 0.0f );
	Paste( &pastePos );
}

void CEdBehaviorGraphEditor::InitCollapseToStageMenu( wxMenu& menu )
{
	if ( !CanBeModify() )
	{
		return;
	}

	TDynArray< CGraphBlock* > selection;

	GetSelectedBlocks( selection );

	if ( selection.Size() > 0 )
	{
		menu.AppendSeparator();
		menu.Append( ID_COLLAPSE_TO_STAGE, TXT("Collapse to Stage"), wxEmptyString );
		menu.Connect( ID_COLLAPSE_TO_STAGE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCollapseToStage ), NULL, this );
	}
}

void CEdBehaviorGraphEditor::InitLinkedDefaultContextMenu( wxMenu& menu )
{
	if ( !CanBeModify() )
	{
		return;
	}

	CEdGraphEditor::InitLinkedDefaultContextMenu( menu );

	menu.AppendSeparator();

	InitClipboardMenu( menu );	

	InitCollapseToStageMenu( menu );
}

void CEdBehaviorGraphEditor::InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu )
{
	if ( !CanBeModify() )
	{
		return;
	}

	if ( block->IsA< CBehaviorGraphStateTransitionNode >() )
	{
		CBehaviorGraphStateTransitionNode* transition = Cast< CBehaviorGraphStateTransitionNode >( block );

		const CBehaviorGraphStateNode* dest = transition->GetDestState();
		const CBehaviorGraphStateNode* src = transition->GetSourceState();

		if ( dest && src )
		{
			if ( dest )
			{
				menu.Append( ID_GOTO_TRANS_DEST, TXT("Go to dest state"), wxEmptyString );
				menu.Connect( ID_GOTO_TRANS_DEST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnGoToTransitionDestState ), NULL, this );
			}
			if ( src )
			{
				menu.Append( ID_GOTO_TRANS_SRC, TXT("Go to src state"), wxEmptyString );
				menu.Connect( ID_GOTO_TRANS_SRC, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnGoToTransitionSrcState ), NULL, this );
			}
			menu.AppendSeparator();
		}
	}

	if ( block->IsA< CBehaviorGraphPointerStateNode >() )
	{
		CBehaviorGraphPointerStateNode* pointer = Cast< CBehaviorGraphPointerStateNode >( block );

		const CBehaviorGraphStateNode* state = pointer->GetPointedState();
		if ( state )
		{
			menu.Append( ID_GOTO_POINTED_STEATE, TXT("Go to pointed state"), wxEmptyString );
			menu.Connect( ID_GOTO_POINTED_STEATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnGoToPointedtState ), NULL, this );
			menu.AppendSeparator();
		}
	}

	CEdGraphEditor::InitLinkedBlockContextMenu( block, menu );

	if ( !block->IsA< CBehaviorGraphValueNode >() && GetEditor()->IsActivationAlphaEnabled() )
	{
		CBehaviorGraphNode *node = SafeCast<CBehaviorGraphNode>( block );

		menu.AppendSeparator();
		if ( m_debugVisualizers.KeyExist( node ) )
		{
			CBehaviorDebugVisualizer **debugVisualizerPtr = m_debugVisualizers.FindPtr( (CBehaviorGraphNode*)block );
			if ( debugVisualizerPtr )
			{
				CBehaviorDebugVisualizer *debugVisualizer = *debugVisualizerPtr;

				menu.Append( ID_REMOVE_VISUALIZER, TXT("Remove debug visualizer"), wxEmptyString );
				menu.Connect( ID_REMOVE_VISUALIZER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnRemoveDebugVisualizer ), NULL, this );

				wxMenu* submenu = new wxMenu();

				submenu->Append( ID_CHANGE_VISUALIZER, TXT("Change bones"), wxEmptyString );
				menu.Connect( ID_CHANGE_VISUALIZER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnChangeDebugVisualizer ), NULL, this );
				submenu->Append( ID_VISUALIZER_CHOOSE_HELPERS, TXT("Choose bones helpers"), wxEmptyString );
				menu.Connect( ID_VISUALIZER_CHOOSE_HELPERS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnDebugVisualizerChooseHelpers ), NULL, this );
				submenu->AppendCheckItem( ID_VISUALIZER_TOGGLE_AXIS, TXT("Toggle axis visibility"), wxEmptyString )->Check( debugVisualizer->IsAxisVisible() );
				menu.Connect( ID_VISUALIZER_TOGGLE_AXIS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnDebugVisualizerToggleBonesAxis), NULL, this );
				submenu->AppendCheckItem( ID_VISUALIZER_TOGGLE_NAMES, TXT("Toggle names visibility"), wxEmptyString )->Check( debugVisualizer->IsNameVisible() );
				menu.Connect( ID_VISUALIZER_TOGGLE_NAMES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnDebugVisualizerToggleBonesName ), NULL, this );
				submenu->AppendCheckItem( ID_VISUALIZER_TOGGLE_BOX, TXT("Toggle box visibility"), wxEmptyString )->Check( debugVisualizer->IsBoxVisible() );
				menu.Connect( ID_VISUALIZER_TOGGLE_BOX, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnDebugVisualizerToggleBox ), NULL, this );

				wxMenu* boneStyleMenu = new wxMenu();
				CBehaviorDebugVisualizer::EBehaviorVisualizerBoneStyle boneStyle = debugVisualizer->GetBoneStyle();
				{
					wxMenuItem* item = boneStyleMenu->AppendRadioItem( ID_VISUALIZER_BONE_STYLE_LINE, TXT("Line"), wxEmptyString );
					menu.Connect( ID_VISUALIZER_BONE_STYLE_LINE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnDebugVisualizerBoneStyle), NULL, this );
					if (debugVisualizer->GetBoneStyle() == CBehaviorDebugVisualizer::BVBS_Line) item->Check(true);
					else item->Check(false);
				}
				{
					wxMenuItem* item = boneStyleMenu->AppendRadioItem( ID_VISUALIZER_BONE_STYLE_3D, TXT("3D"), wxEmptyString );
					menu.Connect( ID_VISUALIZER_BONE_STYLE_3D, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnDebugVisualizerBoneStyle), NULL, this );
					if (debugVisualizer->GetBoneStyle() == CBehaviorDebugVisualizer::BVBS_3D) item->Check(true);
					else item->Check(false);
				}
				submenu->AppendSubMenu(boneStyleMenu, wxT("Bone style"));

				menu.AppendSubMenu(submenu, wxT("Visualizer options"));
			}
		}
		else
		{
			menu.Append( ID_ADD_VISUALIZER, TXT("Add debug visualizer"), TXT("Add debug visualizer without selecting bones") );
			menu.Connect( ID_ADD_VISUALIZER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddDebugVisualizer ), NULL, this );
			menu.Append( ID_ADD_VISUALIZER_SB, TXT("Add debug visualizer (sb)"), TXT("Add debug visualizer with selecting bones") );
			menu.Connect( ID_ADD_VISUALIZER_SB, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddDebugVisualizer ), NULL, this );
		}
	}

	if ( block->GetClass() == CBehaviorGraphStateNode::GetStaticClass() || 
		 block->GetClass() == CBehaviorGraphScriptStateNode::GetStaticClass() || 
		 block->GetClass() == CBehaviorGraphScriptStateReportingNode::GetStaticClass() ||
		 block->GetClass() == CBehaviorGraphScriptComponentStateNode::GetStaticClass() ||
		 block->GetClass() == CBehaviorGraphDefaultSelfActStateNode::GetStaticClass() ||
		 block->GetClass() == CBehaviorGraphRecentlyUsedAnimsStateNode::GetStaticClass() )
	{
		CBehaviorGraphStateNode* state = Cast< CBehaviorGraphStateNode >( block );
		CBehaviorGraphStateMachineNode* parent = state->FindParent< CBehaviorGraphStateMachineNode >();

		if ( parent )
		{
			menu.AppendSeparator();
			menu.Append( ID_SET_START_STATE, TXT("Set state as default"), wxEmptyString );
			menu.Connect( ID_SET_START_STATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnSetDefaultState ), NULL, this );
			menu.AppendSeparator();
			menu.Append( ID_COPY_ALL_TRANSITIONS, TXT("Copy all Transitions"), wxEmptyString );
			menu.Connect( ID_COPY_ALL_TRANSITIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCopyAllTransitions ), NULL, this );
			menu.Append( ID_PASTE_ALL_TRANSITIONS, TXT("Paste all Transitions"), wxEmptyString );
			menu.Connect( ID_PASTE_ALL_TRANSITIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnPasteAllTransitions ), NULL, this );
		}
	}

	if ( block->IsA< CBehaviorGraphStateNode >() && !block->IsA< CBehaviorGraphComboStateNode >() )
	{
		menu.Append( ID_SET_STATE, TXT("Set state active"), wxEmptyString );
		menu.Connect( ID_SET_STATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnSetStateActive ), NULL, this );
	}

	else if ( block->IsA< CBehaviorGraphStateMachineNode >() )
	{
		menu.AppendSeparator();
		menu.Append( ID_SET_START_STATE_MACHINE, TXT("Set state machine as default"), wxEmptyString );
		menu.Connect( ID_SET_START_STATE_MACHINE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnSetDefaultStateMachine ), NULL, this );
	}

	else if ( block->IsA< CBehaviorGraphBlendMultipleNode >() || block->IsA< CBehaviorGraphBlendMultipleCondNode >() || block->IsA< CBehaviorGraphRandomSelectNode >() )
	{
		menu.AppendSeparator();
		menu.Append( ID_ADD_INPUT, TXT("Add input"), wxEmptyString );
		menu.Connect( ID_ADD_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddInput ), NULL, this );
	}

	/*else if ( block->IsA< CBehaviorGraphBlendMultipleWeightNode >() )
	{
		menu.AppendSeparator();
		menu.Append( ID_ADD_INPUT, TXT("Add input"), wxEmptyString );
		menu.Connect( ID_ADD_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddInputWithWeight ), NULL, this );
	}*/

	else if ( block->IsA< CBehaviorGraphAnimationManualSwitchNode >() )
	{
		CBehaviorGraphAnimationManualSwitchNode* node = Cast< CBehaviorGraphAnimationManualSwitchNode >( block );

		menu.AppendSeparator();
		menu.Append( ID_ADD_INPUT, TXT("Add input"), wxEmptyString );
		menu.Connect( ID_ADD_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddSwitchInput ), NULL, this );

		if ( node->GetInputNum() > 0 )
		{
			menu.Append( ID_REMOVE_INPUT, TXT("Remove input"), wxEmptyString );
			menu.Connect( ID_REMOVE_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnRemoveSwitchInput ), NULL, this );
		}
	}
	else if ( block->IsA< CBehaviorGraphAnimationEnumSwitchNode >() )
	{
		CBehaviorGraphAnimationEnumSwitchNode* node = Cast< CBehaviorGraphAnimationEnumSwitchNode >( block );
		menu.Append( ID_REFRESH_ENUM, TXT("Rebuild enum sockets"), wxEmptyString );
		menu.Connect( ID_REFRESH_ENUM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnRefreshEnumInput ), NULL, this );
	}

	else if ( block->IsA< CBehaviorGraphRandomNode >() )
	{
		menu.AppendSeparator();
		menu.Append( ID_ADD_INPUT, TXT("Add input"), wxEmptyString );
		menu.Connect( ID_ADD_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddRandomInput ), NULL, this );
	}

	else if ( block->IsExactlyA< CBehaviorGraphStateTransitionGlobalBlendNode >() )
	{
		menu.AppendSeparator();
		menu.Append( ID_CONVERT_GWS_TRANSITION, TXT("Convert to trans with streaming"), wxEmptyString );
		menu.Connect( ID_CONVERT_GWS_TRANSITION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnConvertGlobalWithStreamingTransitionCondition ), NULL, this );
	}

	if ( block->IsA< CBehaviorGraphContainerNode >() )
	{
		menu.AppendSeparator();
		menu.Append( ID_ADD_ANIMATION_INPUT, TXT("Add animation input..."), wxEmptyString );
		menu.Connect( ID_ADD_ANIMATION_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddContainerAnimInput ), NULL, this );
		menu.Append( ID_ADD_VALUE_INPUT, TXT("Add value input..."), wxEmptyString );
		menu.Connect( ID_ADD_VALUE_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddContainerValueInput ), NULL, this );
		menu.Append( ID_ADD_VECTOR_INPUT, TXT("Add vector value input..."), wxEmptyString );
		menu.Connect( ID_ADD_VECTOR_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddContainerVectorValueInput ), NULL, this );
		menu.Append( ID_ADD_MIMIC_INPUT, TXT("Add mimic input..."), wxEmptyString );
		menu.Connect( ID_ADD_MIMIC_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddContainerMimicInput ), NULL, this );
	}

	else if (	block->IsA< CBehaviorGraphStateTransitionNode >() && 
				!block->IsA< CBehaviorGraphComboTransitionNode >() && !block->IsA< CBehaviorGraphGlobalComboTransitionNode >() && 
				!block->IsA< CBehaviorGraphFlowTransitionNode >() )
	{
		menu.AppendSeparator();
		menu.Append( ID_COPY_TRANSITION_DATA, TXT("Copy transition condition"), wxEmptyString );
		menu.Connect( ID_COPY_TRANSITION_DATA, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCopyTransitionCondition ), NULL, this );
		menu.Append( ID_PASTE_TRANSITION_DATA, TXT("Paste transition condition"), wxEmptyString );
		menu.Connect( ID_PASTE_TRANSITION_DATA, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnPasteTransitionCondition ), NULL, this );

		// Get possible conversion
		TDynArray<IBehaviorStateTransitionCondition*> conversions;
		
		CBehaviorGraphStateTransitionNode* behaviourTransitionNode 
			= SafeCast<CBehaviorGraphStateTransitionNode>( block );
		if ( behaviourTransitionNode != NULL && behaviourTransitionNode->GetTransitionCondition() != NULL )
		{
			behaviourTransitionNode->GetTransitionCondition()->EnumConversions( conversions );
		}
		

		if (!conversions.Empty())
		{
			wxMenu* subMenu = new wxMenu();

			for (Uint32 i=0; i<conversions.Size(); i++)
			{
				String conversionName = conversions[i]->GetFriendlyName();
				subMenu->Append( ID_CONVERT_TRANSITION+i, conversionName.AsChar(), wxEmptyString );
				menu.Connect( ID_CONVERT_TRANSITION+i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnConvertTransitionCondition ), NULL, this );
			}

			menu.AppendSubMenu(subMenu, wxT("Convert transition condition"));
		}
	}

	menu.AppendSeparator();

	InitClipboardMenu( menu );	

	InitCollapseToStageMenu( menu );
}

void CEdBehaviorGraphEditor::InitLinkedSocketContextMenu( CGraphSocket *socket, wxMenu &menu )
{
	CGraphBlock *block = socket->GetBlock();

	if ( !CanBeModify() || !block )
	{
		return;
	}

	if ( socket->IsA< CBehaviorGraphStateOutSocket >() )
	{
		if ( block->IsA< CBehaviorGraphComboStateNode >() )
		{
			menu.Append( ID_ADD_GLOBAL_COMBO_TRANS_BLOCK, TXT("Connect global transition"), wxEmptyString );
			menu.Connect( ID_ADD_GLOBAL_COMBO_TRANS_BLOCK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddGlobalComboTransition ), NULL, this );
		}
		else if ( block->IsA< CBehaviorGraphStateNode >() )
		{
			menu.Append( ID_ADD_GLOBAL_TRANS_BLOCK, TXT("Connect global transition"), wxEmptyString );
			menu.Connect( ID_ADD_GLOBAL_TRANS_BLOCK, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnAddGlobalTransition ), NULL, this );
		}
		
		return;
	}

	CEdGraphEditor::InitLinkedSocketContextMenu( socket, menu );

	if ( ( block->IsA< CBehaviorGraphBlendMultipleNode >() || block->IsA< CBehaviorGraphBlendMultipleCondNode >()  || block->IsA< CBehaviorGraphRandomSelectNode >() ) && socket->IsA< CBehaviorGraphAnimationInputSocket >() )
	{
		menu.AppendSeparator();
		menu.Append( ID_REMOVE_INPUT, TXT("Remove input"), wxEmptyString );
		menu.Connect( ID_REMOVE_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnRemoveInput ), NULL, this );
	}

	if ( block->IsA< CBehaviorGraphContainerNode >() &&
		 ( Cast< CBehaviorGraphContainerNode >(block)->GetAnimationInputs().Exist( socket->GetName() ) || 
		   Cast< CBehaviorGraphContainerNode >(block)->GetValueInputs().Exist( socket->GetName() ) ) )
	{
		menu.AppendSeparator();
		menu.Append( ID_REMOVE_INPUT, TXT("Remove input"), wxEmptyString );
		menu.Connect( ID_REMOVE_INPUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnRemoveContainerInput ), NULL, this );
	}

	menu.AppendSeparator();

	InitClipboardMenu( menu );

	InitCollapseToStageMenu( menu );
}


void CEdBehaviorGraphEditor::OnSetDefaultState( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphStateNode >() );

	CBehaviorGraphStateNode* state = SafeCast< CBehaviorGraphStateNode >( block );
	
	ASSERT( m_currentRoot->IsA< CBehaviorGraphStateMachineNode >() );
	CBehaviorGraphStateMachineNode* stateMachine = SafeCast< CBehaviorGraphStateMachineNode >( m_currentRoot );

	stateMachine->SetDefaultState( state );

	// repaint graph to hilite new default node
	Repaint();
}

void CEdBehaviorGraphEditor::OnSetStateActive( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphStateNode >() );

	CBehaviorGraphStateNode* state = SafeCast< CBehaviorGraphStateNode >( block );

	ASSERT( m_currentRoot->IsA< CBehaviorGraphStateMachineNode >() );
	CBehaviorGraphStateMachineNode* stateMachine = SafeCast< CBehaviorGraphStateMachineNode >( m_currentRoot );

	stateMachine->SetStateActive( state, *GetEditor()->GetBehaviorGraphInstance() );

	Repaint();
}

void CEdBehaviorGraphEditor::OnSetDefaultStateMachine( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphStateMachineNode >() );

	CBehaviorGraphStateMachineNode* stateMachine = SafeCast< CBehaviorGraphStateMachineNode >( block );

	GetBehaviorGraph()->SetDefaultStateMachine( stateMachine );

	// repaint graph to hilite new default node
	Repaint();
}

void CEdBehaviorGraphEditor::OnGoToTransitionDestState( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphStateTransitionNode* transition = static_cast< CBehaviorGraphStateTransitionNode* >( m_activeItem.Get() );
	if ( transition->GetDestState() )
	{
		FocusOnBehaviorNode( transition->GetDestState() );
	}
	else
	{
		ASSERT( transition->GetDestState() );
	}
}

void CEdBehaviorGraphEditor::OnGoToTransitionSrcState( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphStateTransitionNode* transition = static_cast< CBehaviorGraphStateTransitionNode* >( m_activeItem.Get() );
	if ( transition->GetSourceState() )
	{
		FocusOnBehaviorNode( transition->GetSourceState() );
	}
	else
	{
		ASSERT( transition->GetSourceState() );
	}
}

void CEdBehaviorGraphEditor::OnGoToPointedtState( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphPointerStateNode* pointer = static_cast< CBehaviorGraphPointerStateNode* >( m_activeItem.Get() );
	if ( pointer->GetPointedState() )
	{
		FocusOnBehaviorNode( pointer->GetPointedState() );
	}
	else
	{
		ASSERT( pointer->GetPointedState() );
	}
}

void CEdBehaviorGraphEditor::OnAddInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphBlendMultipleNode >() || block->IsA< CBehaviorGraphBlendMultipleCondNode >() || block->IsA< CBehaviorGraphRandomSelectNode >() );

	if ( CBehaviorGraphBlendMultipleNode* nodeA = Cast< CBehaviorGraphBlendMultipleNode >( block ) )
	{
		CUndoBehaviorGraphBlendNodeInput::CreateAddingStep( *m_undoManager, this, nodeA );
		nodeA->AddInput();
	}

	if ( CBehaviorGraphBlendMultipleCondNode* nodeB = Cast< CBehaviorGraphBlendMultipleCondNode >( block ) )
	{
		nodeB->AddInput();
	}

	if ( CBehaviorGraphRandomSelectNode* node = Cast< CBehaviorGraphRandomSelectNode >( block ) )
	{
		node->AddInput();
	}
}

void CEdBehaviorGraphEditor::OnAddInputWithWeight( wxCommandEvent& event )
{
	/*ASSERT( m_activeItem );

	CGraphBlock* block = SafeCast< CGraphBlock >( m_activeItem );
	ASSERT( block->IsA< CBehaviorGraphBlendMultipleWeightNode >() );

	CBehaviorGraphBlendMultipleWeightNode* node = SafeCast< CBehaviorGraphBlendMultipleWeightNode >( block );

	node->AddInput();*/
}

void CEdBehaviorGraphEditor::OnAddRandomInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphRandomNode >() );

	CBehaviorGraphRandomNode* node = SafeCast< CBehaviorGraphRandomNode >( block );

	CUndoBehaviorGraphRandomNodeInput::CreateAddingStep( *m_undoManager, this, node );
	node->AddInput();
}

void CEdBehaviorGraphEditor::OnAddSwitchInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphAnimationManualSwitchNode >() );

	CBehaviorGraphAnimationManualSwitchNode* node = SafeCast< CBehaviorGraphAnimationManualSwitchNode >( block );

	CUndoBehaviorGraphSwitchNodeInput::CreateAddingStep( *m_undoManager, this, node );
	node->AddInput();
}

void CEdBehaviorGraphEditor::OnRemoveInput( wxCommandEvent &event )
{
	ASSERT( m_activeItem.Get() );

	CGraphSocket *socket = static_cast< CGraphSocket* >( m_activeItem.Get() );
	CGraphBlock *block = socket->GetBlock();

	ASSERT( block->IsA< CBehaviorGraphBlendMultipleNode >() || block->IsA< CBehaviorGraphBlendMultipleCondNode >() || block->IsA< CBehaviorGraphRandomSelectNode >() );
	ASSERT( socket->IsA< CBehaviorGraphAnimationInputSocket >() );

	const TDynArray< CGraphSocket* >& sockets = block->GetSockets();

	Uint32 indexToRemove = 0;
	for( Uint32 i=0; i<sockets.Size(); ++i )
	{
		if ( sockets[i] == socket )
			break;

		if ( sockets[i]->IsA< CBehaviorGraphAnimationInputSocket >() )
			++indexToRemove;
	}

	ASSERT( indexToRemove < sockets.Size() );

	if ( CBehaviorGraphBlendMultipleNode* nodeA = Cast< CBehaviorGraphBlendMultipleNode >( block ) )
	{
		CUndoBehaviorGraphBlendNodeInput::CreateRemovingStep( *m_undoManager, this, nodeA );
		nodeA->RemoveInput( indexToRemove );
	}
	else if ( CBehaviorGraphBlendMultipleCondNode* nodeB = Cast< CBehaviorGraphBlendMultipleCondNode >( block ) )
	{
		nodeB->RemoveInput( indexToRemove );
	}
	else if ( CBehaviorGraphRandomSelectNode* node = Cast< CBehaviorGraphRandomSelectNode >( block ) )
	{
		node->RemoveInput( indexToRemove );
	}
}

void CEdBehaviorGraphEditor::OnRemoveContainerInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphSocket *socket = static_cast< CGraphSocket* >( m_activeItem.Get() );
	CGraphBlock *block = socket->GetBlock();

	ASSERT( block->IsA< CBehaviorGraphContainerNode >() );

	ASSERT( socket->IsA< CBehaviorGraphAnimationInputSocket >() || 
		    socket->IsA< CBehaviorGraphVariableInputSocket >() ||
			socket->IsA< CBehaviorGraphVectorVariableInputSocket >() );

	CBehaviorGraphContainerNode* node = SafeCast< CBehaviorGraphContainerNode >( block );

	if ( socket->IsA< CBehaviorGraphAnimationInputSocket >() )
	{
		CUndoBehaviorGraphContainerNodeInput::CreateRemovingStep( *m_undoManager, this, node, CUndoBehaviorGraphContainerNodeInput::ANIM_INPUT, socket->GetName() );
		node->RemoveAnimationInput( socket->GetName() );
	}
	else if ( socket->IsA< CBehaviorGraphVariableInputSocket >() )
	{
		CUndoBehaviorGraphContainerNodeInput::CreateRemovingStep( *m_undoManager, this, node, CUndoBehaviorGraphContainerNodeInput::VALUE_INPUT, socket->GetName() );
		node->RemoveValueInput( socket->GetName() );
	}
	else if ( socket->IsA< CBehaviorGraphVectorVariableInputSocket >() )
	{
		CUndoBehaviorGraphContainerNodeInput::CreateRemovingStep( *m_undoManager, this, node, CUndoBehaviorGraphContainerNodeInput::VECTOR_VALUE_INPUT, socket->GetName() );
		node->RemoveVectorValueInput( socket->GetName() );
	}
}

void CEdBehaviorGraphEditor::OnRemoveSwitchInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock *block = static_cast< CGraphBlock* >( m_activeItem.Get() );

	ASSERT( block->IsA< CBehaviorGraphAnimationManualSwitchNode >() );

	CBehaviorGraphAnimationManualSwitchNode* node = SafeCast< CBehaviorGraphAnimationManualSwitchNode >( block );

	CUndoBehaviorGraphSwitchNodeInput::CreateRemovingStep( *m_undoManager, this, node );
	node->RemoveInput();
}

void CEdBehaviorGraphEditor::OnRefreshEnumInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock *block = static_cast< CGraphBlock* >( m_activeItem.Get() );

	ASSERT( block->IsA< CBehaviorGraphAnimationEnumSwitchNode >() );

	CBehaviorGraphAnimationEnumSwitchNode* node = SafeCast< CBehaviorGraphAnimationEnumSwitchNode >( block );

	node->OnInputListChange();
}

void CEdBehaviorGraphEditor::OnAddContainerAnimInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphContainerNode >() );

	CBehaviorGraphContainerNode* node = SafeCast< CBehaviorGraphContainerNode >( block );
	
	String inputName;
	if ( InputBox( this, TXT("New input"), TXT("Enter input name"), inputName ) )
	{
		CUndoBehaviorGraphContainerNodeInput::CreateAddingStep( *m_undoManager, this, node, CUndoBehaviorGraphContainerNodeInput::ANIM_INPUT, CName( inputName ) );
		node->CreateAnimationInput( CName( inputName ) );
		Repaint();
	}
}

void CEdBehaviorGraphEditor::OnAddContainerValueInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphContainerNode >() );

	CBehaviorGraphContainerNode* node = SafeCast< CBehaviorGraphContainerNode >( block );

	String inputName;
	if ( InputBox( this, TXT("New input"), TXT("Enter input name"), inputName ) )
	{
		CUndoBehaviorGraphContainerNodeInput::CreateAddingStep( *m_undoManager, this, node, CUndoBehaviorGraphContainerNodeInput::VALUE_INPUT, CName( inputName ) );
		node->CreateValueInput( CName( inputName ) );
		Repaint();
	}
}

void CEdBehaviorGraphEditor::OnAddContainerVectorValueInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphContainerNode >() );

	CBehaviorGraphContainerNode* node = SafeCast< CBehaviorGraphContainerNode >( block );

	String inputName;
	if ( InputBox( this, TXT("New input"), TXT("Enter input name"), inputName ) )
	{
		CUndoBehaviorGraphContainerNodeInput::CreateAddingStep( *m_undoManager, this, node, CUndoBehaviorGraphContainerNodeInput::VECTOR_VALUE_INPUT, CName( inputName ) );
		node->CreateVectorValueInput( CName( inputName ) );
		Repaint();
	}
}

void CEdBehaviorGraphEditor::OnAddContainerMimicInput( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
	ASSERT( block->IsA< CBehaviorGraphContainerNode >() );

	CBehaviorGraphContainerNode* node = SafeCast< CBehaviorGraphContainerNode >( block );

	String inputName;
	if ( InputBox( this, TXT("New input"), TXT("Enter input name"), inputName ) )
	{
		CUndoBehaviorGraphContainerNodeInput::CreateAddingStep( *m_undoManager, this, node, CUndoBehaviorGraphContainerNodeInput::MIMIC_INPUT, CName( inputName ) );
		node->CreateMimicInput( CName( inputName ) );
		Repaint();
	}
}

void CEdBehaviorGraphEditor::OnAddDebugVisualizer( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphNode* block = static_cast< CBehaviorGraphNode* >( m_activeItem.Get() );
	CEntity *entity = GetEntity();
	CAnimatedComponent *animatedComponent = GetAnimatedComponent();	

	CBehaviorDebugVisualizer *debugVisualizer = Cast< CBehaviorDebugVisualizer >( entity->CreateComponent( CBehaviorDebugVisualizer::GetStaticClass(), SComponentSpawnInfo() ) );
	debugVisualizer->SetAnimatedComponent( animatedComponent );
	debugVisualizer->SetNodeOfInterest( block );
	debugVisualizer->SetEditor( GetEditor() );

	if (event.GetId() == ID_ADD_VISUALIZER_SB)
	{
		debugVisualizer->ChooseSelectBones();
	}

	static Uint32 colorMask = 1;
	colorMask = (colorMask + 1) % 7;	
	if ( !colorMask ) colorMask = 1;

	Color color = Color( colorMask & 1 ? 255 : 0, 
		colorMask & 2 ? 255 : 0,
		colorMask & 4 ? 255 : 0 );

	if ( color == Color( 0, 0, 255 ) )
	{
		color = Color( 128, 128, 255 );
	}

	debugVisualizer->SetColor( color );

	m_debugVisualizers.Insert( block, debugVisualizer );
}

void CEdBehaviorGraphEditor::OnChangeDebugVisualizer( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphNode* block = static_cast< CBehaviorGraphNode* >( m_activeItem.Get() );
	CBehaviorDebugVisualizer **debugVisualizerPtr = m_debugVisualizers.FindPtr( block );
	if ( debugVisualizerPtr )
	{
		CBehaviorDebugVisualizer *debugVisualizer = *debugVisualizerPtr;
		debugVisualizer->ChooseSelectBones();
	}
}

void CEdBehaviorGraphEditor::OnDebugVisualizerToggleBonesAxis( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphNode* block = static_cast< CBehaviorGraphNode* >( m_activeItem.Get() );
	CBehaviorDebugVisualizer **debugVisualizerPtr = m_debugVisualizers.FindPtr( block );
	if ( debugVisualizerPtr )
	{
		CBehaviorDebugVisualizer *debugVisualizer = *debugVisualizerPtr;
		debugVisualizer->ToggleBonesAxis();
	}
}

void CEdBehaviorGraphEditor::OnDebugVisualizerToggleBonesName( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphNode* block = static_cast< CBehaviorGraphNode* >( m_activeItem.Get() );
	CBehaviorDebugVisualizer **debugVisualizerPtr = m_debugVisualizers.FindPtr( block );
	if ( debugVisualizerPtr )
	{
		CBehaviorDebugVisualizer *debugVisualizer = *debugVisualizerPtr;
		debugVisualizer->ToggleBonesName();
	}
}

void CEdBehaviorGraphEditor::OnDebugVisualizerToggleBox( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphNode* block = static_cast< CBehaviorGraphNode* >( m_activeItem.Get() );
	CBehaviorDebugVisualizer **debugVisualizerPtr = m_debugVisualizers.FindPtr( block );
	if ( debugVisualizerPtr )
	{
		CBehaviorDebugVisualizer *debugVisualizer = *debugVisualizerPtr;
		debugVisualizer->ToggleBox();
	}
}

void CEdBehaviorGraphEditor::OnDebugVisualizerChooseHelpers(wxCommandEvent& event)
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphNode* block = static_cast< CBehaviorGraphNode* >( m_activeItem.Get() );
	CBehaviorDebugVisualizer **debugVisualizerPtr = m_debugVisualizers.FindPtr( block );
	if ( debugVisualizerPtr )
	{
		CBehaviorDebugVisualizer *debugVisualizer = *debugVisualizerPtr;
		debugVisualizer->ChooseBonesHelpers();
	}
}

void CEdBehaviorGraphEditor::OnDebugVisualizerBoneStyle(wxCommandEvent& event)
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphNode* block = SafeCast< CBehaviorGraphNode >( m_activeItem.Get() );
	CBehaviorDebugVisualizer **debugVisualizerPtr = m_debugVisualizers.FindPtr( block );
	if ( debugVisualizerPtr )
	{
		CBehaviorDebugVisualizer *debugVisualizer = *debugVisualizerPtr;

		if (event.GetId() == ID_VISUALIZER_BONE_STYLE_LINE)
		{
			debugVisualizer->SetBoneStyle(CBehaviorDebugVisualizer::BVBS_Line);
		}
		else if (event.GetId() == ID_VISUALIZER_BONE_STYLE_3D)
		{
			debugVisualizer->SetBoneStyle(CBehaviorDebugVisualizer::BVBS_3D);
		}
	}
}

void CEdBehaviorGraphEditor::OnRemoveDebugVisualizer( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphNode* block = SafeCast< CBehaviorGraphNode >( m_activeItem.Get() );	
	CEntity *entity = GetEntity();

	CBehaviorDebugVisualizer **debugVisualizerPtr = m_debugVisualizers.FindPtr( block );
	if ( debugVisualizerPtr )
	{
		CBehaviorDebugVisualizer *debugVisualizer = *debugVisualizerPtr;

		entity->DestroyComponent( debugVisualizer );
	}

	m_debugVisualizers.Erase( block );
}

void CEdBehaviorGraphEditor::RemoveAllVisualizers()
{
	CEntity *entity = GetEntity();

	for( THashMap< CBehaviorGraphNode*, CBehaviorDebugVisualizer* >::iterator it = m_debugVisualizers.Begin();
		 it != m_debugVisualizers.End();
		 ++it )
	{
		entity->DestroyComponent( it->m_second );
	}

	m_debugVisualizers.Clear();
}

void CEdBehaviorGraphEditor::OnCollapseToStage( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > selection;
	GetSelectedBlocks( selection );

	// delete output node from list, if it in the selection
	TDynArray< CGraphBlock* > toRemove;
	for( Uint32 i=0; i<selection.Size(); ++i )
	{
		if ( selection[i]->IsA< CBehaviorGraphOutputNode >() )
		{
			SelectBlock( selection[i], false );			
		}

		if ( selection[i]->IsA< CBehaviorGraphStateNode >() )
		{
			SelectBlock( selection[i], false );			
		}

		if ( selection[i]->IsA< CBehaviorGraphStateTransitionNode >() )
		{
			SelectBlock( selection[i], false );			
		}
	}

	selection.Clear();
	GetSelectedBlocks( selection );

	if ( selection.Empty() )
	{
		wxMessageBox( TXT("No nodes to create stage from!"), TXT("Error") );
		return;
	}

	// find links going into and out of selection
	wxRect selectionRect( 0, 0, 0, 0 );
	TDynArray< TPair< CGraphSocket*, CGraphSocket* > >	inputs;
	TDynArray< TPair< CGraphSocket*, CGraphSocket* > > outputs;
	for( Uint32 i=0; i<selection.Size(); ++i )
	{
		CGraphBlock *currBlock = selection[i];

		BlockLayoutInfo* layout = m_layout.FindPtr( currBlock );

		wxRect currBlockRect( currBlock->GetPosition().X, 
							  currBlock->GetPosition().Y, 
							  layout->m_windowSize.x, 
							  layout->m_windowSize.y );

		if ( i == 0 )		
		{
			selectionRect = currBlockRect;
		}
		else
		{			
			selectionRect.Union( currBlockRect );
		}

		const TDynArray< CGraphSocket* >& blockSockets = currBlock->GetSockets();
		
		for( Uint32 j=0; j<blockSockets.Size(); ++j )
		{
			CGraphSocket *currSocket = blockSockets[j];
			const TDynArray< CGraphConnection* >& socketConnections = currSocket->GetConnections();

			for( Uint32 k=0; k<socketConnections.Size(); ++k )
			{
				CGraphConnection *currConnection = socketConnections[k];

				if (currConnection->GetDestination() && currConnection->GetSource())
				{
					CGraphBlock *destBlock = currConnection->GetDestination()->GetBlock();

					if ( !selection.Exist( destBlock ) )
					{
						if ( currSocket->GetDirection() == LSD_Output )
						{
							outputs.PushBack( MakePair( currConnection->GetSource(), currConnection->GetDestination() ) );
						}
						else if ( currSocket->GetDirection() == LSD_Input )
						{
							inputs.PushBack( MakePair( currConnection->GetSource(), currConnection->GetDestination() ) );
						}
						else
						{
							wxMessageBox( TXT("Trying to create stage from nodes that do not support that operation!"), TXT("Error!") );
							return;
						}
					}
				}
			}
		}
	}

	// no more than one animation input and one animation output allowed
	Uint32 numAnimInputs = 0;
	Uint32 numAnimOutputs = 0;
	for( Uint32 i=0; i<inputs.Size(); ++i )
	{
		if ( inputs[i].m_first->IsA< CBehaviorGraphAnimationInputSocket >() )
		{
			++numAnimInputs;
		}

		if ( inputs[i].m_first->IsA< CBehaviorGraphAnimationOutputSocket >() )
		{
			++numAnimOutputs;
		}
	}

	if ( numAnimOutputs > 1 )
	{
		wxMessageBox( TXT("Create stage cannot have more then one animation input !"), TXT("Error!") );
		return;
	}

	// copy nodes	
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	CDependencySaver saver( writer, NULL );

	// Save
	DependencySavingContext context( (const DependencyMappingContext::TObjectsToMap&) selection );
	if ( !saver.SaveObjects( context ) )
	{		
		WARN_EDITOR( TXT("Unable to copy selected objects") );
		return;
	}

	if ( !PrepareForGraphStructureModified() )
	{
		return;
	}
	
	// create stage node
	GraphBlockSpawnInfo spawnInfo( CBehaviorGraphStageNode::GetStaticClass() );
	spawnInfo.m_position.Set3( selectionRect.x + selectionRect.width / 2,
							   selectionRect.y + selectionRect.height / 2, 
							   0.0f );
	CBehaviorGraphStageNode *stageNode = SafeCast< CBehaviorGraphStageNode >( GraphCreateBlock( spawnInfo ) );

	// store creation step in the undo history
	CUndoGraphBlockExistance::PrepareCreationStep( *m_undoManager, this, stageNode );
	
	// Paste blocks into newly created node, just next to output node of stage
	Vector stageBlocksShift( 200.0f, 0.0f, 0.0f );
	Vector spawnPosition = stageNode->GetRootNode()->GetPosition() + 
						   Vector( selectionRect.width / 2, 0.0f, 0.0f ) + 
						   stageBlocksShift;

	// Deserialize
	CMemoryFileReader reader( buffer, 0 );
	CDependencyLoader loader( reader, NULL );
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = stageNode;
	if ( !loader.LoadObjects( loadingContext ) )
	{
		WARN_EDITOR( TXT("Unable to paste selected blocks!") );
		return;			
	}

	// Get spawned entities
	TDynArray< CGraphBlock* > pastedBlocks;
	for ( Uint32 i=0; i<loadingContext.m_loadedRootObjects.Size(); i++ )
	{
		CGraphBlock* block = Cast< CGraphBlock>( loadingContext.m_loadedRootObjects[i] );
		if ( block )
		{
			if ( stageNode->ChildNodeClassSupported( block->GetClass() ) )
			{
				pastedBlocks.PushBack( block );
			}
		}
	}

	// Call post load of spawned objects
	loader.PostLoad();

	// Offset root components by specified spawn point
	if ( !pastedBlocks.Empty() )
	{
		Vector rootPosition( selectionRect.x + selectionRect.width / 2,
							 selectionRect.y + selectionRect.height / 2, 
							 0.0f );

		for ( Uint32 i=0; i<pastedBlocks.Size(); i++ )
		{
			CGraphBlock* pastedBlock = pastedBlocks[i];
			Vector delta = pastedBlock->GetPosition() - rootPosition;
			pastedBlock->SetPosition( spawnPosition + delta );
		}
	}

	// Add to list of blocks
	for ( Uint32 i=0; i<pastedBlocks.Size(); i++ )
	{
		CGraphBlock* block = pastedBlocks[i];
		block->InvalidateLayout();	

		stageNode->OnChildNodeAdded( block );
	}


	// Remove source blocks from graph
	for ( Uint32 i=0; i<selection.Size(); i++ )
	{
		CUndoGraphBlockExistance::PrepareDeletionStep( *m_undoManager, this, selection[i] );
		m_graph->GraphRemoveBlock( selection[i] );
	}	
	
	// connect output of the stage node to selection output
	// by Dex: WTF with the MapObject crap ?
	if ( outputs.Empty() == false )
	{
		TPair< CGraphSocket*, CGraphSocket* > selectionOutput = outputs[ 0 ];
		CGraphSocket* selectionOutputSourceSocket = selectionOutput.m_first;
		Int32 selectionOutputSourceSocketMapping = saver.MapObject( selectionOutputSourceSocket );
		CGraphSocket* reloadedSelectionOutputSocket 
#if 0 // CGraphSocket and CObject are unrelated types !!! Below Cast would always return nullptr (if it compiled)
			= Cast< CGraphSocket >( loader.MapObject( selectionOutputSourceSocketMapping ).GetObjectPtr() );
#else
			= nullptr;
#endif
		
		CGraphSocket* stageSocket = stageNode->GetRootNode()->GetSockets()[ 0 ];
		stageSocket->ConnectTo( reloadedSelectionOutputSocket );

		//stageNode->GetRootNode()->GetSockets()[0]->ConnectTo( Cast< CGraphSocket >( loader.MapObject( saver.MapObject( outputs[0].m_first ) ) ) );
		stageNode->CGraphBlock::FindSocket( TXT("Output") )->ConnectTo( outputs[0].m_second );
	}

	// create input nodes for all input links, connect to proper sockets
	// connect links going into selection to sockets in stage node
	for( Uint32 i=0; i<inputs.Size(); ++i )
	{
		CName parentInputSocket( String::Printf( TXT("Input%d"), i ) );

		Vector shift = stageNode->GetRootNode()->GetPosition() + 
					   stageBlocksShift * 2.0f + 
					   Vector( 0.0f, 40.0f, 0.0f ) * i;

		if ( inputs[i].m_second->IsA< CBehaviorGraphAnimationOutputSocket >() )
		{
			stageNode->CreateAnimationInput( parentInputSocket );
			CGraphSocket *stageSocket = stageNode->CGraphBlock::FindSocket( parentInputSocket );
			stageSocket->ConnectTo( inputs[i].m_second );

			GraphBlockSpawnInfo spawnInfo( CBehaviorGraphParentInputNode::GetStaticClass() );
			spawnInfo.m_position = shift + Vector( selectionRect.width, 0.0f, 0.0f );
			
			CBehaviorGraphParentInputNode *block = Cast< CBehaviorGraphParentInputNode >( stageNode->CreateChildNode( spawnInfo ) );
#if 0 // CGraphSocket and CObject are unrelated types !!! Below Cast would always return nullptr (if it compiled)
			block->GetSockets()[0]->ConnectTo( Cast< CGraphSocket >( loader.MapObject( saver.MapObject( inputs[i].m_first ) ).GetObjectPtr() ) );
#else
			block->GetSockets()[0]->ConnectTo( nullptr );
#endif
			block->SetParentInputSocket( parentInputSocket );
		}
		else if ( inputs[i].m_second->IsA< CBehaviorGraphVariableOutputSocket >() )
		{
			stageNode->CreateValueInput( parentInputSocket );
			CGraphSocket *stageSocket = stageNode->CGraphBlock::FindSocket( parentInputSocket );
			stageSocket->ConnectTo( inputs[i].m_second );

			GraphBlockSpawnInfo spawnInfo( CBehaviorGraphParentValueInputNode::GetStaticClass() );
			spawnInfo.m_position = shift + Vector( selectionRect.width / 2, 0.0f, 0.0f );

			CBehaviorGraphParentValueInputNode *block = Cast< CBehaviorGraphParentValueInputNode >( stageNode->CreateChildNode( spawnInfo ) );
#if 0 // CGraphSocket and CObject are unrelated types !!! Below Cast would always return nullptr (if it compiled)
			block->GetSockets()[0]->ConnectTo( Cast< CGraphSocket >( loader.MapObject( saver.MapObject( inputs[i].m_first ) ).GetObjectPtr() ) );
#else
			block->GetSockets()[0]->ConnectTo( nullptr );
#endif
			block->SetParentInputSocket( parentInputSocket );
		}
		else if ( inputs[i].m_second->IsA< CBehaviorGraphVectorVariableOutputSocket >() )
		{
			stageNode->CreateVectorValueInput( parentInputSocket );
			CGraphSocket *stageSocket = stageNode->CGraphBlock::FindSocket( parentInputSocket );
			stageSocket->ConnectTo( inputs[i].m_second );

			GraphBlockSpawnInfo spawnInfo( CBehaviorGraphParentVectorValueInputNode::GetStaticClass() );
			spawnInfo.m_position = shift + Vector( selectionRect.width / 2, 0.0f, 0.0f );

			CBehaviorGraphParentVectorValueInputNode *block = Cast< CBehaviorGraphParentVectorValueInputNode >( stageNode->CreateChildNode( spawnInfo ) );
#if 0 // CGraphSocket and CObject are unrelated types !!! Below Cast would always return nullptr (if it compiled)
			block->GetSockets()[0]->ConnectTo( Cast< CGraphSocket >( loader.MapObject( saver.MapObject( inputs[i].m_first ) ).GetObjectPtr() ) );
#else
			block->GetSockets()[0]->ConnectTo( nullptr );
#endif
			block->SetParentInputSocket( parentInputSocket );
		}
		else if ( inputs[i].m_second->IsA< CBehaviorGraphMimicAnimationOutputSocket >() )
		{
			stageNode->CreateMimicInput( parentInputSocket );
			CGraphSocket *stageSocket = stageNode->CGraphBlock::FindSocket( parentInputSocket );
			stageSocket->ConnectTo( inputs[i].m_second );

			GraphBlockSpawnInfo spawnInfo( CBehaviorGraphMimicParentInputNode::GetStaticClass() );
			spawnInfo.m_position = shift + Vector( selectionRect.width / 2, 0.0f, 0.0f );

			CBehaviorGraphMimicParentInputNode *block = Cast< CBehaviorGraphMimicParentInputNode >( stageNode->CreateChildNode( spawnInfo ) );
#if 0 // CGraphSocket and CObject are unrelated types !!! Below Cast would always return nullptr (if it compiled)
			block->GetSockets()[0]->ConnectTo( Cast< CGraphSocket >( loader.MapObject( saver.MapObject( inputs[i].m_first ) ).GetObjectPtr() ) );
#else
			block->GetSockets()[0]->ConnectTo( nullptr );
#endif
			block->SetParentInputSocket( parentInputSocket );
		}
	}

	// Finalize step that contains both creation and deletion entries
	CUndoGraphBlockExistance::FinalizeStep( *m_undoManager );

	// Modified
	CEdGraphEditor::GraphStructureModified();	
}

void CEdBehaviorGraphEditor::OnCopyAllTransitions( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	CBehaviorGraphNode* block = SafeCast< CBehaviorGraphNode >( m_activeItem.Get() );	
	ASSERT( block->IsA< CBehaviorGraphStateNode >() );
	CBehaviorGraphStateNode *const graphState = static_cast<CBehaviorGraphStateNode*>(block);

	m_handleToCopyAllTransitionsGraphState = THandle< CBehaviorGraphStateNode >	( graphState );
}

void CEdBehaviorGraphEditor::OnPasteAllTransitions( wxCommandEvent& event )
{
	CBehaviorGraphStateNode *const srcGraphState = m_handleToCopyAllTransitionsGraphState.Get();
	if ( srcGraphState )
	{
		if ( !PrepareForGraphStructureModified() )
		{
			return;
		}
		ASSERT( m_activeItem.Get() );
		CGraphBlock* block = SafeCast< CGraphBlock >( m_activeItem.Get() );
		ASSERT( block->IsA< CBehaviorGraphStateNode >() );
		CBehaviorGraphStateNode *const destGraphState	= static_cast<CBehaviorGraphStateNode*>(block);
		destGraphState->OnCopyAllTransitionsFrom( srcGraphState );
		CEdGraphEditor::GraphStructureModified();
	}
	m_handleToCopyAllTransitionsGraphState = THandle< CBehaviorGraphStateNode >(NULL);
}

class CreateTransitionPopupWrapper : public wxObject
{
public:
	CClass*								m_objectClass;
	CGraphSocket*						m_srcSocket;
	CGraphSocket*						m_destSocket;
	CBehaviorGraphStateTransitionNode*	m_pasteObject;

public:
	CreateTransitionPopupWrapper( CClass* objectClass, CGraphSocket* srcSocket, CGraphSocket* destSocket )
		: m_objectClass( objectClass )
		, m_srcSocket( srcSocket )
		, m_destSocket( destSocket )
		, m_pasteObject( NULL )
	{}

	CreateTransitionPopupWrapper( CBehaviorGraphStateTransitionNode* pasteObject, CGraphSocket* srcSocket, CGraphSocket* destSocket )
		: m_objectClass( NULL )
		, m_srcSocket( srcSocket )
		, m_destSocket( destSocket )
		, m_pasteObject( pasteObject )
	{}
};

namespace EdBehaviorGraphEditor
{
	Bool ManuallyCreatableObjectFilter( CClass *classField )
	{
		CObject *defaultObject = classField->GetDefaultObject< CObject >();
		return defaultObject && defaultObject->IsManualCreationAllowed();
	}
}

void CEdBehaviorGraphEditor::ConnectSockets( CGraphSocket* srcSocket, CGraphSocket* destSocket )
{
	if ( !CanBeModify() || !srcSocket->GetBlock() || !srcSocket->GetBlock() )
	{
		return;
	}

	if ( ( destSocket->GetBlock()->IsA< CBehaviorGraphStateTransitionGlobalBlendNode >() ||
		 srcSocket->GetBlock()->IsA< CBehaviorGraphStateTransitionGlobalBlendNode >() ) )
	{
		if ( destSocket->IsA< CBehaviorGraphTransitionSocket >() || srcSocket->IsA< CBehaviorGraphTransitionSocket >() )
		{
			return;
		}
		else
		{
			CEdGraphEditor::ConnectSockets( srcSocket, destSocket );
		}
	}
	else if ( srcSocket->IsA< CBehaviorGraphStateOutSocket >() && destSocket->IsA< CBehaviorGraphStateInSocket >() &&
		destSocket->GetBlock()->IsA< CBehaviorGraphStateNode >() )
	{
		wxMenu menu;

		if ( destSocket->GetBlock()->IsA< CBehaviorGraphComboStateNode >() || srcSocket->GetBlock()->IsA< CBehaviorGraphComboStateNode >() )
		{
			CClass* currClass = CBehaviorGraphComboTransitionNode::GetStaticClass();
			CGraphBlock* defaultObject = currClass->GetDefaultObject< CGraphBlock >();

			menu.Append( ID_CREATE_TRANSITION, defaultObject->GetBlockName().AsChar() );
			menu.Connect( ID_CREATE_TRANSITION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
		}
		else if ( destSocket->GetBlock()->IsA< CBehaviorGraphFlowConnectionNode >() || srcSocket->GetBlock()->IsA< CBehaviorGraphFlowConnectionNode >() )
		{
			CClass* currClass = CBehaviorGraphFlowTransitionNode::GetStaticClass();
			CGraphBlock* defaultObject = currClass->GetDefaultObject< CGraphBlock >();

			menu.Append( ID_CREATE_TRANSITION, defaultObject->GetBlockName().AsChar() );
			menu.Connect( ID_CREATE_TRANSITION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );

			if ( destSocket->GetBlock()->IsA< CBehaviorGraphStateNode >() || srcSocket->GetBlock()->IsA< CBehaviorGraphStateNode >() )
			{
				wxMenu* subMenu = new wxMenu();
				subMenu->Append( ID_CREATE_DEFAULT_TRANS_FL, wxT("Foot left") );
				menu.Connect( ID_CREATE_DEFAULT_TRANS_FL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateDefaultTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
				subMenu->Append( ID_CREATE_DEFAULT_TRANS_FR, wxT("Foot right") );
				menu.Connect( ID_CREATE_DEFAULT_TRANS_FR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateDefaultTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
				subMenu->Append( ID_CREATE_DEFAULT_TRANS_END_AUX, wxT("Anim End AUX") );
				menu.Connect( ID_CREATE_DEFAULT_TRANS_END_AUX, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateDefaultTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
				menu.AppendSubMenu( subMenu, wxT("Defaults") );
			}
		}
		else if ( destSocket->GetBlock()->IsA< CBehaviorGraphPointerStateNode >() || srcSocket->GetBlock()->IsA< CBehaviorGraphPointerStateNode >() )
		{
			CClass* currClass = CBehaviorGraphPointerTransitionNode::GetStaticClass();
			CGraphBlock* defaultObject = currClass->GetDefaultObject< CGraphBlock >();

			menu.Append( ID_CREATE_TRANSITION, defaultObject->GetBlockName().AsChar() );
			menu.Connect( ID_CREATE_TRANSITION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
		}
		else if ( destSocket->GetBlock()->IsA< CBehaviorGraphDefaultSelfActAdditiveStateNode >() || srcSocket->GetBlock()->IsA< CBehaviorGraphDefaultSelfActAdditiveStateNode >() )
		{
			CClass* currClass = CBehaviorGraphStateAdditiveTransitionNode::GetStaticClass();
			CGraphBlock* defaultObject = currClass->GetDefaultObject< CGraphBlock >();

			menu.Append( ID_CREATE_TRANSITION, defaultObject->GetBlockName().AsChar() );
			menu.Connect( ID_CREATE_TRANSITION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
		}
		else
		{
			TDynArray< CClass* > transitionClasses;
			SRTTI::GetInstance().EnumClasses( CBehaviorGraphStateTransitionNode::GetStaticClass(), transitionClasses, EdBehaviorGraphEditor::ManuallyCreatableObjectFilter );

			for( Uint32 i=0; i<transitionClasses.Size(); ++i )
			{
				CClass* currClass = transitionClasses[ i ];
				CGraphBlock* defaultObject = currClass->GetDefaultObject< CGraphBlock >();

				// Append menu with block name
				menu.Append( ID_CREATE_TRANSITION + i, defaultObject->GetBlockName().AsChar() );
				menu.Connect( ID_CREATE_TRANSITION  + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
			}

			// Is any transition block in clipboard?
			if ( wxTheClipboard->Open())
			{
				CClipboardData data( String(TXT("Blocks")) + ClipboardChannelName() );
				if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
				{
					// Extract data from the clipboard
					if ( wxTheClipboard->GetData( data ) )
					{
						const TDynArray< Uint8 >& objData = data.GetData();

						// Deserialize
						CMemoryFileReader reader( objData, 0 );
						CDependencyLoader loader( reader, NULL );
						DependencyLoadingContext loadingContext;
						loadingContext.m_parent = Cast< CBehaviorGraphContainerNode >( GraphGetOwner() );
						if ( loader.LoadObjects( loadingContext ) && loadingContext.m_loadedRootObjects.Size() == 1 )
						{
							ASSERT( loadingContext.m_loadedRootObjects[0]->IsA< CGraphBlock >() );

							// Call post load of spawned objects
							loader.PostLoad();

							if (loadingContext.m_loadedRootObjects[0]->IsA<CBehaviorGraphStateTransitionNode>())
							{
								menu.AppendSeparator();

								CBehaviorGraphStateTransitionNode* block = Cast< CBehaviorGraphStateTransitionNode >( loadingContext.m_loadedRootObjects[0] );
								wxString blockName = block->GetBlockName().AsChar();
								menu.Append( ID_PASTE_TRANSITION_NODE, TXT("Paste - ") + blockName );
								menu.Connect( ID_PASTE_TRANSITION_NODE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnPasteTransition ), new CreateTransitionPopupWrapper( block, srcSocket, destSocket ), this );
							}
						}
					}
				} 
				wxTheClipboard->Close();
			}

			if ( destSocket->GetBlock()->IsA< CBehaviorGraphStateNode >() || srcSocket->GetBlock()->IsA< CBehaviorGraphStateNode >() )
			{
				CClass* currClass = CBehaviorGraphStateTransitionBlendNode::GetStaticClass();
				CGraphBlock* defaultObject = currClass->GetDefaultObject< CGraphBlock >();

				wxMenu* subMenu = new wxMenu();
				subMenu->Append( ID_CREATE_DEFAULT_TRANS_FL, wxT("Foot left") );
				menu.Connect( ID_CREATE_DEFAULT_TRANS_FL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateDefaultTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
				subMenu->Append( ID_CREATE_DEFAULT_TRANS_FR, wxT("Foot right") );
				menu.Connect( ID_CREATE_DEFAULT_TRANS_FR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateDefaultTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
				subMenu->Append( ID_CREATE_DEFAULT_TRANS_END_AUX, wxT("Anim End AUX") );
				menu.Connect( ID_CREATE_DEFAULT_TRANS_END_AUX, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBehaviorGraphEditor::OnCreateDefaultTransition ), new CreateTransitionPopupWrapper( currClass, srcSocket, destSocket ), this );
				menu.AppendSubMenu( subMenu, wxT("Defaults") );
			}
		}

		menu.AppendSeparator();
		menu.Append( wxID_ANY, TXT("Cancel") );

		PopupMenu( &menu );
	}
	else
	{
		CEdGraphEditor::ConnectSockets( srcSocket, destSocket );
	}
}

void CEdBehaviorGraphEditor::OnCreateTransition( wxCommandEvent& event )
{
	if ( !PrepareForGraphStructureModified() )
	{
		return;
	}

	CreateTransitionPopupWrapper* wrapper = ( CreateTransitionPopupWrapper* ) event.m_callbackUserData;
	CClass *transitionClass = wrapper->m_objectClass;

	wxPoint srcSocketPos;
	wxPoint destSocketPos;
	wxPoint tempPos;
	GetSocketLinkParams( wrapper->m_srcSocket, srcSocketPos, tempPos );
	GetSocketLinkParams( wrapper->m_destSocket, destSocketPos, tempPos );
	
	// create transition node
	wxPoint graphPoint = ClientToCanvas( m_lastClickPoint );
	GraphBlockSpawnInfo info( transitionClass );
	info.m_position = Vector( ( srcSocketPos.x + destSocketPos.x ) / 2, ( srcSocketPos.y + destSocketPos.y ) / 2, 0, 1 );
	CBehaviorGraphStateTransitionNode* transitionNode = Cast<CBehaviorGraphStateTransitionNode>( m_currentRoot->CreateChildNode( info ) );

	const TDynArray< CGraphSocket* > &sockets = transitionNode->GetSockets();
	ASSERT( sockets.Size() >= 2 );

	CBehaviorGraphTransitionSocket *transitionStartSocket = Cast< CBehaviorGraphTransitionSocket >( sockets[0] );
	CBehaviorGraphTransitionSocket *transitionEndSocket = Cast< CBehaviorGraphTransitionSocket >( sockets[1] );

	// connect src and dest sockets through transition node
	wrapper->m_srcSocket->ConnectTo( transitionStartSocket );

	transitionEndSocket->ConnectTo( wrapper->m_destSocket );	

	CEdGraphEditor::GraphStructureModified();
}

void CEdBehaviorGraphEditor::OnPasteTransition(wxCommandEvent& event)
{
	if ( !PrepareForGraphStructureModified() )
	{
		return;
	}
	
	CreateTransitionPopupWrapper* wrapper = ( CreateTransitionPopupWrapper* ) event.m_callbackUserData;

	wxPoint srcSocketPos;
	wxPoint destSocketPos;
	wxPoint tempPos;
	GetSocketLinkParams( wrapper->m_srcSocket, srcSocketPos, tempPos );
	GetSocketLinkParams( wrapper->m_destSocket, destSocketPos, tempPos );

	wxPoint graphPoint = ClientToCanvas( m_lastClickPoint );
	Vector position = Vector((srcSocketPos.x + destSocketPos.x ) / 2, ( srcSocketPos.y + destSocketPos.y ) / 2, 0, 1 );

	CBehaviorGraphStateTransitionNode* transitionNode = wrapper->m_pasteObject;
	transitionNode->SetPosition(position);

	TDynArray< CGraphSocket* > sockets = transitionNode->GetSockets();

	ASSERT( sockets.Size() >= 2 );
	CBehaviorGraphTransitionSocket *transitionStartSocket = NULL;
	CBehaviorGraphTransitionSocket *transitionEndSocket = NULL;

	for (Uint32 i=0; i<sockets.Size(); i++)
	{
		if (sockets[i]->IsA<CBehaviorGraphTransitionSocket>())
		{
			if (!transitionStartSocket) 
			{
				transitionStartSocket = Cast<CBehaviorGraphTransitionSocket>(sockets[i]);
			}
			else
			{
				ASSERT(!transitionEndSocket);
				transitionEndSocket = Cast<CBehaviorGraphTransitionSocket>(sockets[i]);
			}
		}
	}

	ASSERT(transitionStartSocket && transitionEndSocket);

	// connect src and dest sockets through transition node
	wrapper->m_srcSocket->ConnectTo( transitionStartSocket );

	transitionEndSocket->ConnectTo( wrapper->m_destSocket );

	transitionNode->InvalidateLayout();
	Cast< CBehaviorGraphContainerNode >( GraphGetOwner() )->OnChildNodeAdded( transitionNode );

	CEdGraphEditor::GraphStructureModified();
}

void CEdBehaviorGraphEditor::OnCopyTransitionCondition( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > selection;
	GetSelectedBlocks( selection );

	if(selection.Size()<1) return;

	// Only first block
	CBehaviorGraphStateTransitionNode* transition = SafeCast<CBehaviorGraphStateTransitionNode>(selection[0]);

	// Serialize to memory package
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	CDependencySaver saver( writer, NULL );

	while (selection.Size() != 1)
	{
		selection.PopBack();
	}

	// Save object
	DependencySavingContext context( (const DependencyMappingContext::TObjectsToMap&) selection );
	if ( !saver.SaveObjects( context ) )
	{
		WARN_EDITOR( TXT("Unable to copy selected objects to clipboard") );
		return;
	}

	// Open clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new CClipboardData( TXT("TransitionCondition"), buffer ) );
		wxTheClipboard->Close();
	}
}

void CEdBehaviorGraphEditor::OnPasteTransitionCondition( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > selection;
	GetSelectedBlocks( selection );

	if (selection.Size() < 1) return;

	if ( !PrepareForGraphStructureModified() )
	{
		return;
	}

	CBehaviorGraphStateTransitionNode* patternNode = NULL;

	if ( wxTheClipboard->Open())
	{
		CClipboardData data( TXT("TransitionCondition") );
		if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
		{
			// Extract data from the clipboard
			if ( wxTheClipboard->GetData( data ) )
			{
				const TDynArray< Uint8 >& objData = data.GetData();

				// Deserialize
				CMemoryFileReader reader( objData, 0 );
				CDependencyLoader loader( reader, NULL );
				DependencyLoadingContext loadingContext;
				loadingContext.m_parent = Cast< CBehaviorGraphContainerNode >( GraphGetOwner() );
				if ( loader.LoadObjects( loadingContext ) && loadingContext.m_loadedRootObjects.Size() == 1 )
				{
					ASSERT( loadingContext.m_loadedRootObjects[0]->IsA< CGraphBlock >() );

					// Call post load of spawned objects
					loader.PostLoad();

					if (loadingContext.m_loadedRootObjects[0]->IsA<CBehaviorGraphStateTransitionNode>())
					{
						patternNode = SafeCast<CBehaviorGraphStateTransitionNode>(loadingContext.m_loadedRootObjects[0]);
					}
				}
			}
		}

		wxTheClipboard->Close();
	}

	if (!patternNode)
	{
		return;
	}

	for (Uint32 i=0; i<selection.Size(); i++)
	{
		if (selection[i]->IsA<CBehaviorGraphStateTransitionNode>())
		{
			CBehaviorGraphStateTransitionNode* transition = SafeCast<CBehaviorGraphStateTransitionNode>(selection[i]);
			
			transition->ConvertTransitionConditonTo( patternNode );
		}
	}

	GetEditor()->GetStaticPropertyBrowser()->RefreshValues();
	CEdGraphEditor::GraphStructureModified();
}

void CEdBehaviorGraphEditor::OnConvertTransitionCondition( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > selection;
	GetSelectedBlocks( selection );

	if (selection.Size() < 1) return;

	Uint32 convertNum = event.GetId() - ID_CONVERT_TRANSITION;

	for (Uint32 i=0; i<selection.Size(); i++)
	{
		CBehaviorGraphStateTransitionNode* node = Cast<CBehaviorGraphStateTransitionNode>(selection[i]);
		if (node)
		{
			// Get possible conversion
			TDynArray<IBehaviorStateTransitionCondition*> conversions;
			node->GetTransitionCondition()->EnumConversions(conversions);

			if (convertNum < conversions.Size())
			{
				IBehaviorStateTransitionCondition* selectConversion = conversions[convertNum];
				node->ConvertTransitionConditonTo(selectConversion);
			}
		}
	}
}

void CEdBehaviorGraphEditor::OnConvertGlobalWithStreamingTransitionCondition( wxCommandEvent& event )
{
	if ( !PrepareForGraphStructureModified() )
	{
		return;
	}

	TDynArray< CGraphBlock* > selection;
	GetSelectedBlocks( selection );

	if( selection.Size() < 1 ) 
	{
		return;
	}

	CBehaviorGraphStateTransitionGlobalBlendNode* prevTrans = SafeCast<CBehaviorGraphStateTransitionGlobalBlendNode>( selection[0] );
	if ( !prevTrans )
	{
		return;
	}

	// Get state
	CBehaviorGraphStateNode* state = prevTrans->GetDestState();
	if( !state ) 
	{
		return;
	}

	// Get state's input socket
	CBehaviorGraphStateInSocket* stateSocket = state->CGraphBlock::FindSocket< CBehaviorGraphStateInSocket >( CNAME( In ) );
	ASSERT( stateSocket );

	// Create force transition node
	GraphBlockSpawnInfo info( ClassID< CBehaviorGraphStateTransitionGlobalBlendStreamingNode >() );
	info.m_position = prevTrans->GetPosition();

	// Create new node
	CBehaviorGraphStateTransitionGlobalBlendStreamingNode* transition = Cast<CBehaviorGraphStateTransitionGlobalBlendStreamingNode>( m_currentRoot->CreateChildNode( info ) );
	ASSERT( transition );

	// Transition socket
	CBehaviorGraphTransitionSocket* transitionSocket = transition->CGraphBlock::FindSocket< CBehaviorGraphTransitionSocket >( CNAME( End ) );

	// Connect
	transitionSocket->ConnectTo( stateSocket );	

	// Cache connections
	transition->CacheConnections();

	// Set graph modified
	CEdGraphEditor::GraphStructureModified();

	// Remove prev transition
	GraphRemoveBlock( prevTrans );
}

void CEdBehaviorGraphEditor::CalcSocketLayoutCenter( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout )
{
	// Place sockets
	for ( Uint32 i=0; i<sockets.Size(); i++ )
	{
		CGraphSocket *socket = sockets[i];

		if ( socket->GetBlock()->IsA< CBehaviorGraphStateNode >() && socket->GetName() == TXT("Out") )
		{
			// Create socket layout info
			SocketLayoutInfo socketLayout;

			// Calculate caption position
			socketLayout.m_captionPos.x = clientRect.x + clientRect.width / 2;
			socketLayout.m_captionPos.y = clientRect.y + clientRect.height / 2;

			// Calculate connector position
			socketLayout.m_socketRect.x = clientRect.x + clientRect.width / 2 - 10;
			socketLayout.m_socketRect.y = clientRect.y + clientRect.height / 2 - 10;
			socketLayout.m_socketRect.width = 20;
			socketLayout.m_socketRect.height = 20;

			// Link position
			socketLayout.m_linkDir = wxPoint( 0, 0 );
			socketLayout.m_linkPos.x = clientRect.x + clientRect.width / 2;
			socketLayout.m_linkPos.y = clientRect.y + clientRect.height / 2;

			// Remember layout info
			layout.m_sockets.Insert( socket, socketLayout );
		}
		else
		{
			// Create socket layout info
			SocketLayoutInfo socketLayout;

			// Calculate caption position
			socketLayout.m_captionPos.x = clientRect.x + clientRect.width / 2;
			socketLayout.m_captionPos.y = clientRect.y + clientRect.height / 2;

			// Calculate connector position
			socketLayout.m_socketRect.x = clientRect.x;
			socketLayout.m_socketRect.y = clientRect.y;
			socketLayout.m_socketRect.width = clientRect.width;
			socketLayout.m_socketRect.height = clientRect.height;

			// Link position
			socketLayout.m_linkDir = wxPoint( 0, 0 );
			socketLayout.m_linkPos.x = clientRect.x + clientRect.width / 2;
			socketLayout.m_linkPos.y = clientRect.y + clientRect.height / 2;

			// Remember layout info
			layout.m_sockets.Insert( socket, socketLayout );
		}
	}
}


void CEdBehaviorGraphEditor::DrawTransitionLink( CGraphBlock *startBlock, CGraphBlock *midBlock, CGraphBlock *endBlock, const wxColour &color, Float width )
{
	// Find layout info
	BlockLayoutInfo* startLayout = m_layout.FindPtr( startBlock );
	if ( !startLayout )
		return;

	BlockLayoutInfo* middleLayout = m_layout.FindPtr( midBlock );
	if ( !middleLayout )
		return;

	BlockLayoutInfo* endLayout = m_layout.FindPtr( endBlock );
	if ( !endLayout )
		return;

	if ( !startLayout->m_visible || !middleLayout->m_visible || !endLayout->m_visible || 
		( !startLayout->m_onScreen && !middleLayout->m_onScreen && !endLayout->m_onScreen ) )
	{
		return;
	}

	wxColour colour = color;

	if ( startLayout->m_freeze || middleLayout->m_freeze || endLayout->m_freeze )
	{

		colour = ConvertLinkColorToFreezeMode( colour );
	}

	Vector p1 = startBlock->GetPosition() + Vector( startLayout->m_windowSize.x * 0.5f, startLayout->m_windowSize.y * 0.5f, 0.0f );
	Vector p2 = midBlock->GetPosition() + Vector( middleLayout->m_windowSize.x * 0.5f, middleLayout->m_windowSize.y * 0.5f, 0.0f );
	Vector p3 = endBlock->GetPosition() + Vector( endLayout->m_windowSize.x * 0.5f, endLayout->m_windowSize.y * 0.5f, 0.0f );

	TDynArray< wxPoint > points;
	points.PushBack( wxPoint( (Int32)p1.X, (Int32)p1.Y ) );
	points.PushBack( wxPoint( (Int32)p2.X, (Int32)p2.Y ) );
	points.PushBack( wxPoint( (Int32)p3.X, (Int32)p3.Y ) );
	DrawCardinalCurve( points.TypedData(), points.Size(), colour, width );

	{
		Float t = 0.5f;
		Float t2 = t * t;
		Float t3 = t2 * t;
		Vector m0 = ( p2 - p1 ) * 0.5f; 
		Vector m1 = ( p3 - p1 ) * 0.5f;

		Vector arrowPoint = p1 * ( 2.0f * t3 - 3.0f * t2 + 1.0f ) +
							m0 * ( t3 - 2.0f * t2 + t ) +
							p2 * ( -2.0f * t3 + 3.0f * t2 ) +
							m1 * ( t3 - t2 );

		Vector arrowTangent = p1 * ( 6.0f * t2 - 6.0f * t ) +
							  m0 * ( 3.0f * t2 - 4.0f * t + 1.0f ) +
							  p2 * ( -6.0f * t2 + 6.0f * t ) +
							  m1 * ( 3.0f * t2 - 2.0f * t );

		arrowTangent.Normalize3();

		Vector arrowNormal( -arrowTangent.Y, arrowTangent.X, 0.0f );

		Vector arm1 = arrowPoint - arrowTangent * 10.0f + arrowNormal * 5.0f;
		Vector arm2 = arrowPoint - arrowTangent * 10.0f - arrowNormal * 5.0f;

		FillTriangle( (Int32)arrowPoint.X, (Int32)arrowPoint.Y, (Int32)arm1.X, (Int32)arm1.Y, (Int32)arm2.X, (Int32)arm2.Y, colour );
	}

	{
		Float t = 0.5f;
		Float t2 = t * t;
		Float t3 = t2 * t;
		Vector m0 = ( p3 - p1 ) * 0.5f; 
		Vector m1 = ( p3 - p2 ) * 0.5f;

		Vector arrowPoint = p2 * ( 2.0f * t3 - 3.0f * t2 + 1.0f ) +
							m0 * ( t3 - 2.0f * t2 + t ) +
							p3 * ( -2.0f * t3 + 3.0f * t2 ) +
							m1 * ( t3 - t2 );

		Vector arrowTangent = p2 * ( 6.0f * t2 - 6.0f * t ) +
							  m0 * ( 3.0f * t2 - 4.0f * t + 1.0f ) +
							  p3 * ( -6.0f * t2 + 6.0f * t ) +
							  m1 * ( 3.0f * t2 - 2.0f * t );

		arrowTangent.Normalize3();

		Vector arrowNormal( -arrowTangent.Y, arrowTangent.X, 0.0f );

		Vector arm1 = arrowPoint - arrowTangent * 10.0f + arrowNormal * 5.0f;
		Vector arm2 = arrowPoint - arrowTangent * 10.0f - arrowNormal * 5.0f;

		FillTriangle( (Int32)arrowPoint.X, (Int32)arrowPoint.Y, (Int32)arm1.X, (Int32)arm1.Y, (Int32)arm2.X, (Int32)arm2.Y, colour );
	}
}


void CEdBehaviorGraphEditor::DrawBlockBackground( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data )
{
	CEdGraphEditor::DrawBlockBackground( block, layout, data );
	if ( block->IsA< CBehaviorGraphVariableBaseNode >() )
	{
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{
			FillRect( block->GetPosition().X, block->GetPosition().Y + layout->m_windowSize.y - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, layout->m_windowSize.x - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, wxColour( 64, 64, 64 ) );

			CBehaviorGraphVariableBaseNode* b = Cast<CBehaviorGraphVariableBaseNode>( block );
			CBehaviorGraphInstance& bgi = *GetEditor()->GetBehaviorGraphInstance();
			Int32 p = (Int32) ( ( layout->m_windowSize.x - 2 * BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE ) * Max( 0.0f, Min( 1.0f, ( b->GetEditorValue( bgi ) - b->GetMin( bgi ) ) / ( b->GetMax( bgi ) - b->GetMin( bgi ) ) ) ) );
			FillRect( block->GetPosition().X + p , block->GetPosition().Y + layout->m_windowSize.y - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, wxColour( 128, 128, 128 ) );

			DrawRect( block->GetPosition().X, block->GetPosition().Y + layout->m_windowSize.y - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, layout->m_windowSize.x - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, wxColour( 0, 0, 0 ) );

			FillRect( block->GetPosition().X + layout->m_windowSize.x - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, block->GetPosition().Y + layout->m_windowSize.y - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, wxColour( 255, 255, 255 ) );
			DrawRect( block->GetPosition().X + layout->m_windowSize.x - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, block->GetPosition().Y + layout->m_windowSize.y - BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE, wxColour( 0, 0, 0 ) );
		}
	}
}

void CEdBehaviorGraphEditor::DrawBlockLayout( CGraphBlock* block )
{
	// draw block as usual
	CEdGraphEditor::DrawBlockLayout( block );

	// Display special info if node is deprecated
	CBehaviorGraphNode* behaviorNode = Cast< CBehaviorGraphNode >( block );
	if ( !behaviorNode )
	{
		return;
	}
	
	if ( behaviorNode->IsDeprecated() )
	{
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{				
			wxColour borderColor( block->GetBorderColor().R, block->GetBorderColor().G, block->GetBorderColor().B );
			wxColour interiorColor( 255, 0, 0 );

			wxPoint location( block->GetPosition().X, block->GetPosition().Y );
			wxPoint corner( location + layout->m_windowSize );

			DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 8 );

			wxRect textRect = TextRect( GetGdiDrawFont(), behaviorNode->GetDeprecateComment() );
			wxPoint textPos(  block->GetPosition().X + layout->m_windowSize.GetWidth() / 2 - textRect.GetWidth() / 2
							, block->GetPosition().Y - textRect.height - 5 );

			DrawText( textPos, GetGdiDrawFont(), behaviorNode->GetDeprecateComment(), wxColour(255,0,0) );	
		}
	}

#ifdef TRACK_BEH_NODES_SAMPLE_ID
	if ( behaviorNode->GetSampledNum( *GetEditor()->GetBehaviorGraphInstance() ) > 0 )
	{
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{
			String str = String::Printf( TXT("Sampled [%d] times - please debug"), behaviorNode->GetSampledNum( *GetEditor()->GetBehaviorGraphInstance() ) );
			wxColour borderColor( block->GetBorderColor().R, block->GetBorderColor().G, block->GetBorderColor().B );
			wxColour interiorColor( 255, 0, 0 );

			wxPoint location( block->GetPosition().X, block->GetPosition().Y );
			wxPoint corner( location + layout->m_windowSize );

			DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 8 );

			wxRect textRect = TextRect( GetGdiDrawFont(), str );
			wxPoint textPos(  block->GetPosition().X + layout->m_windowSize.GetWidth() / 2 - textRect.GetWidth() / 2
				, block->GetPosition().Y - textRect.height - 5 );

			DrawText( textPos, GetGdiDrawFont(), str, wxColour(255,0,0) );
		}
	}
#endif

	EBehaviorLod lod = GetEditor()->GetLodLevel();

	if ( lod != BL_Lod0 && !behaviorNode->WorkWithLod( lod ) )
	{
		/*static const wxColor lodColors[ BL_NoLod ] = { wxColor( 0, 0 ,0 ), wxColor( 255, 128, 128 ), wxColor( 255, 255, 128 ) };

		const wxColor& color = lodColors[ lod ];

		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{				
			wxPoint location( block->GetPosition().X, block->GetPosition().Y );
			wxPoint corner( location + layout->m_windowSize );

			DrawTipRect( location.x, location.y, corner.x, corner.y, *wxBLACK, color, 8 );
		}*/
	}

	// highlight default state machine node
	if ( block->GetClass() == CBehaviorGraphStateMachineNode::GetStaticClass() )
	{
		CBehaviorGraphStateMachineNode *machine = SafeCast< CBehaviorGraphStateMachineNode >( block );

		if ( GetBehaviorGraph()->GetDefaultStateMachine() == machine )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( block );
			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{				
				wxColour borderColor( block->GetBorderColor().R, block->GetBorderColor().G, block->GetBorderColor().B );
				wxColour interiorColor( 64, 255, 64 );

				wxPoint location( block->GetPosition().X, block->GetPosition().Y );
				wxPoint corner( location + layout->m_windowSize );

				DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 4 );
			}
		}
	}
	else if ( block->IsA< CBehaviorGraphSelfActivatingStateMachineNode >() )
	{
		CBehaviorGraphSelfActivatingStateMachineNode *machine = SafeCast< CBehaviorGraphSelfActivatingStateMachineNode >( block );

		if ( machine->IsRunning( *GetEditor()->GetBehaviorGraphInstance() ) )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( block );
			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{
				wxColour borderColor( block->GetBorderColor().R, block->GetBorderColor().G, block->GetBorderColor().B );
				wxColour interiorColor( 128, 128, 255 );

				wxPoint location( block->GetPosition().X, block->GetPosition().Y );
				wxPoint corner( location + layout->m_windowSize );

				DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 4 );
			}
		}
	}
	else if ( block->IsA< CBehaviorGraphMimicBlinkControllerNode_Setter >() )
	{
		CBehaviorGraphMimicBlinkControllerNode_Setter *node = SafeCast< CBehaviorGraphMimicBlinkControllerNode_Setter >( block );

		String desc;
		if ( node->GetBlinkStateDesc( *GetEditor()->GetBehaviorGraphInstance(), desc ) )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( block );
			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{
				wxColour borderColor( block->GetBorderColor().R, block->GetBorderColor().G, block->GetBorderColor().B );
				wxColour interiorColor( 255, 128, 128 );

				wxPoint location( block->GetPosition().X, block->GetPosition().Y );
				wxPoint corner( location + layout->m_windowSize );

				DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 4 );

				wxPoint textPos( block->GetPosition().X, block->GetPosition().Y + layout->m_windowSize.GetHeight() );

				DrawText( textPos, GetGdiDrawFont(), desc, wxColour( 255, 255, 255 ) );
			}
		}
	}
	else if ( block->IsA< CBehaviorGraphMimicBlinkControllerNode_Blend >() )
	{
		CBehaviorGraphMimicBlinkControllerNode_Blend *node = SafeCast< CBehaviorGraphMimicBlinkControllerNode_Blend >( block );

		String desc;
		if ( node->GetBlinkStateDesc( *GetEditor()->GetBehaviorGraphInstance(), desc ) )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( block );
			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{
				wxColour borderColor( block->GetBorderColor().R, block->GetBorderColor().G, block->GetBorderColor().B );
				wxColour interiorColor( 255, 128, 128 );

				wxPoint location( block->GetPosition().X, block->GetPosition().Y );
				wxPoint corner( location + layout->m_windowSize );

				DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 4 );

				wxPoint textPos( block->GetPosition().X, block->GetPosition().Y + layout->m_windowSize.GetHeight() );

				DrawText( textPos, GetGdiDrawFont(), desc, wxColour( 255, 255, 255 ) );
			}
		}
	}
	else if ( block->IsA< CBehaviorGraphComboStateNode >() )
	{
		CBehaviorGraphComboStateNode *combo = SafeCast< CBehaviorGraphComboStateNode >( block );

		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{
			wxPoint location( block->GetPosition().X, block->GetPosition().Y );

			String desc = combo->GetComboType();
			Bool alpha = GetEditor()->IsActivationAlphaEnabled();

			wxRect rect;
			rect.x = location.x + layout->m_innerRect.x;
			rect.y = location.y + layout->m_innerRect.y;
			rect.width = layout->m_innerRect.width;
			rect.height = layout->m_innerRect.height;

			wxPoint size = TextExtents( GetGdiDrawFont(), desc.AsChar() );
			wxPoint offset( rect.x + rect.width/2 - size.x/2, rect.y + rect.height/2 - size.y/2 );

			wxColour shadowColor( 0, 0, 0 );

			if ( combo->IsConnected() == false )
			{
				String err = TXT("Combo hasn't got global transition");

				wxColour interiorColor( 255, 0, 0, 160 );
				wxColour borderColor( 0, 0, 0 );

				wxPoint corner( location + layout->m_windowSize );

				wxPoint size = TextExtents( GetGdiDrawFont(), err.AsChar() );
				wxPoint offset( rect.x + rect.width/2 - size.x/2, rect.y + rect.height/2 - size.y/2 );

				DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 6 );

				DrawText( offset + wxPoint( 1, 1 ), GetGdiBoldFont(), err.AsChar(), shadowColor );
				DrawText( offset, GetGdiBoldFont(), err.AsChar(), interiorColor );
			}
			else if ( alpha )
			{
				Float cooldown = combo->GetCooldownProgress( *GetEditor()->GetBehaviorGraphInstance() );

				if ( cooldown > 0.f )
				{
					Color color = combo->GetBorderColor();
					color.A = Clamp< Uint8 >( ( 1.f - cooldown )* 255, 0, 255 );

					wxColour interiorColor( color.R, color.G, color.B, color.A );
					wxColour borderColor( 0, 0, 0 );

					wxPoint corner( location + layout->m_windowSize );

					DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 6 );
				}

				desc += TXT("\n") + combo->GetComboDesc( *GetEditor()->GetBehaviorGraphInstance() );

				wxRect rect;
				rect.x = location.x + layout->m_innerRect.x;
				rect.y = location.y + layout->m_innerRect.y;
				rect.width = layout->m_innerRect.width;
				rect.height = layout->m_innerRect.height;

				// Draw text
				size = TextExtents( GetGdiDrawFont(), desc.AsChar() );

				wxColour textColor( 200, 200, 200 );

				DrawText( offset + wxPoint(1,1), GetGdiDrawFont(), desc.AsChar(), shadowColor );
				DrawText( offset, GetGdiDrawFont(), desc.AsChar(), textColor );
			}
			else
			{
				wxColour textColor( 255, 255, 255 );

				DrawText( offset + wxPoint( 1, 1 ), GetGdiBoldFont(), desc.AsChar(), shadowColor );
				DrawText( offset, GetGdiBoldFont(), desc.AsChar(), textColor );
			}
		}
	}
	else if ( block->IsA< CBehaviorGraphFlowConnectionNode >() )
	{
		CBehaviorGraphFlowConnectionNode *flow = SafeCast< CBehaviorGraphFlowConnectionNode >( block );

		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( flow->HasAnimation() == false && layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{
			wxPoint location( block->GetPosition().X, block->GetPosition().Y );

			wxRect rect;
			rect.x = location.x + layout->m_innerRect.x;
			rect.y = location.y + layout->m_innerRect.y;
			rect.width = layout->m_innerRect.width;
			rect.height = layout->m_innerRect.height;

			wxColour shadowColor( 0, 0, 0 );

			String err = TXT("Flow state hasn't got animation");

			wxColour interiorColor( 255, 0, 0, 160 );
			wxColour borderColor( 0, 0, 0 );

			wxPoint corner( location + layout->m_windowSize );

			wxPoint size = TextExtents( GetGdiDrawFont(), err.AsChar() );
			wxPoint offset( rect.x + rect.width/2 - size.x/2, rect.y + rect.height/2 - size.y/2 );

			DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 6 );

			DrawText( offset + wxPoint( 1, 1 ), GetGdiBoldFont(), err.AsChar(), shadowColor );
			DrawText( offset, GetGdiBoldFont(), err.AsChar(), interiorColor );
		}
	}
	// highlight default node in state machine
	else if ( block->IsA< CBehaviorGraphStateNode >() )
	{
		CBehaviorGraphNode *node = Cast< CBehaviorGraphNode >( block );
		ASSERT( node );

		CBehaviorGraphStateMachineNode *machine = Cast< CBehaviorGraphStateMachineNode >( node->GetParentNode() );
		ASSERT( machine );

		if ( machine->GetDefaultState() == node )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( block );
			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{				
				wxColour borderColor( block->GetBorderColor().R, block->GetBorderColor().G, block->GetBorderColor().B );
				wxColour interiorColor( 64, 255, 64 );

				wxPoint location( block->GetPosition().X, block->GetPosition().Y );
				wxPoint corner( location + layout->m_windowSize );

				DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 4 );
			}
		}
	}
	else if ( block->IsA< CBehaviorGraphMimicsConverterNode >() )
	{
		CBehaviorGraphMimicsConverterNode *node = Cast< CBehaviorGraphMimicsConverterNode >( block );

		if ( node->CanWork( *GetEditor()->GetBehaviorGraphInstance() ) == false )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( node );

			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{				
				String text = TXT("Mimic converter couldn't work");

				wxColour borderColor( block->GetBorderColor().R, block->GetBorderColor().G, block->GetBorderColor().B );
				wxColour interiorColor( 255, 0, 0 );

				wxPoint location( block->GetPosition().X, block->GetPosition().Y );
				wxPoint corner( location + layout->m_windowSize );

				DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 8 );

				wxRect textRect = TextRect( GetGdiDrawFont(), text );
				wxPoint textPos(  block->GetPosition().X + layout->m_windowSize.GetWidth() / 2 - textRect.GetWidth() / 2
					, block->GetPosition().Y - textRect.height - 5 );

				DrawText( textPos, GetGdiDrawFont(), text, wxColour(255,0,0) );	
			}
		}
	}
	else if ( GetEditor()->IsActivationAlphaEnabled() && block->IsA< CBehaviorGraphMimicPoseNode >() )
	{
		CBehaviorGraphMimicPoseNode *node = static_cast< CBehaviorGraphMimicPoseNode* >( block );

		String descStr = TXT("Empty");
		Int32 desc = -1;
		Uint32 num = 0;

		const CAnimatedComponent* ac = GetEditor()->GetAnimatedComponent();
		if ( ac && Cast< const CMimicComponent >( ac ) )
		{
			const CMimicComponent* h = static_cast< const CMimicComponent* >( ac );
			if ( h )
			{
				const CMimicFace* face = h->GetMimicFace();
				if ( face )
				{
					CExtendedMimics mimics = h->GetExtendedMimics();

					const Int32 selectedPose = node->GetSelectedPose( *GetEditor()->GetBehaviorGraphInstance() );
					desc = selectedPose;

					if ( selectedPose != -1 )
					{
						if ( node->IsPoseType() )
						{
							const Int32 size = (Int32)mimics.GetNumTrackPoses();
							if ( selectedPose != -1 && selectedPose < size )
							{
								CName poseName = mimics.GetTrackPoseName( selectedPose );
								descStr = poseName.AsString();
								num = mimics.GetNumTrackPoses();
							}
						}
						else
						{
							const Int32 size = (Int32)mimics.GetNumFilterPoses();
							if ( selectedPose != -1 && selectedPose < size )
							{
								CName poseName = mimics.GetFilterPoseName( selectedPose );
								descStr = poseName.AsString();
								num = mimics.GetNumFilterPoses();
							}
						}
					}
				}
			}

			BlockLayoutInfo* layout = m_layout.FindPtr( node );

			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{				
				String text = descStr + String::Printf( TXT(" [%d-%d]"), desc, num );

				wxRect textRect = TextRect( GetGdiDrawFont(), text );

				wxPoint textPos( block->GetPosition().X, block->GetPosition().Y + layout->m_windowSize.GetHeight() );

				DrawText( textPos, GetGdiBoldFont(), text, wxColour( 255, 255, 255 ) );
			}
		}
	}
	else if ( block->IsA< CBehaviorGraph2DVariableNode >() )
	{
		CBehaviorGraph2DVariableNode* b = Cast< CBehaviorGraph2DVariableNode >( block );
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{
			CBehaviorGraphInstance& bgi = *GetEditor()->GetBehaviorGraphInstance();
			Vector min = b->GetMinVal( bgi );
			Vector max = b->GetMaxVal( bgi );
			Vector d = max - min;
			Vector f = b->GetVectorValue( bgi );
			f -= min;
			for ( Uint32 i = 0; i < 4; ++i ) f.A[i] /= d.A[i];
			Int32 rectSize = 9;
			Int32 w = layout->m_windowSize.x - 2 * BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
			Int32 h = ( layout->m_windowSize.y - layout->m_titleRect.height ) - 2 * BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN;
			f.X = w * f.X;
			f.Y = h * f.Y;

			wxRect rect( f.X - ( rectSize >> 1 ), layout->m_windowSize.y - f.Y - ( rectSize >> 1 ) - BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN, rectSize, rectSize );
			rect.width += rect.x;
			rect.height += rect.y;

			rect.x = Max( 0, rect.x );
			rect.y = Max( layout->m_titleRect.height + BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN, rect.y );

			rect.width = Min( rect.width, w );
			rect.height = Min( rect.height, layout->m_windowSize.y - BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN );

			FillRect( block->GetPosition().X + rect.x + BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN, block->GetPosition().Y + rect.y, rect.width - rect.x, rect.height - rect.y, wxColour( 255, 255, 255 ) );
			DrawRect( block->GetPosition().X + BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN, block->GetPosition().Y + layout->m_titleRect.height + BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN, w, h, wxColour( 0, 0, 0 ) );
		}
	}
	else if ( block->IsA< CBehaviorGraphAnimationNode >() )
	{
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{				
			wxColour black( 0, 0, 0 );
			Float progress = SafeCast< CBehaviorGraphAnimationNode >( block )->GetAnimProgress( *GetEditor()->GetBehaviorGraphInstance() );

			wxPoint location( block->GetPosition().X, block->GetPosition().Y );
			wxPoint corner( location + layout->m_titleRect.GetSize() );
			wxRect progRect( location, corner );

			progRect.Offset( 0, - progRect.GetHeight() / 2 );
			progRect.SetHeight( progRect.GetHeight() / 3 );

			wxRect barRect = progRect;
			barRect.Offset( progRect.GetWidth() * progress, 0 );
			barRect.SetWidth( progRect.GetWidth() * 0.01f ); // 1%

			wxPoint textPos( block->GetPosition().X, block->GetPosition().Y + layout->m_windowSize.GetHeight() );

			// Draw anim progress
			DrawRect( progRect, black );
			FillRect( barRect, black );	

			if ( CBehaviorGraphAnimationNode* animNode = SafeCast< CBehaviorGraphAnimationNode >( block ) )
			{
				CName animationName;
				if ( CSkeletalAnimationSetEntry* animation = animNode->GetAnimation( *GetEditor()->GetBehaviorGraphInstance() ) )
				{
					animationName = animation->GetName();
				}

				const String text = animationName.AsString();

				const String txt = String::Printf( TXT("Anim: %s"), text.AsChar() );

				wxRect textRect = TextRect( GetGdiDrawFont(), txt );

				DrawText( textPos, GetGdiDrawFont(), txt, wxColour( 255, 255, 255 ) );
				textPos.y += 10;
			}

			if ( CBehaviorGraphAnimationSlotNode* slot = Cast< CBehaviorGraphAnimationSlotNode >( block ) )
			{
				if ( slot->IsBlending( *GetEditor()->GetBehaviorGraphInstance() ) )
				{
					const String txt = String::Printf( TXT("Blending") );

					DrawText( textPos, GetGdiDrawFont(), txt, wxColour( 255, 255, 255 ) );
					textPos.y += 10;
				}
			}
		}
	}
	// display value next to value sockets
	else if ( GetEditor()->IsActivationAlphaEnabled() && block->IsA< CBehaviorGraphValueNode >() )
	{
		CBehaviorGraphValueNode *valueNode = SafeCast< CBehaviorGraphValueNode >( block );

		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{
			const TDynArray< CGraphSocket* > &sockets = block->GetSockets();
			for( Uint32 i=0; i<sockets.Size(); ++i )
			{
				CGraphSocket *socket = sockets[i];
				if ( socket->IsA< CBehaviorGraphVariableOutputSocket >() )
				{
					THashMap< THandle< CGraphSocket >, SocketLayoutInfo>::iterator it = layout->m_sockets.Find( socket );

					if ( it != layout->m_sockets.End() )
					{
						const SocketLayoutInfo& info = it->m_second;

						String socketText;

						if ( valueNode->IsActive( *GetEditor()->GetBehaviorGraphInstance() ) )
						{
							Float value = valueNode->GetValue( *GetEditor()->GetBehaviorGraphInstance() );
							socketText = String::Printf( TXT("%.2f"), value );
						}
						else if ( CBehaviorGraphVariableBaseNode* variableNode = Cast< CBehaviorGraphVariableBaseNode >( valueNode  ) )
						{
							Float value = variableNode->GetEditorValue( *GetEditor()->GetBehaviorGraphInstance() );
							socketText = String::Printf( TXT("%.2f <Inactive>"), value );
						}
						else
						{
							socketText = TXT("<Inactive>");
						}

						wxRect textRect = TextRect( GetGdiDrawFont(), socketText );
						
						{
							wxPoint textPos( block->GetPosition().X + info.m_socketRect.x - textRect.width + info.m_socketRect.width - 2, 
								block->GetPosition().Y + info.m_socketRect.y - textRect.height );	

							DrawText( textPos, GetGdiDrawFont(), socketText, wxColour(0,0,0) );	
						}

						{
							wxPoint textPos( block->GetPosition().X + layout->m_clientRect.GetWidth() / 2 - textRect.GetWidth() / 2,
								block->GetPosition().Y + layout->m_titleRect.GetHeight() + layout->m_clientRect.GetHeight() / 2 - textRect.height );

							DrawText( textPos, GetGdiDrawFont(), socketText, wxColour(0,0,0) );	
						}				
					}				
				}
				else if ( socket->IsA< CBehaviorGraphVectorVariableOutputSocket >() && valueNode->IsA< CBehaviorGraphVectorValueNode >()  )
				{
					CBehaviorGraphVectorValueNode* vecValueNode = Cast< CBehaviorGraphVectorValueNode > ( valueNode );
					THashMap< THandle< CGraphSocket >, SocketLayoutInfo>::iterator it = layout->m_sockets.Find( socket );

					if ( it != layout->m_sockets.End() )
					{
						const SocketLayoutInfo& info = it->m_second;

						Vector value = vecValueNode->GetVectorValue( *GetEditor()->GetBehaviorGraphInstance() );

						String socketText = String::Printf( TXT("%.2f %.2f %.2f %.2f"), value.X, value.Y, value.Z, value.W );
						wxRect textRect = TextRect( GetGdiDrawFont(), socketText );
						
						wxPoint textPos( block->GetPosition().X + info.m_socketRect.x - textRect.width + info.m_socketRect.width - 2, 
										 block->GetPosition().Y + info.m_socketRect.y - textRect.height );	

						DrawText( textPos, GetGdiDrawFont(), socketText, wxColour(0,0,0) );					
					}				
				}
			}
		}
	}
	// display names of start and end nodes over selected transition nodes
	else if ( IsBlockSelected( block ) && block->IsA< CBehaviorGraphStateTransitionNode >() )
	{
		CBehaviorGraphStateTransitionNode* transition = Cast< CBehaviorGraphStateTransitionNode >( block );
		CBehaviorGraphStateNode* startState = transition->CacheStateBlock( TXT("Start") );
		CBehaviorGraphStateNode* endState = transition->CacheStateBlock( TXT("End") );
		if ( startState && endState )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( block );
			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{
				String text = startState->GetCaption() + TXT(" -> ") + endState->GetCaption();
				wxRect textRect = TextRect( GetGdiDrawFont(), text );
				wxPoint textPos( block->GetPosition().X + layout->m_windowSize.GetWidth() / 2 - textRect.GetWidth() / 2
					, block->GetPosition().Y - textRect.height - 5 );
				DrawText( textPos, GetGdiBoldFont(), text, wxColour( 255, 255, 255 ) );
			}
		}
	}
	else if ( block->IsA< CBehaviorGraphGameplayAdditiveNode >() )
	{
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{				
			CBehaviorGraphGameplayAdditiveNode* addNode = static_cast< CBehaviorGraphGameplayAdditiveNode* >( block );

			wxColour black( 0, 0, 0 );
			Float progress1 = 0.f;
			Float progress2 = 0.f;

			CName name1;
			CName name2;
			
			addNode->GetAnimProgress( *GetEditor()->GetBehaviorGraphInstance(), progress1, progress2 );
			addNode->GetAnimNames( *GetEditor()->GetBehaviorGraphInstance(), name1, name2 );

			wxPoint location( block->GetPosition().X, block->GetPosition().Y );
			
			{
				wxPoint corner( location + layout->m_titleRect.GetSize() );
				wxRect progRect( location, corner );
				progRect.Offset( 0, - progRect.GetHeight() / 2 - progRect.GetHeight() / 3 );
				progRect.SetHeight( progRect.GetHeight() / 3 );

				wxRect barRect = progRect;
				barRect.Offset( progRect.GetWidth() * progress1, 0 );
				barRect.SetWidth( progRect.GetWidth() * 0.01f ); // 1%

				// Draw anim progress
				DrawRect( progRect, black );
				FillRect( barRect, black );	

				String text = name1.AsString();
				wxRect textRect = TextRect( GetGdiDrawFont(), text );
				wxPoint textPos( progRect.GetRightTop() - wxPoint( 0, textRect.GetHeight() / 2) );
				DrawText( textPos, GetGdiDrawFont(), text, black );
			}

			{
				wxPoint corner( location + layout->m_titleRect.GetSize() );
				wxRect progRect( location, corner );
				progRect.Offset( 0, - progRect.GetHeight() / 2 );
				progRect.SetHeight( progRect.GetHeight() / 3 );

				wxRect barRect = progRect;
				barRect.Offset( progRect.GetWidth() * progress2, 0 );
				barRect.SetWidth( progRect.GetWidth() * 0.01f ); // 1%

				// Draw anim progress
				DrawRect( progRect, black );
				FillRect( barRect, black );	

				String text = name2.AsString();
				wxRect textRect = TextRect( GetGdiDrawFont(), text );
				wxPoint textPos( progRect.GetRightTop() - wxPoint( 0, textRect.GetHeight() / 2) );
				DrawText( textPos, GetGdiDrawFont(), text, black );
			}
		}
	}
	else if ( block->IsA< CBehaviorGraphAnimationManualSlotNode >() )
	{
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{				
			CBehaviorGraphAnimationManualSlotNode* slot = static_cast< CBehaviorGraphAnimationManualSlotNode* >( block );
			
			wxColour black( 0, 0, 0 );

			const CName animationA = slot->GetAnimationAName( *GetEditor()->GetBehaviorGraphInstance() );
			const CName animationB = slot->GetAnimationBName( *GetEditor()->GetBehaviorGraphInstance() );

			const String textA = animationA.AsString();
			const String textB = animationB.AsString();

			const String txt = String::Printf( TXT("A: %s\nB: %s"), textA.AsChar(), textB.AsChar() );

			wxRect textRect = TextRect( GetGdiDrawFont(), txt );

			wxPoint textPos( block->GetPosition().X, block->GetPosition().Y + layout->m_windowSize.GetHeight() );

			DrawText( textPos, GetGdiDrawFont(), txt, wxColour( 255, 255, 255 ) );
		}
	}

	// Validate given node, report founded problems to user.
	TDynArray< String > errorStringVector;
	if ( !behaviorNode->ValidateInEditor(errorStringVector) )
	{
		String singleText;

		// concatenate errors:
		for ( const auto& text : errorStringVector)
		{
			singleText += text;
			singleText += TXT("\n");
		}

		BlockLayoutInfo* layout = m_layout.FindPtr( behaviorNode );

		if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
		{				
			wxColour borderColor( block->GetBorderColor().R, block->GetBorderColor().G, block->GetBorderColor().B );
			wxColour interiorColor( 255, 0, 0 );

			wxPoint location( block->GetPosition().X, block->GetPosition().Y );
			wxPoint corner( location + layout->m_windowSize );

			DrawTipRect( location.x, location.y, corner.x, corner.y, borderColor, interiorColor, 8 );

			wxRect textRect = TextRect( GetGdiDrawFont(), singleText );
			wxPoint textPos(  block->GetPosition().X + layout->m_windowSize.GetWidth() / 2 - textRect.GetWidth() / 2
				, block->GetPosition().Y - textRect.height - 5 );

			DrawText( textPos, GetGdiDrawFont(), singleText, wxColour(255,0,0) );	
		}
	}

	if ( block->GetClass() == CBehaviorGraphEngineValueNode::GetStaticClass() )
	{
		CBehaviorGraphEngineValueNode* node = Cast< CBehaviorGraphEngineValueNode >( block );

		if ( !node->IsManuallyControlled() )
		{
			BlockLayoutInfo* layout = m_layout.FindPtr( block );
			if ( layout && layout->m_visible && layout->m_onScreen )
			{
				wxColour color( 166, 0, 0 );

				if ( layout->m_freeze )
				{
					color = ConvertBlockColorToFreezeMode( color );
				}

				wxRect borderRect;
				borderRect.x		= block->GetPosition().X - 1;
				borderRect.y		= block->GetPosition().Y - 1;
				borderRect.width	= layout->m_windowSize.x + 2;
				borderRect.height	= layout->m_windowSize.y + 2;

				DrawRect( borderRect, color, 1 );
			}
		}
	}

	// draw debug visualizer icon
	if ( GetEditor()->IsActivationAlphaEnabled() )
	{
		CBehaviorDebugVisualizer **debugVisualizerPtr = m_debugVisualizers.FindPtr( behaviorNode );
		if ( debugVisualizerPtr )
		{
			CBehaviorDebugVisualizer *debugVisualizer = *debugVisualizerPtr;

			BlockLayoutInfo* layout = m_layout.FindPtr( behaviorNode );
			if ( layout && layout->m_visible && layout->m_onScreen && !layout->m_freeze )
			{			
				wxRect debugVisualizerRect;
				debugVisualizerRect.x		= block->GetPosition().X + 4;
				debugVisualizerRect.y		= block->GetPosition().Y + 6;
				debugVisualizerRect.width	= 10;
				debugVisualizerRect.height	= 10;

				FillRect( debugVisualizerRect, wxColour( debugVisualizer->GetColor().R, 
					debugVisualizer->GetColor().G, 
					debugVisualizer->GetColor().B, 
					255 ) );

				DrawRect( debugVisualizerRect, wxColour( 0, 0, 0, 255 ) );
			}
		}
	}
}


void CEdBehaviorGraphEditor::DrawBlockLinks( CGraphBlock* block )
{
	CEdGraphEditor::DrawBlockLinks( block );

	if ( block->IsA< CBehaviorGraphStateTransitionGlobalBlendNode >() )
	{
		CBehaviorGraphStateTransitionGlobalBlendNode* globalTransition = Cast< CBehaviorGraphStateTransitionGlobalBlendNode >( block );

		CBehaviorGraphStateNode* sourceState = globalTransition->GetSourceState();
		CBehaviorGraphStateNode* destState = globalTransition->GetDestState();

		if ( sourceState )
		{
			Color linkColor( Color::BLACK );

			SetTransitionColor( sourceState, destState, linkColor );

			Float width = 1.0f;

			if ( IsBlockActivated( sourceState ) || IsBlockActivated( destState ) )
			{
				Float alpha = IsBlockActivated( sourceState ) ? GetBlockActivationAlpha( sourceState ) : 0.0f;
				alpha = Min( alpha, IsBlockActivated( destState ) ? GetBlockActivationAlpha( destState ) : 0.0f );

				linkColor.R = linkColor.R + (Uint8)( ( 255 - linkColor.R ) * alpha );
				linkColor.G = linkColor.G + (Uint8)( ( 255 - linkColor.G ) * alpha );
				linkColor.B = (Uint8)( ( 1.0f - alpha ) * linkColor.B );
				linkColor.A = 255;
				width = 1.0 + alpha * 4.0f;
			}

			DrawTransitionLink( sourceState, globalTransition, destState, wxColour( linkColor.R, linkColor.G, linkColor.B ), width );
		}
		else
		{
			Color linkColor( Color::BLACK );

			SetTransitionColor( sourceState, destState, linkColor );

			Float width = 1.0f;

			DrawTransitionLink( globalTransition, globalTransition, destState, wxColour( linkColor.R, linkColor.G, linkColor.B ), width );
		}
	}
	else if ( block->IsA< CBehaviorGraphStateNode >() == false )
	{		
		return;
	}

	// special shit for drawing fancy transition connections
	for( Uint32 i=0; i<block->GetSockets().Size(); ++i )	
	{
		// Get socket info
		CGraphSocket* socket = block->GetSockets()[i];

		if ( !socket->IsA< CBehaviorGraphStateOutSocket >() )
		{
			continue;
		}

		const TDynArray< CGraphConnection* >& connections = socket->GetConnections();
		for( Uint32 j=0; j<connections.Size(); ++j )
		{
			CGraphConnection *connection = connections[j];

			if (connection->GetDestination() && connection->GetSource())
			{
				CGraphBlock *transitionBlock = connection->GetDestination()->GetBlock();

				CGraphSocket *transitionEndSocket = transitionBlock->FindSocket( TXT("End") );
				// ASSERT( transitionEndSocket );
				// ASSERT( transitionEndSocket->GetConnections().Size() == 1 );

				auto endConn = transitionEndSocket->GetConnections();
				if( endConn.Size() > 0 )
				{
					CGraphBlock *endBlock = endConn[0]->GetDestination()->GetBlock();

					Color linkColor = connection->GetSource()->GetLinkColor();

					SetTransitionColor( block, endBlock, linkColor );

					Float width = 1.0f;

					if ( IsBlockActivated( connection->GetSource()->GetBlock() ) || IsBlockActivated( connection->GetDestination()->GetBlock() ) )
					{
						Float alpha = IsBlockActivated( connection->GetSource()->GetBlock() ) ? GetBlockActivationAlpha( connection->GetSource()->GetBlock() ) : 0.0f;
						alpha = Min( alpha, IsBlockActivated( connection->GetDestination()->GetBlock() ) ? GetBlockActivationAlpha( connection->GetDestination()->GetBlock() ) : 0.0f );

						linkColor.R = linkColor.R + (Uint8)( ( 255 - linkColor.R ) * alpha );
						linkColor.G = linkColor.G + (Uint8)( ( 255 - linkColor.G ) * alpha );
						linkColor.B = (Uint8)( ( 1.0f - alpha ) * linkColor.B );
						linkColor.A = 255;
						width = 1.0 + alpha * 4.0f;
					}

					DrawTransitionLink( block, transitionBlock, endBlock, wxColour( linkColor.R, linkColor.G, linkColor.B ), width );
				}
				else
				{
					connection->SetActive( false );
					RED_ASSERT( false, TXT( "connections list is empty - behavior graph editor" ) );
				}
			}
		}
	}
}

void CEdBehaviorGraphEditor::SetTransitionColor( CGraphBlock* startBlock, CGraphBlock* endBlock, Color& linkColor )
{
	if ( IsBlockSelected( startBlock ) )
	{
		if ( IsBlockSelected( endBlock ) )
		{
			linkColor = Color::WHITE;
		}
		else
		{
			linkColor = Color::GREEN;
		}
	}
	else if ( IsBlockSelected( endBlock ) )
	{
		linkColor = Color::LIGHT_BLUE;
	}
}

void CEdBehaviorGraphEditor::OnBreakAllLinks( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	if ( m_activeItem.Get()->IsA< CBehaviorGraphStateNode >() )
	{
		if ( !PrepareForGraphStructureModified() )
		{
			return;
		}
		
		CBehaviorGraphStateNode *node = SafeCast< CBehaviorGraphStateNode >( m_activeItem.Get() );
		for( Uint32 i=0; i<node->GetSockets().Size(); ++i )
		{
			// dont touch in/out sockets
			if ( node->GetSockets()[i]->IsA< CBehaviorGraphStateOutSocket >() ||
				 node->GetSockets()[i]->IsA< CBehaviorGraphStateInSocket >() )
				continue;

			node->GetSockets()[i]->BreakAllLinks();
		}

		CEdGraphEditor::GraphStructureModified();
	} 
	else if ( m_activeItem.Get()->IsA< CBehaviorGraphStateOutSocket >() )
	{
		ASSERT( !"Shouldn't try to break links from stateOut socket" );				
	}
	else if ( m_activeItem.Get()->IsA< CBehaviorGraphStateTransitionNode >() )
	{
		if ( !PrepareForGraphStructureModified() )
		{
			return;
		}

		CBehaviorGraphStateTransitionNode *node = SafeCast< CBehaviorGraphStateTransitionNode >( m_activeItem.Get() );
		for( Uint32 i=0; i<node->GetSockets().Size(); ++i )
		{
			// dont touch transition sockets
			if ( node->GetSockets()[i]->IsA< CBehaviorGraphTransitionSocket >() )
				continue;

			node->GetSockets()[i]->BreakAllLinks();
		}
		
		CEdGraphEditor::GraphStructureModified();
	}
	else
	{
		CEdGraphEditor::OnBreakAllLinks( event );
	}
}

void CEdBehaviorGraphEditor::OnBreakLink( wxCommandEvent& event )
{
	if ( m_activeItem.Get()->IsA< CBehaviorGraphStateOutSocket >() )
	{
		wxMessageBox( TXT("To delete transition, remove transition block instead"), TXT("Warning") );
	}
	else
	{
		CEdGraphEditor::OnBreakLink( event );
	}
}

void CEdBehaviorGraphEditor::AdjustBlockColors( CGraphBlock* block, Color& borderColor, Float& borderWidth )
{
	if ( block->IsA< CBehaviorGraphStateTransitionNode >() )
	{
		CBehaviorGraphStateTransitionNode* transition = Cast< CBehaviorGraphStateTransitionNode >( block );
		CBehaviorGraphStateNode* startNode = transition->CacheStateBlock( TXT("Start") );
		CBehaviorGraphStateNode* endNode = transition->CacheStateBlock( TXT("End") );
		SetTransitionColor( startNode, endNode, borderColor );
	}
}

void CEdBehaviorGraphEditor::OnEditTrackNames(wxCommandEvent &event)
{

	//GetBehaviorGraph()->SetFloatTrackName();
}

void CEdBehaviorGraphEditor::CreateGlobalTransition( CClass* blockClass )
{
	ASSERT( m_activeItem.Get() );

	if ( m_activeItem.Get()->IsA< CBehaviorGraphStateOutSocket >() )
	{
		if ( !PrepareForGraphStructureModified() )
		{
			return;
		}

		// Get socket's owner
		CGraphBlock* block = ( SafeCast< CBehaviorGraphStateOutSocket >( m_activeItem.Get() ) )->GetBlock();
		ASSERT( block );

		CBehaviorGraphStateNode* state = Cast< CBehaviorGraphStateNode >( block );
		if( !state ) return;

		// Get state's input socket
		CBehaviorGraphStateInSocket* stateSocket = state->CGraphBlock::FindSocket< CBehaviorGraphStateInSocket >( CNAME( In ) );
		ASSERT( stateSocket );

		// Create force transition node
		wxPoint graphPoint = ClientToCanvas( m_lastClickPoint );
		GraphBlockSpawnInfo info( blockClass );
		info.m_position = Vector( graphPoint.x, graphPoint.y, 0, 1 );

		ASSERT( m_currentRoot && m_currentRoot->IsA< CBehaviorGraphStateMachineNode >() );

		// Create new node
		CBehaviorGraphStateTransitionGlobalBlendNode* transition = Cast<CBehaviorGraphStateTransitionGlobalBlendNode>( m_currentRoot->CreateChildNode( info ) );
		ASSERT( transition );

		// Transition socket
		CBehaviorGraphTransitionSocket* transitionSocket = transition->CGraphBlock::FindSocket< CBehaviorGraphTransitionSocket >( CNAME( End ) );

		// Connect
		transitionSocket->ConnectTo( stateSocket );	

		// Cache connections
		transition->CacheConnections();

		// Set graph modified
		CEdGraphEditor::GraphStructureModified();
	}
}

void CEdBehaviorGraphEditor::OnAddGlobalTransition( wxCommandEvent& event )
{
	CreateGlobalTransition( ClassID<CBehaviorGraphStateTransitionGlobalBlendNode>() );
}

void CEdBehaviorGraphEditor::OnAddGlobalComboTransition( wxCommandEvent& event )
{
	CreateGlobalTransition( ClassID< CBehaviorGraphGlobalComboTransitionNode >() );	
}

void CEdBehaviorGraphEditor::OnCreateDefaultTransition( wxCommandEvent& event )
{
	if ( !PrepareForGraphStructureModified() )
	{
		return;
	}

	OnCreateTransition( event );

	CreateTransitionPopupWrapper* wrapper = ( CreateTransitionPopupWrapper* ) event.m_callbackUserData;

	const TDynArray< CGraphConnection* >& conn = wrapper->m_srcSocket->GetConnections();
	
	if ( conn.Size() > 0 )
	{
		Uint32 num = conn.Size() - 1;

		CGraphBlock* block = conn[ num ]->GetDestination( true ) ? conn[ num ]->GetDestination( true )->GetBlock() : NULL;
		if ( block )
		{
			CBehaviorGraphStateTransitionBlendNode* blendTrans = SafeCast< CBehaviorGraphStateTransitionBlendNode >( block );
			if ( blendTrans )
			{
				if ( event.GetId() == ID_CREATE_DEFAULT_TRANS_FL )
				{
					blendTrans->CreateDefault_FootLeft();
				}
				else if ( event.GetId() == ID_CREATE_DEFAULT_TRANS_FR )
				{
					blendTrans->CreateDefault_FootRight();
				}
				else if ( event.GetId() == ID_CREATE_DEFAULT_TRANS_END_AUX )
				{
					blendTrans->CreateDefault_AnimEndAUX();
				}
				else
				{
					ASSERT( 0 );
				}
			}
		}
		else
		{
			ASSERT( block );
		}
	}
	else
	{
		ASSERT( conn.Size() == 1 );
	}	

	CEdGraphEditor::GraphStructureModified();
}

void CEdBehaviorGraphEditor::OnGoToSnapshot( wxCommandEvent& event )
{
	Int32 num = event.GetId() - ID_GOTO_SNAPSHOT;
	TDynArray<String> snapshots;
	GetGraphSnaphots(snapshots);

	ASSERT(num < (Int32)snapshots.Size());

	//GetBehaviorGraph()->GoToSnapshotPose(snapshots[num]);
}

void CEdBehaviorGraphEditor::GetGraphSnaphots(TDynArray<String>& snapshots)
{
	C2dArray* snapshotsDef = LoadResource<C2dArray>(BEHAVIOR_SNAPSHOT_CSV);
	if (snapshotsDef)
	{
		Uint32 rowSize, colSize;
		snapshotsDef->GetSize(colSize, rowSize);

		// | Behavior | Pose | Param name | Param value |
		ASSERT(colSize == 4);

		if (GetBehaviorGraph()->GetFile())
		{
			String behaviorName = GetBehaviorGraph()->GetFile()->GetFileName();
			behaviorName = behaviorName.StringBefore(TXT("."), true);

			// Find behavior
			Uint32 behaviorIdx;
			VERIFY(snapshotsDef->FindHeader(TXT("Behavior"), behaviorIdx));
			Int32 temp = snapshotsDef->GetRowIndex(TXT("Behavior"), behaviorName);
			if (temp == -1) return;
			
			Uint32 rowStart = (Uint32)temp;

			// Find next behavior
			Uint32 rowEnd = rowStart+1;
			while ( rowEnd < rowSize)
			{
				String nextBehName = snapshotsDef->GetValue(behaviorIdx,rowEnd);
				if (behaviorName != nextBehName)
				{
					break;
				}
				rowEnd++;
			}

			// Find all snapshot poses
			Uint32 poseIdx;
			VERIFY(snapshotsDef->FindHeader(TXT("Pose"), poseIdx));
			for (Uint32 i=rowStart; i<rowEnd; i++)
			{
				String poseName = snapshotsDef->GetValue(poseIdx,i);

				Bool found = false;
				for (Uint32 k=0; k<snapshots.Size(); k++)
				{
					if (snapshots[k] == poseName)
					{
						found = true;
						break;
					}
				}

				if (!found) snapshots.PushBack(poseName);
			}

		}
	}
}

void CEdBehaviorGraphEditor::DisplayBlockActivations( Bool flag )
{
	m_debugDisplayBlocksActivation = flag;
}

void CEdBehaviorGraphEditor::PasteOnCenter()
{
	Vector center;

	wxSize windowSize = GetPanelWindow()->GetSize();
	Float minX = 0.f;
	Float minY = 0.f;
	Float maxX = windowSize.x;
	Float maxY = windowSize.y;

	wxPoint graphCenter( ( maxX + minX ) / 2, ( maxY + minY ) / 2 );

	wxPoint centerPoint = ClientToCanvas( graphCenter );

	center.X = centerPoint.x;
	center.Y = centerPoint.y;
	center.Z = 0.f;
	center.W = 1.f;

	Paste( &center );
}

wxSize CEdBehaviorGraphEditor::GetGraphArea() const
{
	return GetPanelWindow()->GetSize();
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdBehaviorGraphLayerEditor, CEdBehaviorEditorSimplePanel )
	EVT_LIST_END_LABEL_EDIT( XRCID("listL"), CEdBehaviorGraphLayerEditor::OnLabelEditEnd )
	EVT_LIST_ITEM_ACTIVATED( XRCID("listV"), CEdBehaviorGraphLayerEditor::OnItemClickVisible )
	EVT_LIST_ITEM_ACTIVATED( XRCID("listF"), CEdBehaviorGraphLayerEditor::OnItemClickFreeze )
	EVT_MENU( XRCID("add"), CEdBehaviorGraphLayerEditor::OnAddNodes )
	EVT_MENU( XRCID("remove"), CEdBehaviorGraphLayerEditor::OnRemoveNodes )
	EVT_MENU( XRCID("clear"), CEdBehaviorGraphLayerEditor::OnClearLayer )
END_EVENT_TABLE()

CEdBehaviorGraphLayerEditor::CEdBehaviorGraphLayerEditor( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )		
{	
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorEditorLayerPanel") );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	CreateImageList();

	m_listLayer = XRCCTRL( *this, "listL", wxListCtrl );
	m_listVisible = XRCCTRL( *this, "listV", wxListCtrl );
	m_listFreeze = XRCCTRL( *this, "listF", wxListCtrl );

	FillList();

	SetSizer( sizer );	
	Layout();
}

wxAuiPaneInfo CEdBehaviorGraphLayerEditor::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Floatable( true ).Float().BestSize( 175, 70 );

	return info;
}

void CEdBehaviorGraphLayerEditor::OnReset()
{
	UpdateList();
}

void CEdBehaviorGraphLayerEditor::CreateImageList()
{
	m_imageList = new wxImageList( 16, 16, true );
	m_imageList->Add( SEdResources::GetInstance().LoadBitmap(_T("IMG_EYE")) );
	m_imageList->Add( SEdResources::GetInstance().LoadBitmap(_T("IMG_EYE_NOTLOADED")) );
	m_imageList->Add( SEdResources::GetInstance().LoadBitmap(_T("IMG_ASTERISK")) );
	m_imageList->Add( SEdResources::GetInstance().LoadBitmap(_T("IMG_ASTERISK_DISABLE")) );
	m_imageList->Add( SEdResources::GetInstance().LoadBitmap(_T("IMG_LAYERS_ON")) );
	m_imageList->Add( SEdResources::GetInstance().LoadBitmap(_T("IMG_LAYERS_OFF")) );
	m_imageList->Add( SEdResources::GetInstance().LoadBitmap(_T("IMG_LAYERS_FREEZE")) );
}

void CEdBehaviorGraphLayerEditor::FillList()
{
	// Set image list
	m_listLayer->SetImageList( m_imageList, wxIMAGE_LIST_SMALL );
	m_listVisible->SetImageList( m_imageList, wxIMAGE_LIST_SMALL );
	m_listFreeze->SetImageList( m_imageList, wxIMAGE_LIST_SMALL );

	m_listLayer->Hide();
	m_listVisible->Hide();
	m_listFreeze->Hide();

	for ( Uint32 i = 0; i < 0; i++ )
	{
		m_listLayer->InsertItem( i, wxT(""), -1 );
		m_listVisible->InsertItem( i, wxT(""), -1 );
		m_listFreeze->InsertItem( i, wxT(""), -1 );
	}

	 m_listLayer->Show();
	 m_listVisible->Show();
	 m_listFreeze->Show();
}

void CEdBehaviorGraphLayerEditor::UpdateList()
{
	// Get graph
	CBehaviorGraph* graph = GetBehaviorGraph();

	Uint32 layerNum = 0;

	m_listLayer->Hide();
	m_listVisible->Hide();
	m_listFreeze->Hide();

	for ( Uint32 i=0; i<layerNum; ++i )
	{
		const SGraphLayer* layer = NULL;
		ASSERT( layer );

		if ( layer )
		{
			// Name
			{
				wxListItem info;
				info.SetImage( 4 );
				info.SetId( i );
				info.SetText( layer->m_name.AsChar() );
				m_listLayer->SetItem( info );
			}

			// Visible
			Bool visible = false;
			if ( layer->m_state == GLS_Visible || layer->m_state == GLS_Freeze )
			{
				visible = true;
			}

			{
				wxListItem info;
				info.SetImage( visible ? 0 : 1 );
				info.SetId( i );
				m_listVisible->SetItem( info );
			}

			// Freeze
			{
				wxListItem info;
				info.SetImage( layer->m_state == GLS_Freeze ? 2 : 3 );
				info.SetId( i );
				m_listFreeze->SetItem( info );
			}
		}
	}

	m_listLayer->Show();
	m_listVisible->Show();
	m_listFreeze->Show();
}

void CEdBehaviorGraphLayerEditor::OnLabelEditEnd( wxListEvent& event )
{
	String name = event.GetText().wc_str();
	Int32 index = event.GetIndex();

	SGraphLayer* layer = NULL;
	ASSERT( layer );

	if ( layer )
	{
		layer->m_name = name;
		UpdateList();
	}
}

void CEdBehaviorGraphLayerEditor::OnItemClickFreeze( wxListEvent& event )
{
	Int32 index = event.GetIndex();

	SGraphLayer* layer = NULL;
	ASSERT( layer );

	if ( layer )
	{
		layer->m_state = layer->m_state == GLS_Freeze ? GLS_Visible : GLS_Freeze;
		UpdateList();
	}
}

void CEdBehaviorGraphLayerEditor::OnItemClickVisible( wxListEvent& event )
{
	Int32 index = event.GetIndex();

	SGraphLayer* layer = NULL;
	ASSERT( layer );

	if ( layer )
	{
		layer->m_state = layer->m_state == GLS_Visible ? GLS_Hide : GLS_Visible;
		UpdateList();
	}
}

void CEdBehaviorGraphLayerEditor::GetSelectedLayers( TDynArray< Uint32 >& layers )
{
	Int32 selCount = m_listLayer->GetSelectedItemCount();

	long item = m_listLayer->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	while ( item != -1 )
	{
		layers.PushBack( item );
		item = m_listLayer->GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}
}

void CEdBehaviorGraphLayerEditor::OnAddNodes( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > nodes;
	GetEditor()->GetSelectedNodes( nodes );

	TDynArray< Uint32 > layers;
	GetSelectedLayers( layers );

	for ( Uint32 i=0; i<nodes.Size(); ++i )
	{
		CGraphBlock* node = nodes[i];

		for ( Uint32 j=0; j<layers.Size(); ++j )
		{
			Uint32 layer = layers[j];

			node->AddToLayer( layer );
		}
	}
}

void CEdBehaviorGraphLayerEditor::OnRemoveNodes( wxCommandEvent& event )
{
	TDynArray< CGraphBlock* > nodes;
	GetEditor()->GetSelectedNodes( nodes );

	TDynArray< Uint32 > layers;
	GetSelectedLayers( layers );

	for ( Uint32 i=0; i<nodes.Size(); ++i )
	{
		CGraphBlock* node = nodes[i];

		for ( Uint32 j=0; j<layers.Size(); ++j )
		{
			Uint32 layer = layers[j];

			node->RemoveFromLayer( layer );
		}
	}
}

void CEdBehaviorGraphLayerEditor::OnClearLayer( wxCommandEvent& event )
{
	TDynArray< CBehaviorGraphNode* > nodes;
	GetBehaviorGraph()->GetAllNodes( nodes );

	TDynArray< Uint32 > layers;
	GetSelectedLayers( layers );

	for ( Uint32 i=0; i<nodes.Size(); ++i )
	{
		CBehaviorGraphNode* node = nodes[i];

		for ( Uint32 j=0; j<layers.Size(); ++j )
		{
			Uint32 layer = layers[j];

			node->RemoveFromLayer( layer );
		}
	}
}

void CEdBehaviorGraphLayerEditor::OnNodesSelect( const TDynArray< CGraphBlock* >& nodes )
{
	
}

void CEdBehaviorGraphLayerEditor::OnNodesDeselect()
{

}


CBehaviorEditorEffectInterpolateFloat::CBehaviorEditorEffectInterpolateFloat( CName variableName, CEdBehaviorGraphEditor* editor, Float accumulationSpeed )
	: CBehaviorEditorEffect()
	, m_variableName ( variableName )
	, m_accumulationSpeed ( accumulationSpeed )
{
	m_type = BEET_INTERPOLATE_FLOAT;
	CBehaviorGraphInstance* instance = editor->GetBehaviorGraphInstance();
	const Float* valuePtr = instance->GetFloatValuePtr( m_variableName );
	if ( valuePtr )
	{
		m_accumulator = Clamp( ( *valuePtr - instance->GetFloatValueMin( m_variableName ) ) / ( instance->GetFloatValueMax( m_variableName ) - instance->GetFloatValueMin( m_variableName ) ), 0.0f, 1.0f );
		if ( m_accumulator > 0.5f )
		{
			m_accumulationSpeed = - Abs( accumulationSpeed );
		}
	}
}

void CBehaviorEditorEffectInterpolateFloat::OnTick( Float delta, CEdBehaviorGraphEditor* editor )
{
	m_accumulator += m_accumulationSpeed * delta;
	CBehaviorGraphInstance* instance = editor->GetBehaviorGraphInstance();
	const Bool hasVar = instance->HasFloatValue( m_variableName );

	if ( m_accumulator >= 1.0f || m_accumulator <= 0.0f )
	{
		Remove();
		Clamp( m_accumulator, 0.0f, 1.0f );
	}
	if ( hasVar )
	{
		instance->SetFloatValue( m_variableName, instance->GetFloatValueMin( m_variableName ) + m_accumulator * ( instance->GetFloatValueMax( m_variableName ) - instance->GetFloatValueMin( m_variableName ) ) );
	}
}
