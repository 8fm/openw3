
#include "build.h"
#include "spawnTreeEditor.h"

#include "../../common/core/feedback.h"

#include "../../common/engine/meshTypeComponent.h"
#include "../../common/engine/meshTypeResource.h"

#include "../../common/game/edSpawnTreeNode.h"
#include "../../common/game/spawnTreeIncludeNode.h"
#include "../../common/game/spawnTreeMultiEntry.h"
#include "../../common/game/spawnTreeEntry.h"


#include "undoSpawnTreeEditor.h"
#include "popupNotification.h"
#include "itemSelectionDialogs/spawnTreeNodeSelector.h"
#include "spawnTreeNodeProxy.h"
#include "meshStats.h"

enum
{
	MAX_SPECIAL_OPTIONS = 16,

	ID_MENU_HIDE_BRANCH = wxID_HIGHEST,
	ID_MENU_SHOW_ALL,
	ID_MENU_SHOW_STATS,
	ID_MENU_DEFAULT_VISIBILITY,
	ID_MENU_DELETE,
	ID_MENU_RESTORE_LAYOUT,
	ID_MENU_COPY,
	ID_MENU_COPY_BRANCH,
	ID_MENU_CUT,
	ID_MENU_PASTE,
	ID_MENU_DECORATE,
	ID_MENU_REGENERATE_IDS,
	ID_MENU_GENERATE_ID,
	ID_MENU_SPECIAL_OPTION,				// MAX_SPECIAL_OPTIONS slots goes here
	ID_MENU_DEBUG_OPTION = ID_MENU_SPECIAL_OPTION + MAX_SPECIAL_OPTIONS,
	ID_MENU_CHECK_OUT = ID_MENU_DEBUG_OPTION + MAX_SPECIAL_OPTIONS,
};

BEGIN_EVENT_TABLE( CEdSpawnTreeEditor, CEdTreeEditor )
	EVT_KEY_DOWN( CEdSpawnTreeEditor::OnKeyDown )
	END_EVENT_TABLE()

	IEdSpawnTreeNode* CEdSpawnTreeEditor::m_clipboard = NULL;

CEdSpawnTreeEditor::CEdSpawnTreeEditor( wxWindow* parent, IHook* hook )
	: CEdTreeEditor( parent, hook, true, true, true )
	, m_rootNode( nullptr )
	, m_rootNodeProxy( nullptr )
	, m_usedProxies( 0 )
	, m_dragged( false )
	, m_showStats( false )
{
	SEvents::GetInstance().RegisterListener( CNAME( EditorSpawnTreeResourceModified ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPreUndoStep ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPostUndoStep ), this );
}

CEdSpawnTreeEditor::~CEdSpawnTreeEditor()
{
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdSpawnTreeEditor::SetTree( ICreatureDefinitionContainer* rootNode )
{
	m_rootNode = rootNode;

	DeselectAllObjects( false );
	SetActiveItem( NULL );

	m_rootNode->SetDefaultVisibilityRec();

	if ( !GGame->IsActive() && IsLockedResource() )
	{
		m_rootNode->OnEditorOpened();
	}

	ForceLayoutUpdate();
	ZoomExtents();
	Repaint();
}

/*virtual*/ 
void CEdSpawnTreeEditor::ForceLayoutUpdate() /*override*/
{
	ClearNodeProxies();
	ClearLayout();

	if ( !m_rootNode )
	{
		return;
	}

	// Do not show the root, instead iterate through all the root children

	CEdSpawntreeNodeProxy* rootProxy = CreateNodeProxy( m_rootNode, nullptr );
	m_rootNodeProxy = rootProxy;

	for ( Int32 i = 0; i < m_rootNode->GetNumChildren(); ++i )
	{
		FillNodeLayout( m_rootNode->GetChild( i ), rootProxy, nullptr );
	}
}

void CEdSpawnTreeEditor::FillNodeLayout( IEdSpawnTreeNode* node, CEdSpawntreeNodeProxy* parentProxy, LayoutInfo* parentLayout )
{
	if ( !node )
	{
		return;
	}

	CEdSpawntreeNodeProxy* proxy = CreateNodeProxy( node, parentProxy );

	LayoutInfo* layout = UpdateBlockLayout( proxy, parentLayout );

	if ( parentProxy && parentProxy->GetEngineObject()->IsA< CSpawnTreeIncludeTreeNode >() )
	{
		// fake the block position to not depend on the original sub-tree starting point
		layout->m_windowPos.x = layout->m_parent->m_windowPos.x + layout->m_parent->m_windowSize.x/2 - layout->m_windowSize.x/2;
		layout->m_windowPos.y = layout->m_parent->m_windowPos.y + 100;
	}

	Color clr = node->GetBlockColor();
	layout->m_bgColor = wxColour( clr.R, clr.G, clr.B );

	if ( !node->IsHidden() )
	{
		// fill the layout of children nodes
		for ( Int32 i = 0; i < node->GetNumChildren(); ++i )
		{
			FillNodeLayout( node->GetChild( i ), proxy, layout );
		}
	}
}

/*virtual*/ 
void CEdSpawnTreeEditor::PaintCanvas( Int32 width, Int32 height ) /*override*/
{
	CEdTreeEditor::PaintCanvas( width, height );

	if ( IsDebugMode() )
	{
		Float scale = GetScale();
		wxPoint offset = GetOffset();

		SetScale( 1.0f, false );
		SetOffset( wxPoint( 0, 0 ), false );

		DrawText( wxPoint( 10, 10 ), GetGdiBoldFont(), TXT("[DEBUG MODE]"), *wxRED );

		SetScale( scale, false );
		SetOffset( offset, false );
	}
}

/*virtual*/ 
void CEdSpawnTreeEditor::DrawBlockLayout( LayoutInfo & layout ) /*override*/
{
	CEdTreeEditor::DrawBlockLayout( layout );

	if ( !IsDebugMode() )
	{
		return;
	}

	CEdSpawntreeNodeProxy* nodeProxy = Cast< CEdSpawntreeNodeProxy >( layout.m_owner );

	Bool drawFrame = false;
	wxColour frameClr;
	if ( const CSpawnTreeInstance* instance = nodeProxy->GetInstanceBuffer() )
	{
		IEdSpawnTreeNode::EDebugState state = nodeProxy->GetSpawnTreeNode()->GetDebugState( instance );
		switch ( state )		
		{
		case IEdSpawnTreeNode::EDEBUG_ACTIVE:
			frameClr = wxColor( 0, 255, 0 );
			drawFrame = true;
			break;
		case IEdSpawnTreeNode::EDEBUG_INVALID:
			frameClr = wxColor( 255, 0, 0 );
			drawFrame = true;
			break;
		case IEdSpawnTreeNode::EDEBUG_DEACTIVATED:
		case IEdSpawnTreeNode::EDEBUG_NOT_RELEVANT:
			// Do not draw frame
			break;
		}
	}
	else
	{
		// gray
		frameClr = wxColor( 96, 96, 96 );
		drawFrame = true;
	}

	if ( drawFrame )
	{
		DrawRoundedRect( layout.m_windowPos.x-4, layout.m_windowPos.y-4, layout.m_windowSize.x+8, layout.m_windowSize.y+8, frameClr, 10, 4 );
	}
}


void CEdSpawnTreeEditor::PreStructureModification()
{
	ASSERT ( !GGame->IsActive() );
	m_rootNode->PreStructureModification();
}

void CEdSpawnTreeEditor::PostStructureModification()
{
	ForceLayoutUpdate();
	Repaint();
	m_rootNode->AsCObject()->MarkModified();
}

CEdSpawntreeNodeProxy* CEdSpawnTreeEditor::GetActiveNode() const
{
	return GetActiveItem()
		? Cast< CEdSpawntreeNodeProxy >( GetActiveItem()->m_owner )
		: m_rootNodeProxy;
}

Bool CEdSpawnTreeEditor::IsLockedResource() const
{
	if ( !m_rootNode )
	{
		return false;
	}

	if ( CResource* res = Cast< CResource >( m_rootNode->AsCObject() ) )
	{
		if ( !res->CanModify() )
		{
			return true;
		}
	}

	return false;
}

Bool CEdSpawnTreeEditor::IsDebugMode() const
{
	return GGame->IsActive() && !m_rootNode->AsCObject()->IsA< CResource >();
}

Bool CEdSpawnTreeEditor::IsLocked( IScriptable& block ) const
{
	CEdSpawntreeNodeProxy* proxy = Cast< CEdSpawntreeNodeProxy >( &block );
	ASSERT( proxy );
	return GGame->IsActive() || IsLockedResource() || proxy->GetSpawnTreeNode()->IsLocked() || proxy->IsExternalNode();
}

Bool CEdSpawnTreeEditor::IsHiddenBranch( IScriptable & block ) const
{
	CEdSpawntreeNodeProxy* proxy = Cast< CEdSpawntreeNodeProxy >( &block );
	ASSERT( proxy );
	return proxy->GetSpawnTreeNode()->IsHidden() && proxy->GetSpawnTreeNode()->GetNumChildren() > 0;
}

void CEdSpawnTreeEditor::DeleteActiveNode( const String& undoStepNameOverride )
{
	CEdSpawntreeNodeProxy* nodeProxy = GetActiveNode();
	IEdSpawnTreeNode* node = nodeProxy->GetSpawnTreeNode();

	PreStructureModification();

	ASSERT ( nodeProxy != m_rootNodeProxy ); // removing the root is not supported
	IEdSpawnTreeNode* parent = node->GetParentNode();
	ASSERT ( parent );
	parent->RemoveChild( node );

	CUndoSpawnTreeBlockExistance::PrepareDeletionStep( *GetUndoManager(), node->AsCObject() );
	CUndoSpawnTreeBlockExistance::FinalizeStep( *GetUndoManager(), undoStepNameOverride );

	DeselectAllObjects();

	PostStructureModification();
}

void CEdSpawnTreeEditor::CopyActiveNode()
{
	CEdSpawntreeNodeProxy* nodeProxy = GetActiveNode();
	IEdSpawnTreeNode* activeNode = nodeProxy->GetSpawnTreeNode();
	ASSERT( activeNode != NULL );

	if ( m_clipboard )
	{
		m_clipboard->AsCObject()->RemoveFromRootSet();
		m_clipboard->AsCObject()->Discard();
		m_clipboard = NULL;
	}

	CObject* clonedBranch = nodeProxy->GetEngineObject()->Clone( NULL );
	clonedBranch->AddToRootSet();
	m_clipboard = dynamic_cast< IEdSpawnTreeNode* >( clonedBranch );
	ASSERT( m_clipboard );
}

/*virtual*/ 
void CEdSpawnTreeEditor::DeleteSelection() /*override*/
{
	CleanUpSelection( true, true );
	PreStructureModification();

	const TDynArray< IScriptable* >& selection = GetSelectedObjects();
	for ( Uint32 i = 0; i < selection.Size(); ++i )
	{
		if ( CEdSpawntreeNodeProxy* nodeProxy = Cast< CEdSpawntreeNodeProxy >( selection[i] ) )
		{
			ASSERT ( nodeProxy != m_rootNodeProxy ); // removing the encounter itself is not supported
			IEdSpawnTreeNode* node = nodeProxy->GetSpawnTreeNode();
			IEdSpawnTreeNode* parent = node->GetParentNode();
			ASSERT ( parent );
			parent->RemoveChild( node );

			CUndoSpawnTreeBlockExistance::PrepareDeletionStep( *GetUndoManager(), node->AsCObject() );
		}
	}

	CUndoSpawnTreeBlockExistance::FinalizeStep( *GetUndoManager() );

	DeselectAllObjects();

	PostStructureModification();
}

/// wxWrapper for CObject
class PopupActiveNodeWrapper : public wxObject
{
public:
	IEdSpawnTreeNode*		m_activeNode;

public:
	PopupActiveNodeWrapper( IEdSpawnTreeNode* activeNode )
		: m_activeNode( activeNode )
	{};

	~PopupActiveNodeWrapper()
	{
	}
};

void CEdSpawnTreeEditor::FillUpAddMenu( wxMenu& menu )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();
	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	if ( m_rootNode->GetSpawnTreeType() != STT_gameplay )
	{
		TArrayMap< String, CClass * >	childClassArray;
		activeNode->GetPossibleChildClasses( childClassArray, m_rootNode->GetSpawnTreeType(), true );

		for ( Uint32 i = 0, count = childClassArray.Size(); i < count; ++i )
		{
			String caption = TXT("Add ") + childClassArray[ i ].m_first;
			menu.Append( i, caption.AsChar() );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnSpawnNode, this, i, -1, new PopupClassWrapper( childClassArray[ i ].m_second ) );
		}
	}
	else
	{
		TDynArray< CClass* > rootClasses;
		rootClasses.Reserve( 3 );
		activeNode->GetRootClassForChildren( rootClasses, m_rootNode->GetSpawnTreeType() );
		Bool baseNodeEntryAdded = false;
		Bool initNodeEntryAdded = false;
		Bool subDefAdded		= false;

		Uint32 i = 0;
		for ( auto it = rootClasses.Begin(), end = rootClasses.End(); it != end; ++it )
		{
			if ( (*it)->IsA<ISpawnTreeInitializer>() && initNodeEntryAdded == false )
			{
				initNodeEntryAdded = true;
				menu.Append( i, TXT("Add initializer node") );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::AddInitialiserNode, this, i, -1, new PopupActiveNodeWrapper( activeNode ) );
				++i;
			}
			else if ( (*it)->IsA<ISpawnTreeBaseNode>() && baseNodeEntryAdded == false)
			{
				baseNodeEntryAdded = true;
				menu.Append( i, TXT("Add spawn node") );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::AddBaseNode, this, i, -1, new PopupActiveNodeWrapper( activeNode ) );
				++i;
			}
			else if ( (*it)->IsA<CSpawnTreeEntrySubDefinition>() && subDefAdded == false)
			{
				subDefAdded = true;
				menu.Append( i, TXT("Add sub definition") );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::AddSubDefNode, this, i, -1, new PopupActiveNodeWrapper( activeNode ) );
				++i;
			}
		}

	}
}

void CEdSpawnTreeEditor::FillUpDecorateMenu( wxMenu& menu )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();
	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	if ( m_rootNode->GetSpawnTreeType() == STT_gameplay )
	{
		IEdSpawnTreeNode* parentNode = activeNode->GetParentNode();
		if ( parentNode )
		{
			TDynArray< CClass* > rootClasses;
			rootClasses.Reserve( 3 );
			parentNode->GetRootClassForChildren( rootClasses, m_rootNode->GetSpawnTreeType() );
			for ( auto it = rootClasses.Begin(), end = rootClasses.End(); it != end; ++it )
			{
				if ( (*it)->IsA<ISpawnTreeBaseNode>() )
				{
					menu.Append( ID_MENU_DECORATE, TXT("Decorate") );
					menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::Decorate, this, ID_MENU_DECORATE, -1, new PopupActiveNodeWrapper( activeNode ) );
					break;
				}
			}
		}
	}
}

/*virtual*/ 
void CEdSpawnTreeEditor::OnOpenContextMenu() /*override*/
{
	if ( !m_rootNode )
	{
		return;
	}

	wxMenu menu;

	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();
	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();
	Bool hasChildren = activeNode->GetNumChildren() > 0;
	Bool locked = IsLocked( *activeNodeProxy );

	if ( IsDebugMode() )
	{
		CSpawnTreeInstance* instance = activeNodeProxy->GetInstanceBuffer();
		if ( instance )
		{
			TDynArray< String > debugOptions;
			activeNode->GetContextMenuDebugOptions( *instance, debugOptions );
			if ( !debugOptions.Empty() )
			{
				for ( Uint32 i = 0, n = debugOptions.Size(); i != n; ++i )
				{
					menu.Append( ID_MENU_DEBUG_OPTION+i, debugOptions[ i ].AsChar() ); // encode special option id in item id
					menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnDebugOption, this, ID_MENU_DEBUG_OPTION+i );
				}
			}
		}
	}

	menu.AppendSeparator();

	if ( hasChildren )
	{
		menu.AppendCheckItem( ID_MENU_SHOW_ALL, TXT("Show all") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnShowAll, this, ID_MENU_SHOW_ALL );

		menu.AppendCheckItem( ID_MENU_DEFAULT_VISIBILITY, TXT("Reset visibility") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnDefaultVisibility, this, ID_MENU_DEFAULT_VISIBILITY );

		menu.AppendCheckItem( ID_MENU_SHOW_STATS, TXT("Show Stats") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnShowStats, this, ID_MENU_SHOW_STATS );
	}

	if ( activeNode->CanBeHidden() && hasChildren )
	{
		menu.AppendCheckItem( ID_MENU_HIDE_BRANCH, TXT("Hide branch") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnHideBranch, this, ID_MENU_HIDE_BRANCH );
		menu.Check( ID_MENU_HIDE_BRANCH, activeNode->IsHidden() );
	}

	if ( !locked && hasChildren )
	{
		menu.Append( ID_MENU_RESTORE_LAYOUT, TXT("Auto layout branch") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnRestoreDefaultLayout, this, ID_MENU_RESTORE_LAYOUT );
	}

	menu.AppendSeparator();

	if ( IsLockedResource() )
	{
		menu.Append( ID_MENU_CHECK_OUT, TXT("Check out for editing") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnCheckOut, this, ID_MENU_CHECK_OUT );
		wxMenuUtils::CleanUpSeparators( &menu );
		PopupMenu( &menu );
		return;
	}
	else
	{
		menu.Append( ID_MENU_REGENERATE_IDS, TXT("Regenerate ids of all editable nodes") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnRegenerateIds, this, ID_MENU_REGENERATE_IDS );
	}

	// 'Add' option

	if ( !locked && activeNode->CanAddChild() )
	{
		FillUpAddMenu( menu );
		FillUpDecorateMenu( menu );
	}

	menu.AppendSeparator();

	{
		// 'special options' - custom node stuff
		IEdSpawnTreeNode::SpecialOptionsList specialOptions;
		activeNode->GetContextMenuSpecialOptions( specialOptions );
		if ( !specialOptions.Empty() )
		{
			for ( Uint32 i = 0, n = specialOptions.Size(); i != n; ++i )
			{
				menu.Append( ID_MENU_SPECIAL_OPTION+i, specialOptions[ i ].AsChar() ); // encode special option id in item id
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnSpecialOption, this, ID_MENU_SPECIAL_OPTION+i );
			}
		}
	}

	menu.AppendSeparator();

	if ( activeNode->CanBeCopied() )
	{
		menu.Append( ID_MENU_COPY, TXT("Copy") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnCopyNode, this, ID_MENU_COPY );

		if ( hasChildren )
		{
			menu.Append( ID_MENU_COPY_BRANCH, TXT("Copy branch") );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnCopyBranch, this, ID_MENU_COPY_BRANCH );
		}

		if ( !locked )
		{
			menu.Append( ID_MENU_CUT, hasChildren ? TXT("Cut branch") : TXT("Cut") );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnCopyBranch, this, ID_MENU_CUT );
		}
	}

	if ( !locked && ( activeNodeProxy->GetEngineObject()->IsA< ISpawnTreeBaseNode >() || activeNodeProxy->GetEngineObject()->IsA< CSpawnTreeEntrySubDefinition >() || activeNodeProxy->GetEngineObject()->IsA< ISpawnTreeInitializer >() ) )
	{
		menu.Append( ID_MENU_GENERATE_ID, TXT("Generate node id") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnGenerateId, this, ID_MENU_GENERATE_ID );
	}

	if ( !locked && m_clipboard && activeNode->CanAddChild() )
	{
		CClass* clipClass = m_clipboard->AsCObject()->GetClass();
		if ( activeNode->IsPossibleChildClass( clipClass, m_rootNode->GetSpawnTreeType() ) )
		{
			String caption = TXT("Paste ") + m_clipboard->GetEditorFriendlyName();
			menu.Append( ID_MENU_PASTE, caption.AsChar() );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnPaste, this, ID_MENU_PASTE  );
		}
	}

	menu.AppendSeparator();

	if ( activeNode->GetParentNode() ) // do not show remove for the root
	{
		if ( !locked )
		{
			menu.Append( ID_MENU_DELETE, hasChildren ? TXT("Remove branch") : TXT("Remove") );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSpawnTreeEditor::OnDeleteNode, this, ID_MENU_DELETE );
		}
	}

	wxMenuUtils::CleanUpSeparators( &menu );

	if ( menu.GetMenuItemCount() > 0 )
	{
		PopupMenu( &menu );
	}
}

/*virtual*/ 
void CEdSpawnTreeEditor::OnMoveEnded() /*override*/
{
	PreStructureModification();

	m_rootNode->UpdateChildrenOrder();

	PostStructureModification();
}

/*virtual*/ 
Bool CEdSpawnTreeEditor::LoadBlockPosition( IScriptable* block, wxPoint & pos ) /*override*/
{
	if ( CEdSpawntreeNodeProxy* nodeProxy = Cast< CEdSpawntreeNodeProxy >( block ) )
	{
		IEdSpawnTreeNode* node = nodeProxy->GetSpawnTreeNode();
		pos.x = node->GetGraphPosX();
		pos.y = node->GetGraphPosY();
		return true;
	}

	return false;
}

/*virtual*/ 
Bool CEdSpawnTreeEditor::SaveBlockPosition( IScriptable * block, const wxPoint & pos ) /*override*/
{
	CEdSpawntreeNodeProxy* nodeProxy = Cast< CEdSpawntreeNodeProxy >( block );
	if ( nodeProxy )
	{
		nodeProxy->GetSpawnTreeNode()->SetGraphPosition( pos.x, pos.y );
		return true;
	}

	return false;
}

/*virtual*/ 
String CEdSpawnTreeEditor::GetBlockName( IScriptable& block ) const /*override*/
{
	CEdSpawntreeNodeProxy* nodeProxy = Cast< CEdSpawntreeNodeProxy >( &block );
	ASSERT ( nodeProxy );
	IEdSpawnTreeNode* node = nodeProxy->GetSpawnTreeNode();
	if ( IsDebugMode() )
	{
		if ( const CSpawnTreeInstance* instance = nodeProxy->GetInstanceBuffer() )
		{
			return node->GetBlockDebugCaption( *instance );
		}
	}

	return node->GetBlockCaption();
}

/*virtual*/ 
String CEdSpawnTreeEditor::GetBlockComment( IScriptable & block ) const /*override*/
{
	CEdSpawntreeNodeProxy* nodeProxy = Cast< CEdSpawntreeNodeProxy >( &block );
	ASSERT ( nodeProxy );
	IEdSpawnTreeNode* node = nodeProxy->GetSpawnTreeNode();
	if ( m_showStats )
	{
		String textureData = TXT("");
		TPair<Float, Uint32> data;
		if ( m_nodeStatistics.Find( node, data ) )
		{
			textureData = String::Printf( TXT(" Total Textures %.2f MB in %i Spawns"), data.m_first, data.m_second );
		}

		return node->GetComment() + textureData;
	}
	return node->GetComment();
}

/*virtual*/ 
String CEdSpawnTreeEditor::GetBitmapName( IScriptable& block ) const /*override*/
{
	CEdSpawntreeNodeProxy* nodeProxy = Cast< CEdSpawntreeNodeProxy >( &block );
	ASSERT ( nodeProxy );
	IEdSpawnTreeNode* node = nodeProxy->GetSpawnTreeNode();
	return node->GetBitmapName();
}

/*virtual*/ 
Bool CEdSpawnTreeEditor::CanHaveChildren( IScriptable& block ) const /*override*/
{
	CEdSpawntreeNodeProxy* nodeProxy = Cast< CEdSpawntreeNodeProxy >( &block );
	ASSERT ( nodeProxy );
	IEdSpawnTreeNode* node = nodeProxy->GetSpawnTreeNode();
	return node->CanAddChild();
}

void CEdSpawnTreeEditor::InsertNode( IEdSpawnTreeNode* newNode, IEdSpawnTreeNode* parentNode, const wxPoint* pos, Bool runCreationCallback )
{
	ASSERT( newNode && parentNode );

	if ( runCreationCallback )
	{
		newNode->AsCObject()->OnCreatedInEditor();
	}

	if ( pos )
	{
		newNode->SetGraphPosition( pos->x, pos->y );
	}
	else
	{
		Int32 maxX = OFFSET_X_SINGLE;
		Int32 maxY = OFFSET_Y_SINGLE;
		for ( Uint32 i = 0, n = parentNode->GetNumChildren(); i < n; ++i )
		{
			IEdSpawnTreeNode* sibling = parentNode->GetChild( i );
			maxX = Max( sibling->GetGraphPosX() + OFFSET_X_MANY, maxX );
			maxY = Max( sibling->GetGraphPosY() + OFFSET_Y_MANY, maxY );
		}
		newNode->SetGraphPosition( maxX, maxY );
	}

	newNode->GenerateId();
	parentNode->AddChild( newNode );

	m_rootNode->UpdateChildrenOrder();

	CUndoSpawnTreeBlockExistance::PrepareCreationStep( *GetUndoManager(), newNode->AsCObject() );
	CUndoSpawnTreeBlockExistance::FinalizeStep( *GetUndoManager() );
}

void CEdSpawnTreeEditor::OnSpawnNode( wxCommandEvent& event )
{
	CClass* nodeClass = static_cast< PopupClassWrapper* >( event.m_callbackUserData )->m_objectClass;
	ASSERT( nodeClass );
	CreateNode( nodeClass, GetActiveNode()->GetSpawnTreeNode() );
}

void CEdSpawnTreeEditor::AddBaseNode( wxCommandEvent& event )
{
	IEdSpawnTreeNode* activeNode = static_cast< PopupActiveNodeWrapper* >( event.m_callbackUserData )->m_activeNode;
	CEdSpawnTreeNodeSelectorDialog selector( this, ISpawnTreeBaseNode::GetStaticClass(), activeNode, m_rootNode->GetSpawnTreeType() );
	if ( CClass* nodeClass = selector.Execute() )
	{
		CreateNode( nodeClass, activeNode );
	}
}

void CEdSpawnTreeEditor::AddInitialiserNode( wxCommandEvent& event )
{
	IEdSpawnTreeNode* activeNode = static_cast< PopupActiveNodeWrapper* >( event.m_callbackUserData )->m_activeNode;
	CEdSpawnTreeNodeSelectorDialog selector( this, ISpawnTreeInitializer::GetStaticClass(), activeNode, m_rootNode->GetSpawnTreeType() );
	if ( CClass* nodeClass = selector.Execute() )
	{
		CreateNode( nodeClass, activeNode );
	}
}

void CEdSpawnTreeEditor::AddSubDefNode( wxCommandEvent& event )
{
	IEdSpawnTreeNode* activeNode = static_cast< PopupActiveNodeWrapper* >( event.m_callbackUserData )->m_activeNode;
	CreateNode( CSpawnTreeEntrySubDefinition::GetStaticClass(), activeNode );
}

void CEdSpawnTreeEditor::Decorate( wxCommandEvent& event )
{
	IEdSpawnTreeNode* activeNode  = static_cast< PopupActiveNodeWrapper* >( event.m_callbackUserData )->m_activeNode;
	IEdSpawnTreeNode* parentNode  = activeNode->GetParentNode();
	CEdSpawnTreeNodeSelectorDialog selector( this, ISpawnTreeBaseNode::GetStaticClass(), parentNode, m_rootNode->GetSpawnTreeType(), activeNode );
	if ( CClass* nodeClass = selector.Execute() )
	{
		CEdUndoManager::Transaction undoTransaction( *GetUndoManager(), TXT("decorate") );

		PreStructureModification();

		wxPoint pos = m_dragged ? m_draggedPos : wxPoint( activeNode->GetGraphPosX()/2, activeNode->GetGraphPosY()/2 );

		// break current active<->parent nodes linkage
		parentNode->RemoveChild( activeNode );

		// create new node
		CObject* newNode = CreateObject< CObject >( nodeClass, parentNode->AsCObject(), 0 );
		IEdSpawnTreeNode* nodeAsNode = dynamic_cast< IEdSpawnTreeNode* >( newNode );
		nodeAsNode->SetHidden( false ); // do not hide new when decorating, even if it's hidden by default - will keep the existing layout
		nodeAsNode->AsCObject()->OnCreatedInEditor();
		nodeAsNode->SetGraphPosition( pos.x, pos.y );
		nodeAsNode->GenerateId();
		parentNode->AddChild( nodeAsNode );

		// add clicked node as new node child
		activeNode->AsCObject()->SetParent( newNode );
		nodeAsNode->AddChild( activeNode );

		// adjust position of the decorated node to preserve an existing layout
		activeNode->SetGraphPosition( activeNode->GetGraphPosX() - pos.x, activeNode->GetGraphPosY() - pos.y ); 

		CUndoSpawnTreeBlockDecorate::CreateStep( *GetUndoManager(), newNode, activeNode->AsCObject() );

		PostStructureModification();
	}
}

void CEdSpawnTreeEditor::CreateNode( CClass*const nodeClass, IEdSpawnTreeNode*const parentNode )
{
	PreStructureModification();

	ASSERT( parentNode && parentNode->IsPossibleChildClass( nodeClass, m_rootNode->GetSpawnTreeType() ) );

	CObject* newNode = CreateObject< CObject >( nodeClass, parentNode->AsCObject(), 0 );
	IEdSpawnTreeNode* nodeAsNode = dynamic_cast< IEdSpawnTreeNode* >( newNode );

	InsertNode( nodeAsNode, parentNode, m_dragged ? &m_draggedPos : nullptr );

	if ( nodeAsNode->IsHiddenByDefault() && nodeAsNode->CanBeHidden() && nodeAsNode->GetNumChildren() > 0 )
	{
		nodeAsNode->SetHidden( true );
	}

	PostStructureModification();
}

void CEdSpawnTreeEditor::OnRestoreDefaultLayout( wxCommandEvent& event )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();

	if ( !activeNodeProxy )
	{
		return;
	}

	PreStructureModification();

	AutoLayout( activeNodeProxy->GetEngineObject() );

	PostStructureModification();
}

void CEdSpawnTreeEditor::OnDeleteNode( wxCommandEvent& event )
{
	DeleteActiveNode();
}

void CEdSpawnTreeEditor::OnCopyNode( wxCommandEvent& event )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();

	if ( !activeNodeProxy )
	{
		return;
	}

	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	if ( !activeNode->CanBeCopied() )	
	{
		return;
	}

	CopyActiveNode();

	if ( m_clipboard )
	{
		while ( m_clipboard->GetNumChildren() ) 
		{
			m_clipboard->RemoveChild( m_clipboard->GetChild( m_clipboard->GetNumChildren() - 1 ) );
		}

		String friendlyName = m_clipboard->GetEditorFriendlyName();
		SEdPopupNotification::GetInstance().Show( this, TXT("COPY"), friendlyName );
	}
}

void CEdSpawnTreeEditor::OnCopyBranch( wxEvent& event )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();

	if ( !activeNodeProxy )
	{
		return;
	}

	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	if ( !activeNode->CanBeCopied() )	
	{
		return;
	}

	CopyActiveNode();

	if ( m_clipboard )
	{
		String friendlyName = m_clipboard->GetEditorFriendlyName();
		SEdPopupNotification::GetInstance().Show( this, TXT("COPY"), friendlyName );
	}
}

void CEdSpawnTreeEditor::OnCutBranch( wxEvent& event )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();

	if ( !activeNodeProxy || activeNodeProxy == m_rootNodeProxy || IsLocked( *activeNodeProxy ) )
	{
		return;
	}

	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	if ( !activeNode->CanBeCopied() )	
	{
		return;
	}

	CopyActiveNode();

	if ( m_clipboard )
	{
		String friendlyName = m_clipboard->GetEditorFriendlyName();
		DeleteActiveNode( TXT("cut ") + friendlyName );
		SEdPopupNotification::GetInstance().Show( this, TXT("CUT"), friendlyName );
	}
}

void CEdSpawnTreeEditor::OnPaste( wxEvent& event )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();

	if ( !m_clipboard || !activeNodeProxy )
	{
		return;
	}

	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	if ( !activeNode->CanAddChild() || !activeNode->IsPossibleChildClass( m_clipboard->AsCObject()->GetClass(), m_rootNode->GetSpawnTreeType() ) )
	{
		return;
	}

	ASSERT( m_clipboard != NULL );

	PreStructureModification();

	CObject* objectToPaste = m_clipboard->AsCObject()->Clone( activeNodeProxy->GetEngineObject() );
	IEdSpawnTreeNode* nodeToPaste = dynamic_cast< IEdSpawnTreeNode* >( objectToPaste );
	InsertNode( nodeToPaste, activeNode, nullptr, false );

	String friendlyName = nodeToPaste->GetEditorFriendlyName();
	SEdPopupNotification::GetInstance().Show( this, TXT("PASTE"), friendlyName );

	PreStructureModification();
}

void CEdSpawnTreeEditor::OnDebugOption( wxCommandEvent& event )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();
	if ( !activeNodeProxy )
	{
		return;
	}

	CSpawnTreeInstance* instance = activeNodeProxy->GetInstanceBuffer();
	if ( !instance )
	{
		return;
	}

	activeNodeProxy->GetSpawnTreeNode()->RunDebugOption( *instance, event.GetId() - ID_MENU_DEBUG_OPTION );

	ForceLayoutUpdate();
	Repaint();
}

void CEdSpawnTreeEditor::OnSpecialOption( wxCommandEvent& event )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();
	if ( !activeNodeProxy )
	{
		return;
	}

	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	PreStructureModification();

	activeNode->RunSpecialOption( event.GetId() - ID_MENU_SPECIAL_OPTION );

	PostStructureModification();
}
void CEdSpawnTreeEditor::OnShowAll( wxCommandEvent& event )
{
	wxMenu* menu = static_cast< wxMenu* >( event.GetEventObject() );

	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();
	ASSERT( activeNodeProxy != NULL );
	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	activeNode->SetHiddenRec( false );

	ForceLayoutUpdate();
	Repaint();
}
void CEdSpawnTreeEditor::OnDefaultVisibility( wxCommandEvent& event )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();
	ASSERT( activeNodeProxy != NULL );
	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	activeNode->SetDefaultVisibilityRec( false );

	ForceLayoutUpdate();
	Repaint();
}
void CEdSpawnTreeEditor::OnHideBranch( wxCommandEvent& event )
{
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();
	ASSERT( activeNodeProxy != NULL );
	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();

	activeNode->SetHidden( !activeNode->IsHidden() );

	ForceLayoutUpdate();
	Repaint();
}

void CEdSpawnTreeEditor::OnCheckOut( wxCommandEvent& event )
{
	CResource* res = Cast< CResource >( m_rootNode->AsCObject() );
	ASSERT( res != NULL );
	res->GetFile()->CheckOut();

	ForceLayoutUpdate();
	Repaint();
}

void CEdSpawnTreeEditor::OnRegenerateIds( wxCommandEvent& event )
{
	PreStructureModification();
	m_rootNode->GenerateIdsRecursively();
	PostStructureModification();
}

void CEdSpawnTreeEditor::OnGenerateId( wxCommandEvent& event )
{
	CEdSpawntreeNodeProxy* const activeNodeProxy = GetActiveNode();
	ASSERT( activeNodeProxy != nullptr );
	IEdSpawnTreeNode* const activeNode = activeNodeProxy->GetSpawnTreeNode();

	PreStructureModification();
	activeNode->GenerateId();
	PostStructureModification();

	SelectObject( activeNodeProxy, false, true );
	SelectObject( activeNodeProxy, true, true );
}

/*virtual*/
void CEdSpawnTreeEditor::DispatchEditorEvent( const CName& name, IEdEventData* data ) /*override*/
{
	if ( name == CNAME( EditorSpawnTreeResourceModified ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if ( IEdSpawnTreeNode* resAsNode = dynamic_cast< IEdSpawnTreeNode* >( res ) )
		{
			// check if changed resource is currently attached to our editor
			Bool isResEdited = false;
			for ( Int32 i = 0; i < resAsNode->GetNumChildren(); ++i )
			{
				TDynArray< CEdSpawntreeNodeProxy* > proxies;
				if ( FindNodeProxies( resAsNode->GetChild( i )->AsCObject(), proxies ) )
				{
					for ( auto it = proxies.Begin(), end = proxies.End(); it != end; ++it )
					{
						if ( (*it)->IsExternalNode() )
						{
							isResEdited = true;
							break;
						}
					}
					if ( isResEdited )
					{
						break;
					}
				}

			}

			if ( isResEdited )
			{
				// Force the tree to reflect an included file change
				ForceLayoutUpdate();
				Repaint();
			}
		}
	}
	else if ( name == CNAME( EditorPreUndoStep ) )
	{
		if ( GetEventData< CEdUndoManager* >( data ) == GetUndoManager() )
		{
			PreStructureModification();
		}
	}
	else if ( name == CNAME( EditorPostUndoStep ) )
	{
		if ( GetEventData< CEdUndoManager* >( data ) == GetUndoManager() )
		{
			PostStructureModification();
		}
	}
}

/*virtual*/ 
wxColour CEdSpawnTreeEditor::GetCanvasColor() const /*override*/
{
	return IsDebugMode() ? wxColour( 128, 32, 32 ) : wxColour( 48, 96, 48 );
}

/*virtual*/ 
void CEdSpawnTreeEditor::OnGizmoClicked( LayoutInfo* layout, GizmoLocation location ) /*override*/
{
	if ( location == GL_Bottom && layout->m_bottomGizmo == LayoutInfo::ELLIPSIS )
	{
		CEdSpawntreeNodeProxy* nodeProxy = Cast< CEdSpawntreeNodeProxy >( layout->m_owner );
		ASSERT ( nodeProxy != NULL );
		nodeProxy->GetSpawnTreeNode()->SetHidden( false );
		ForceLayoutUpdate();
		Repaint();
	}
}

/*virtual*/ 
void CEdSpawnTreeEditor::OnAddByDragging( wxPoint pos, GizmoLocation location ) /*override*/
{
	wxMenu menu;

	if ( location == CEdTreeEditor::GL_Top )
	{
		FillUpDecorateMenu( menu );
	}
	else
	{
		FillUpAddMenu( menu );
	}

	if ( menu.GetMenuItemCount() > 0 )
	{
		m_dragged = true;
		ASSERT( GetActiveItem() != NULL );
		m_draggedPos = pos;
		PopupMenu( &menu );
		m_dragged = false;
	}
}

void CEdSpawnTreeEditor::OnKeyDown( wxKeyEvent& event )
{
	if ( event.ControlDown() )
	{
		switch ( event.GetKeyCode() )
		{
		case 'C':
			OnCopyBranch( event );
			break;
		case 'X':
			OnCutBranch( event );
			break;
		case 'V':
			OnPaste( event );
			break;
		}
	}
}

void CEdSpawnTreeEditor::OnShowStats( wxCommandEvent& event )
{
	m_showStats = true;
	THashMap< IEdSpawnTreeNode*, Uint32 > TNodeMap;
	CEdSpawntreeNodeProxy* activeNodeProxy = GetActiveNode();
	ASSERT( activeNodeProxy != NULL );
	IEdSpawnTreeNode* activeNode = activeNodeProxy->GetSpawnTreeNode();
	UINT32 entCount = 0;
	if( activeNode )
	{
		SBudgetingStats stats;
		activeNode->GatherBudgetingStats( m_rootNode, stats );
		Uint32 tdata = 0;
		TDynArray< CBitmapTexture* > usedTextures;
		TPair<Float, Uint32> mPair;
		const Uint32 amount = stats.m_usedTemplates.Size();
		GFeedback->BeginTask( TXT("Checking Texture Costs"), false);
		GFeedback->UpdateTaskProgress( 0, amount );

		for ( Uint32 temp=0; temp < stats.m_usedTemplates.Size(); ++temp )
		{
			for ( Uint32 j = 0; j < stats.m_usedTemplates[temp].m_usedApperances.Size(); ++j )
			{
				// This is a list of all the texture data this appearance is using. This will later be sorted and then we only take the correct amount from this list
				TDynArray< Uint32 > textureData;
				TDynArray< THandle<CEntityTemplate> > incTemplate = stats.m_usedTemplates[temp].m_usedApperances[j]->GetIncludedTemplates();

				for ( Uint32 k = 0; k < incTemplate.Size(); ++k )
				{
					if ( !incTemplate[k] )
					{
						continue;
					}

					// For each instance we calculate the texture costs
					CEntity* ent = incTemplate[k]->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
					ASSERT( ent, TXT("Failed to create entity") );
					ent->CreateStreamedComponents( SWN_DoNotNotifyWorld );
					ent->ForceFinishAsyncResourceLoads();

					Uint32 usedTextureData = 0;
					TDynArray< CComponent* > comps = ent->GetComponents();
					for ( Uint32 i = 0; i < comps.Size(); ++i )
					{
						if ( comps[i]->IsA<CMeshTypeComponent>() )
						{
							CMeshTypeComponent* compMesh = Cast< CMeshTypeComponent >( comps[i] );
							CMeshTypeResource* meshResource = compMesh->GetMeshTypeResource();

							const CMeshTypeResource::TMaterials& materials = meshResource->GetMaterials();

							for ( Uint32 m=0; m<materials.Size(); m++ )
							{
								IMaterial* material = materials[m].Get();

								TDynArray< MeshTextureInfo* > usedTexturesInfo;
								MeshStatsNamespace::GatherTexturesUsedByMaterial( material, m, usedTexturesInfo );
								for ( Uint32 tex = 0; tex < usedTexturesInfo.Size(); ++tex )
								{
									usedTextures.PushBackUnique( usedTexturesInfo[tex]->m_texture );
								}
							}
						}
					}
					textureData.PushBack( usedTextureData );
					ent->Discard();
				}
			}
			entCount += stats.m_usedTemplates[temp].m_maxSpawnLimit;
			GFeedback->UpdateTaskProgress( temp, amount );
		}
		for ( Uint32 it = 0; it < usedTextures.Size(); ++it )
		{
			tdata += MeshStatsNamespace::CalcTextureDataSize( usedTextures[it] );
		}
		mPair.m_first = tdata / (1024.0f*1024.0f);
		mPair.m_second = entCount;
		m_nodeStatistics.Insert( activeNode, mPair );
		GFeedback->EndTask();
	}
}

void CEdSpawnTreeEditor::GetStats( IEdSpawnTreeNode* node, Uint32& count )
{
	// fill the layout of children nodes
	for ( Int32 i = 0; i < node->GetNumChildren(); ++i )
	{
		IEdSpawnTreeNode* child = node->GetChild(i);
		for ( Int32 j = 0; j < child->GetNumChildren(); ++j )
		{
			String name = child->GetChild(j)->GetEditorFriendlyName();
			name = name;
			CObject* obj = child->GetChild(j)->AsCObject();
			CName creatureDefinitionName;
			if ( obj )
			{
				if ( obj->IsA< CBaseCreatureEntry >() )
				{
					if ( obj->IsA< CCreatureEntry >() )
					{
						CCreatureEntry* entry = static_cast< CCreatureEntry* >( obj );
						creatureDefinitionName = entry->GetCreatureDefinitionName();

						count += 1;
					}
				}
			}	
		}
		GetStats( node->GetChild(i), count);
	}
}

void CEdSpawnTreeEditor::ClearNodeProxies()
{
	for ( Uint32 i = 0; i < m_usedProxies; ++i )
	{
		m_proxies[ i ]->Invalidate();
	}
	m_usedProxies = 0;
}
CEdSpawntreeNodeProxy* CEdSpawnTreeEditor::CreateNodeProxy( IEdSpawnTreeNode* obj, CEdSpawntreeNodeProxy* parentProxy )
{
	if ( m_usedProxies == m_proxies.Size() )
	{
		m_proxies.PushBack( CEdSpawntreeNodeProxy::GetStaticClass()->CreateObject< CEdSpawntreeNodeProxy >() );
	}

	Bool isExternal = false;
	if ( parentProxy && ( parentProxy->IsExternalNode() || parentProxy->GetEngineObject()->IsA< CSpawnTreeIncludeTreeNode >() ) )
	{
		isExternal = true;
	}

	CEdSpawntreeNodeProxy* instanceHolder =
		!parentProxy
		? nullptr
		: parentProxy->GetSpawnTreeNode()->HoldsInstanceBuffer() 
		? parentProxy
		: parentProxy->GetInstanceHolder();

	CEdSpawntreeNodeProxy* proxy = m_proxies[ m_usedProxies++ ];
	proxy->Initialize( obj, isExternal, instanceHolder );
	return proxy;
}
Bool CEdSpawnTreeEditor::FindNodeProxies( CObject* obj, TDynArray< CEdSpawntreeNodeProxy* >& outProxies )
{
	Bool b = false;
	for ( Uint32 i = 0; i < m_usedProxies; ++i )
	{
		CEdSpawntreeNodeProxy* proxy = m_proxies[ i ].Get();
		if ( proxy->GetEngineObject() == obj )
		{
			outProxies.PushBack( proxy );
			b = true;
		}
	}
	return b;
}
