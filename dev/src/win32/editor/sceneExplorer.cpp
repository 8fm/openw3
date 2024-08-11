/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneExplorer.h"

#include "../../common/core/codeParser.h"
#include "../../common/engine/pathlibNavmesh.h"
#include "../../common/engine/pathlibNavmeshComponent.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/collisionCache.h"
#include "../../common/engine/foliageEditionController.h"
#include "../../common/engine/grassMask.h"
#include "../../common/game/encounter.h"
#include "../../common/engine/swarmRenderComponent.h"
#include "../../common/core/gameConfiguration.h"
#include "../../common/game/boidAreaComponent.h"
#include "../../games/r4/flyingCritterslairEntity.h"
#include "../../common/core/exporter.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/feedback.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/engine/clipMap.h"
#include "../../common/engine/streamingSectorData.h"
#include "meshStats.h"
#include "../../common/engine/worldSceneDependencyInfo.h"
#include "../../common/game/boidActivationTriggerComponent.h"
#include "../../common/engine/appearanceComponent.h"

#include "cutsceneEntityControlPanel.h"
#include "emptyEntityCollector.h"
#include "lightEntityCollector.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/environmentComponentArea.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/entityGroup.h"
#include "../../common/engine/worldTick.h"
#include "../../common/engine/worldIterators.h"
#include "undoGroupingObjects.h"

#include "massActionDialog.h"
#include "entityList.h"
#include "missingTemplatesErrorsDisplayer.h"
#include "errorsListDlg.h"
#include "../../common/engine/layersEntityCompatibilityChecker.h"
#include "../../common/engine/lightComponent.h"
#include "checkListDlg.h"
#include "duplicatesFinder.h"
#include "../../games/r4/gameplayLightComponent.h"
#include "worldMassActions.h"

#define SCENEVIEW_ICON_SIZE			16
#define SCENEVIEW_MINIICON_SIZE		9
#define SCENEVIEW_MIAREA_SIZE		16
#define SCENEVIEW_HEADER_HEIGHT		16
#define SCENEVIEW_ROW_HEIGHT		16

#define ID_SCENEVIEW_STATUSMENU_CHECK_OUT	100
#define ID_SCENEVIEW_STATUSMENU_SUBMIT		101
#define ID_SCENEVIEW_STATUSMENU_REVERT		102
#define ID_SCENEVIEW_STATUSMENU_ADD			103
#define ID_SCENEVIEW_STATUSMENU_SYNC		104

#define ID_MENU_LOOK_AT_NODE				100
#define ID_MENU_MOVE_PLAYER_THERE			101
#define ID_MENU_ADD_LAYER_GROUP				102
#define ID_MENU_ADD_LAYER					103
#define ID_MENU_REMOVE_LAYER_GROUP			104
#define ID_MENU_REMOVE_LAYER				105
#define ID_MENU_LOAD_LAYER_GROUP			106
#define ID_MENU_UNLOAD_LAYER_GROUP			107
#define ID_MENU_SHOW_LAYER					108
#define ID_MENU_HIDE_LAYER					109
#define ID_MENU_ACTIVATE_LAYER				110
#define ID_MENU_LOAD_LAYER_GROUP_RECURSIVE	111
#define ID_MENU_SAVE_LAYER_GROUP			112
#define ID_MENU_SAVE_LAYER_GROUP_MODIFIED	113
#define ID_MENU_SAVE_LAYER					114
#define ID_MENU_LOAD_LAYER					115
#define ID_MENU_IMPORT_RESOURCE				116
#define ID_MENU_EXPORT_RESOURCE				117
#define ID_MENU_MEM_REPORT					120
#define ID_MENU_COPY_NODE					121
#define ID_MENU_CUT_NODE					122
#define ID_MENU_DELETE_NODE					123
#define ID_MENU_COPY_LAYER					124
#define ID_MENU_PASTE_LAYER					125
#define ID_MENU_ADD_LAYER_TO_PARENT			126
#define ID_MENU_PASTE_LAYER_TO_PARENT		127
#define ID_MENU_CUT_LAYER					128
#define ID_MENU_RENAME_LAYER_GROUP			129
#define ID_MENU_ACTIVATE_ENVIRONMENT		140
#define ID_MENU_EDIT_ENVIRONMENT			141
#define ID_MENU_ADD_SHADOWS					144
#define ID_MENU_REMOVE_SHADOWS				145
#define ID_MENU_ADD_SHADOWS_L				146
#define ID_MENU_REMOVE_SHADOWS_L			147
#define ID_MENU_CONVERT_LAYER				148
#define ID_MENU_CONVERT_GROUP				149
#define ID_MENU_ADD_SHADOWS_LOCAL_LIGHTS_L	150
#define ID_MENU_ADD_SHADOWS_LOCAL_LIGHTS	151

#define ID_MENU_SELECT_ALL_OBJECTS			160
#define ID_MENU_SELECT_ALL					161
#define ID_MENU_SELECT_ALL_IN_ENV			162

#define ID_MENU_SAVE_ITEM_ONLY				169
#define ID_UNGROUP_ITEMS					170
#define ID_MENU_GENERATE_NAVMESH			171
#define ID_MENU_COMPUTE_NAVI_BOUNDS			172
#define ID_MENU_GENERATE_NAVMESH_RECURSIVE	173
#define ID_MENU_STOP_NAVMESH_GENERATION		174
#define ID_MENU_GENERATE_NAVGRAPH			175
#define ID_MENU_RESET_NAVMESH_PARAMS		176
#define ID_MENU_REMOVE_EMPTY				177
#define ID_MENU_VALIDATE_STREAMING_TILES	178
#define ID_MENU_COLLECT_EMPTY_ENTITIES		179
#define ID_MENU_ENCOUNTER_EDIT				180
#define ID_MENU_BUILD_STEAMING_DATA			181
#define ID_MENU_GENERATE_RESOURCE_GRAPH		182
#define ID_MENU_CONVERT_STREAMING_DATA		183
#define ID_MENU_SET_GROUP_NONSTATIC			184
#define ID_MENU_SET_GROUP_STATIC			185
#define ID_MENU_INSPECT_OBJECT				186
#define ID_MENU_RUN_WORLD_ANALYSIS			187
#define ID_MENU_BUNDLE_WORLD_DATA			188
#define ID_MENU_GENERATE_QUEST_METADATA		189
#define ID_MENU_BUNDLE_QUEST_DATA			190
#define ID_MENU_CONVERT_NAVIGATION_DATA		191
#define ID_MENU_SAVE_BACKEND_DATABASE		192
#define ID_MENU_GENERATE_SWARM_COLLISIONS	193
#define ID_MENU_COLLECT_LIGHT_ENTITIES		194
#define ID_MENU_RUN_LAYER_ANALYSIS			195
#define ID_MENU_BUNDLE_LAYER_DATA			196
#define ID_MENU_GENERATE_STREAMING_FILES	197
#define ID_MENU_RESET_INSTANCE_PROPERTIES	198
#define ID_MENU_FIX_SWARM_LAIR_COMPONENTS	199

#define ID_MENU_PRESETACTION_SAVE			200
#define ID_MENU_PRESETACTION_RENAME			201
#define ID_MENU_PRESETACTION_DELETE			202
#define ID_MENU_PRESETACTION_COPY			203
#define ID_MENU_PRESETACTION_PASTE			204
#define ID_MENU_CONVERTLAYERTO				205
#define ID_MENU_MASS_ACTIONS				206
#define ID_MENU_ADD_TO_ENTITY_LIST			207
#define ID_MENU_LOAD_ENTITY_LIST			208
#define ID_MENU_CHECK_DUPLICATES			209
#define ID_MENU_CHECK_DUPLICATE_IDS			210

#define ID_MENU_SHOW_STREAMING_INFO			220
#define ID_MENU_FORCE_STREAM_IN				221
#define ID_MENU_FORCE_STREAM_OUT			222
#define ID_MENU_IGNORE_STREAM				223
#define ID_MENU_UNIGNORE_STREAM				224
#define ID_MENU_CLEAR_IGNORE_LIST			225
#define ID_MENU_SHOW_IGNORED_ENTITIES		226

#define ID_MENU_FIX_ALL_SWARM_LAIRS			227
#define ID_MENU_COMPATIBILITY_REPORT		228

#define ID_MENU_DISPLAY_LBTCOLORS			500
#define ID_MENU_NO_SORTING					501
#define ID_MENU_ALPHABETIC_SORTING			502
#define ID_MENU_REMOVE_OBJECT_FILTERS		503

#define ID_GROUP_ITEMS						405
#define ID_LOCK_GROUP						406
#define ID_UNLOCK_GROUP						407
#define ID_REMOVE_FROM_GROUP				408


#define ID_MENU_LAYER_CHECK_OUT				2000
#define ID_MENU_LAYER_SUBMIT				2001
#define ID_MENU_LAYER_REVERT				2002
#define ID_MENU_LAYER_ADD					2003
#define ID_MENU_LAYER_DELETE				2004
#define ID_MENU_LAYER_SYNC					2005

#define ID_MENU_LAYER_RENAME				2100
#define ID_MENU_LAYER_COPY_PATH				2101

#define ID_MENU_GROUP_CHECK_OUT				3000
#define ID_MENU_GROUP_SUBMIT				3001
#define ID_MENU_GROUP_REVERT				3002
#define ID_MENU_GROUP_ADD					3003
#define ID_MENU_GROUP_DELETE				3004
#define ID_MENU_GROUP_SYNC					3005
#define ID_MENU_GROUP_EDIT					3006
#define ID_MENU_GROUP_REMOVE_LGFILES		3007
#define ID_MENU_SAVE_DEPENDENCY_FILE		3008
#define ID_MENU_SAVE_FOLIAGE_DEPENDENCY_FILE 3009

#define ID_MENU_CS_ENT_SHOW					4000
#define ID_MENU_CS_ENT_PLAY					4001
#define ID_MENU_CS_ENT_STOP					4002
#define ID_MENU_CS_ENT_PANEL				4003

#define ID_MENU_REMOVE_FORCE_NO_AUTOHIDE		299
#define ID_MENU_RESET_LIGHTCHANNELS			300

#define ID_MOVE_CONTENT_TO_LAYER			301
#define ID_MERGE_LAYERS						302
#define ID_DETACH_TEMPLATES					303

#define ID_SHOW_RESOURCE_NODE				5000
#define ID_SELECT_RESOURCE_NODE				6000

#define COLUMN_NAME				0
#define COLUMN_STATUS			1
#define COLUMN_CLASS			2
#define COLUMN_TAGS				3
#define COLUMN_SYSTEM_MEMORY	4
#define COLUMN_VIDEO_MEMORY		5

BEGIN_EVENT_TABLE( CEdSceneExplorer, wxPanel )

EVT_COMBOBOX( XRCID("presetComboBox"), CEdSceneExplorer::OnPresetChanged )
EVT_TEXT_ENTER( XRCID("presetComboBox"), CEdSceneExplorer::OnSavePresets )

EVT_BUTTON( XRCID("SavePresets"), CEdSceneExplorer::OnSavePresets )
EVT_BUTTON( XRCID("PresetActions"), CEdSceneExplorer::OnPresetActions )
EVT_BUTTON( XRCID("DisplayOptions"), CEdSceneExplorer::OnDisplayOptions )

EVT_TEXT( XRCID("FilterBox"), CEdSceneExplorer::OnFilterBoxText )

EVT_MENU( ID_MENU_SHOW_LAYER, CEdSceneExplorer::OnLayerShow )
EVT_MENU( ID_MENU_HIDE_LAYER, CEdSceneExplorer::OnLayerHide )

EVT_MENU( ID_MOVE_CONTENT_TO_LAYER, CEdSceneExplorer::OnMoveContentToLayer )
EVT_MENU( ID_MERGE_LAYERS, CEdSceneExplorer::OnMergeLayers )

EVT_MENU( ID_MENU_LAYER_CHECK_OUT, CEdSceneExplorer::OnLayerCheckOut )
EVT_MENU( ID_MENU_LAYER_SUBMIT, CEdSceneExplorer::OnLayerSubmit )
EVT_MENU( ID_MENU_LAYER_REVERT, CEdSceneExplorer::OnLayerRevert )
EVT_MENU( ID_MENU_LAYER_ADD, CEdSceneExplorer::OnLayerAdd )
EVT_MENU( ID_MENU_LAYER_SYNC, CEdSceneExplorer::OnLayerSync )

EVT_MENU( ID_MENU_GROUP_CHECK_OUT, CEdSceneExplorer::OnGroupCheckOut )
EVT_MENU( ID_MENU_GROUP_SUBMIT, CEdSceneExplorer::OnGroupSubmit )
EVT_MENU( ID_MENU_GROUP_REVERT, CEdSceneExplorer::OnGroupRevert )
EVT_MENU( ID_MENU_GROUP_ADD, CEdSceneExplorer::OnGroupAdd )
EVT_MENU( ID_MENU_GROUP_SYNC, CEdSceneExplorer::OnGroupSync )
EVT_MENU( ID_MENU_GROUP_REMOVE_LGFILES, CEdSceneExplorer::OnWorldDeleteLayerGroupFiles )

EVT_MENU( ID_MENU_PRESETACTION_SAVE, CEdSceneExplorer::OnSavePresets )
EVT_MENU( ID_MENU_PRESETACTION_RENAME, CEdSceneExplorer::OnRenamePreset )
EVT_MENU( ID_MENU_PRESETACTION_DELETE, CEdSceneExplorer::OnDeletePreset )
EVT_MENU( ID_MENU_PRESETACTION_COPY, CEdSceneExplorer::OnCopyPreset )
EVT_MENU( ID_MENU_PRESETACTION_PASTE, CEdSceneExplorer::OnPastePreset )

EVT_MENU( ID_MENU_DISPLAY_LBTCOLORS, CEdSceneExplorer::OnDisplayLBTColors )
EVT_MENU( ID_MENU_NO_SORTING, CEdSceneExplorer::OnNoSorting )
EVT_MENU( ID_MENU_ALPHABETIC_SORTING, CEdSceneExplorer::OnAlphabeticSorting )
EVT_MENU( ID_MENU_REMOVE_OBJECT_FILTERS, CEdSceneExplorer::OnRemoveObjectFilters )

EVT_MOUSE_CAPTURE_LOST( CEdSceneExplorer::OnMouseCaptureLost )
END_EVENT_TABLE()

// Flag for drawing layer backgrounds - when a proper SceneViewer for layerinfos
// is written (instead of the big fat ugly class below), this should be moved
// there.  For now, it needs to be accessible from the event handler without
// being exposed to the rest of the editor, so i'm leaving it here.
static Bool DrawLBTBackgrounds = false;

//! This structure contains the state for painting an object viewer in the
//! scene viewer.  It is passed by the scene view to ISceneObjectViewer
//! subclasses.
struct SSceneObjectPaintState
{
	wxRect			m_rect;			//!< Rectangle to paint into in dc's coordinates
	Int32			m_depth;		//!< Object's depth (0=root, 1=direct child, 2=grandchild, etc)
	ISerializable*	m_object;		//!< The object itself
	Bool			m_selected;		//!< True if the object is selected
	Bool			m_activated;	//!< True if the object is activated
	Bool			m_expanded;		//!< True if the object is expanded
	wxString		m_filter;		//!< Filter text
};

//! Interface for object viewers in the scene view.  This is used to paint
//! and handle object views.  Note: DO NOT and i repeat DO NOT store
//! state about objects here (or anywhere related to views).  A viewer is
//! not supposed to know anything about the object's lifetime nor modify
//! the object itself.  All it is expected to do is simply draw the object
//! in a meaningful way in the tree hierarchy.  Nothing more.  Do not save
//! state in the viewer or you (and possibly i) will regret it down the road.
//! It is not "smart" to shove functionality where functionality is not
//! expected to be shoved in (and this can be extended to more than
//! functionality actually - follow the protocols people).            -bs
class ISceneObjectViewer
{
	friend class SceneView;

protected:
	//! Called to paint the element in the passed DC using the passed state
	//! under the main (Name) header
	virtual void Paint( wxDC& dc, const SSceneObjectPaintState& state ) const=0;

	//! Called to obtain the heights for the shrunk and expanded states of
	//! the passed object (should be the same for all objects of the same class)
	//! in the passed DC
	virtual void GetHeights( wxDC& dc, ISerializable* obj, Int32& shrunkHeight, Int32& expandedHeight ) const=0;

	//! Called to return a bitmap to use at the left side of the node
	virtual wxBitmap GetBitmap( ISerializable* obj ) const=0;

	//! Called to return an optional bitmap overlay
	virtual Bool GetBitmapOverlay( ISerializable* obj, wxBitmap& bmp ) const=0;

	//! Called to get the visible name of the object
	virtual wxString GetVisibleName( ISerializable* obj ) const=0;

	//! Called to check if the passed object has a toggleable visibility
	//! and, thus, will respond to IsVisible and SetVisible calls below
	virtual Bool ToggleableVisibility( ISerializable* obj ) const=0;

	//! Called to check if the passed object is visible or not
	virtual Bool IsVisible( ISerializable* obj ) const=0;

	//! Called to set the visibility of the passed object
	virtual void SetVisible( ISerializable* obj, Bool visible ) const=0;

	//! Called to check if the object is expandable or not.  If an object is expandable
	//! it is expected that GetChildren() will provide at least a child object
	virtual Bool IsExpandable( ISerializable* obj ) const=0;

	//! Called to obtain the viewable children of this object.  Keep in mind that this
	//! is called every single time the view is being painted so make sure this doesn't
	//! take too long
	virtual void GetChildren( ISerializable* obj, TDynArray< ISerializable* >& children ) const=0;

	//! Called to obtain the disk file where this object/resource is saved.  It is
	//! expected to return NULL if the object has no associated file
	virtual CDiskFile* GetFile( ISerializable* obj ) const=0;

	//! Called to populate the passed menu with menu items relevant to the passed objects
	virtual void PopulatePopupMenu( const THashSet<ISerializable*>& objects, wxMenu& menu ) const=0;

	//! Called to activate the passed object (usually done when the user double clicks on it)
	virtual void Activate( ISerializable* obj ) const=0;

	//! Called to check if this viewer supports the passed object - return True if so
	virtual Bool Supports( ISerializable* obj ) const=0;
public:
	virtual ~ISceneObjectViewer(){};
};

// SceneView
class SceneView : public wxScrolled<wxWindow>
{
	enum HeaderID {
		HID_NAME,
		HID_VISIBLE,
		HID_STATUS,
		HID_CLASS,
		//
		HID_COUNT
	};

	// A header
	struct Header
	{
		wxString					m_caption;
		wxBitmap					m_icon;
		Int32						m_pos;
		Int32						m_size;	

		Header(){}
		Header( const wxString& caption, const wxBitmap& icon, Int32 pos ) : m_caption( caption ), m_icon( icon ), m_pos( pos ) {}
	};

	// A single painted element
	struct Element
	{
		wxRect						m_rect;
		THandle< ISerializable >	m_object;
		Int32						m_depth;
	};

	TDynArray< Element* >							m_elements;
	TDynArray< ISceneObjectViewer* >				m_viewers;
	THashSet< ISerializable* >						m_selected;
	ISerializable*									m_singleClickSelection;
	THashSet< ISerializable* >						m_expanded;
	THashSet< ISerializable* >						m_filteredOut;
	Bool											m_worldExpanded;
	Header											m_headers[HID_COUNT];
	HeaderID										m_movingHeader;
	Int32											m_mousex, m_mousey;
	Int32											m_firstClickedItemIndex;

	wxBitmap										m_expandIcon;
	wxBitmap										m_collapseIcon;
	wxBitmap										m_visibleIcon;
	wxBitmap										m_invisibleIcon;
	wxBitmap										m_localIcon;
	wxBitmap										m_deletedIcon;
	wxBitmap										m_checkedInIcon;
	wxBitmap										m_checkedOutIcon;
	wxBitmap										m_saveIcon;

	ISerializable*									m_menuObject;
	wxString										m_filterString;
	
	struct {
		Int32 depth, offset;
		Int32 x, y, width;
		TDynArray< Bool > last;
		wxSize size;
		Bool noclip;
	} m_paintState;

	void InitializeViewers();
	ISceneObjectViewer* FindViewer( ISerializable* obj );

	void ClearElements();
	Element* GetElementAt( const wxPoint& point ) const;
	Element* GetElementByObject( ISerializable* obj );
	HeaderID GetHeaderAt( const wxPoint& point ) const;
	Bool GetElementIndex( Element* element, Int32& index ) const;

	void CalcHeaderSizes( wxDC& dc );
	void PaintHeaders( wxDC& dc );
	void PaintObject( wxDC& dc, ISerializable* obj );
	void PaintScene( wxDC& dc );

	void ExpandObject( ISerializable* obj, Bool expand );
	Bool IsExpanded( ISerializable* obj );

	void ShowStatusPopupForObject( ISerializable* obj );

	Bool SetMultiSelection( ISerializable* startNode, ISerializable* endNode, ISerializable* currentNode );
	void SortChildren( TDynArray< ISerializable* >& children ) const;

protected:
	void OnPaint( wxPaintEvent& event );
	void OnLeftDown( wxMouseEvent& event );
	void OnLeftUp( wxMouseEvent& event );
	void OnLeftDClick( wxMouseEvent& event );
	void OnRightDown( wxMouseEvent& event );
	void OnMouseMove( wxMouseEvent& event );
	void OnScroll( wxScrollEvent& event );
	void OnCaptureLost( wxMouseCaptureLostEvent& event );
	void OnKeyDown( wxKeyEvent& event );

	void OnStatusMenuCheckOut( wxCommandEvent &event );
	void OnStatusMenuSubmit( wxCommandEvent &event );
	void OnStatusMenuRevert( wxCommandEvent &event );
	void OnStatusMenuAdd( wxCommandEvent &event );
	void OnStatusMenuSync( wxCommandEvent &event );

	void PopulatePopupMenu( wxMenu& menu );

public:
	SceneView( wxWindow *parent, wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL|wxBORDER_THEME, const wxString& name=wxPanelNameStr );
	virtual ~SceneView();

	void ResetState();

	ISerializable* GetObjectAt( const wxPoint& point ) const;

	void ClearSelection();
	void Select( ISerializable* obj );
	void Deselect( ISerializable* obj );
	Bool IsSelected( ISerializable* obj ) const;
	void ToggleSelection( ISerializable* obj );

	RED_INLINE const THashSet<ISerializable*>& GetSelection() { return m_selected; }
	template < typename T >
	void GetSelection( TDynArray<T*>& selection )
	{
		for ( auto it=m_selected.Begin(); it != m_selected.End(); ++it )
		{
			ISerializable* obj = *it;
			if ( m_filteredOut.Exist( obj ) )
			{
				continue;
			}
			if ( obj->GetClass()->IsA( T::GetStaticClass() ) )
			{
				selection.PushBack( static_cast<T*>( obj ) );
			}
		}
	}
	
	// Like GetSelection<CLayerGroup>, except that it also includes CWorld's root
	void GetSelectedLayerGroups( TDynArray<CLayerGroup*>& layerGroups );

	// Collects all selected entities and all entities in selected layers and groups
	void CollectEntitiesFromSelection( TDynArray<CEntity*>& entities );

	void ExpandToSelection();
	void ScrollToSelection();
	ISerializable* GetFirstSelectedObject();
	Bool CalculateFirstSelectedElementOffset( ISerializable* currentNode, ISerializable* firstSelectedObject, Int32& offset );

	void Activate( ISerializable* obj );
	
	RED_INLINE CLayerInfo* GetActiveLayer() const;
	void SetActiveLayer( CLayerInfo* layer );

	void SetFilterString( const wxString& filterString );
	void ClearFilterString();

	void RefreshLater();
	void UpdateWorldSelection();
	void UpdateWorldSelectionLater();
};

void SceneView::InitializeViewers()
{

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                                  // //
	// IMPORTANT: NOTE: BIG WARNING: KEEP IN MIND: ETC: This will be broken to different classes later  // //
	//													and other editor parts will be able to register // //
	//													viewers and menu items independently instead of // //
	//													having everything shoved under a single class   // //
	//																						-badsector  // //
	//																						            // //
	////////////////////////////////////////////////////////////////////////////////////////////////////// //
	  /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// ///

	static struct : public ISceneObjectViewer {

		virtual void Paint( wxDC& dc, const SSceneObjectPaintState& state ) const
		{
			// Configure font
			wxFont previousFont = dc.GetFont();
			Bool restoreFont = false;
			if ( state.m_activated )
			{
				dc.SetFont( dc.GetFont().Bold() );
				restoreFont = true;
			}
			if ( state.m_object->IsA< CLayerGroup >() && !static_cast<CLayerGroup*>( state.m_object )->IsVisibleOnStart() )
			{
				dc.SetFont( dc.GetFont().Italic() );
				restoreFont = true;
			}

			// Get name
			wxString name = GetVisibleName( state.m_object );
			wxSize nameSize = dc.GetTextExtent( name );

			// Special case: layer background color
			if ( state.m_object->IsA< CLayerInfo >() ) 
			{
				CLayerInfo* layerInfo = static_cast<CLayerInfo*>( state.m_object );

				// Draw a background if the flag is set or if the layer has no layer build tag
				if ( DrawLBTBackgrounds || layerInfo->GetLayerBuildTag() == LBT_None )
				{
					Color layerColor = LayerBuildTagColors::GetColorFor( layerInfo->GetLayerBuildTag() );
					wxColour bgColour( layerColor.R, layerColor.G, layerColor.B );
					dc.SetBrush( wxBrush( bgColour ) );
					dc.SetPen( wxPen( bgColour, 1, wxPENSTYLE_SOLID ) );
					dc.DrawRectangle( state.m_rect );
				}
			}

			// If we have a filterstring, highlight the filter part
			if ( !state.m_filter.IsEmpty() )
			{
				int offset = name.Find( state.m_filter );
				if ( offset != -1 )
				{
					int offsetX = dc.GetTextExtent( name.SubString( 0, offset - 1 ) ).GetWidth();
					int highlightWidth = dc.GetTextExtent( state.m_filter ).GetWidth();
					wxRect rect = state.m_rect;
					rect.x += offsetX;
					rect.width = highlightWidth + 1;
					dc.SetBrush( wxBrush( wxColour( 221, 250, 107 ) ) );
					dc.SetPen( wxPen( wxColour( 173, 215, 9 ), 1, wxPENSTYLE_SOLID ) );
					dc.DrawRectangle( rect );
				}
			}

			// Paint background
			if ( state.m_selected )
			{
				wxColour& bgColour = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
				wxRect rect = state.m_rect;
				rect.width = nameSize.GetWidth();
				dc.SetBrush( wxBrush( bgColour ) );
				dc.SetPen( wxPen( bgColour, 1, wxPENSTYLE_SOLID ) );
				dc.DrawRectangle( rect );
			}

			// Paint column Name
			wxPoint pos = state.m_rect.GetTopLeft();
			dc.SetTextForeground( wxSystemSettings::GetColour( state.m_selected ? wxSYS_COLOUR_HIGHLIGHTTEXT : wxSYS_COLOUR_WINDOWTEXT ) );
			dc.DrawText( name, wxPoint( pos.x, pos.y + ( state.m_rect.GetHeight() - nameSize.GetHeight() ) / 2 ) );

			// Restore font
			if ( restoreFont )
			{
				dc.SetFont( previousFont );
			}
		}

		virtual void GetHeights( wxDC& dc, ISerializable* obj, Int32& shrunkHeight, Int32& expandedHeight ) const override
		{
			wxString name = GetVisibleName( obj );
			wxSize extent = dc.GetTextExtent( name );
			shrunkHeight = expandedHeight = Max<Int32>( extent.GetHeight(), SCENEVIEW_ICON_SIZE );
		}

		virtual wxBitmap GetBitmap( ISerializable* obj ) const override
		{
			if ( obj->IsA< CWorld >() )
			{
				return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_WORLD16") );
			}
			else if ( obj->IsA< CLayerGroup >() )
			{
				const CLayerGroup* group = static_cast<const CLayerGroup*>( obj );
				if ( group->IsVisibleOnStart() )
				{
					if ( group->IsFullyUnloaded() )
					{
						return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DIR_CLOSED_NOTLOADED") );
					}
					else
					{
						if ( group->IsLoaded( true ) )
						{
							return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DIR_CLOSED") );
						}
						else
						{
							return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DIR_CLOSED_PARTLYLOADED") );
						}
					}
				}
				else
				{
					if ( group->IsFullyUnloaded() )
					{
						return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DIR_CLOSED_NOTLOADED_HIDDEN") );
					}
					else
					{
						if ( group->IsLoaded( true ) )
						{
							return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DIR_CLOSED_HIDDEN") );
						}
						else
						{
							return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DIR_CLOSED_PARTLYLOADED_HIDDEN") );
						}
					}
				}
			}
			else if ( obj->IsA< CLayerInfo >() )
			{
				const CLayerInfo* layerInfo = static_cast<const CLayerInfo*>( obj );
				if ( layerInfo->IsLoaded() )
				{
					return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LAYER") );
				}
				else
				{
					return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LAYER_BW") );
				}
			}
			else if ( obj->IsA< CComponent >() )
			{
				return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_BRICKS") );
			}
			else if ( obj->IsA< CEntityGroup >() )
			{
				return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ENTITY_GROUP") );
			}
			else if ( obj->IsA< CEntity >() )
			{
				CEntity* entity = static_cast<CEntity*>( obj );
				if ( entity->ShouldBeStreamed() )
				{
					if ( entity->IsStreamedIn() )
					{
						return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ENTITY_STREAMED") );
					}
					else
					{
						return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ENTITY_STREAMED_UNLOADED") );
					}
				}
				else
				{
					return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ENTITY") );
				}
			}

			return SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ENTITY") );
		}

		virtual Bool GetBitmapOverlay( ISerializable* obj, wxBitmap& bmp ) const override
		{
			if ( obj->IsA< CLayerGroup >() && static_cast< CLayerGroup* >( obj )->IsDLC() )
			{
				bmp = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DLC") );
				return true;
			}
			else if ( obj->IsA< CLayerInfo >() && static_cast< CLayerInfo* >( obj )->GetLayerGroup()->IsDLC() )
			{
				bmp = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DLC") );
				return true;
			}
			return false;
		}

		virtual wxString GetVisibleName( ISerializable* obj ) const override
		{
			if ( obj->IsA< CWorld >() )
			{
				CFilePath path( static_cast<const CWorld*>( obj )->GetDepotPath() );
				return path.GetFileName().AsChar();
			}
			else if ( obj->IsA< CLayerGroup >() )
			{
				return static_cast<const CLayerGroup*>( obj )->GetName().AsChar();
			}
			else if ( obj->IsA< CLayerInfo >() )
			{
				return static_cast<const CLayerInfo*>( obj )->GetShortName().AsChar();
			}
			else if ( obj->IsA< CNode >() )
			{
				return static_cast<const CNode*>( obj )->GetName().AsChar();
			}
			else if ( obj->IsA< CObject >() )
			{
				return static_cast<const CObject*>( obj )->GetFriendlyName().AsChar();
			}
			else
			{
				return obj->GetClass()->GetName().AsChar();
			}
		}

		virtual Bool ToggleableVisibility( ISerializable* obj ) const override
		{
			return obj->IsA<CLayerInfo>();
		}

		virtual Bool IsVisible( ISerializable* obj ) const override
		{
			if ( obj->IsA<CLayerInfo>() )
			{
				return static_cast<CLayerInfo*>( obj )->IsVisible();
			}
			return true;
		}

		virtual void SetVisible( ISerializable* obj, Bool visible ) const override
		{
			if ( obj->IsA<CLayerInfo>() )
			{
				THashSet< ISerializable* > saveSelected( wxTheFrame->GetSceneExplorer()->m_sceneView->m_selected );
				wxCommandEvent cmd;
				wxTheFrame->GetSceneExplorer()->m_sceneView->m_selected.Clear();
				wxTheFrame->GetSceneExplorer()->m_sceneView->m_selected.Insert( obj );
				if ( visible )
				{
					wxTheFrame->GetSceneExplorer()->OnLayerShow( cmd );
				}
				else
				{
					wxTheFrame->GetSceneExplorer()->OnLayerHide( cmd );
				}
				wxTheFrame->GetSceneExplorer()->m_sceneView->m_selected = saveSelected;
			}
		}

		virtual Bool IsExpandable( ISerializable* obj ) const override
		{
			if ( obj->IsA< CWorld >() )
			{
				return IsExpandable( static_cast<const CWorld*>( obj )->GetWorldLayers() );
			}
			else if ( obj->IsA< CLayerGroup >() )
			{
				const CLayerGroup* group = static_cast<const CLayerGroup*>( obj );
				const CLayerGroup::TGroupList& subgroups = group->GetSubGroups();
				const CLayerGroup::TLayerList& layers = group->GetLayers();

				return subgroups.Size() > 0 || layers.Size() > 0;
			}
			else if ( obj->IsA< CLayerInfo >() )
			{
				const CLayerInfo* layerInfo = static_cast<const CLayerInfo*>( obj );
				CLayer* layer = layerInfo->IsLoaded() ? layerInfo->GetLayer() : NULL;

				return layer && layer->GetEntities().Size() > 0;
			}
			else if ( obj->IsA< CEntityGroup >() )
			{
				return !static_cast<CEntityGroup*>( obj )->GetEntities().Empty();
			}
			else if ( obj->IsA< CEntity >() )
			{
				return static_cast<CEntity*>( obj )->GetComponents().Size() > 0;
			}

			return false;
		}

		virtual void GetChildren( ISerializable* obj, TDynArray< ISerializable* >& children ) const override
		{
			if ( obj->IsA< CWorld >() )
			{
				GetChildren( static_cast<const CWorld*>( obj )->GetWorldLayers(), children );
			}
			else if ( obj->IsA< CLayerGroup >() )
			{
				const CLayerGroup* group = static_cast<const CLayerGroup*>( obj );
				const CLayerGroup::TGroupList& subgroups = group->GetSubGroups();
				const CLayerGroup::TLayerList& layers = group->GetLayers();

				for ( auto it=subgroups.Begin(); it != subgroups.End(); ++it )
				{
					children.PushBack( *it );
				}

				for ( auto it=layers.Begin(); it != layers.End(); ++it )
				{
					children.PushBack( *it );
				}
			}
			else if ( obj->IsA< CLayerInfo >() )
			{
				const CLayerInfo* layerInfo = static_cast<const CLayerInfo*>( obj );
				CLayer* layer = layerInfo->IsLoaded() ? layerInfo->GetLayer() : NULL;
				if ( layer )
				{
					const LayerEntitiesArray& entities = layer->GetEntities();
					for ( auto it=entities.Begin(); it != entities.End(); ++it )
					{
						if ( !(*it)->GetPartOfAGroup() )
						{
							children.PushBack( *it );
						}
					}
				}
			}
			else if ( obj->IsA< CEntityGroup >() )
			{
				CEntityGroup* group = static_cast<CEntityGroup*>( obj );
				const TDynArray<CEntity*>& entities = group->GetEntities();
				
				for ( auto it=entities.Begin(); it != entities.End(); ++it )
				{
					children.PushBack( *it );
				}
			}
			else if ( obj->IsA< CEntity >() )
			{
				CEntity* entity = static_cast<CEntity*>( obj );
				const TDynArray<CComponent*>& components = entity->GetComponents();

				for ( auto it=components.Begin(); it != components.End(); ++it )
				{
					children.PushBack( *it );
				}
			}
		}

		virtual CDiskFile* GetFile( ISerializable* obj ) const override
		{
			if ( ( obj->IsA<CResource>() && static_cast<CResource*>( obj )->GetFile() ) || obj->IsA<CLayerInfo>() && static_cast<CLayerInfo*>( obj )->IsLoaded() )
			{
				// Find the relevant file
				CLayerInfo* layerInfo = Cast<CLayerInfo>( obj );
				CDiskFile* file = layerInfo ? layerInfo->GetLayer()->GetFile() : static_cast<CResource*>( obj )->GetFile();
				return file;
			}
			return NULL;
		}

		virtual void PopulatePopupMenu( const THashSet<ISerializable*>& objects, wxMenu& menu ) const override
		{
			CEdSceneExplorer* explorer = wxTheFrame->GetSceneExplorer();

			Bool addedAny = false;
			Bool addedLayerGroup = false;
			Bool addedLayerInfo = false;
			Bool addedResource = false;
			Bool addedEntity = false;

#define POPULATEPOPUPMENU_CLASS_CHECK(flag) \
			if ( flag ) \
			{ \
				continue; \
			} \
			flag = true; \
			if ( !addedAny ) \
			{ \
				addedAny = true; \
			} \
			else \
			{ \
				menu.AppendSeparator(); \
			} 

			for ( auto it=objects.Begin(); it != objects.End(); ++it )
			{
				ISerializable* object = *it;

				// We don't really care about the world itself, but about its layers
				if ( object->IsA< CWorld >() )
				{
					object = static_cast<CWorld*>( object )->GetWorldLayers();

					menu.Append( ID_MENU_GROUP_REMOVE_LGFILES, TXT( "Remove layer group files" ) );
					menu.Connect( ID_MENU_GROUP_REMOVE_LGFILES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnWorldDeleteLayerGroupFiles ), NULL, explorer );
					menu.AppendSeparator();

					menu.Append( ID_MENU_SAVE_DEPENDENCY_FILE, TXT( "Save dependency file" ) );
					menu.Connect( ID_MENU_SAVE_DEPENDENCY_FILE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnWorldSaveDependencyFile ), NULL, explorer );
					menu.Append( ID_MENU_SAVE_FOLIAGE_DEPENDENCY_FILE, TXT( "Save Foliage dependency file" ) );
					menu.Connect( ID_MENU_SAVE_FOLIAGE_DEPENDENCY_FILE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnWorldSaveDependencyFile ), NULL, explorer );
					menu.AppendSeparator();

					menu.Append( ID_MENU_LOAD_ENTITY_LIST, TXT( "Load entity list..." ) );
					menu.Connect( ID_MENU_LOAD_ENTITY_LIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLoadEntityList ), NULL, explorer );
					menu.AppendSeparator();

					menu.Append( ID_MENU_FIX_ALL_SWARM_LAIRS, TXT( "Fix all swarm lairs" ) );
					menu.Connect( ID_MENU_FIX_ALL_SWARM_LAIRS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnFixAllSwarmLairs ), NULL, explorer );

					menu.Append( ID_MENU_COMPATIBILITY_REPORT, TXT( "Generate compatibility errors report" ) );
					menu.Connect( ID_MENU_COMPATIBILITY_REPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnGenerateCompatibilityErrorsReport ), NULL, explorer );
					menu.AppendSeparator();
				}

				wxMenu* selectAllSubmenu = new wxMenu();
				selectAllSubmenu->Append( ID_MENU_SELECT_ALL, TXT("from all layers"));
				menu.Connect( ID_MENU_SELECT_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSelectFromSpecifiedLayers ), NULL, explorer );

				selectAllSubmenu->Append( ID_MENU_SELECT_ALL_IN_ENV, TXT("from environment layers"));
				menu.Connect( ID_MENU_SELECT_ALL_IN_ENV, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSelectFromSpecifiedLayers ), NULL, explorer );

				if ( object->IsA< CLayerGroup >() )
				{
					POPULATEPOPUPMENU_CLASS_CHECK( addedLayerGroup )
					CLayerGroup* group = SafeCast< CLayerGroup >( object );

					menu.Append( ID_MENU_SAVE_LAYER_GROUP, TXT( "Save hierarchy" ) );
					menu.Connect( ID_MENU_SAVE_LAYER_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSaveLayerGroupHierarchy ), NULL, explorer );

					menu.Append( ID_MENU_SAVE_LAYER_GROUP_MODIFIED, TXT( "Save modified only" ) );
					menu.Connect( ID_MENU_SAVE_LAYER_GROUP_MODIFIED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSaveLayerGroupHierarchy ), NULL, explorer );

//					menu.Append( ID_MENU_SAVE_ITEM_ONLY, TXT( "Save explorer" ) );
//					menu.Connect( ID_MENU_SAVE_ITEM_ONLY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSaveLayerGroupexplorer ), NULL, explorer );

					menu.Append( ID_MENU_MASS_ACTIONS, TXT( "Mass actions..." ) );
					menu.Connect( ID_MENU_MASS_ACTIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnMassActions ), NULL, explorer );

					menu.Append( ID_MENU_CHECK_DUPLICATES, TXT("Find possible duplicates") );
					menu.Connect( ID_MENU_CHECK_DUPLICATES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnFindDuplicates ), NULL, explorer );
					menu.Append( ID_MENU_CHECK_DUPLICATE_IDS, TXT("Find duplicate idTags") );
					menu.Connect( ID_MENU_CHECK_DUPLICATE_IDS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnFindDuplicateIdTags ), NULL, explorer );

					menu.AppendSeparator();
					menu.Append( ID_MENU_SELECT_ALL_OBJECTS, TXT("Select all"));
					menu.Connect( ID_MENU_SELECT_ALL_OBJECTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSelectAllEntities ), NULL, explorer );
					
					menu.AppendSubMenu( selectAllSubmenu, wxT( "Select all..." ) );

					menu.AppendSeparator();
					if (!group->IsSystemGroup())
					{
						menu.Append( ID_MENU_LOAD_LAYER_GROUP, TXT( "Load group" ) );
						menu.Connect( ID_MENU_LOAD_LAYER_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLoadLayerGroup ), NULL, explorer );
						menu.Append( ID_MENU_LOAD_LAYER_GROUP_RECURSIVE, TXT( "Load all in group" ) );
						menu.Connect( ID_MENU_LOAD_LAYER_GROUP_RECURSIVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLoadLayerGroup ), NULL, explorer );
						menu.Append( ID_MENU_UNLOAD_LAYER_GROUP, TXT( "Unload group" ) );
						menu.Connect( ID_MENU_UNLOAD_LAYER_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnUnloadLayerGroup ), NULL, explorer );
						menu.Append( ID_MENU_RENAME_LAYER_GROUP, TXT("Rename group..."));
						menu.Connect( ID_MENU_RENAME_LAYER_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRenameGroup ), NULL, explorer );
						menu.AppendSeparator();

						wxMenu* streamingMenu = new wxMenu();

						streamingMenu->Append( ID_MENU_FORCE_STREAM_IN, TXT( "Force stream in" ) );
						menu.Connect( ID_MENU_FORCE_STREAM_IN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnForceStreamIn ), NULL, explorer );
						streamingMenu->Append( ID_MENU_FORCE_STREAM_OUT, TXT( "Force stream out" ) );
						menu.Connect( ID_MENU_FORCE_STREAM_OUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnForceStreamOut ), NULL, explorer );
						streamingMenu->Append( ID_MENU_IGNORE_STREAM, TXT( "Ignore entities when streaming" ) );
						menu.Connect( ID_MENU_IGNORE_STREAM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnIgnoreStream ), NULL, explorer );
						streamingMenu->Append( ID_MENU_UNIGNORE_STREAM, TXT( "Unignore entities when streaming" ) );
						menu.Connect( ID_MENU_UNIGNORE_STREAM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnUnignoreStream ), NULL, explorer );
						streamingMenu->AppendSeparator();
						streamingMenu->Append( ID_MENU_CLEAR_IGNORE_LIST, TXT( "Clear streaming ignore list" ) );
						menu.Connect( ID_MENU_CLEAR_IGNORE_LIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnClearIgnoreList ), NULL, explorer );
						streamingMenu->Append( ID_MENU_SHOW_IGNORED_ENTITIES, TXT( "Show ignored entities" ) );
						menu.Connect( ID_MENU_SHOW_IGNORED_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnShowIgnoredEntities ), NULL, explorer );

						if ( group->GetParentGroup() == NULL )
						{
							//menu.Append( ID_MENU_VALIDATE_STREAMING_TILES, TXT( "Validate streaming tiles" ) );
							//menu.Connect( ID_MENU_VALIDATE_STREAMING_TILES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnValidateStreamingTiles ), NULL, explorer );
							menu.Append( ID_MENU_COLLECT_EMPTY_ENTITIES, TXT("Collect and inspect empty entities") );
							menu.Connect( ID_MENU_COLLECT_EMPTY_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnCollectEmptyEntities ), NULL, explorer );
							menu.Append( ID_MENU_COLLECT_LIGHT_ENTITIES, TXT("Collect light entities") );
							menu.Connect( ID_MENU_COLLECT_LIGHT_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnCollectLightEntities ), NULL, explorer );
						}

						menu.AppendSubMenu( streamingMenu, wxT( "Streaming" ) );
						menu.AppendSeparator();

						menu.Append( ID_MENU_ADD_LAYER, TXT( "Add layer..." ) );
						menu.Connect( ID_MENU_ADD_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnAddLayer ), NULL, explorer );
						menu.Append( ID_MENU_PASTE_LAYER, TXT("Paste layer") );
						menu.Connect( ID_MENU_PASTE_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnPasteLayer ), NULL, explorer );
						menu.Append( ID_MENU_ADD_LAYER_GROUP, TXT( "Add group..." ) );
						menu.Connect( ID_MENU_ADD_LAYER_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnAddLayerGroup ), NULL, explorer );
						menu.AppendSeparator();
						menu.Append( ID_MENU_REMOVE_LAYER_GROUP, TXT( "Remove" ) );
						menu.Connect( ID_MENU_REMOVE_LAYER_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRemoveLayerGroup ), NULL, explorer );
						menu.AppendSeparator();
					}
					menu.Append(ID_MENU_GROUP_CHECK_OUT, TXT("Check out group"));
					menu.Append(ID_MENU_GROUP_SUBMIT, TXT("Submit group"));
					menu.Append(ID_MENU_GROUP_REVERT, TXT("Revert group"));
					menu.Append(ID_MENU_GROUP_SYNC, TXT("Sync group"));

					menu.Append( ID_MENU_LOAD_ENTITY_LIST, TXT( "Load entity list..." ) );
					menu.Connect( ID_MENU_LOAD_ENTITY_LIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLoadEntityList ), NULL, explorer );
					menu.AppendSeparator();

					menu.AppendSeparator();
					menu.Append( ID_MENU_ADD_SHADOWS, TXT("Add shadows to group"));
					menu.Connect( ID_MENU_ADD_SHADOWS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnAddShadowsToGroup ), NULL, explorer );
					menu.Append( ID_MENU_REMOVE_SHADOWS, TXT("Remove shadows to group"));
					menu.Connect( ID_MENU_REMOVE_SHADOWS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRemoveShadowsToGroup ), NULL, explorer );

					menu.Append( ID_MENU_ADD_SHADOWS_LOCAL_LIGHTS, TXT("Add shadows from local lights only to group"));
					menu.Connect( ID_MENU_ADD_SHADOWS_LOCAL_LIGHTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnAddShadowsFromLocalLightsToGroup ), NULL, explorer );					

					if (!group->IsSystemGroup())
					{
						menu.AppendSeparator();
						menu.Append( ID_MENU_REMOVE_EMPTY, TXT( "Remove empty layers..." ) );
						menu.Connect( ID_MENU_REMOVE_EMPTY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRemoveEmptyLayers ), NULL, explorer );
					}

					// DM: Hack to fix light channels
					menu.AppendSeparator();
					menu.Append( ID_MENU_RESET_LIGHTCHANNELS, TXT("Reset lightchannels") );
					menu.Connect( ID_MENU_RESET_LIGHTCHANNELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnResetLightChannels ), NULL, explorer );

					menu.Append( ID_MENU_REMOVE_FORCE_NO_AUTOHIDE, TXT("Remove FORCE NO AUTOHIDE") );
					menu.Connect( ID_MENU_REMOVE_FORCE_NO_AUTOHIDE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRemoveForceNoAutohide ), NULL, explorer );

#if 0
					menu.AppendSeparator();
					menu.Append( ID_MENU_CONVERT_GROUP, TXT( "Convert group to streamed" ) );
					menu.Append( ID_MENU_CONVERT_GROUP, TXT( "Convert group to streamed (templates only) TEMP" ) );
					menu.Connect( ID_MENU_CONVERT_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnConvertGroupToStreamed ), NULL, explorer );
					menu.Connect( ID_MENU_CONVERT_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnConvertGroupToStreamedTemplatesOnly ), NULL, explorer );
#endif

					menu.AppendSeparator();
					menu.Append( ID_MENU_SET_GROUP_NONSTATIC, TXT( "Change all layers to non-static" ) );
					menu.Append( ID_MENU_SET_GROUP_STATIC, TXT( "Change all layers to static" ) );

					menu.Append( ID_MENU_CONVERTLAYERTO, TXT( "Convert all layers to:" ) );
					menu.SetHelpString( ID_MENU_SET_GROUP_NONSTATIC, TXT("Umbra: meshes on layers WILL NOT be included into occlusion generation") );
					menu.SetHelpString( ID_MENU_SET_GROUP_STATIC, TXT("Umbra: meshes on layers WILL be included into occlusion generation") );
					menu.Connect( ID_MENU_SET_GROUP_NONSTATIC, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnConvertGroupToNonStatic ), NULL, explorer );
					menu.Connect( ID_MENU_SET_GROUP_STATIC, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnConvertGroupToStatic ), NULL, explorer );
					menu.Connect( ID_MENU_CONVERTLAYERTO, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnConvertAllLayersTo ), NULL, explorer );
					
				}
				else if ( object->IsA< CLayerInfo >() )
				{
					POPULATEPOPUPMENU_CLASS_CHECK( addedLayerInfo )
					CLayerInfo* info = SafeCast< CLayerInfo >( object );
					CLayerGroup* group = info->GetLayerGroup();
					CDiskFile *file = GDepot->FindFile(info->GetDepotPath());

					// Save
					if ( info->IsLoaded() )
					{
						menu.Append( ID_MENU_SAVE_LAYER, TXT( "Save layer" ) );
						menu.Connect( ID_MENU_SAVE_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSaveLayer ), NULL, explorer );
						menu.AppendSeparator();
					}

					if ( info->IsLoaded() )
					{
						menu.Append( ID_MENU_SELECT_ALL_OBJECTS, TXT("Select all inside the layer"));
						menu.Connect( ID_MENU_SELECT_ALL_OBJECTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSelectAllEntities ), NULL, explorer );

						menu.AppendSubMenu( selectAllSubmenu, wxT( "Select all..." ) );

						menu.Append( ID_MENU_MASS_ACTIONS, TXT( "Mass actions..." ) );
						menu.Connect( ID_MENU_MASS_ACTIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnMassActions ), NULL, explorer );

						menu.Append( ID_MENU_CHECK_DUPLICATES, TXT("Find possible duplicates") );
						menu.Connect( ID_MENU_CHECK_DUPLICATES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnFindDuplicates ), NULL, explorer );

						menu.Append( ID_MENU_CHECK_DUPLICATE_IDS, TXT("Find duplicate idTags") );
						menu.Connect( ID_MENU_CHECK_DUPLICATE_IDS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnFindDuplicateIdTags ), NULL, explorer );
						menu.AppendSeparator();
					}

					// Activate
					if ( info->IsLoaded() && info != explorer->GetActiveLayerInfo() )
					{
						menu.Append( ID_MENU_ACTIVATE_LAYER, TXT( "Make layer active" ) );
						menu.Connect( ID_MENU_ACTIVATE_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSetActiveLayer ), NULL, explorer );
						menu.AppendSeparator();
					}

					menu.Append( ID_MENU_SHOW_LAYER, TXT("Show layer") );
					menu.Connect( ID_MENU_SHOW_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLayerShow ), NULL, explorer );
					menu.Append( ID_MENU_HIDE_LAYER, TXT("Hide layer") );
					menu.Connect( ID_MENU_HIDE_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLayerHide ), NULL, explorer );

					// Setup menu
					if ( !group || !group->IsSystemGroup() )
					{	
						//copy, paste, etc
						menu.Append( ID_MENU_LAYER_RENAME, TXT("Rename layer..."));
						menu.Connect( ID_MENU_LAYER_RENAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRenameLayer ), NULL, explorer );
						menu.Append( ID_MENU_LAYER_COPY_PATH, TXT("Copy layer path to clipboard..."));
						menu.Connect( ID_MENU_LAYER_COPY_PATH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnCopyLayerPath ), NULL, explorer );
						menu.AppendSeparator();

						//lo res and hi res should not be deleted.
						menu.Append( ID_MENU_REMOVE_LAYER, TXT( "Remove layer" ) );
						menu.Connect( ID_MENU_REMOVE_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRemoveLayer ), NULL, explorer );

						menu.Append( ID_MENU_COPY_LAYER, TXT("Copy layer") );
						menu.Connect( ID_MENU_COPY_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnCopyLayer ), NULL, explorer );
						menu.Append( ID_MENU_CUT_LAYER, TXT("Cut layer") );
						menu.Connect( ID_MENU_CUT_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnCutLayer ), NULL, explorer );
						menu.AppendSeparator();

						menu.Append( ID_MENU_ADD_LAYER_TO_PARENT, TXT( "Add layer to parent group..." ) );
						menu.Connect( ID_MENU_ADD_LAYER_TO_PARENT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnAddLayer ), NULL, explorer );
						menu.Append( ID_MENU_PASTE_LAYER_TO_PARENT, TXT("Paste layer to parent group") );
						menu.Connect( ID_MENU_PASTE_LAYER_TO_PARENT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnPasteLayer ), NULL, explorer );
						menu.AppendSeparator();

						// Streaming menu
						if ( info->IsLoaded() )
						{
							wxMenu* streamingMenu = new wxMenu();

							streamingMenu->Append( ID_MENU_FORCE_STREAM_IN, TXT( "Force stream in" ) );
							menu.Connect( ID_MENU_FORCE_STREAM_IN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnForceStreamIn ), NULL, explorer );
							streamingMenu->Append( ID_MENU_FORCE_STREAM_OUT, TXT( "Force stream out" ) );
							menu.Connect( ID_MENU_FORCE_STREAM_OUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnForceStreamOut ), NULL, explorer );
							streamingMenu->Append( ID_MENU_IGNORE_STREAM, TXT( "Ignore entities when streaming" ) );
							menu.Connect( ID_MENU_IGNORE_STREAM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnIgnoreStream ), NULL, explorer );
							streamingMenu->Append( ID_MENU_UNIGNORE_STREAM, TXT( "Unignore entities when streaming" ) );
							menu.Connect( ID_MENU_UNIGNORE_STREAM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnUnignoreStream ), NULL, explorer );
							streamingMenu->AppendSeparator();
							streamingMenu->Append( ID_MENU_CLEAR_IGNORE_LIST, TXT( "Clear streaming ignore list" ) );
							menu.Connect( ID_MENU_CLEAR_IGNORE_LIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnClearIgnoreList ), NULL, explorer );
							streamingMenu->Append( ID_MENU_SHOW_IGNORED_ENTITIES, TXT( "Show ignored entities" ) );
							menu.Connect( ID_MENU_SHOW_IGNORED_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnShowIgnoredEntities ), NULL, explorer );

							menu.AppendSubMenu( streamingMenu, wxT( "Streaming" ) );
							menu.AppendSeparator();
						}
					}

					if ( file )
					{
						if ( file->IsCheckedOut() || file->IsDeleted() )
						{
							menu.Append(ID_MENU_LAYER_SUBMIT, TXT("Submit layer"));
						}
						if ( file->IsCheckedIn() )
						{
							menu.Append(ID_MENU_LAYER_CHECK_OUT, TXT("Check out layer"));
						}
						if ( file->IsLocal() || file->IsAdded() )
						{
							menu.Append(ID_MENU_LAYER_SUBMIT, TXT("Submit layer"));
						}

						if ( info->IsLoaded() )
						{
							//lo res and hi res should not be sync
							menu.Append(ID_MENU_LAYER_REVERT, TXT("Revert layer"));
							menu.Append(ID_MENU_LAYER_SYNC, TXT("Sync layer"));
						}
					}

					menu.Append( ID_MENU_LOAD_ENTITY_LIST, TXT( "Load entity list..." ) );
					menu.Connect( ID_MENU_LOAD_ENTITY_LIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLoadEntityList ), NULL, explorer );
					menu.AppendSeparator();

					menu.AppendSeparator();
					menu.Append( ID_MENU_ADD_SHADOWS_L, TXT("Add shadows to layer"));
					menu.Connect( ID_MENU_ADD_SHADOWS_L, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnAddShadowsToLayer ), NULL, explorer );
					menu.Append( ID_MENU_REMOVE_SHADOWS_L, TXT("Remove shadows from layer"));
					menu.Connect( ID_MENU_REMOVE_SHADOWS_L, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRemoveShadowsToLayer ), NULL, explorer );				
					
					menu.Append( ID_MENU_ADD_SHADOWS_LOCAL_LIGHTS_L, TXT("Add shadows from LOCAL LIGHTS ONLY to layer"));
					menu.Connect( ID_MENU_ADD_SHADOWS_LOCAL_LIGHTS_L, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnAddShadowsFromLocalLightsToLayer ), NULL, explorer );
					
#if 0
					if ( info->IsLoaded() && info->GetLayerGroup() && !info->GetLayerGroup()->IsSystemGroup())
					{
						menu.AppendSeparator();
						menu.Append( ID_MENU_CONVERT_LAYER, TXT( "Convert layer to streamed" ) );
						menu.Connect( ID_MENU_CONVERT_LAYER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnConvertLayerToStreamed ), NULL, explorer );
					}
#endif
				}
				else if ( object->IsA< CEntity >() )
				{
					POPULATEPOPUPMENU_CLASS_CHECK( addedEntity )
					CEntity *entity = Cast< CEntity >( object );

					// Grouping
					{
						Bool areGroups = false;
						Bool isInGroup = false;
						CEntityGroup* entityGroup = nullptr;

						if( entity->IsA< CEntityGroup >() == true )
						{
							entityGroup = Cast< CEntityGroup >( entity );
							areGroups = true;
						}
						if( entity->GetPartOfAGroup() == true )
						{
							entityGroup = entity->GetContainingGroup();
							isInGroup = true;
						}

						if( areGroups == true || isInGroup == true )
						{
							wxMenu* groupMenu = new wxMenu();

							if( entityGroup->IsLocked() == false )
							{
								groupMenu->Append( ID_LOCK_GROUP, TXT("Lock"), wxEmptyString, false );
								menu.Connect( ID_LOCK_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLockGroup ), nullptr, explorer );

								if( isInGroup == true )
								{
									groupMenu->Append( ID_REMOVE_FROM_GROUP, TXT("Remove from group"), wxEmptyString, false );
									menu.Connect( ID_REMOVE_FROM_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRemoveFromGroup ), nullptr, explorer );
								}
								if( areGroups == true )
								{
									groupMenu->Append( ID_UNGROUP_ITEMS, TXT("Ungroup"), wxEmptyString, false );
									menu.Connect( ID_UNGROUP_ITEMS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnUngroupItems ), nullptr, explorer );
								}
							}
							else
							{
								groupMenu->Append( ID_UNGROUP_ITEMS, TXT("Ungroup"), wxEmptyString, false );
								menu.Connect( ID_UNGROUP_ITEMS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnUngroupItems ), nullptr, explorer );
								groupMenu->Append( ID_UNLOCK_GROUP, TXT("Unlock"), wxEmptyString, false );
								menu.Connect( ID_UNLOCK_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnUnlockGroup ), nullptr, explorer );
							}

							menu.Append( wxID_ANY, TXT("Group"), groupMenu  );
						}
						else
						{
							if( objects.Size() > 1 )
							{
								menu.Append( ID_GROUP_ITEMS, TXT("Group"), wxEmptyString, false );
								menu.Connect( ID_GROUP_ITEMS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnGroupItems ), nullptr, explorer );
							}
						}

						menu.AppendSeparator();
					}

					menu.Append( ID_MENU_LOOK_AT_NODE, TXT( "Look At Node" ) );
					menu.Connect( ID_MENU_LOOK_AT_NODE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLookAtNode ), NULL, explorer );

					menu.Append( ID_MENU_ADD_TO_ENTITY_LIST, TXT( "Add to entity list" ) );
					menu.Connect( ID_MENU_ADD_TO_ENTITY_LIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnAddToEntityList ), NULL, explorer );
					menu.Append( ID_MENU_LOAD_ENTITY_LIST, TXT( "Load entity list..." ) );
					menu.Connect( ID_MENU_LOAD_ENTITY_LIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnLoadEntityList ), NULL, explorer );

					menu.AppendSeparator();
					menu.Append( ID_MENU_MASS_ACTIONS, TXT( "Mass actions..." ) );
					menu.Connect( ID_MENU_MASS_ACTIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnMassActions ), NULL, explorer );

					if ( GGame->IsActive() && GGame->GetPlayerEntity() && GGame->GetPlayerEntity()->IsA< CPlayer >() )
					{
						menu.AppendSeparator();
						menu.Append( ID_MENU_MOVE_PLAYER_THERE, TXT( "Move Player There" ) );
						menu.Connect( ID_MENU_MOVE_PLAYER_THERE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnMovePlayerThere ), NULL, explorer );
					}

					// Special case for AREA environment entities
					{
						Bool hasAreaEnviromentComponent = false;
						Bool hasSwarmPOIComponent		= false;
						Bool hasSwarmActivationTrigger	= false;
						Bool hasSwarmArea				= false;
#ifndef NO_NAVMESH_GENERATION
						Bool hasNavmeshComponent = false;
						Bool isNavmeshComponentProcessing = false;
						Bool isNavmeshComponentProcessingRecursiveGeneration = false;
#endif
						const auto& components = entity->GetComponents();
						for ( auto it = components.Begin(), end = components.End(); it != end; ++it )
						{
							CComponent* component = *it;
							if ( component->IsA< CAreaEnvironmentComponent >() )
							{
								hasAreaEnviromentComponent = true;
							}
#ifndef NO_NAVMESH_GENERATION
							else if ( component->IsA< CNavmeshComponent >() )
							{
								hasNavmeshComponent = true;
								CNavmeshComponent* navi = static_cast< CNavmeshComponent* >( component );
								isNavmeshComponentProcessing = navi->IsNavmeshGenerationRunning();
								if ( isNavmeshComponentProcessing )
								{
									isNavmeshComponentProcessingRecursiveGeneration = navi->IsRunningRecursiveGeneration();
								}
							}
#endif
							else if ( component->IsA< CBoidPointOfInterestComponent >() )
							{
								hasSwarmPOIComponent = true;
							}
							else if ( component->IsA< CBoidActivationTriggerComponent >() )
							{
								hasSwarmActivationTrigger = true;
							}
							else if ( component->IsA< CBoidAreaComponent >() )
							{
								hasSwarmArea = true;
							}
						}

						if ( entity->IsA< CEncounter >() )
						{
							menu.AppendSeparator();

							menu.Append( ID_MENU_ENCOUNTER_EDIT, ( GGame && GGame->IsActive() ) ? TXT("Debug spawn tree") : TXT("Edit spawn tree") );
							menu.Connect( ID_MENU_ENCOUNTER_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnEditEncounter ), NULL, explorer );
						}
					
						if ( entity->IsA< CFlyingCrittersLairEntity >() )
						{
							menu.AppendSeparator();

							menu.Append( ID_MENU_GENERATE_SWARM_COLLISIONS, TXT("Generate swarm collisions") );
							menu.Connect( ID_MENU_GENERATE_SWARM_COLLISIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnGenerateSwarmCollisions ), NULL, explorer );
						}

						if ( entity->IsA< IBoidLairEntity >() )
						{
							menu.AppendSeparator();

							menu.Append( ID_MENU_FIX_SWARM_LAIR_COMPONENTS, TXT("Fix broken swarm") );
							menu.Connect( ID_MENU_FIX_SWARM_LAIR_COMPONENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnFixSwarm ), NULL, explorer );
						}

						if ( hasSwarmPOIComponent )
						{
							menu.AppendSeparator();

							menu.Append( ID_MENU_GENERATE_SWARM_COLLISIONS, TXT("ChangePOIType") );
							menu.Connect( ID_MENU_GENERATE_SWARM_COLLISIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnChangeSwarmPOIType ), NULL, explorer );

							menu.Append( ID_MENU_FIX_SWARM_LAIR_COMPONENTS, TXT("Fix broken swarm") );
							menu.Connect( ID_MENU_FIX_SWARM_LAIR_COMPONENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnFixSwarm ), NULL, explorer );
						}

						if ( hasSwarmActivationTrigger )
						{
							menu.AppendSeparator();

							menu.Append( ID_MENU_FIX_SWARM_LAIR_COMPONENTS, TXT("Fix broken swarm") );
							menu.Connect( ID_MENU_FIX_SWARM_LAIR_COMPONENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnFixSwarm ), NULL, explorer );
						}

						if ( hasSwarmArea )
						{
							menu.AppendSeparator();

							menu.Append( ID_MENU_FIX_SWARM_LAIR_COMPONENTS, TXT("Fix broken swarm") );
							menu.Connect( ID_MENU_FIX_SWARM_LAIR_COMPONENTS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnFixSwarm ), NULL, explorer );
						}

						if ( hasAreaEnviromentComponent )
						{
							String selectedResource;
							GetActiveResource(selectedResource);

							menu.AppendSeparator();

							menu.Append( ID_MENU_EDIT_ENVIRONMENT, TXT("Edit...") );
							menu.Connect( ID_MENU_EDIT_ENVIRONMENT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnEditEnvironment ), NULL, explorer );
						}
#ifndef NO_NAVMESH_GENERATION
						if ( hasNavmeshComponent )
						{
							menu.AppendSeparator();
							if ( isNavmeshComponentProcessing )
							{
								if ( isNavmeshComponentProcessingRecursiveGeneration )
								{
									menu.Append( ID_MENU_STOP_NAVMESH_GENERATION, TXT("Stop recursive generation") );
									menu.Connect( ID_MENU_STOP_NAVMESH_GENERATION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnStopNavmeshGeneration ), NULL, explorer );
								}
								else
								{
									menu.Append( ID_MENU_STOP_NAVMESH_GENERATION, TXT("Processing...") );
								}
							}
							else
							{
								menu.Append( ID_MENU_GENERATE_NAVMESH, TXT("Generate navmesh") );
								menu.Connect( ID_MENU_GENERATE_NAVMESH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnGenerateNavmesh ), NULL, explorer );
								menu.Append( ID_MENU_COMPUTE_NAVI_BOUNDS, TXT("Compute navmesh based bounds") );
								menu.Connect( ID_MENU_COMPUTE_NAVI_BOUNDS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnComputeNavmeshBasedBounds ), NULL, explorer );
								menu.Append( ID_MENU_GENERATE_NAVMESH_RECURSIVE, TXT("Navmesh recursive uber-generator") );
								menu.Connect( ID_MENU_GENERATE_NAVMESH_RECURSIVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnGenerateNavmesh ), NULL, explorer );

								menu.Append( ID_MENU_GENERATE_NAVGRAPH, TXT("Navgraph generation") );
								menu.Connect( ID_MENU_GENERATE_NAVGRAPH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnGenerateNavgraph ), NULL, explorer );

								menu.Append( ID_MENU_RESET_NAVMESH_PARAMS, TXT("Reset navmesh params") );
								menu.Connect( ID_MENU_RESET_NAVMESH_PARAMS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnResetNavmeshParams ), NULL, explorer );

							}
						}
#endif
					}

					menu.AppendSeparator();

					menu.Append( ID_MENU_COPY_NODE, TXT("Copy") );
					menu.Connect( ID_MENU_COPY_NODE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnCopyNode ), NULL, explorer );

					menu.Append( ID_MENU_CUT_NODE, TXT("Cut") );
					menu.Connect( ID_MENU_CUT_NODE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnCutNode ), NULL, explorer );

					menu.Append( ID_MENU_DELETE_NODE, TXT( "Delete" ) );
					menu.Connect( ID_MENU_DELETE_NODE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnRemoveNode ), NULL, explorer );

					// Get resources used by entity
					TDynArray< CResource* > usedResources;
					entity->CollectUsedResources( usedResources );

					// We have some resources, show menu
					if ( usedResources.Size() )
					{
						wxMenu* showResourcesSubMenu = new wxMenu();
						//wxMenu* selectResourcesSubMenu = new wxMenu();

						// Add items
						for ( Uint32 i=0; i<usedResources.Size(); i++ )
						{
							CResource* resource = usedResources[i];
							showResourcesSubMenu->Append( ID_SHOW_RESOURCE_NODE + i, resource->GetFriendlyName().AsChar() );
							menu.Connect( ID_SHOW_RESOURCE_NODE + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnShowResource ), new ResourceWrapper( resource ), wxTheFrame->GetWorldEditPanel() );

//							selectResourcesSubMenu->Append( ID_SELECT_RESOURCE_NODE + i, resource->GetFriendlyName().AsChar() );
//							menu.Connect( ID_SELECT_RESOURCE_NODE + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnSelectByResource ), new ResourceLayerWrapper( resource, entity->GetLayer() ), explorer );
						}

						// Resources
						menu.AppendSeparator();
						menu.Append( wxID_ANY, TXT("Show resources"), showResourcesSubMenu );
						//menu.Append( wxID_ANY, TXT("Select resources"), selectResourcesSubMenu );
					}

					if ( entity->GetTemplate() )
					{
						menu.Append( ID_DETACH_TEMPLATES, TXT("Detach templates") );
						menu.Connect( ID_DETACH_TEMPLATES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdWorldEditPanel::OnDetachTemplates ), NULL, wxTheFrame->GetWorldEditPanel() );
					}

					// Streaming menu
					{
						wxMenu* streamingMenu = new wxMenu();

						streamingMenu->Append( ID_MENU_SHOW_STREAMING_INFO, TXT("Show streaming information") );
						menu.Connect( ID_MENU_SHOW_STREAMING_INFO, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnShowStreamingInfo ), NULL, explorer );
						streamingMenu->AppendSeparator();

						streamingMenu->Append( ID_MENU_FORCE_STREAM_IN, TXT( "Force stream in" ) );
						menu.Connect( ID_MENU_FORCE_STREAM_IN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnForceStreamIn ), NULL, explorer );
						streamingMenu->Append( ID_MENU_FORCE_STREAM_OUT, TXT( "Force stream out" ) );
						menu.Connect( ID_MENU_FORCE_STREAM_OUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnForceStreamOut ), NULL, explorer );
						streamingMenu->Append( ID_MENU_IGNORE_STREAM, TXT( "Ignore entity when streaming" ) );
						menu.Connect( ID_MENU_IGNORE_STREAM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnIgnoreStream ), NULL, explorer );
						streamingMenu->Append( ID_MENU_UNIGNORE_STREAM, TXT( "Unignore entity when streaming" ) );
						menu.Connect( ID_MENU_UNIGNORE_STREAM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnUnignoreStream ), NULL, explorer );
						streamingMenu->AppendSeparator();
						streamingMenu->Append( ID_MENU_CLEAR_IGNORE_LIST, TXT( "Clear streaming ignore list" ) );
						menu.Connect( ID_MENU_CLEAR_IGNORE_LIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnClearIgnoreList ), NULL, explorer );
						streamingMenu->Append( ID_MENU_SHOW_IGNORED_ENTITIES, TXT( "Show ignored entities" ) );
						menu.Connect( ID_MENU_SHOW_IGNORED_ENTITIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnShowIgnoredEntities ), NULL, explorer );

						menu.AppendSubMenu( streamingMenu, wxT( "Streaming" ) );
						menu.AppendSeparator();
					}
				}
				else if ( object->IsA< CComponent >() )
				{
					CComponent* component = static_cast<CComponent*>( object );
					if ( component->HasInstanceProperties() && component->GetEntity()->GetEntityTemplate() != nullptr )
					{
						menu.Append( ID_MENU_RESET_INSTANCE_PROPERTIES, TXT("Reset instance properties") );
						menu.Connect( ID_MENU_RESET_INSTANCE_PROPERTIES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnResetInstanceProperties ), NULL, explorer );
					}
				}
				else if ( object->IsA< CResource >() )
				{
					POPULATEPOPUPMENU_CLASS_CHECK( addedResource )
					menu.Append( ID_MENU_IMPORT_RESOURCE, TXT( "Import from file..." ) );
					menu.Connect( ID_MENU_IMPORT_RESOURCE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnImportResource ), NULL, explorer );

					menu.Append( ID_MENU_EXPORT_RESOURCE, TXT( "Export to file..." ) );
					menu.Connect( ID_MENU_EXPORT_RESOURCE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneExplorer::OnExportResource ), NULL, explorer );
				}
			}

			// Add inspector
			if ( IsObjectInspectorAvailable() )
			{
				menu.AppendSeparator();
				menu.Append( ID_MENU_INSPECT_OBJECT, wxT("Inspect object...") );
				menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneExplorer::OnInspectObject, explorer, ID_MENU_INSPECT_OBJECT );
			}

#undef POPULATEPOPUPMENU_CLASS_CHECK

			// End of stuff
		}

		virtual void Activate( ISerializable* obj ) const override
		{
			// Activated a layer, just set it as the active layer
			if ( obj->IsA<CLayerInfo>() )
			{
				wxTheFrame->GetSceneExplorer()->ChangeActiveLayer( static_cast<CLayerInfo*>( obj ) );
			}
			// Activated a node, look at it
			else if ( obj->IsA<CNode>() )
			{
				wxTheFrame->GetWorldEditPanel()->LookAtNode( static_cast<CNode*>( obj ) );
			}
		}

		virtual Bool Supports( ISerializable* obj ) const override
		{
			return true;
		}
	} viewer;
	
	m_viewers.PushBack( &viewer );
}

ISceneObjectViewer* SceneView::FindViewer( ISerializable* obj )
{
	// Initialize the viewers if necessary
	if ( m_viewers.Empty() )
	{
		InitializeViewers();
	}

	// TODO: cache the results for each paint if necessary to avoid
	// doing this loop for every single visible object

	// Find a viewer that supports the passed object
	for ( auto it=m_viewers.Begin(); it != m_viewers.End(); ++it )
	{
		if ( (*it)->Supports( obj ) )
		{
			return *it;
		}
	}

	// Viewer not found
	return NULL;
}

void SceneView::ClearElements()
{
	for ( auto it=m_elements.Begin(); it != m_elements.End(); ++it )
	{
		delete *it;
	}

	m_elements.Clear();
}

SceneView::Element* SceneView::GetElementAt( const wxPoint& point ) const
{
	for ( auto it=m_elements.Begin(); it != m_elements.End(); ++it )
	{
		if ( (*it)->m_rect.Contains( point ) )
		{
			return *it;
		}
	}

	return NULL;
}

SceneView::Element* SceneView::GetElementByObject( ISerializable* obj )
{
	for ( auto it=m_elements.Begin(); it != m_elements.End(); ++it )
	{
		if ( (*it)->m_object == obj )
		{
			return *it;
		}
	}

	return NULL;
}

SceneView::HeaderID SceneView::GetHeaderAt( const wxPoint& point ) const
{
	HeaderID maxPos = HID_NAME;

	for ( HeaderID i=HID_NAME; i<HID_COUNT; i = (HeaderID)(i + 1) )
	{
		if ( point.x >= m_headers[i].m_pos && point.x < m_headers[i].m_pos + m_headers[i].m_size )
		{
			return i;
		}
		if ( m_headers[i].m_pos > m_headers[maxPos].m_pos )
		{
			maxPos = i;
		}
	}

	if ( point.x < m_headers[HID_NAME].m_pos )
	{
		return HID_NAME;
	}

	return maxPos;
}

Bool SceneView::GetElementIndex( Element* element, Int32& index ) const
{
	auto maybeidx = m_elements.GetIndex( element );
	if ( maybeidx != -1 )
	{
		index = static_cast<Uint32>( maybeidx );
		return true;
	}

	index = -1;
	return false;
}

void SceneView::CalcHeaderSizes( wxDC& dc )
{
	wxRect headersRect( 0, 0, m_paintState.size.GetWidth(), SCENEVIEW_HEADER_HEIGHT );

	for ( Int32 i=0; i<HID_COUNT; ++i )
	{
		// Calculate header size and position
		Header& header = m_headers[i];
		wxRect rect = headersRect;

		// Last header is a special case because it covers the rest of available space
		if ( i == HID_COUNT - 1 )
		{
			header.m_size = headersRect.width - rect.x + 1;
		}
		else
		{
			header.m_size = m_headers[i + 1].m_pos - header.m_pos;
		}
	}
}

void SceneView::PaintHeaders( wxDC& dc )
{
	for ( Int32 i=0; i<HID_COUNT; ++i )
	{
		// Calculate header size and position
		const Header& header = m_headers[i];
		wxRect rect( header.m_pos, 0, header.m_size, SCENEVIEW_HEADER_HEIGHT );

		// Invisible, skip
		if ( rect.width < 1 )
		{
			continue;
		}

		// Draw background
		dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE ) ) );
		dc.SetPen( *wxTRANSPARENT_PEN );
		dc.DrawRectangle( rect );
		if ( i > 0 )
		{
			dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_3DHIGHLIGHT ), 1, wxPENSTYLE_SOLID ) );
			dc.DrawLine( rect.x, rect.y, rect.x, rect.y + rect.height - 1 );
		}
		dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW ), 1, wxPENSTYLE_SOLID ) );
		dc.DrawLine( rect.x, rect.y + rect.height - 1, rect.x + rect.width, rect.y + rect.height - 1 );
		dc.DrawLine( rect.x + rect.width - 1, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1 );

		// Draw title
		if ( !header.m_caption.IsEmpty() )
		{
			dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
			dc.SetTextBackground( wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE ) );
			wxSize textSize = dc.GetTextExtent( header.m_caption );
			dc.DrawText( header.m_caption, rect.x + 2, rect.y + ( SCENEVIEW_HEADER_HEIGHT - textSize.GetHeight() ) / 2 );
		}

		// Draw icon
		if ( header.m_icon.IsOk() )
		{
			wxSize isize = header.m_icon.GetSize();
			dc.DrawBitmap( header.m_icon, rect.x + ( rect.width - isize.GetWidth() ) / 2, rect.y + ( rect.height - isize.GetHeight() ) / 2, true );
		}
	}
}

void SceneView::PaintObject( wxDC& dc, ISerializable* obj )
{
	if ( !obj ) return;

	// Check filtering
	CEdSceneExplorer* explorer = wxTheFrame->GetSceneExplorer();
	if ( !m_filterString.IsEmpty() || explorer->HasGlobalFilters() )
	{
		struct {
			Bool FilterOutObject( const wxString& filterString, ISerializable* obj, SceneView* sceneView, CEdSceneExplorer* explorer )
			{
				ISceneObjectViewer* viewer = sceneView->FindViewer( obj );

				// Check this object
				if ( !viewer || ( explorer->HasGlobalFilters() && explorer->IsFilteredOut( obj ) ) || viewer->GetVisibleName( obj ).Find( filterString ) == -1 )
				{
					// Check the object's children
					TDynArray< ISerializable* > children;
					viewer->GetChildren( obj, children );

					for ( auto it=children.Begin(); it != children.End(); ++it )
					{
						if ( !FilterOutObject( filterString, *it, sceneView, explorer ) )
						{
							return false;
						}
					}

					// Yep, filter this object out
					return true;
				}

				// No, the filterstring is part of the object's visible name
				return false;
			}
		} local;

		// Check if we should filter out this object
		if ( local.FilterOutObject( m_filterString, obj, this, explorer ) )
		{
			m_filteredOut.Insert( obj );
			return;
		}
	}

	// Find the viewer for this object
	ISceneObjectViewer* viewer = FindViewer( obj );
	if ( viewer )
	{
		// Get the world
		CWorld* world = GGame->GetActiveWorld();
		CLayerInfo* activeLayer = nullptr;
		if( world != nullptr )
		{
			CSelectionManager* selectionManager = world->GetSelectionManager();
			activeLayer = selectionManager->GetActiveLayer();
		}

		// Calculate/obtain object state and sizes
		Bool expandable = viewer->IsExpandable( obj );
		Bool expanded = expandable && ( IsExpanded( obj ) || !m_filterString.IsEmpty() || explorer->HasGlobalFilters() );
		Bool drawMiniIcon = false;
		wxPoint miniIconPos;
		wxBitmap& miniIcon = expanded ? m_collapseIcon : m_expandIcon;
		Int32 miniIconAreaWidth = Max<Int32>( miniIcon.GetWidth(), SCENEVIEW_MIAREA_SIZE );
		Int32 shrunkHeight, expandedHeight, height;
		viewer->GetHeights( dc, obj, shrunkHeight, expandedHeight );
		height = expanded ? expandedHeight : shrunkHeight;

		// Prepare the paint state
		wxRect clientRect = GetClientRect();
		SSceneObjectPaintState state;
		state.m_rect.x = m_paintState.x;
		state.m_rect.y = m_paintState.y;
		state.m_rect.width = m_paintState.width - m_paintState.x + 1;
		state.m_rect.height = height;
		state.m_depth = m_paintState.depth;
		state.m_object = obj;
		state.m_selected = IsSelected( obj );
		state.m_activated = ( obj == activeLayer );
		state.m_expanded = expanded;
		state.m_filter = m_filterString;
		clientRect.y = SCENEVIEW_HEADER_HEIGHT;
		clientRect.height -= SCENEVIEW_HEADER_HEIGHT;
		// Process only if the rectangular area for the object
		// is inside the client area of the view
		if ( m_paintState.noclip || clientRect.Intersects( state.m_rect ) )
		{
			// Add visible element for this object
			Element* el = new Element();
			el->m_rect = wxRect( 0, state.m_rect.y, m_paintState.width, height );
			el->m_object = obj;
			el->m_depth = state.m_depth;
			m_elements.PushBack( el );

			// Draw element background
			wxColour bgColour = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
			const Bool odd = ( ( ( m_paintState.y + m_paintState.offset ) / SCENEVIEW_ROW_HEIGHT ) % 2 ) != 0;
			if ( odd )
			{
				unsigned int r = Clamp<unsigned int>( (unsigned int)( ( (float)bgColour.Red() ) * 0.9f ), 0, 255 );
				unsigned int g = Clamp<unsigned int>( (unsigned int)( ( (float)bgColour.Green() ) * 0.9f ), 0, 255 );
				unsigned int b = Clamp<unsigned int>( (unsigned int)( ( (float)bgColour.Blue() ) * 0.9f ), 0, 255 );
				bgColour.Set( r, g, b );
			}
			dc.SetBrush( wxBrush( bgColour, wxBRUSHSTYLE_SOLID ) );
			dc.SetPen( wxPen( bgColour, 1, wxPENSTYLE_TRANSPARENT ) );
			dc.DrawRectangle( -1, m_paintState.y, m_paintState.size.GetWidth() + 1, height );

			// Setup clip for Name header
			dc.SetClippingRegion( m_headers[HID_NAME].m_pos, 0, m_headers[HID_NAME].m_size, m_paintState.size.GetHeight() );

			// Draw the hierarchy lines
			if ( m_paintState.depth > 0 )
			{
				// Horizontal line for the element
				dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE ), 1, wxPENSTYLE_SOLID ) );
				dc.DrawLine( state.m_rect.x - SCENEVIEW_MIAREA_SIZE/2, state.m_rect.y + SCENEVIEW_ICON_SIZE/2 - 1, state.m_rect.x + SCENEVIEW_MIAREA_SIZE/2 - 1, state.m_rect.y + SCENEVIEW_ICON_SIZE/2 - 1 );

				// Vertical lines for the parents
				for ( Int32 i=m_paintState.depth - 1; i>=0; --i )
				{
					if ( i == 0 || !m_paintState.last[m_paintState.last.Size() - i - 1] )
					{
						dc.DrawLine( state.m_rect.x - i*SCENEVIEW_MIAREA_SIZE - SCENEVIEW_MIAREA_SIZE/2, state.m_rect.y - SCENEVIEW_ICON_SIZE/2, state.m_rect.x - i*SCENEVIEW_MIAREA_SIZE - SCENEVIEW_MIAREA_SIZE/2, state.m_rect.y + height - SCENEVIEW_MIAREA_SIZE/2 );
					}
				}
			}

			// Paint the object miniicon
			if ( expandable )
			{
				miniIconPos = state.m_rect.GetLeftTop();
				miniIconPos.x += ( miniIconAreaWidth - miniIcon.GetWidth() ) / 2;
				miniIconPos.y += ( height - miniIcon.GetHeight() ) / 2;
				drawMiniIcon = true;
			}
			state.m_rect.x += SCENEVIEW_MIAREA_SIZE;

			// Paint the object's own icon and optional overlay
			wxBitmap icon = viewer->GetBitmap( obj ), overlay;
			dc.DrawBitmap( icon, state.m_rect.x, state.m_rect.y + ( height - icon.GetHeight() ) / 2, true );
			if ( viewer->GetBitmapOverlay( obj, overlay ) )
			{
				dc.DrawBitmap( overlay, state.m_rect.x + ( icon.GetWidth() - overlay.GetWidth() ), state.m_rect.y + ( height - icon.GetHeight() ) / 2 + ( icon.GetHeight() - overlay.GetHeight() ), true );
			}
			state.m_rect.x += icon.GetWidth() + 2;

			// Paint the object using its viewer
			viewer->Paint( dc, state );

			// Kill clip
			dc.DestroyClippingRegion();

			// If the object has a toggleable visibility, paint its visibility status icon
			if ( viewer->ToggleableVisibility( obj ) )
			{
				Bool visible = viewer->IsVisible( obj );
				wxBitmap& icon = visible ? m_visibleIcon : m_invisibleIcon;
				wxPoint pos(
					m_headers[HID_VISIBLE].m_pos + ( m_headers[HID_VISIBLE].m_size - icon.GetSize().GetWidth() )/2,
					state.m_rect.y + ( height - icon.GetHeight() ) / 2
				);
				dc.SetClippingRegion( m_headers[HID_VISIBLE].m_pos, 0, m_headers[HID_VISIBLE].m_size, m_paintState.size.GetHeight() );
				dc.DrawBitmap( icon, pos, true );
				dc.DestroyClippingRegion();
			}

			// If the object has a file, draw its status icon
			CDiskFile* file = viewer->GetFile( obj );
  			if ( file )
  			{
				// Find the relevant file
				CLayerInfo* layerInfo = Cast<CLayerInfo>( obj );
 				CDiskFile* file = layerInfo ? layerInfo->GetLayer()->GetFile() : static_cast<CResource*>( obj )->GetFile();
  				wxBitmap icon;

				// Find proper icon depending on the file's state
				if ( file->IsModified() )
				{
					icon = m_saveIcon;
				}
				else if ( file->IsLocal() )
 				{
 					icon = m_localIcon;
 				}
 				else if ( file->IsCheckedOut() || file->IsEdited() )
 				{
 					icon = m_checkedOutIcon;
 				}
 				else if ( file->IsDeleted() )
 				{
 					icon = m_deletedIcon;
 				}
				else if ( file->IsCheckedIn() )
				{
					icon = m_checkedInIcon;
				}

				// Draw the icon, if any
				if ( icon.IsOk() )
				{
					wxPoint pos(
						m_headers[HID_STATUS].m_pos + ( m_headers[HID_STATUS].m_size - icon.GetSize().GetWidth() )/2,
						state.m_rect.y + ( height - icon.GetHeight() ) / 2
						);
  					dc.SetClippingRegion( m_headers[HID_STATUS].m_pos, 0, m_headers[HID_STATUS].m_size, m_paintState.size.GetHeight() );
  					dc.DrawBitmap( icon, pos, true );
  					dc.DestroyClippingRegion();
				}
  			}
 			
			// Draw object's class name
			{
				wxString className( obj->GetClass()->GetName().AsString().AsChar() );
				wxSize cnSize = dc.GetTextExtent( className );
				dc.SetClippingRegion( m_headers[HID_CLASS].m_pos, 0, m_headers[HID_CLASS].m_size, m_paintState.size.GetHeight() );
				dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW ) );
				dc.DrawText( className, m_headers[HID_CLASS].m_pos + 2, state.m_rect.y + ( state.m_rect.GetHeight() - cnSize.GetHeight() )/2 );
				dc.DestroyClippingRegion();
			}
		}

		// Advance the cursor vertical coordinate
		m_paintState.y += state.m_rect.height;

		// Make sure we're still inside the client area
		if ( expanded )
		{
			// Obtain the children from the viewer
			TDynArray< ISerializable* > children;
			viewer->GetChildren( obj, children );
			SortChildren( children );

			if ( !children.Empty() )
			{
				// Advance the depth and cursor
				m_paintState.depth++;
				m_paintState.x += SCENEVIEW_ICON_SIZE;
				m_paintState.last.PushBack( false );

				// Paint all children
				for ( auto it=children.Begin(); it != children.End(); ++it )
				{
					m_paintState.last[ m_paintState.last.SizeInt() - 1 ] = ( it + 1 ) == children.End();
					PaintObject( dc, *it );
				}

				// Bring back the depth and cursor
				m_paintState.last.PopBack();
				m_paintState.x -= SCENEVIEW_ICON_SIZE;
				m_paintState.depth--;
			}
		}

		// Draw mini icon (drawn here to avoid overlapping from hierarchy lines above)
		if ( drawMiniIcon )
		{
			dc.SetClippingRegion( m_headers[HID_NAME].m_pos, 0, m_headers[HID_NAME].m_size, m_paintState.size.GetHeight() );
			dc.DrawBitmap( miniIcon, miniIconPos, true );
			dc.DestroyClippingRegion();
		}
	}
}

void SceneView::PaintScene( wxDC& dc )
{
	wxSize size = GetClientSize();

	// Clear previous elements
	ClearElements();

	// Clear filtered out items
	m_filteredOut.Clear();

	// Fill background
	dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) ) );
	dc.DrawRectangle( -1, -1, size.GetWidth() + 2, size.GetHeight() + 2 );

	// Prepare headers
	CalcHeaderSizes( dc );

	// Prepare paint state
	int previousVirtualHeight = GetVirtualSize().GetHeight();
	int offset = GetScrollPos( wxVERTICAL );
	Red::System::MemorySet( &m_paintState, 0, sizeof(m_paintState) );
	m_paintState.size = size;
	m_paintState.width = size.GetWidth();
	m_paintState.offset = offset*4;
	m_paintState.x = m_headers[HID_NAME].m_pos;
	m_paintState.y = SCENEVIEW_HEADER_HEIGHT - m_paintState.offset;

	// Paint world
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		PaintObject( dc, world );
	}

	// Paint headers
	PaintHeaders( dc );
	m_paintState.noclip = false;

	// Update scrollbars if necessary
	m_paintState.y += offset*4;
	if ( previousVirtualHeight != m_paintState.y )
	{
		RunLaterOnce( [ this ]() {  
			SetVirtualSize( 1, m_paintState.y );
			SetScrollRate( 0, 4 );
		} );
	}
}

void SceneView::ExpandObject( ISerializable* obj, Bool expand )
{
	if ( IsExpanded( obj ) != expand )
	{
		if ( obj->IsA<CWorld>() )
		{
			m_worldExpanded = expand;
		}
		else
		{
			if ( expand )
			{
				m_expanded.Insert( obj );
			}
			else
			{
				m_expanded.Erase( obj );
			}
		}
		RefreshLater();
	}
}

Bool SceneView::IsExpanded( ISerializable* obj )
{
	return obj->IsA<CWorld>() ? m_worldExpanded : m_expanded.Exist( obj );
}

void SceneView::ShowStatusPopupForObject( ISerializable* obj )
{
	// Find the file (if any)
	ISceneObjectViewer* viewer = FindViewer( obj );
	CDiskFile* file = viewer ? viewer->GetFile( obj ) : NULL;
	if ( !file )
	{
		return;
	}	

	// Construct the menu
	wxMenu menu;
	file->GetStatus();
	if ( file->IsLocal() )
	{
		menu.Append( ID_SCENEVIEW_STATUSMENU_ADD, wxT("Add to version control"), wxT("Add this file to version control"), false );
	}
	else
	{
		if ( file->IsCheckedIn() )
		{
			menu.Append( ID_SCENEVIEW_STATUSMENU_CHECK_OUT, wxT("Check out"), wxT("Check out the file"), false );
		}
		else
		{
			menu.Append( ID_SCENEVIEW_STATUSMENU_SUBMIT, wxT("Submit"), wxT("Submit the file"), false );
			menu.Append( ID_SCENEVIEW_STATUSMENU_REVERT, wxT("Revert"), wxT("Revert the file"), false );
		}
		menu.Append( ID_SCENEVIEW_STATUSMENU_SYNC, wxT("Sync"), wxT("Synchronize the local copy of the file with the latest version"), false );
	}
	m_menuObject = obj;
	
	// Show the popup under the status rect
	wxPoint pos;
	Element* el = GetElementByObject( obj );
	if ( el )
	{
		pos = el->m_rect.GetBottomLeft();
		pos.x += m_headers[HID_STATUS].m_pos;
	}
	else // failed to find the status rect, use mouse position
	{
		pos = ScreenToClient( wxGetMousePosition() );
	}
	PopupMenu( &menu, pos );
}

void SceneView::OnPaint( wxPaintEvent& event )
{
	wxPaintDC dc( this );

	PaintScene( dc );
}

void SceneView::OnLeftDown( wxMouseEvent& event )
{
	wxPoint pos = event.GetPosition();
	HeaderID hid = GetHeaderAt( pos );

	SetFocus();
	CaptureMouse();

	// Clicked on a header
	if ( pos.y <= SCENEVIEW_HEADER_HEIGHT )
	{
		// Clicked on the separator
		if ( pos.x <= m_headers[hid].m_pos + 4 || pos.x >= m_headers[hid].m_pos + m_headers[hid].m_size - 4 )
		{
			// Move this header
			if ( pos.x <= m_headers[hid].m_pos + 4 )
			{
				m_movingHeader = hid;
			}
			else // Move next header
			{
				m_movingHeader = GetHeaderAt( wxPoint( m_headers[hid].m_pos + m_headers[hid].m_size + 1, pos.y ) );
			}
		}
	}
	else // clicked on an element
	{
		Element* el = GetElementAt( pos );
		if ( el )
		{
			ISerializable* obj = el->m_object.Get();
			ISceneObjectViewer* viewer = FindViewer( obj );

			if ( hid == HID_NAME )
			{
				// Toggle expand
				if ( pos.x < m_headers[HID_NAME].m_pos + SCENEVIEW_MIAREA_SIZE + SCENEVIEW_MIAREA_SIZE*el->m_depth )
				{
					ExpandObject( obj, !IsExpanded( obj ) );
					return;
				}
			}
			else if ( hid == HID_VISIBLE )
			{
				// Toggle visibility
				if ( viewer && viewer->ToggleableVisibility( obj ) )
				{
					viewer->SetVisible( obj, !viewer->IsVisible( obj ) );
					RefreshLater();
					return;
				}
			}
			else if ( hid == HID_STATUS )
			{
				ShowStatusPopupForObject( obj );
				return;
			}

			// No special click, just select
			if ( event.GetModifiers() == wxMOD_SHIFT && !m_selected.Empty() )
			{
				CWorld* world = GGame->GetActiveWorld();
				if ( world )
				{
					m_selected.Clear();
					SetMultiSelection( m_singleClickSelection, obj, world );
				}
			}
			else
			{
				if ( event.GetModifiers() != wxMOD_CONTROL )
				{
					ClearSelection();
					m_singleClickSelection = obj;
					GetElementIndex( GetElementByObject( obj ), m_firstClickedItemIndex );
				}
				ToggleSelection( obj );
			}
		}
	}
}

void SceneView::OnLeftUp( wxMouseEvent& event )
{
	// Release the capture if we have it
	if ( GetCapture() == this )
	{
		ReleaseMouse();
	}

	// Stop moving the header
	if ( m_movingHeader < HID_COUNT )
	{
		m_movingHeader = HID_COUNT;
		return;
	}
}

void SceneView::OnLeftDClick( wxMouseEvent& event )
{
	if ( m_selected.Size() == 1 && m_singleClickSelection )
	{
		Activate( m_singleClickSelection );
	}
}

void SceneView::OnRightDown( wxMouseEvent& event )
{
	wxMenu menu;

	// Select whatever is under the mouse cursor if there aren't
	// multiple selections
	if ( m_selected.Size() < 2 )
	{
		OnLeftDown( event );
		ReleaseMouse();
		Refresh( false );
	}

	// Populate the menu
	PopulatePopupMenu( menu );

	// If any menu items were added, show it
	if ( menu.GetMenuItems().GetCount() > 0 )
	{
		PopupMenu( &menu );
	}
}

void SceneView::OnMouseMove( wxMouseEvent& event )
{
	wxPoint pos = ScreenToClient( wxGetMousePosition() );
	HeaderID hid = GetHeaderAt( pos );
	wxStockCursor cursor = wxCURSOR_ARROW;

	// Moving a header
	if ( m_movingHeader < HID_COUNT )
	{
		Int32 newpos, delta, curpos;
		// Cannot move the first header
		if ( m_headers[m_movingHeader].m_pos == 0 )
		{
			return;
		}

		// Find the previous header
		curpos = m_headers[m_movingHeader].m_pos;
		HeaderID hid = HID_COUNT;
		for ( Uint32 i=HID_NAME; i<HID_COUNT; ++i )
		{
			if ( m_headers[i].m_pos < curpos )
			{
				if ( hid == HID_COUNT || m_headers[hid].m_pos < m_headers[i].m_pos )
				{
					hid = (HeaderID)i;
				}
			}
		}

		// Failed to find such a header
		if ( hid == HID_COUNT )
		{
			return;
		}

		// Calculate minpos ( prevheader's pos +1 pixels for text and +20 for icons )
		Int32 minpos = m_headers[hid].m_pos + ( m_headers[hid].m_icon.IsOk() ? 20 : 1 );

		// Clamp header new position
		newpos = pos.x;
		if ( newpos < minpos )
		{
			newpos = minpos;
		}

		// Move the headers after the current one
		delta = newpos - curpos;
		for ( Int32 i=HID_NAME; i<HID_COUNT; ++i )
		{
			if ( m_headers[i].m_pos >= curpos )
			{
				m_headers[i].m_pos += delta;
			}
		}

		RefreshLater();

		event.StopPropagation();
		return;
	}

	// Moved over a header
	if ( pos.y <= SCENEVIEW_HEADER_HEIGHT )
	{
		// Moved over the header separator
		if ( pos.x <= m_headers[hid].m_pos + 4 || pos.x >= m_headers[hid].m_pos + m_headers[hid].m_size - 4 )
		{
			cursor = wxCURSOR_SIZEWE;
		}
	}

	// Set mouse cursor
	SetCursor( wxCursor( cursor ) );
}

void SceneView::OnScroll( wxScrollEvent& event )
{
	Refresh( false );
}

void SceneView::OnCaptureLost( wxMouseCaptureLostEvent& event )
{
	// does nothing and it is very good at it, so keep it that way
	// otherwise wxWidgets complains about not handling the event
}

void SceneView::OnKeyDown( wxKeyEvent& event )
{
	switch ( event.GetKeyCode() )
	{
	case WXK_DOWN:
	case WXK_UP:
		// If we have no single click selection (that is, at least a single selection) use first element
		if ( !m_singleClickSelection && m_elements.Size() > 0 )
		{
			m_singleClickSelection = m_elements[0]->m_object.Get();
			m_firstClickedItemIndex = 0;
		}

		// Got a selection
		if ( m_singleClickSelection )
		{
			// Find its index
			Int32 index;
			Element* el = GetElementByObject( m_singleClickSelection );
			if ( el && GetElementIndex( el, index ) )
			{
				// If the index is at the bounds, try to scroll a bit and re-send the event
				if ( ( event.GetKeyCode() == WXK_DOWN && index == m_elements.Size() - 1 ) ||
					 ( event.GetKeyCode() == WXK_UP && index == 0 ) )
				{
					Int32 prevScroll = GetScrollPos( wxVERTICAL );
					SetScrollPos( wxVERTICAL, prevScroll + ( event.GetKeyCode() == WXK_UP ? - 32 : 32 ) );
					Refresh( false );
					if ( prevScroll != GetScrollPos( wxVERTICAL ) )
					{
						RunLaterOnce( [ this, event ]() mutable { OnKeyDown( event ); } );
					}
					return;
				}

				// Shift+arrow should keep current selection
				if ( wxGetKeyState( WXK_SHIFT ) == false )
				{
					ClearSelection();

					index += event.GetKeyCode() == WXK_DOWN ? 1 : -1;
					Select( m_elements[index]->m_object.Get() );
					m_singleClickSelection = m_elements[index]->m_object.Get();
					GetElementIndex( m_elements[index], m_firstClickedItemIndex );
					return;
				}
				else
				{
					// Select or unselect item
					if( index == m_firstClickedItemIndex )
					{
						index += event.GetKeyCode() == WXK_DOWN ? 1 : -1;
						Select( m_elements[index]->m_object.Get() );
						m_singleClickSelection = m_elements[index]->m_object.Get();
					}
					else if( index < m_firstClickedItemIndex )
					{
						if( event.GetKeyCode() == WXK_DOWN )
						{
							Deselect( m_elements[index]->m_object.Get() );
							m_singleClickSelection = m_elements[index+1]->m_object.Get();
						}
						else
						{
							Select( m_elements[index-1]->m_object.Get() );
							m_singleClickSelection = m_elements[index-1]->m_object.Get();
						}
					}
					else
					{
						if( event.GetKeyCode() == WXK_DOWN )
						{
							Select( m_elements[index+1]->m_object.Get() );
							m_singleClickSelection = m_elements[index+1]->m_object.Get();
						}
						else
						{
							Deselect( m_elements[index]->m_object.Get() );
							m_singleClickSelection = m_elements[index-1]->m_object.Get();
						}
					}
				}
			}
		}
		break;
	}
}

void SceneView::PopulatePopupMenu( wxMenu& menu )
{
	THashSet< ISceneObjectViewer* > viewers;

	// Find the viewers for the selection
	for ( auto it=m_selected.Begin(); it != m_selected.End(); ++it )
	{
		if ( m_filteredOut.Exist( *it ) )
		{
			continue;
		}
		ISceneObjectViewer* viewer = FindViewer( *it );
		viewers.Insert( viewer );
	}

	// Ask the viewers to fill the menu
	for ( auto it=viewers.Begin(); it != viewers.End(); ++it )
	{
		(*it)->PopulatePopupMenu( m_selected, menu );
	}
}

// DRY
#define STATUS_MENU_COMMAND_IMPLEMENTATION(cmd,errmsg) \
{ \
	/* Obtain file */ \
	ISceneObjectViewer* viewer = FindViewer( m_menuObject ); \
	CDiskFile* file = viewer ? viewer->GetFile( m_menuObject ) : NULL; \
	ASSERT( file, TXT("Failed to find the associated file for the object command") ); \
	if ( !file ) \
	{ \
		return; \
	} \
	\
	/* Do command */ \
	if ( !file->cmd() ) \
	{ \
		wxMessageBox( wxT(errmsg) + wxString( file->GetAbsolutePath().AsChar() ) + wxT("'"), wxT("Operation Error"), wxICON_ERROR|wxOK|wxCENTRE ); \
	} \
	\
	RefreshLater(); \
}

void SceneView::OnStatusMenuCheckOut( wxCommandEvent &event ) STATUS_MENU_COMMAND_IMPLEMENTATION( CheckOut, "Failed to check out '" )
void SceneView::OnStatusMenuSubmit( wxCommandEvent &event ) STATUS_MENU_COMMAND_IMPLEMENTATION( Submit, "Failed to submit '" )
void SceneView::OnStatusMenuRevert( wxCommandEvent &event ) STATUS_MENU_COMMAND_IMPLEMENTATION( Revert, "Failed to revert '" )
void SceneView::OnStatusMenuAdd( wxCommandEvent &event ) STATUS_MENU_COMMAND_IMPLEMENTATION( Add, "Failed to add '" )
void SceneView::OnStatusMenuSync( wxCommandEvent &event ) STATUS_MENU_COMMAND_IMPLEMENTATION( Sync, "Failed to synchronize '" )

SceneView::SceneView( wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name )
	: wxScrolled<wxWindow>( parent, id, pos, size, style, name )
	, m_movingHeader( HID_COUNT )
	, m_singleClickSelection( NULL )
{
	m_expandIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TREE_EXPAND") );
	m_collapseIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TREE_COLLAPSE") );
	m_visibleIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_EYE") );
	m_invisibleIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_EYE_CLOSED") );
	m_localIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LOCAL") );
	m_checkedInIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LOCK_PAPER") );
	m_checkedOutIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECKED_OUT") );
	m_deletedIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_MARKED_DELETE") );
	m_saveIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_SAVE16") );

	m_paintState.noclip = false;

	m_headers[HID_NAME] = Header( wxT("Name"), wxBitmap(), 0 );
	m_headers[HID_VISIBLE] = Header( wxEmptyString, SEdResources::GetInstance().LoadBitmap( TEXT("IMG_EYE") ), 200 );
	m_headers[HID_STATUS] = Header( wxEmptyString, SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DEPOT") ), 220 );
	m_headers[HID_CLASS] = Header( wxT("Class"), wxBitmap(), 240 );

	Bind( wxEVT_PAINT, &SceneView::OnPaint, this );
	Bind( wxEVT_LEFT_DOWN, &SceneView::OnLeftDown, this );
	Bind( wxEVT_LEFT_UP, &SceneView::OnLeftUp, this );
	Bind (wxEVT_LEFT_DCLICK, &SceneView::OnLeftDClick, this );
	Bind( wxEVT_RIGHT_DOWN, &SceneView::OnRightDown, this );
	Bind( wxEVT_MOTION, &SceneView::OnMouseMove, this );
	Bind( wxEVT_SCROLL_THUMBTRACK, &SceneView::OnScroll, this );
	Bind( wxEVT_SCROLL_THUMBRELEASE, &SceneView::OnScroll, this );
	Bind( wxEVT_MOUSE_CAPTURE_LOST, &SceneView::OnCaptureLost, this );
	Bind( wxEVT_KEY_DOWN, &SceneView::OnKeyDown, this );

	Connect( ID_SCENEVIEW_STATUSMENU_CHECK_OUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneView::OnStatusMenuCheckOut), NULL, this );
	Connect( ID_SCENEVIEW_STATUSMENU_SUBMIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneView::OnStatusMenuSubmit), NULL, this );
	Connect( ID_SCENEVIEW_STATUSMENU_REVERT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneView::OnStatusMenuRevert), NULL, this );
	Connect( ID_SCENEVIEW_STATUSMENU_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneView::OnStatusMenuAdd), NULL, this );
	Connect( ID_SCENEVIEW_STATUSMENU_SYNC, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneView::OnStatusMenuSync), NULL, this );

	SetDoubleBuffered( true );
	ResetState();
}

SceneView::~SceneView()
{
	ClearElements();
}

void SceneView::ResetState()
{
	CWorld* world = GGame->GetActiveWorld();
	if( world != nullptr )
	{
		CSelectionManager* selectionManager = world->GetSelectionManager();
		selectionManager->SetActiveLayer( nullptr );
	}

	ClearElements();
	ClearSelection();
	m_expanded.Clear();
	m_worldExpanded = true;
	RefreshLater();
}

ISerializable* SceneView::GetObjectAt( const wxPoint& point ) const
{
	Element* el = GetElementAt( point );
	return el ? el->m_object.Get() : NULL;
}

void SceneView::ClearSelection()
{
	m_selected.Clear();
	m_singleClickSelection = NULL;
	UpdateWorldSelectionLater();
}

void SceneView::Select( ISerializable* obj )
{
	if ( obj && !m_selected.Exist( obj ) && !m_filteredOut.Exist( obj ) )
	{
		m_selected.Insert( obj );

		// Expand the object's ancestors
		{
			struct
			{
				ISerializable* GetParent( ISerializable* obj )
				{
					if ( obj->IsA< CLayerInfo >() )
					{
						return static_cast<CLayerInfo*>( obj )->GetLayerGroup();
					}
					else if ( obj->IsA< CLayerGroup >() )
					{
						return static_cast<CLayerGroup*>( obj )->GetParentGroup();
					}
					else if ( obj->IsA< CObject >() )
					{
						return static_cast<CObject*>( obj )->GetParent();
					}
					return nullptr;
				}
			} local;

			for ( ISerializable* parent = local.GetParent( obj ); parent != nullptr; parent = local.GetParent( parent ) )
			{
				m_expanded.Insert( parent );
			}
		}

		// special case for group selecting
		{
			if( obj->IsA< CEntity >() == true )
			{
				CEntity* entity = Cast< CEntity >( obj );

				if( entity->IsA< CEntityGroup >() == true )
				{
					CEntityGroup* entityGroup = Cast< CEntityGroup >( obj );

					if( entityGroup->IsLocked() == true )
					{
						const TDynArray< CEntity* >& entities = entityGroup->GetEntities();
						const Uint32 entityCount = entities.Size();
						for( Uint32 i=0; i<entityCount; ++i )
						{
							m_selected.Insert( entities[i] );
						}
					}
				}
				else if( entity->GetPartOfAGroup() == true )
				{
					CEntityGroup* entityGroup = entity->GetContainingGroup();

					if( entityGroup->IsLocked() == true )
					{
						Select( entityGroup );
					}
				}
			}
		}

		UpdateWorldSelectionLater();
		RefreshLater();
	}
}

void SceneView::Deselect( ISerializable* obj )
{
	if ( m_selected.Exist( obj ) )
	{
		if ( obj == m_singleClickSelection )
		{
			m_singleClickSelection = NULL;
		}
		m_selected.Erase( obj );
		UpdateWorldSelectionLater();
		RefreshLater();
	}
}

Bool SceneView::IsSelected( ISerializable* obj ) const
{
	return m_selected.Exist( obj );
}

void SceneView::ToggleSelection( ISerializable* obj )
{
	if ( IsSelected( obj ) )
	{
		Deselect( obj );
	}
	else
	{
		Select( obj );
	}
}

void SceneView::ExpandToSelection()
{
	// The struct is just to allow for a local recursive function
	THashSet< ISerializable* > notFound = m_selected;
	struct Worker {
		SceneView* self;
		THashSet< ISerializable* > notFound;
		void RecursiveScan( ISerializable* obj, Bool& anyFound )
		{
			anyFound = false;
			if ( !obj ) return;

			// Remove from not found objects
			if ( notFound.Erase( obj ) )
			{
				anyFound = true;
			}

			// Stop if we have reached the end
			if ( notFound.Empty() )
			{
				return;
			}

			// Get the viewer for this object
			ISceneObjectViewer* viewer = self->FindViewer( obj );

			// Get the children
			TDynArray< ISerializable* > children;
			viewer->GetChildren( obj, children );

			// If there are children, expand this object and scan them
			if ( !children.Empty() )
			{
				// Expand the object
				Bool wasExpanded = self->IsExpanded( obj );
				if ( !wasExpanded )
				{
					self->ExpandObject( obj, true );
				}

				// Scan the children
				for ( auto it=children.Begin(); it != children.End() && !notFound.Empty(); ++it )
				{
					Bool anyFoundHere;
					RecursiveScan( *it, anyFoundHere );
					if ( anyFoundHere )
					{
						anyFound = true;
					}
				}

				// If no node was found and the object wasn't expanded, collapse it
				if ( !anyFound && !wasExpanded )
				{
					self->ExpandObject( obj, false );
				}
			}
		};
		Worker( SceneView* sceneView, THashSet< ISerializable* >& not_found ) : self( sceneView ), notFound( not_found ){}
	} worker( this, notFound );

	// Don't bother if there is no selection
	if ( m_selected.Empty() )
	{
		return;
	}

	Bool anyFound;
	worker.RecursiveScan( GGame->GetActiveWorld(), anyFound );
	if ( anyFound )
	{
		RefreshLater();

		// scrolling process need expanded tree
		RunLaterOnce( [ this ]() {
			RunLaterOnce( [ this ]() {
				RunLaterOnce( [ this ]() {
					ScrollToSelection();
				} );
			} );
		} );
	}
}

ISerializable* SceneView::GetFirstSelectedObject()
{
	ISerializable* object = nullptr;
	for ( auto it=m_selected.Begin(); it != m_selected.End(); ++it )
	{
		if ( !m_filteredOut.Exist( *it ) )
		{
			object = *it;
			break;
		}
	}
	if ( object == nullptr )
	{
		return nullptr;
	}

	if( object->IsA< CEntity >() == true )
	{
		return object;
	}
	else
	{
		CComponent* component = Cast< CComponent >( object );
		if( component != nullptr )
		{
			return component->GetEntity();
		}
	}

	return nullptr;
}

void SceneView::ScrollToSelection()
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		Int32 firstSelectedItemOffset = 0;
		ISerializable* firstSelectedObject = GetFirstSelectedObject();
		CalculateFirstSelectedElementOffset( world, firstSelectedObject, firstSelectedItemOffset );

		Int32 xUnit = 0;
		Int32 yUnit = 0;
		GetScrollPixelsPerUnit( &xUnit, &yUnit );

		firstSelectedItemOffset = firstSelectedItemOffset / yUnit;

		Scroll( -1, firstSelectedItemOffset );
	}
}

Bool SceneView::CalculateFirstSelectedElementOffset( ISerializable* currentNode, ISerializable* firstSelectedObject, Int32& offset )
{
	if( currentNode == firstSelectedObject )
	{
		return false;	// stop selection
	}
	else
	{
		offset += SCENEVIEW_ICON_SIZE;

		// Find the viewer for this object
		ISceneObjectViewer* viewer = FindViewer( currentNode );
		if( viewer != nullptr )
		{
			// Calculate/obtain object state and sizes
			Bool expandable = viewer->IsExpandable( currentNode );
			Bool expanded = expandable && ( IsExpanded( currentNode ) || !m_filterString.IsEmpty() || wxTheFrame->GetSceneExplorer()->HasGlobalFilters() );

			if( expanded == true )
			{
				// Obtain the children from the viewer
				TDynArray< ISerializable* > children;
				viewer->GetChildren( currentNode, children );
				SortChildren( children );

				if( children.Empty() == false )
				{
					// Paint all children
					for ( auto it=children.Begin(); it != children.End(); ++it )
					{
						ISerializable* firstSelectedNode = ( *m_selected.Begin() );
						if( CalculateFirstSelectedElementOffset( *it, firstSelectedObject, offset ) == false )
						{
							return false;	// stop selection
						}
					}
				}
			}
		}

		return true;
	}
}

void SceneView::GetSelectedLayerGroups( TDynArray<CLayerGroup*>& layerGroups )
{
	for ( auto it=m_selected.Begin(); it != m_selected.End(); ++it )
	{
		ISerializable* obj = *it;
		if ( m_filteredOut.Exist( obj ) )
		{
			continue;
		}
		if ( obj->IsA<CLayerGroup>() )
		{
			layerGroups.PushBack( static_cast<CLayerGroup*>( obj ) );
		}
		else if ( obj->IsA<CWorld>() )
		{
			layerGroups.PushBack( static_cast<CWorld*>( obj )->GetWorldLayers() );
		}
	}
}

void SceneView::CollectEntitiesFromSelection( TDynArray<CEntity*>& entities )
{
	// Collect from layergroups
	struct {
		void CollectLayer( CLayer* layer, THashSet< CEntity* >& entitiesSet )
		{
			auto entities = layer->GetEntities();
			for ( CEntity* entity : entities )
			{
				entitiesSet.Insert( entity );
			}
		}

		void CollectGroup( CLayerGroup* group, THashSet< CEntity* >& entitiesSet )
		{
			auto layers = group->GetLayers();
			for ( CLayerInfo* info : layers )
			{
				CLayer* layer = info->IsLoaded() ? info->GetLayer() : nullptr;
				if ( layer )
				{
					CollectLayer( layer, entitiesSet );
				}
			}
			auto groups = group->GetSubGroups();
			for ( CLayerGroup* group : groups )
			{
				CollectGroup( group, entitiesSet );
			}
		}
	} local;

	THashSet< CEntity* > entitiesSet;
	for ( auto it=m_selected.Begin(); it != m_selected.End(); ++it )
	{
		ISerializable* obj = *it;
		if ( obj->IsA<CLayerGroup>() )
		{
			local.CollectGroup( static_cast< CLayerGroup* >( obj ), entitiesSet );
		}
		else if ( obj->IsA<CWorld>() )
		{
			local.CollectGroup( static_cast<CWorld*>( obj )->GetWorldLayers(), entitiesSet );
		}
		else if ( obj->IsA<CLayer>() )
		{
			local.CollectLayer( static_cast< CLayer* >( obj ), entitiesSet );
		}
		else if ( obj->IsA<CLayerInfo>() )
		{
			if ( static_cast< CLayerInfo* >( obj )->IsLoaded() )
			{
				local.CollectLayer( static_cast< CLayerInfo* >( obj )->GetLayer(), entitiesSet );
			}
		}
		else if ( obj->IsA<CEntity>() )
		{
			entitiesSet.Insert( static_cast< CEntity* >( obj ) );
		}
	}

	// Push set to array
	for ( CEntity* entity : entitiesSet )
	{
		// Note: we do the filter out here to avoid ignoring layers, groups, etc
		// that are themselves filtered out but their entities are not
		if ( m_filteredOut.Exist( entity ) )
		{
			continue;
		}
		entities.PushBack( entity );
	}
}

void SceneView::Activate( ISerializable* obj )
{
	ISceneObjectViewer* viewer = FindViewer( obj );
	if ( viewer )
	{
		viewer->Activate( obj );
	}
}

void SceneView::SetActiveLayer( CLayerInfo* layer )
{
	SEvents::GetInstance().QueueEvent( CNAME( ActiveLayerChanging ), CreateEventData( NULL ) );
	
	CWorld* world = GGame->GetActiveWorld();
	if( world != nullptr )
	{
		CSelectionManager* selectionManager = world->GetSelectionManager();
		selectionManager->SetActiveLayer( layer );
	}

	SEvents::GetInstance().QueueEvent( CNAME( ActiveLayerChanged ), CreateEventData( NULL ) );
	RefreshLater();
}

void SceneView::SetFilterString( const wxString& filterString )
{
	m_filterString = filterString;
	RefreshLater();
}

void SceneView::ClearFilterString()
{
	SetFilterString( wxEmptyString );
}

void SceneView::RefreshLater()
{
	::RefreshLater( this );
}

void SceneView::UpdateWorldSelection()
{
	// Get selection manager or exit
	CSelectionManager* manager = GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetSelectionManager() : NULL;
	if ( manager == NULL )
	{
		return;
	}

	// Wrap the transaction to scope to make sure that the scene explorer selection event
	// is sent after the selection events
	ISerializable* layerToSelect = NULL;
	{
		CSelectionManager::CSelectionTransaction selTrans( *manager );

		// Sync the selected nodes here with the world
		manager->DeselectAll();
		THashSet< ISerializable* > selected = m_selected;
		TDynArray< CNode* > nodesToSelect;
		for ( auto it=selected.Begin(); it != selected.End(); ++it )
		{
			CNode* node = Cast<CNode>( *it );
			if ( node )
			{
				nodesToSelect.PushBack( node );
			}
			else if ( (*it)->IsA< CLayerInfo >() || (*it)->IsA< CLayerGroup >() )
			{
				layerToSelect = *it;
			}
		}
		manager->Select( nodesToSelect );
	}

	// If there was a layer to select, do it
	if ( layerToSelect )
	{
		manager->SelectLayer( layerToSelect );
	}
	else
	{
		// Emit scene explorer selection event
		SEvents::GetInstance().DispatchEvent( CNAME( SceneExplorerSelectionChanged ), nullptr );
	}
}

void SceneView::UpdateWorldSelectionLater()
{
	RunLaterOnce( [ this ](){ UpdateWorldSelection(); } );
}

Bool SceneView::SetMultiSelection( ISerializable* startNode, ISerializable* endNode, ISerializable* currentNode )
{
	static Bool selectionIsActive = false;

	if ( currentNode == nullptr )
	{
		return true;	// continue selection
	}

	if ( currentNode == startNode || currentNode == endNode )
	{
		if( selectionIsActive == false )
		{
			selectionIsActive = true;
		}
		else
		{
			Select( currentNode );
			selectionIsActive = false;
			return false;// stop selection
		}
	}

	if ( selectionIsActive == true )
	{
		Select( currentNode );
	}

	// Find the viewer for this object
	ISceneObjectViewer* viewer = FindViewer( currentNode );
	if( viewer != nullptr )
	{
		// Calculate/obtain object state and sizes
		Bool expandable = viewer->IsExpandable( currentNode );
		Bool expanded = expandable && ( IsExpanded( currentNode ) || !m_filterString.IsEmpty() || wxTheFrame->GetSceneExplorer()->HasGlobalFilters() );

		if ( expanded == true )
		{
			// Obtain the children from the viewer
			TDynArray< ISerializable* > children;
			viewer->GetChildren( currentNode, children );
			SortChildren( children );

			if ( children.Empty() == false )
			{
				// Paint all children
				for ( auto it=children.Begin(); it != children.End(); ++it )
				{
					if ( SetMultiSelection( startNode, endNode, *it ) == false )
					{
						return false;	// stop selection
					}
				}
			}
		}
	}

	return true;	// continue selection
}

void SceneView::SortChildren( TDynArray< ISerializable* >& children ) const
{
	if ( wxTheFrame->GetSceneExplorer()->GetSortingMode() == SSM_Alphabetically )
	{
		::Sort( children.Begin(), children.End(), []( ISerializable* a, ISerializable* b ) {
			// Always put groups before layers 
			if ( a->IsA< CLayerGroup >() && b->IsA< CLayerInfo >() ) return true;
			if ( b->IsA< CLayerGroup >() && a->IsA< CLayerInfo >() ) return false;

			// Sort alphabetically
			const Char* nameA = TXT("");
			const Char* nameB = TXT("");
			if ( a->IsA< CNode >() ) nameA = static_cast< CNode* >( a )->GetName().AsChar();
			else if ( a->IsA< CLayerInfo >() ) nameA = static_cast< CLayerInfo* >( a )->GetShortName().AsChar();
			else if ( a->IsA< CLayerGroup >() ) nameA = static_cast< CLayerGroup* >( a )->GetName().AsChar();
			if ( b->IsA< CNode >() ) nameB = static_cast< CNode* >( b )->GetName().AsChar();
			else if ( b->IsA< CLayerInfo >() ) nameB = static_cast< CLayerInfo* >( b )->GetShortName().AsChar();
			else if ( b->IsA< CLayerGroup >() ) nameB = static_cast< CLayerGroup* >( b )->GetName().AsChar();
			return StringNaturalCompare( nameA, nameB ) < 0;
		});
	}
}

RED_INLINE CLayerInfo* SceneView::GetActiveLayer() const
{
	CWorld* world = GGame->GetActiveWorld();
	if( world != nullptr )
	{
		CSelectionManager* selectionManager = world->GetSelectionManager();
		return selectionManager->GetActiveLayer();
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////

// Mass action object iterator for an array of entities which goes through
// both the entities and the components
class CEdEntitiesAndComponentsMassActionIterator : public IEdMassActionIterator
{
	LayerEntitiesArray					m_entities;
	TDynArray< IEdMassActionObject* >	m_proxies;
	Uint32								m_currentEntity;
	Uint32								m_currentComponent;
	SEntityStreamingState				m_currentEntityStreamingState;
	Bool								m_updateCurrentEntityStreamingBuffers;
	Bool								m_firstEntity;

	// Proxy class for nodes
	class NodeProxy : public CEdMassActionRTTIProxy
	{
		CEdEntitiesAndComponentsMassActionIterator*	m_iterator;
	public:
		NodeProxy( CNode* node, CEdEntitiesAndComponentsMassActionIterator* iterator )
			: CEdMassActionRTTIProxy( node )
			, m_iterator( iterator )
		{
		}

		virtual bool SetPropertyString( int n, const wxString& value ) override
		{
			// We need special handling for component properties and streamed components
			if ( GetClass()->IsA< CComponent >() )
			{
				CComponent* component = static_cast<CComponent*>( GetObject() );
				CEntity* entity = component->GetEntity();

				// If the entity has a template, check if this property is an instance property
				// and if not, ignore the request since we cannot modify it here
				if ( entity->GetEntityTemplate() != nullptr )
				{
					TDynArray< CName > properties;
					entity->GetEntityTemplate()->GetInstancePropertiesForComponent( CName( component->GetName() ), properties );
					if ( !properties.Exist( GetPropertyName( n ) ) )
					{
						return false;
					}
				}

				// Streamed component, make sure we update the streaming buffers of its entity
				if ( entity->ShouldBeStreamed() && component->IsStreamed() )
				{
					m_iterator->m_updateCurrentEntityStreamingBuffers = true;
				}
			}

			return CEdMassActionRTTIProxy::SetPropertyString( n, value );
		}
	};

	void UpdateEntityBuffers()
	{
		if ( m_updateCurrentEntityStreamingBuffers )
		{
			m_updateCurrentEntityStreamingBuffers = false;
			// Update streamed component data buffers without creating them again
			// since we already have them loaded in memory from Next()
			m_entities[m_currentEntity]->UpdateStreamedComponentDataBuffers( false );
			m_entities[m_currentEntity]->UpdateStreamingDistance();
		}
	}

	void DeleteProxies()
	{
		for ( auto it=m_proxies.Begin(); it != m_proxies.End(); ++it )
		{
			delete *it;
		}
		m_proxies.Clear();
	}

public:
	CEdEntitiesAndComponentsMassActionIterator::CEdEntitiesAndComponentsMassActionIterator( const LayerEntitiesArray& entities )
		: m_entities( entities )
	{
		Rewind();
	}

	~CEdEntitiesAndComponentsMassActionIterator()
	{
		DeleteProxies();
	}

	virtual void Rewind() override
	{
		GFeedback->BeginTask( TXT("Scanning the world, this will take a while"), false );

		// Release any previous proxies
		DeleteProxies();

		// Reset scan state
		m_currentEntity = 0;
		m_currentComponent = 0;
		m_firstEntity = m_entities.Size() > 0;
	}

	virtual bool HasMore() override
	{
		// First entity => we have the entity itself
		if ( m_firstEntity )
		{
			return true;
		}

		// No more entities
		if ( m_currentEntity >= m_entities.Size() )
		{
			GFeedback->EndTask();
			return false;
		}

		// No more components
		if ( m_currentComponent >= m_entities[m_currentEntity]->GetComponents().Size() && m_currentEntity == m_entities.Size() - 1 )
		{
			GFeedback->EndTask();
			return false;
		}

		// More components in the current entity
		return true;
	}

	virtual IEdMassActionObject* Next() override
	{
		if ( HasMore() )
		{
			// First entity, stream it in and return the entity itself
			if ( m_firstEntity )
			{
				m_firstEntity = false;
				m_entities[0]->PrepareStreamingComponentsEnumeration( m_currentEntityStreamingState, false );
				IEdMassActionObject* obj = new NodeProxy( m_entities[0], this );
				m_proxies.PushBack( obj );
				return obj;
			}

			// We have more components for this entity
			if ( m_currentComponent < m_entities[m_currentEntity]->GetComponents().Size() )
			{
				IEdMassActionObject* obj = new NodeProxy( m_entities[m_currentEntity]->GetComponents()[m_currentComponent], this );
				m_proxies.PushBack( obj );
				m_currentComponent++;
				return obj;
			}
			else // Ran out of components, stream in and return the next entity
			{
				// Save any streaming modifications and restore previous streaming state
				UpdateEntityBuffers();
				m_entities[m_currentEntity]->FinishStreamingComponentsEnumeration( m_currentEntityStreamingState );

				// Get the next entity
				m_currentEntity++;
				m_currentComponent = 0;
				m_entities[m_currentEntity]->PrepareStreamingComponentsEnumeration( m_currentEntityStreamingState, false );
				IEdMassActionObject* obj = new NodeProxy( m_entities[m_currentEntity], this );
				m_proxies.PushBack( obj );
				GFeedback->UpdateTaskProgress( m_currentEntity, m_entities.Size() );
				return obj;
			}
		}
		else // nothing more to return
		{
			GFeedback->EndTask();
			return nullptr;
		}
	}
};

// Mass action condition that checks if the node is an entity and
// it contains a component of the specified class
class CEdMassActionContainsComponentCondition : public IEdMassActionConditionType
{
	class Condition : public IEdMassActionCondition
	{
		wxPanel*	m_panel;
		wxChoice*	m_class;
		int			m_previousClass;

		void CreatePanel( wxWindow* parent )
		{
			m_panel = new wxPanel( parent );
			m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
			m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

			m_panel->GetSizer()->Add( new wxStaticText( m_panel, wxID_ANY, wxT("of class") ), 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );

			m_class = new wxChoice( m_panel, wxID_ANY );
			m_class->Freeze();
				
			TDynArray< CClass* > componentClasses;
			TDynArray< CName > componentClassNames;
			SRTTI::GetInstance().EnumClasses( CComponent::GetStaticClass(), componentClasses, nullptr, true );
			for ( auto it=componentClasses.Begin(); it != componentClasses.End(); ++it )
			{
				componentClassNames.PushBack( (*it)->GetName() );
			}
			SortAsCharArray( componentClassNames );
			for ( auto it=componentClassNames.Begin(); it != componentClassNames.End(); ++it )
			{
				m_class->Append( (*it).AsChar() );
			}

			m_class->Thaw();
			m_class->SetSelection( m_previousClass );
			m_panel->GetSizer()->Add( m_class, 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
			m_panel->Layout();
		}

		virtual void OnConditionSelect( IEdMassActionCondition* previousSelection ) override
		{
			Condition* prevSel;
			if ( ( prevSel = dynamic_cast<Condition*>(previousSelection) ) )
			{
				m_previousClass = prevSel->m_class->GetSelection();
			}
		}
			
		virtual void ExportToXML( wxXmlNode* node )
		{
			node->AddAttribute( wxT("componentclass"), m_class->GetStringSelection() );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			int index;
				
			index = m_class->FindString( node->GetAttribute( wxT("componentclass") ) );
			if ( index == -1 )
			{
				wxMessageBox( wxString::Format( wxT("Unknown component class: %s"), node->GetAttribute( wxT("componentclass") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
				index = 0;
			}
			m_class->SetSelection( index );
		}

	public:
		Condition( IEdMassActionConditionType* type, CEdMassActionContext* context )
			: IEdMassActionCondition( context )
			, m_panel( NULL )
			, m_class( NULL )
			, m_previousClass( 0 )
		{
			m_type = type;
		}

		virtual wxWindow* GetWindow( wxWindow* parent )
		{
			if ( !m_panel )
			{
				CreatePanel( parent );
			}
			return m_panel;
		}

		virtual wxString GetDescription() const
		{
			return wxString::Format( wxT("The entity contains a component of class %s"), m_class->GetStringSelection() );
		}

		virtual bool Check( IEdMassActionObject* object )
		{
			// Check if the object is an entity
			if ( !object->GetClass()->IsA< CEntity >() )
			{
				return false;
			}

			// Find the class
			CClass* cls = SRTTI::GetInstance().FindClass( CName( m_class->GetStringSelection() ) );
			if ( cls == nullptr )
			{
				return false;
			}

			// Find the component
			CEntity* entity = static_cast< CEntity* >( static_cast< CEdMassActionRTTIProxy* >( object )->GetObject() );
			const TDynArray< CComponent* >& components = entity->GetComponents();
			for ( auto it=components.Begin(); it != components.End(); ++it )
			{
				CComponent* component = *it;
				if ( component->GetClass()->IsA( cls ) )
				{
					return true;
				}
			}

			return false;
		}
	};

public:
	CEdMassActionContainsComponentCondition()
		: IEdMassActionConditionType()
	{}

	virtual wxString GetName() const
	{
		return wxT("The entity contains a component");
	}

	virtual IEdMassActionCondition*	Create()
	{
		return new Condition( this, m_context );
	}
};

// Mass action condition that asks each node for a node-specific condition
class CEdMassActionNodeSpecificCondition : public IEdMassActionConditionType
{
	class Condition : public IEdMassActionCondition
	{
		wxPanel*	m_panel;
		wxTextCtrl*	m_text;

		void CreatePanel( wxWindow* parent )
		{
			m_panel = new wxPanel( parent );
			m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
			m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

			m_text = new wxTextCtrl( m_panel, wxID_ANY );
			m_panel->GetSizer()->Add( m_text, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
			m_panel->Layout();
		}
			
		virtual void ExportToXML( wxXmlNode* node )
		{
			node->AddAttribute( wxT("conditiontext"), m_text->GetValue() );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			m_text->SetValue( node->GetAttribute( wxT("conditiontext") ) );
		}

	public:
		Condition( IEdMassActionConditionType* type, CEdMassActionContext* context )
			: IEdMassActionCondition( context )
			, m_panel( nullptr )
			, m_text( nullptr )
		{
			m_type = type;
		}

		virtual wxWindow* GetWindow( wxWindow* parent )
		{
			if ( !m_panel )
			{
				CreatePanel( parent );
			}
			return m_panel;
		}

		virtual wxString GetDescription() const
		{
			return wxString::Format( wxT("The node has condition '%s'"), m_text->GetValue() );
		}

		virtual bool Check( IEdMassActionObject* object )
		{
			// Check if the object is an entity
			if ( !object->GetClass()->IsA< CNode >() )
			{
				return false;
			}

			return static_cast< CNode* >( static_cast< CEdMassActionRTTIProxy* >( object )->GetObject() )->CheckMassActionCondition( m_text->GetValue().wc_str() );
		}
	};

public:
	CEdMassActionNodeSpecificCondition()
		: IEdMassActionConditionType()
	{}

	virtual wxString GetName() const
	{
		return wxT("The node has condition");
	}

	virtual IEdMassActionCondition*	Create()
	{
		return new Condition( this, m_context );
	}
};

////////////////////////////////////////////////////////////////////////////


// CEdScenePresetManager
#define PRESET_NONE	(TXT("None"))

static String EscapeStringForDQuotes( const String& str )
{
	String result = str;
	result.ReplaceAll( TXT("\\"), TXT("\\\\"), false );
	result.ReplaceAll( TXT("\""), TXT("\\\""), false );
	return result;
}

static String UnescapeStringForDQuotes( const String& str )
{
	String result = str;
	result.ReplaceAll( TXT("\\\\"), TXT("\\"), false );
	result.ReplaceAll( TXT("\\\""), TXT("\""), false );
	return result;
}

static Bool LayerGroupFullyLoaded( CLayerGroup* group )
{
	const CLayerGroup::TLayerList& layers = group->GetLayers();

	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		if ( (*it)->IsLoaded() == false )
		{
			return false;
		}
	}

	return !layers.Empty();
}

CEdScenePresetManager::SPreset::~SPreset()
{
	Clear();
}

void CEdScenePresetManager::SPreset::Clear()
{
	for ( auto it=m_entries.Begin(); it != m_entries.End(); ++it )
	{
		delete *it;
	}
	m_entries.Clear();
}

void CEdScenePresetManager::SPreset::Assign( const CEdScenePresetManager::SPreset& preset )
{
	Clear();
	m_name = preset.m_name;
	m_root = preset.m_root;
	m_rootHidden = preset.m_rootHidden;
	for ( auto it=preset.m_entries.Begin(); it != preset.m_entries.End(); ++it )
	{
		SPresetEntry* entry = new SPresetEntry();
		entry->m_hidden = (*it)->m_hidden;
		entry->m_path = (*it)->m_path;
		m_entries.PushBack( entry );
	}
}

String CEdScenePresetManager::SPreset::ToString() const
{
	// Add name
	String data = String::Printf( TXT("{\"%s\""), EscapeStringForDQuotes( m_name ).AsChar() );

	// Add root flag
	data += m_root ? TXT(" load-root") : TXT(" no-root");

	// Add root hidden paths
	if ( m_root && !m_rootHidden.Empty() )
	{
		data += L'[';
		for ( auto it=m_rootHidden.Begin(); it != m_rootHidden.End(); ++it )
		{
			if ( it != m_rootHidden.Begin() )
			{
				data += L' ';
			}
			data += String::Printf( TXT("\"%s\""), EscapeStringForDQuotes( (*it) ).AsChar() );
		}
		data += L']';
	}

	// Add all entries
	for ( auto it=m_entries.Begin(); it != m_entries.End(); ++it )
	{
		data += String::Printf( TXT(" \"%s\""), EscapeStringForDQuotes( (*it)->m_path ).AsChar() );

		// Add hidden entries
		if ( !(*it)->m_hidden.Empty() )
		{
			data += L'[';
			for ( auto hit=(*it)->m_hidden.Begin(); hit != (*it)->m_hidden.End(); ++hit )
			{
				if ( hit != (*it)->m_hidden.Begin() )
				{
					data += L' ';
				}
				data += String::Printf( TXT("\"%s\""), EscapeStringForDQuotes( (*hit) ).AsChar() );
			}
			data += L']';
		}
	}

	// Close the list
	data += L'}';

	return data;
}

static Bool ParseScenePresetHiddenListString( CCodeParser& parser, TDynArray< String >& hidden )
{
	parser.SkipWhitespace();

	// Check if there is the beginning of a hidden list
	if ( parser.GetNextCharacter() == L'[' )
	{
		// Skip [
		parser.Skip();

		// Parse list
		while ( parser.HasMore() ) 
		{
			parser.SkipWhitespace();
			if ( parser.GetNextCharacter() == ']' )
			{
				break;
			}

			// Parse and add hidden layer path
			hidden.PushBack( UnescapeStringForDQuotes( parser.ScanToken() ) );
		}

		// Make sure we are at a valid end-of-list
		if ( parser.GetNextCharacter() != ']' )
		{
			return false;
		}

		// Skip ]
		parser.Skip();
	}

	return true;
}

Bool CEdScenePresetManager::SPreset::FromString( const String& data )
{
	// Prepare the parser
	CCodeParser parser( data );
	parser.AddDelimiters( TXT("[]{}\"") );
	parser.SetParseStrings();
	parser.SetIncludeStringQuotes( false );

	// Make sure we have a list there
	parser.SkipWhitespace();
	if ( parser.GetNextCharacter() != L'{' )
	{
		return false;
	}
	parser.Skip();

	// Scan the name
	m_name = UnescapeStringForDQuotes( parser.ScanToken() );

	// Scan the root flag
	m_root = parser.ScanToken() == TXT("load-root");

	// Parse root hidden layers
	if ( !ParseScenePresetHiddenListString( parser, m_rootHidden ) )
	{
		Clear();
		return false;
	}

	// Scan the layer group entries
	Clear();
	while ( parser.HasMore() )
	{
		parser.SkipWhitespace();

		// Check end of list
		if ( parser.GetNextCharacter() == L'}' )
		{
			break;
		}
		else
		{
			// Parse path
			SPresetEntry* entry = new SPresetEntry();
			entry->m_path = UnescapeStringForDQuotes( parser.ScanToken() );

			// Check for hidden layers list
			if ( !ParseScenePresetHiddenListString( parser, entry->m_hidden ) )
			{
				Clear();
				delete entry;
				return false;
			}

			// Store entry
			m_entries.PushBack( entry );
		}
	}

	// Make sure we got a full list, not just some truncated part
	if ( parser.GetNextCharacter() != L'}' )
	{
		Clear();
		return false;
	}

	return true;
}

void CEdScenePresetManager::Clear()
{
	for ( auto it=m_presets.Begin(); it != m_presets.End(); ++it )
	{
		delete *it;
	}
	m_presets.Clear();
}

CEdScenePresetManager::SPreset* CEdScenePresetManager::Find( const String& name ) const
{
	// NOTE: linear search because of the assumption that there will be few
	//       presets per level to not guarantee the overhead of a hashmap
	for ( auto it=m_presets.Begin(); it != m_presets.End(); ++it )
	{
		if ( (*it)->m_name == name )
		{
			return *it;
		}
	}
	return NULL;
}

CEdScenePresetManager::SPreset* CEdScenePresetManager::FindOrCreate( const String& name )
{
	SPreset* preset = Find( name );
	if ( !preset )
	{
		preset = new SPreset( name );
		m_presets.PushBack( preset );
	}
	return preset;
}

CEdScenePresetManager::CEdScenePresetManager()
	: m_active( NULL )
{
}

CEdScenePresetManager::~CEdScenePresetManager()
{
	Clear();
}

void CEdScenePresetManager::SetActive( const String& name, Bool create )
{
	m_active = create ? FindOrCreate( name ) : Find( name );

	// Create a new preset if the given preset was not found
	if ( !m_active )
	{
		m_active = Find( PRESET_NONE );
	}

	RestoreGroupsFrom( m_active->m_name );
}

const String& CEdScenePresetManager::GetActive() const
{
	return m_active->m_name;
}

void CEdScenePresetManager::GetPresetNames( TDynArray< String >& names ) const
{
	for ( auto it=m_presets.Begin(); it != m_presets.End(); ++it )
	{
		names.PushBack( (*it)->m_name );
	}
}

void CEdScenePresetManager::Reset()
{
	Clear();
	m_presets.PushBack( new SPreset( PRESET_NONE ) );
	m_active = Find( PRESET_NONE );
}

namespace 
{
	static bool HasLoadedLayers( const CLayerGroup* group )
	{
		const CLayerGroup::TLayerList& layers = group->GetLayers();
		for ( auto it = layers.Begin(); it != layers.End(); ++it )
		{
			if ( (*it)->IsLoaded() )
			{
				return true;
			}
		}

		return false;
	}
}
void CEdScenePresetManager::StoreGroupsTo( const String& name )
{
	// Make sure the preset doesn't have an empty name and isn't "None"
	ASSERT( !name.Empty() );
	if ( name.Empty() || name == PRESET_NONE )
	{
		return;
	}

	// Get the world
	CWorld *world = GGame ? GGame->GetActiveWorld() : NULL;
	if ( world == NULL )
	{
		return;
	}

	// Find/create the requested preset
	SPreset* preset = FindOrCreate( name );

	// Get all global layer groups and put them to the preset
	TDynArray< CLayerGroup* > groups;
	world->GetWorldLayers()->GetLayerGroups( groups );

	// ...root
	TDynArray< CLayerInfo* > layers;
	world->GetWorldLayers()->GetLayers( layers, true, false, true );
	preset->m_root = LayerGroupFullyLoaded( world->GetWorldLayers() );
	preset->m_rootHidden.Clear();
	if ( preset->m_root )
	{
		for ( auto it=layers.Begin(); it != layers.End(); ++it )
		{
			if ( (*it)->IsVisible() == false )
			{
				preset->m_rootHidden.PushBack( (*it)->GetDepotPath() );
			}
		}
	}

	// ...subgroups
	preset->Clear();
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;

		if ( HasLoadedLayers( group ) )
		{
			SPresetEntry* entry = new SPresetEntry();
			entry->m_path = group->GetDepotPath();

			TDynArray< CLayerInfo* > layers;
			group->GetLayers( layers, false, false, true );

			for ( auto it=layers.Begin(); it != layers.End(); ++it )
			{
				if ( (*it)->IsVisible() == false )
				{
					entry->m_hidden.PushBack( (*it)->GetDepotPath() );
				}
			}

			preset->m_entries.PushBack( entry );
		}
	}
}

void CEdScenePresetManager::RestoreGroupsFrom( const String& name )
{
	// Make sure there is a world out there
	CWorld *world = GGame ? GGame->GetActiveWorld() : NULL;
	if ( world == NULL )
	{
		return;
	}

	// Find the requested preset or fail
	SPreset* preset = Find( name );
	if ( !preset )
	{
		return;
	}

	// List of layers referenced in the operations, regardless if they are loaded or not
	// This is used to update the hidden flag down the road without enumerating every single
	// layer in the world - also always add the root layers since they are implicitly
	// referenced by all presets
	TDynArray< CLayerInfo* > layersReferenced;
	world->GetWorldLayers()->GetLayers( layersReferenced, false, false, true);

	// Get all global layer groups and check which need to be loaded and unloaded
	TDynArray< CLayerGroup* > groups;
	TDynArray< SPresetEntry* > entriesToLoad = preset->m_entries;
	TDynArray< CLayerGroup* > groupsToUnload;
	world->GetWorldLayers()->GetLayerGroups( groups );
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;
		
		if ( HasLoadedLayers( group ) )
		{
			const String path = group->GetDepotPath();

			// Check if the preset has an entry with the group's path
			Bool found = false;
			for ( auto it=preset->m_entries.Begin(); it != preset->m_entries.End(); ++it )
			{
				const CFilePath prevPath( (*it)->m_path );
				const String prePathDir = prevPath.GetPathString( TXT("\\") );

				if ( path.EqualsNC( prePathDir ) )
				{
					entriesToLoad.Remove( *it ); // group is on the loading list but it's already loaded, remove it
					found = true;
					break;
				}
			}

			// Group doesn't exist, mark it for unload
			if ( !found ) 
			{
				groupsToUnload.PushBack( group );
			}
		}
	}

	// Update world
	world->DelayedActions();

	// Unload groups to be unloaded
	if ( !groupsToUnload.Empty() )
	{
		for ( auto it=groupsToUnload.Begin(); it != groupsToUnload.End(); ++it )
		{
			(*it)->SyncUnload();
		}
		world->UpdateLoadingState();
		world->DelayedActions();
	}

	// Unload root if not in the preset
	if ( !preset->m_root && LayerGroupFullyLoaded( world->GetWorldLayers() ) )
	{
		// Only unload the immediate layers, not everything
		TDynArray< CLayerInfo* > rootLayers;
		world->GetWorldLayers()->GetLayers( rootLayers, true, false, true );
		for ( auto it=rootLayers.Begin(); it != rootLayers.End(); ++it )
		{
			(*it)->SyncUnload();
		}
		world->UpdateLoadingState();
		world->DelayedActions();
	}

	// List with layers to hide after everything is said and done
	THashSet< String > layersToHide;

	// Add layers to hide from the entries
	for ( auto it=preset->m_entries.Begin(); it != preset->m_entries.End(); ++it )
	{
		for ( auto hit=(*it)->m_hidden.Begin(); hit != (*it)->m_hidden.End(); ++hit )
		{
			layersToHide.Insert( *hit );
		}
	}

	// Add layers to hide from the root
	for ( auto hit=preset->m_rootHidden.Begin(); hit != preset->m_rootHidden.End(); ++hit )
	{
		layersToHide.Insert( *hit );
	}

	// Load groups to be loaded
	if ( !entriesToLoad.Empty() )
	{
		TDynArray< CLayerInfo* > layersToLoad;

		// Collect all layers to load
		for ( auto it=entriesToLoad.Begin(); it != entriesToLoad.End(); ++it )
		{
			// Convert stored path to group path
			const CFilePath storedPath( (*it)->m_path );
			String fullPath = storedPath.GetPathString( TXT("\\") );

			// Skip the world path
			const String worldDepotPath = world->GetFile()->GetDirectory()->GetDepotPath();
			String shortPath;
			
			// Special case for DLC
			if ( fullPath.BeginsWith( TXT("dlc") ) && !worldDepotPath.BeginsWith( TXT("dlc") ) )
			{
				String worldName = world->GetFile()->GetFileName();
				worldName.Replace( TXT(".w2w"), TXT("") );
				shortPath = fullPath;
				shortPath.Replace( TXT("dlc\\"), TXT("DLC\\") );
				shortPath.Replace( TXT("\\levels\\") + worldName, TXT("") );
			}
			else
			{
				shortPath = fullPath.StringAfter( worldDepotPath );
			}

			// Get layer group resource
			CLayerGroup* group = GGame->GetActiveWorld()->GetWorldLayers()->FindGroupByPath( shortPath );
			if ( !group )
			{
				continue;
			}

			// Get the list of layers to load
			TDynArray< CLayerInfo* > groupLayersToLoad;
			group->GetLayers( groupLayersToLoad, false, false, true );
			layersToLoad.PushBackUnique( groupLayersToLoad );
		}

		// Load layers
		LayerGroupLoadingContext loadingContext;
		loadingContext.m_loadHidden = true;
		LoadLayers( layersToLoad, loadingContext );
		layersReferenced.PushBack( layersToLoad );

		// Update world
		world->UpdateLoadingState();
		world->DelayedActions();
	}

	// Load the root if is in the preset
	if ( preset->m_root && !LayerGroupFullyLoaded( world->GetWorldLayers() ) )
	{
		TDynArray< CLayerInfo* > groupLayersToLoad;
		world->GetWorldLayers()->GetLayers( groupLayersToLoad, false, false, true );
		
		LayerGroupLoadingContext loadingContext;
		loadingContext.m_loadHidden = true;
		LoadLayers( groupLayersToLoad, loadingContext );

		world->UpdateLoadingState();
		world->DelayedActions();
	}

	// Show/Hide layers
	for ( auto it=layersReferenced.Begin(); it != layersReferenced.End(); ++it )
	{
		(*it)->Show( !layersToHide.Exist( (*it)->GetDepotPath() ) );
	}

	// Flush collision cache
	GCollisionCache->Flush();

	// Run the collector
	SGarbageCollector::GetInstance().CollectNow();
}

void CEdScenePresetManager::Remove( const String& name )
{
	// Make sure we're not trying to remove "None"
	if ( name == PRESET_NONE )
	{
		return;
	}
	
	// If the preset is the active one, activate None
	if ( name == GetActive() )
	{
		SetActive( PRESET_NONE, false );
	}

	// Forget the preset
	SPreset* preset = Find( name );
	m_presets.Remove( preset );
	delete preset;
}

Bool CEdScenePresetManager::Rename( const String& oldName, const String& newName )
{
	// Try to find the preset
	SPreset* preset = Find( oldName );
	if ( !preset )
	{
		return false;
	}

	// Make sure no other preset with that name exists
	if ( Find( newName ) )
	{
		return false;
	}

	// Set the new name
	preset->m_name = newName;

	return true;
}

Bool CEdScenePresetManager::SetPresetFromString( const String& data, String& parsedPresetName )
{
	// Make sure we get no empty stuff
	if ( data.Empty() )
	{
		parsedPresetName = String::EMPTY;
		return false;
	}

	// Create temporary preset from the data
	SPreset tempPreset;
	if ( !tempPreset.FromString( data )  )
	{
		return false;
	}

	// Make sure we have proper data
	if ( tempPreset.m_name.Empty() || ( tempPreset.m_entries.Empty() && !tempPreset.m_root ) )
	{
		parsedPresetName = String::EMPTY;
		return false;
	}

	// Find or create preset with the name
	SPreset* preset = FindOrCreate( tempPreset.m_name );
	preset->Assign( tempPreset );
	parsedPresetName = preset->m_name;

	return true;
}

String CEdScenePresetManager::GetPresetAsString( const String& name ) const
{
	SPreset* preset = Find( name );
	return preset ? preset->ToString() : String::EMPTY;
}

void CEdScenePresetManager::Save()
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world && world->GetFile() )
	{
		// Get preset configuration files
		String absoluteConfigFilePath = GFileManager->GetBaseDirectory() + GGameConfig::GetInstance().GetName() + TXT("LavaEditor2.presets.ini");
		wxString localFilename = absoluteConfigFilePath.AsChar();
		wxFileConfig *config = new wxFileConfig( wxTheApp->GetAppName(), wxT("CDProjektRed"), localFilename, wxEmptyString, wxCONFIG_USE_LOCAL_FILE );

		// Clean presets for this session
		String path = TXT("Session/") + world->GetFile()->GetDepotPath();
		config->DeleteGroup( path.AsChar() );
		config->SetPath( path.AsChar() );

		// Save all presets
		String presetList = String::EMPTY;
		config->Write( wxT("ScenePresetCount"), m_presets.SizeInt() );
		for ( Int32 i=0; i<m_presets.SizeInt(); ++i )
		{
			SPreset* preset = m_presets[i];

			// Ignore the None preset
			if ( preset->m_name == PRESET_NONE )
			{
				continue;
			}

			// Save preset as string
			config->Write( wxString::Format( wxT("ScenePreset%d"), i ), preset->ToString().AsChar() );
		}

		// Save active preset
		config->Write( wxT("ActiveScenePresetName"), m_active->m_name.AsChar() );

		delete config;
		config = NULL;
	}
}

void CEdScenePresetManager::Load()
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world && world->GetFile() )
	{
		// Get preset configuration files
		String absoluteConfigFilePath = GFileManager->GetBaseDirectory() + GGameConfig::GetInstance().GetName() + TXT("LavaEditor2.presets.ini");
		wxString localFilename = absoluteConfigFilePath.AsChar();
		wxFileConfig *config = new wxFileConfig( wxTheApp->GetAppName(), wxT("CDProjektRed"), localFilename, wxEmptyString, wxCONFIG_USE_LOCAL_FILE );

		// Clean presets for this session
		String path = TXT("Session/") + world->GetFile()->GetDepotPath();
		config->SetPath( path.AsChar() );

		// Load all presets
		String presetList = String::EMPTY;
		Int32 presetCount = static_cast<int>( config->ReadLong( wxT("ScenePresetCount"), 0 ) );
		Reset();
		for ( Int32 i=0; i<presetCount; ++i )
		{
			String presetData = config->Read( wxString::Format( wxT("ScenePreset%d"), i ), wxT("") ).wc_str();
			SPreset* preset = new SPreset();

			if ( preset->FromString( presetData ) )
			{
				// Sanity check
				if ( preset->m_name.Empty() || preset->m_name == PRESET_NONE || ( preset->m_entries.Empty() && !preset->m_root ) )
				{
					delete preset;
					continue;
				}

				m_presets.PushBack( preset );
			}
		}

		// Set active preset
		String activePresetName = config->Read( wxT("ActiveScenePresetName"), PRESET_NONE ).wc_str();
		SetActive( activePresetName, false );

		delete config;
		config = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////

// CEdSceneExplorer
CEdSceneExplorer::CEdSceneExplorer( wxWindow* parent )
	: CDropTarget( this )
	, m_presetComboBox( NULL )
	, m_entityListManager( nullptr )
{
    // Load layout from XRC
    wxXmlResource::Get()->LoadPanel( this, parent, wxT("SceneExplorer") );

    // Preset combobox
    m_presetComboBox = XRCCTRL( *this, "presetComboBox", wxComboBox );
	m_presetComboBox->SetWindowStyle( wxCB_DROPDOWN | wxTE_PROCESS_ENTER );
    ClearPresets();
    UpdatePresetsUI();

    // Scene tree
    wxTreeListCtrl* sceneTree = XRCCTRL( *this, "SceneTree", wxTreeListCtrl );
	wxWindow* par = sceneTree->GetParent();
	sceneTree->Destroy();

	m_sceneView = new SceneView( par );
	par->GetSizer()->Add( m_sceneView, 1, wxEXPAND );

    // Register event listeners
    SEvents::GetInstance().RegisterListener( CNAME( ActiveLayerChanged ), this );
    SEvents::GetInstance().RegisterListener( CNAME( LayerChildrenChanged ), this );
    SEvents::GetInstance().RegisterListener( CNAME( LayerRemoved ), this );
    SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanged ), this );
    SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanging ), this );
    SEvents::GetInstance().RegisterListener( CNAME( LayerVisibilityChanged ), this );
    SEvents::GetInstance().RegisterListener( CNAME( NodeNameChanged ), this );	
    SEvents::GetInstance().RegisterListener( CNAME( NodeTagsChanged ), this );	
    SEvents::GetInstance().RegisterListener( CNAME( UpdateSceneTree ), this );	
    SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
    SEvents::GetInstance().RegisterListener( CNAME( VersionControlStatusChanged ), this);
    SEvents::GetInstance().RegisterListener( CNAME( LayerLoaded ), this);
    SEvents::GetInstance().RegisterListener( CNAME( LayerUnloading ), this);
    SEvents::GetInstance().RegisterListener( CNAME( LayerUnloaded ), this);
    SEvents::GetInstance().RegisterListener( CNAME( ResourceReloaded ), this);
	SEvents::GetInstance().RegisterListener( CNAME( GameEnded ), this);
	SEvents::GetInstance().RegisterListener( CNAME( FileModified ), this );
	SEvents::GetInstance().RegisterListener( CNAME( OnObjectDiscarded ), this );

    Layout();
    Show();
}

CEdSceneExplorer::~CEdSceneExplorer()
{
	SEvents::GetInstance().UnregisterListener( this );
	if ( m_duplicatesErrorsList )
	{
		delete m_duplicatesErrorsList;
	}
}

void CEdSceneExplorer::OnInternalIdle()
{
}

void CEdSceneExplorer::UpdateLayer( CLayer* layer )
{
	m_sceneView->RefreshLater();
}

CLayerInfo* CEdSceneExplorer::GetActiveLayerInfo()
{
    return m_sceneView->GetActiveLayer();
}

CLayer* CEdSceneExplorer::GetActiveLayer()
{
    return ( GetActiveLayerInfo() && GetActiveLayerInfo()->IsLoaded() ) ? GetActiveLayerInfo()->GetLayer() : NULL;
}

void CEdSceneExplorer::ChangeActiveLayer( CLayerInfo* layerInfo )
{
	m_sceneView->SetActiveLayer( layerInfo );
}

void CEdSceneExplorer::SelectSceneObject( ISerializable* object, Bool clearSelection /*= false*/ )
{
	if ( clearSelection )
	{
		m_sceneView->ClearSelection();
	}
	m_sceneView->Select( object );
}

void CEdSceneExplorer::DeselectSceneObject( ISerializable* object )
{
	m_sceneView->Deselect( object );
}

void CEdSceneExplorer::AddSceneObject( ISerializable *object )
{
	m_sceneView->RefreshLater();
}

void CEdSceneExplorer::RemoveSceneObject( ISerializable *object )
{
	m_sceneView->RefreshLater();
}


// Actions handler for entity lists managed by the scene explorer
static void WorldEntityListMassActionsHandler( CEntityList* entityList, const LayerEntitiesArray& entities, void* userData )
{
	CEdSceneExplorer* explorer = (CEdSceneExplorer*)userData;
	explorer->OnEntityListMassActions( entityList, entities );
}

void CEdSceneExplorer::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
    if( name == CNAME( ActiveWorldChanging ) )
    {
    }
    // World was changed
    else if ( name == CNAME( ActiveWorldChanged ) )
    {
		// Clear global filter
		ClearGlobalFilter();

		// (Re)create entity list manager
		delete m_entityListManager;
		m_entityListManager = nullptr;
		if ( GGame->GetActiveWorld() != nullptr )
		{
			m_entityListManager = new CEntityListManager( GGame->GetActiveWorld() );
			m_entityListManager->SetActionsHandler( WorldEntityListMassActionsHandler, this );
		}

        // set scene tab as active after level loaded
        wxTheFrame->GetSolutionBar()->SetSelection( 0 );

		// Reset the scene view state
		m_sceneView->ResetState();

        // Update presets UI
        UpdatePresetsUI();
    }
	
	if ( name == CNAME( ActiveLayerChanged ) )
    {
		m_sceneView->RefreshLater();
    }
    else if ( name == CNAME( NodeNameChanged ) || name == CNAME( NodeTagsChanged ))
    {
		m_sceneView->RefreshLater();
    }
    else if ( name == CNAME( UpdateSceneTree ) )
    {
		m_sceneView->RefreshLater();
    }
    else if ( name == CNAME( VersionControlStatusChanged ) )
    {
		m_sceneView->RefreshLater();
    }
    else if ( name == CNAME( LayerVisibilityChanged ) )
    {
		m_sceneView->RefreshLater();
    }
	else if ( name == CNAME( LayerChildrenChanged ) )
	{	    
		m_sceneView->RefreshLater();
	}	    
	else if ( name == CNAME( LayerRemoved ) )
	{	    
		CLayerInfo* layer = GetEventData< CLayerInfo* >( data );
		if ( layer == m_sceneView->GetActiveLayer() )
		{
			m_sceneView->SetActiveLayer( NULL );
		}
		m_sceneView->Deselect( layer );
	}	    
	else if ( name == CNAME( LayerLoaded ) )
	{	    
		m_sceneView->RefreshLater();
	}	    
	else if ( name == CNAME( GameEnded ) )
	{	    
		m_sceneView->RefreshLater();
	}	    
	else if ( name == CNAME( LayerUnloading ) )
	{	    
		if ( GetEventData< CLayerInfo* >( data ) == m_sceneView->GetActiveLayer() )
		{
			m_sceneView->SetActiveLayer( NULL );
		}
		m_sceneView->RefreshLater();
	}	    
	else if ( name == CNAME( LayerUnloaded ) )
	{	    
		m_sceneView->RefreshLater();
	}	    
	else if ( name == CNAME( ResourceReloaded ) )
    {
		m_sceneView->RefreshLater();
    }
	else if ( name == CNAME( FileModified ) )
	{
		m_sceneView->RefreshLater();
	}
	else if ( name == CNAME( SelectionChanged ) )
	{
		m_sceneView->RefreshLater();
	}
	else if ( name == CNAME( OnObjectDiscarded ) )
	{
		ISerializable *obj = GetEventData< ISerializable* >( data );

		if ( m_sceneView->IsSelected( obj ) )
		{
			m_sceneView->Deselect( obj );
		}
	}
}

void CEdSceneExplorer::UpdateSelected( Bool expandToSelection )
{
	// Get selection manager or exit
	CSelectionManager* manager = GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetSelectionManager() : NULL;
	if ( manager == NULL )
	{
		return;
	}

	// Get selected nodes
	TDynArray< CNode* > nodes;
	manager->GetSelectedNodes( nodes );

	// Set sceneview selection to nodes
	m_sceneView->ClearSelection();
	for ( auto it=nodes.Begin(); it != nodes.End(); ++it )
	{
		m_sceneView->Select( *it );
	}

	// Expand sceneview nodes to show the selected stuff
	if ( expandToSelection )
	{
		m_sceneView->ExpandToSelection();
	}
}

void CEdSceneExplorer::GetSelection( TDynArray< ISerializable* >& selection )
{
	auto theSelection = m_sceneView->GetSelection();
	for ( auto it=theSelection.Begin(); it != theSelection.End(); ++it )
	{
		selection.PushBack( *it );
	}
}

void CEdSceneExplorer::OnEditEncounter( wxCommandEvent& event )
{
	CEncounter* encounter = NULL;

	{
		TDynArray< CEntity* > entitySelection;
		m_sceneView->GetSelection( entitySelection );

		for ( auto nodeIt = entitySelection.Begin(); nodeIt != entitySelection.End(); ++nodeIt )
		{
			encounter = Cast< CEncounter >( *nodeIt );
			if ( encounter )
			{
				break;
			}
		}
	}

	if ( encounter )
	{
		wxTheFrame->GetWorldEditPanel()->OpenSpawnTreeEditor( encounter );
	}
}

void CEdSceneExplorer::OnChangeSwarmPOIType( wxCommandEvent& event )
{
	

	String poiName = InputBox( this, TXT( "Change POI name" ), TXT( "Please enter the name of your spawn point without the number ( ex: if you want FishSpawn1,2,3 enter FishSpawn )" ), TXT( "FishSpawn" ) );

	TDynArray< CEntity* > entitySelection;
	m_sceneView->GetSelection( entitySelection );
	Int32 suffix = 0;
	for ( auto nodeIt = entitySelection.Begin(); nodeIt != entitySelection.End(); ++nodeIt )
	{
		CEntity *entity		=  *nodeIt;
		CLayer *const layer = entity->GetLayer();
		if ( layer )
		{
			layer->MarkModified();
		}
		for ( ComponentIterator< CBoidPointOfInterestComponent > it( entity ); it; ++it )
		{
			CBoidPointOfInterestComponent *const poiComponent = *it;
			
			
			if ( poiComponent->GetParameters().m_type == CNAME( FlyingSpawn1 ) )
			{
				suffix = 1;	
			}
			else if ( poiComponent->GetParameters().m_type == CNAME( FlyingSpawn2 ) )
			{
				suffix = 2;	
			}
			else if ( poiComponent->GetParameters().m_type == CNAME( FlyingSpawn3 ) )
			{
				suffix = 3;	
			}
			else if ( poiComponent->GetParameters().m_type == CNAME( FlyingSpawn4 ) )
			{
				suffix = 4;	
			}
			else if ( poiComponent->GetParameters().m_type == CNAME( FlyingSpawn5 ) )
			{
				suffix = 5;	
			}
			else if ( poiComponent->GetParameters().m_type == CNAME( FlyingSpawn6 ) )
			{
				suffix = 6;	
			}
			else if ( poiComponent->GetParameters().m_type == CNAME( FlyingSpawn7 ) )
			{
				suffix = 7;	
			}
			else if ( poiComponent->GetParameters().m_type == CNAME( FlyingSpawn8 ) )
			{
				suffix = 8;	
			}
			else if ( poiComponent->GetParameters().m_type == CNAME( FlyingSpawn9 ) )
			{
				suffix = 9;
			}
			else
			{
				++suffix;
			}
			String fullPoiName = poiName + String::Printf( TXT("%d"), suffix );
			CName  name;
			name.Set( fullPoiName.AsChar() );
			poiComponent->ChangePoiTypeFromTool( name );
		}
	}
}

void FixLair( IBoidLairEntity * boidLairEntity )
{
	boidLairEntity->DetachTemplate();
	boidLairEntity->GetLayer()->MarkModified();
	CSwarmRenderComponent * swarmRenderComponent				= boidLairEntity->FindComponent< CSwarmRenderComponent >(  );
	if ( swarmRenderComponent )
	{
		swarmRenderComponent->SetStreamed( false );
	}
	else
	{
		swarmRenderComponent = static_cast< CSwarmRenderComponent * >( boidLairEntity->CreateComponent( ClassID< CSwarmRenderComponent >(), SComponentSpawnInfo() ) );
	}

	CSoundEmitterComponent * swarmSoundEmitterComponent	= boidLairEntity->FindComponent< CSoundEmitterComponent >(  );
	if ( swarmSoundEmitterComponent )
	{
		if ( swarmSoundEmitterComponent->IsA< CSwarmSoundEmitterComponent >( ) == false )
		{
			boidLairEntity->RemoveComponent( swarmSoundEmitterComponent );
			swarmSoundEmitterComponent = nullptr;
		}
		else
		{
			swarmSoundEmitterComponent->SetStreamed( false );
		}
	}
	if ( swarmSoundEmitterComponent == nullptr )
	{
		swarmSoundEmitterComponent = static_cast< CSwarmSoundEmitterComponent * >( boidLairEntity->CreateComponent( ClassID< CSwarmSoundEmitterComponent >(), SComponentSpawnInfo() ) );
	}
}

void CEdSceneExplorer::OnFixAllSwarmLairs( wxCommandEvent& event )
{
	for ( WorldAttachedEntitiesIterator it( GGame->GetActiveWorld() ); it; ++it ) 
	{ 
		CEntity* entity = *it; 
		if ( entity->IsA< IBoidLairEntity >() )
		{
			FixLair( static_cast< IBoidLairEntity* >( entity ) );
		}
	}
}

void CEdSceneExplorer::OnGenerateCompatibilityErrorsReport( wxCommandEvent& event )
{
	if ( GGame->GetActiveWorld() )
	{
		CLayerGroup* worldLayers = GGame->GetActiveWorld()->GetWorldLayers();
		TDynArray< CLayerInfo* > layers;
		worldLayers->GetLayers( layers, false );

		CheckLayersEntitiesCompatibility( layers, false );
	}
}

void CEdSceneExplorer::OnFixSwarm( wxCommandEvent& event )
{
	TDynArray< CEntity* > entitySelection;
	m_sceneView->GetSelection( entitySelection );
	Int32 lairEntityCount = 0;

	// Creating missing component
	for ( auto nodeIt = entitySelection.Begin(); nodeIt != entitySelection.End(); ++nodeIt )
	{
		IBoidLairEntity * lairEntity = Cast< IBoidLairEntity >( *nodeIt );
		if ( lairEntity )
		{
			lairEntityCount++;

			FixLair( lairEntity );
		}
	}

	if ( lairEntityCount == 1 )
	{
		// Fix links
		IBoidLairEntity * lairEntity										= nullptr;
		CBoidActivationTriggerComponent * boidActivationTriggerComponent	= nullptr;
		CBoidAreaComponent * areaComponent									= nullptr;
		CFlyingCrittersLairEntity *flyingLairEntity							= nullptr;
		for ( auto nodeIt = entitySelection.Begin(); nodeIt != entitySelection.End(); ++nodeIt )
		{
			CEntity *entity = *nodeIt;
			IBoidLairEntity * localLairEntity		= Cast< IBoidLairEntity >( entity );
			if ( localLairEntity )
			{
				lairEntity = localLairEntity;
				flyingLairEntity = Cast< CFlyingCrittersLairEntity >( lairEntity );
			}
			CBoidActivationTriggerComponent * localBoidActivationTriggerComponent				= entity->FindComponent< CBoidActivationTriggerComponent >(  );
			if ( localBoidActivationTriggerComponent )
			{
				boidActivationTriggerComponent = localBoidActivationTriggerComponent;
			}

			CBoidAreaComponent * localAreaComponent				= entity->FindComponent< CBoidAreaComponent >(  );
			if ( localAreaComponent )
			{
				areaComponent = localAreaComponent;
			}
		}

		if ( lairEntity && boidActivationTriggerComponent && areaComponent )
		{
		
			lairEntity->SetupFromTool( areaComponent->GetEntity() );
			boidActivationTriggerComponent->SetupFromTool( lairEntity );
			if ( flyingLairEntity )
			{
				flyingLairEntity->OnGenerateSwarmCollisions();
			}
			lairEntity->GetLayer()->MarkModified();
		}
	}
}

void CEdSceneExplorer::OnGenerateSwarmCollisions( wxCommandEvent& event )
{
	CFlyingCrittersLairEntity* flyingLairEntity = NULL;
	{
		TDynArray< CEntity* > entitySelection;
		m_sceneView->GetSelection( entitySelection );

		for ( auto nodeIt = entitySelection.Begin(); nodeIt != entitySelection.End(); ++nodeIt )
		{
			flyingLairEntity = Cast< CFlyingCrittersLairEntity >( *nodeIt );
			if ( flyingLairEntity )
			{
				break;
			}
		}
	}
	
	if ( flyingLairEntity )
	{
		if ( flyingLairEntity->OnGenerateSwarmCollisions() )
		{
			flyingLairEntity->GetLayer()->MarkModified();
			flyingLairEntity->GetLayer()->Save();
		}
	}
}

void CEdSceneExplorer::OnShowStreamingInfo( wxCommandEvent& event )
{
	TDynArray< CEntity* > entitySelection;
	m_sceneView->GetSelection( entitySelection );

	String html( TXT("<html><head></head><body><h1>Resource streaming info</h1><table width='100%' border='1'>")
		TXT("<tr><td><b>Entity</b></td><td>Streamed Components</td></tr>") );

	for ( auto it=entitySelection.Begin(); it != entitySelection.End(); ++it )
	{
		class CEntityPeeker : public CEntity
		{
		public:
			String PeekInformation()
			{
				String html = String::Printf( TXT("<tr><td>%s<br>Streaming Distance: %i</td><td>"), GetFriendlyName().AsChar(), (int)GetStreamingDistance() );
				int dataSize = (int)m_streamingDataBuffer.GetSize();
				html += String::Printf( TXT("<b>Streamed data of %i bytes:</b><br>"), dataSize );
				for ( auto it=m_streamingComponents.Begin(); it != m_streamingComponents.End(); ++it )
				{
					CComponent* component = (*it).Get();
					CMeshComponent* meshComponent = Cast< CMeshComponent >( component );
					if ( component == nullptr )
					{
						continue;
					}

					html += component->GetFriendlyName();
					html += String::Printf( TXT(" Streaming distance: <b>%i</b> "), component->GetMinimumStreamingDistance() );

					if ( meshComponent != nullptr && meshComponent->GetMeshNow() != nullptr && meshComponent->GetMeshNow()->GetFile() != nullptr )
					{
						html += String::Printf( TXT(" (%s)"), meshComponent->GetMeshNow()->GetFile()->GetDepotPath().AsChar() );
					}

					html += TXT("<br>");
				}
				html + TXT("<br></td></tr>");
				return html;
			}
		};

		CEntityPeeker* entity = (CEntityPeeker*)*it;
		if ( !entity->ShouldBeStreamed() )
		{
			continue;
		}

		SEntityStreamingState streamingState;
		entity->PrepareStreamingComponentsEnumeration( streamingState, true );
		entity->ForceFinishAsyncResourceLoads();
		html += entity->PeekInformation();
		entity->FinishStreamingComponentsEnumeration( streamingState );
	}

	html += TXT("</table></body></html>");
	HtmlBox( this, TXT("Streaming Information"), html );
}

void CEdSceneExplorer::OnForceStreamIn( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities;
	m_sceneView->CollectEntitiesFromSelection( entities );
	GFeedback->BeginTask( TXT("Forced stream in"), true );
	for ( Uint32 i=0; i < entities.Size(); ++i )
	{
		CEntity* entity = entities[i];
		if ( GFeedback->IsTaskCanceled() ) break;
		GFeedback->UpdateTaskInfo( entity->GetFriendlyName().AsChar() );
		GFeedback->UpdateTaskProgress( i, entities.Size() - 1 );
		entity->CreateStreamedComponents( SWN_NotifyWorld );
	}
	GFeedback->EndTask();
}

void CEdSceneExplorer::OnForceStreamOut( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities;
	m_sceneView->CollectEntitiesFromSelection( entities );
	GFeedback->BeginTask( TXT("Forced stream out"), true );
	for ( Uint32 i=0; i < entities.Size(); ++i )
	{
		CEntity* entity = entities[i];
		if ( GFeedback->IsTaskCanceled() ) break;
		GFeedback->UpdateTaskInfo( entity->GetFriendlyName().AsChar() );
		GFeedback->UpdateTaskProgress( i, entities.Size() - 1 );
		entity->DestroyStreamedComponents( SWN_NotifyWorld );
	}
	GFeedback->EndTask();
}

void CEdSceneExplorer::OnIgnoreStream( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities;
	m_sceneView->CollectEntitiesFromSelection( entities );
	for ( CEntity* entity : entities )
	{
		entity->GetLayer()->GetWorld()->IgnoreEntityStreaming( entity );
	}
}

void CEdSceneExplorer::OnUnignoreStream( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities;
	m_sceneView->CollectEntitiesFromSelection( entities );
	for ( CEntity* entity : entities )
	{
		entity->GetLayer()->GetWorld()->UnignoreEntityStreaming( entity );
	}
}

void CEdSceneExplorer::OnClearIgnoreList( wxCommandEvent& event )
{
	GGame->GetActiveWorld()->ClearStreamingIgnoreList();
}

void CEdSceneExplorer::OnShowIgnoredEntities( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities;
	GGame->GetActiveWorld()->GetStreamingSectorData()->GetIgnoreList().GetIgnoredEntities( entities );
	ClearEntityList( TXT("Ignored Entities" ) );
	AddEntitiesToEntityList( TXT("Ignored Entities"), entities );
}

void CEdSceneExplorer::OnResetInstanceProperties( wxCommandEvent& event )
{
	TDynArray< CComponent* > componentSelection;
	CComponent* component;
	m_sceneView->GetSelection( componentSelection );

	// We can only work with one component ATM
	if ( componentSelection.Size() != 1 )
	{
		wxMessageBox( wxT("You can only reset the instance properties of a single component"), wxT("Wrong"), wxOK|wxCENTRE|wxICON_ERROR, wxTheFrame );
		return;
	}
	component = componentSelection[0];

	// Construct a list with modified instance properties
	TDynArray< CProperty* > modifiedProperties;
	component->CollectModifiedInstanceProperties( modifiedProperties );

	// Ask the user which properties to reset
	TDynArray< Bool > resetPropertyCheck;
	resetPropertyCheck.Grow( modifiedProperties.Size() );
	String overridesList = CEdFormattedDialog::ArrayToList( modifiedProperties, []( const CProperty* prop ) { return prop->GetName().AsString(); } );
	String code = TXT("H{V{H{'Modified Properties:'|||||||}M") + overridesList + TXT("=160}|V{B@'&Reset Checked'|B'Reset All';~B'&Cancel';}}");
	Int32 button = FormattedDialogBox( wxT("Instance Properties"), code.AsChar(), resetPropertyCheck.TypedData() );

	// Cancel if button 0 or 1 was not clicked
	if ( button != 0 && button != 1 )
	{
		return;
	}

	// "Check" everything if the Reset All button was clicked
	if ( button == 1 )
	{
		// "Check" everything
		for ( Uint32 i=0; i < resetPropertyCheck.Size(); ++i )
		{
			resetPropertyCheck[i] = true;
		}
	}

	// If everything is checked, show a confirmation box
	Uint32 resetCount = 0;
	for ( Uint32 i=0; i < resetPropertyCheck.Size(); ++i )
	{
		resetCount += resetPropertyCheck[i] ? 1 : 0;
	}
	if ( resetCount == resetPropertyCheck.Size() && wxMessageBox( wxT("Are you sure that you want to reset ALL instance properties?"), wxT("Reset Properties"), wxYES_NO|wxICON_WARNING|wxCENTRE ) != wxYES )
	{
		return;
	}

	// Reset the selected properties
	CEntity* cplent = component->GetEntity();
	CEntityTemplate* tpl = cplent->GetEntityTemplate();
	CEntity* tplent = tpl->GetEntityObject();
	CComponent* srccmp = tplent->FindComponent( CName( component->GetName() ), component->GetClass()->GetName() );
	if ( srccmp == nullptr )
	{
		wxMessageBox( wxT("Failed to find the original component in the entity's template"), wxT("Component Not Found"), wxOK|wxCENTRE|wxICON_ERROR, wxTheFrame );
		return;
	}

	for ( Uint32 i=0; i < resetPropertyCheck.Size(); ++i )
	{
		if ( resetPropertyCheck[i] && component->MarkModified() )
		{
			CProperty* prop = modifiedProperties[i];
			const void* src = prop->GetOffsetPtr( srccmp );
			SetPropertyValueIndirect( component, prop->GetName(), src, true );
		}
	}
}

void CEdSceneExplorer::OnMassActions( wxCommandEvent& event )
{
	LayerEntitiesArray entities;
	const THashSet< ISerializable* >& selection = m_sceneView->GetSelection();

	// Get selected entities
	for ( auto it=selection.Begin(); it != selection.End(); ++it )
	{
		CEntity* entity = Cast< CEntity >( *it );
		CLayerGroup* layerGroup = Cast< CLayerGroup >( *it );
		CLayerInfo* layerInfo = Cast< CLayerInfo >( *it );
		CWorld* world = Cast< CWorld >( *it );

		if ( world != nullptr )
		{
			layerGroup = world->GetWorldLayers();
		}

		if ( entity != nullptr )
		{
			entities.PushBackUnique( entity );
		}
		else if ( layerGroup != nullptr )
		{
			TDynArray< CLayerInfo* > layers;
			layerGroup->GetLayers( layers, true, true, true );
			for ( auto it2=layers.Begin(); it2 != layers.End(); ++it2 )
			{
				CLayerInfo* layerInfo = *it2;
				const auto& layerEntities = layerInfo->GetLayer()->GetEntities();
				entities.PushBackUnique( layerEntities );
			}
		}
		else if ( layerInfo != nullptr )
		{
			const auto& layerEntities = layerInfo->GetLayer()->GetEntities();
			entities.PushBackUnique( layerEntities );
		}
	}

	// Check if we've got any entities
	if ( entities.Empty() )
	{
		return;
	}
	
	// Construct the mass actions context
	CEdEntitiesAndComponentsMassActionIterator iterator( entities );
	ShowSceneMassActionsDialog( &iterator );
}

void CEdSceneExplorer::OnFindDuplicates( wxCommandEvent& event )
{
	const THashSet< ISerializable* >& selection = m_sceneView->GetSelection();
	TDynArray< CLayerInfo* > layers;

	// Get selected entities
	for ( auto it=selection.Begin(); it != selection.End(); ++it )
	{
		if ( CLayerInfo* layerInfo = Cast< CLayerInfo >( *it ) )
		{
			layers.PushBackUnique( layerInfo );
			continue;
		}
		
		CLayerGroup* layerGroup = Cast< CLayerGroup >( *it );
		if ( CWorld* world = Cast< CWorld >( *it ) ) // that means layerGroup is null
		{
			layerGroup = world->GetWorldLayers();
		}
		
		if ( layerGroup )
		{
			TDynArray< CLayerInfo* > tmp;
			layerGroup->GetLayers( tmp, true );
			layers.PushBackUnique( tmp );
		}
	}

	TDynArray< String > duplicatesInfo;
	CDuplicatesFinder::FindDuplicates( duplicatesInfo, layers, false );

	if ( !m_duplicatesErrorsList )
	{
		m_duplicatesErrorsList = new CEdErrorsListDlg( wxTheFrame, false );
		m_duplicatesErrorsList->SetHeader( TXT("Possible duplicates found!") );
		m_duplicatesErrorsList->SetTitle( TXT("Report") );
		Uint32 style = m_duplicatesErrorsList->GetWindowStyle();
		m_duplicatesErrorsList->SetWindowStyle( style & ~wxSTAY_ON_TOP );
	}
	m_duplicatesErrorsList->Execute( duplicatesInfo );
}

void CEdSceneExplorer::OnFindDuplicateIdTags( wxCommandEvent& event )
{
	THashMap< IdTag, TDynArray< CEntity * > > tagToEntity;
	TDynArray< CEntity* > duplicates, entitySelection;
	m_sceneView->CollectEntitiesFromSelection( entitySelection );

	// Collect entities
	for ( CEntity* ent : entitySelection )
	{
		CPeristentEntity* pe = Cast< CPeristentEntity >( ent );
		if ( pe != nullptr )
		{
			tagToEntity[pe->GetIdTag()].PushBack( pe );
		}
	}

	// Check tags with more than one entity
	TDynArray< IdTag > keys;
	tagToEntity.GetKeys( keys );
	for ( IdTag key : keys )
	{
		if ( tagToEntity[key].Size() > 1 )
		{
			duplicates.PushBack( tagToEntity[key] );
		}
	}

	// Show list
	if ( duplicates.Empty() )
	{
		wxMessageBox( wxT("No duplicates found"), wxT("Duplicate IdTags"), wxICON_INFORMATION|wxCENTRE, wxTheFrame );
	}
	else
	{
		ClearEntityList( TXT("Entities with duplicate IdTags") );
		AddEntitiesToEntityList( TXT("Entities with duplicate IdTags"), duplicates );
		ShowEntityList( TXT("Entities with duplicate IdTags") );
	}
}

void CEdSceneExplorer::OnAddToEntityList( wxCommandEvent& event )
{
	String name;
	if ( !InputBox( wxTheFrame, TXT("Add selected entities to entity list"), wxT("Please enter the name of the entity list"), name ) )
	{
		return;
	}

	TDynArray< CEntity* > entitySelection;
	m_sceneView->GetSelection( entitySelection );
	AddEntitiesToEntityList( name, entitySelection );
}

void CEdSceneExplorer::OnLoadEntityList( wxCommandEvent& event )
{
	wxFileDialog* fd = new wxFileDialog( wxTheFrame, wxT("Load Entity List"), "", "", "Entity List (*.entlist)|*.entlist", wxFD_OPEN|wxFD_FILE_MUST_EXIST );
	if ( fd->ShowModal() == wxID_OK )
	{
		String absolutePath( fd->GetPath().wc_str() );
		CEntityList* list = LoadEntityListFromFile( absolutePath );
		if ( list != nullptr )
		{
			list->Show();
		}
	}
	fd->Destroy();
}

void CEdSceneExplorer::OnEditEnvironment( wxCommandEvent& event )
{
	wxTheFrame->OpenWorldEnvironmentEditor();
	wxTheFrame->EditEnvironmentParams( GetSelectedEntity() );	
}

CNavmeshComponent* CEdSceneExplorer::GetSelectedNavmeshComponent()
{
#ifndef NO_NAVMESH_GENERATION
	TDynArray<CNavmeshComponent*> componentSelection;
	m_sceneView->GetSelection( componentSelection );
	if ( componentSelection.Size() == 1 )
	{
		return componentSelection[ 0 ];
	}
	else
	{
		TDynArray< CEntity* > entitySelection;
		m_sceneView->GetSelection( entitySelection );
		for ( auto it = entitySelection.Begin(), end = entitySelection.End(); it != end; ++it )
		{
			ComponentIterator< CNavmeshComponent > componentIterator( *it );
			if ( !(componentIterator) )
			{
				continue;
			}
			return *componentIterator;
		}
	}
#endif
	return NULL;
}

void CEdSceneExplorer::OnGenerateNavmesh( wxCommandEvent& event )
{
#ifndef NO_NAVMESH_GENERATION
	CNavmeshComponent* navmeshComponent = GetSelectedNavmeshComponent();
	if ( navmeshComponent == NULL )
	{
		return;
	}

	switch ( event.GetId() )
	{
	default:
	case ID_MENU_GENERATE_NAVMESH:
		navmeshComponent->GenerateNavmeshAsync();
		break;
	case ID_MENU_GENERATE_NAVMESH_RECURSIVE:
		navmeshComponent->GenerateNavmeshRecursiveAsync();
		break;
	}
	
#endif // NO_NAVMESH_GENERATION
}

void CEdSceneExplorer::OnStopNavmeshGeneration( wxCommandEvent& event )
{
#ifndef NO_NAVMESH_GENERATION
	CNavmeshComponent* navmeshComponent = GetSelectedNavmeshComponent();
	if ( navmeshComponent == NULL )
	{
		return;
	}
	navmeshComponent->StopRecursiveGeneration();
#endif // NO_NAVMESH_GENERATION
}
void CEdSceneExplorer::OnGenerateNavgraph( wxCommandEvent& event )
{
#ifndef NO_NAVMESH_GENERATION
	CNavmeshComponent* navmeshComponent = GetSelectedNavmeshComponent();
	if ( navmeshComponent == NULL )
	{
		return;
	}
	navmeshComponent->GenerateNavgraph();
#endif // NO_NAVMESH_GENERATION
}

void CEdSceneExplorer::OnResetNavmeshParams( wxCommandEvent& event )
{
#ifndef NO_NAVMESH_GENERATION
	CNavmeshComponent* navmeshComponent = GetSelectedNavmeshComponent();
	if ( navmeshComponent == NULL )
	{
		return;
	}
	navmeshComponent->GetNavmeshParams().ResetToDefaults();
#endif // NO_NAVMESH_GENERATION
}

void CEdSceneExplorer::OnComputeNavmeshBasedBounds( wxCommandEvent& event )
{
#ifndef NO_NAVMESH_GENERATION
	CNavmeshComponent* navmeshComponent = GetSelectedNavmeshComponent();
	if ( navmeshComponent == NULL )
	{
		return;
	}
	navmeshComponent->ComputeNavmeshBasedBounds();
#endif // NO_NAVMESH_GENERATION
}

void CEdSceneExplorer::OnLookAtNode( wxCommandEvent& event )
{
	TDynArray< CNode* > nodes;
	m_sceneView->GetSelection( nodes );
	if ( nodes.Size() > 0 )
	{
		wxTheFrame->GetWorldEditPanel()->LookAtNode( nodes[0] );
	}
}

void CEdSceneExplorer::OnMovePlayerThere( wxCommandEvent& event )
{
	TDynArray< CEntity* > entities;
	m_sceneView->GetSelection( entities );
	if ( entities.Size() > 0 && GGame->IsActive() && GGame->GetPlayerEntity() && GGame->GetPlayerEntity()->IsA< CPlayer >() )
	{
		CPlayer* player = static_cast< CPlayer* >( GGame->GetPlayerEntity() );
		Vector worldPosition = entities[0]->GetWorldPosition();
		worldPosition.Z = entities[0]->CalcBoundingBox().Max.Z + 0.5f;
		player->Teleport( worldPosition, player->GetRotation() );
	}
}
void CEdSceneExplorer::OnRemoveNode( wxCommandEvent& event )
{
	/*TDynArray<CNode*> nodes;
	m_sceneView->GetSelection( nodes );
	m_sceneView->ClearSelection();

	GGame->GetActiveWorld()->GetSelectionManager()->DeselectAll();
	GGame->GetActiveWorld()->DelayedActions();
	for ( auto it=nodes.Begin(); it != nodes.End(); ++it )
	{
	CNode* node = *it;
	GGame->GetActiveWorld()->GetSelectionManager()->Select( node );
	GGame->GetActiveWorld()->DelayedActions();
	}*/

	TDynArray< CEntity* > entities;
	GGame->GetActiveWorld()->GetSelectionManager()->GetSelectedEntities( entities );

	// special case for group of objects
	{
		Bool foundGrouppedObject = false;

		const Uint32 entityCount = entities.Size();
		for( Uint32 i=0; i<entityCount; ++i )
		{
			CEntityGroup* group = entities[i]->GetContainingGroup();
			if( group != nullptr && group->IsLocked() == false )
			{
				foundGrouppedObject = true;
			}
		}

		// if one or more objects are grouped
		if( foundGrouppedObject == true )
		{
			GFeedback->ShowWarn( TXT("One or more selected objects are in group and their group is unlocked. Please remove them from group or lock group before deleting.") );
			return;
		}
	}

	Bool askForRemovalConfirmation = true;
	if ( event.GetClientData() && (Bool*)event.GetClientData() )
	{
		askForRemovalConfirmation = *( (Bool*)event.GetClientData() );
	}

	wxTheFrame->GetWorldEditPanel()->DeleteEntities( entities, askForRemovalConfirmation );

	Refresh( false );
}

void CEdSceneExplorer::OnCopyNode( wxCommandEvent& event )
{
    OnCopy( CM_Entities );
}

void CEdSceneExplorer::OnCutNode( wxCommandEvent& event )
{
	OnCopy( CM_Entities, true );
}

void CEdSceneExplorer::OnPresetChanged( wxCommandEvent& event )
{
    String newPreset = m_presetComboBox->GetValue().wc_str();

    wxString message = wxT("Changing layer preset can unload current layers and load new ones.\nAre you sure you want to continue?");
    wxString caption = wxT("Changing layer preset");
    if ( wxMessageBox( message, caption, wxYES_NO | wxCENTER, this ) == wxYES )
    {
		m_presetManager.SetActive( newPreset, false );
    }
    else
    {
        m_presetComboBox->SetStringSelection( m_presetManager.GetActive().AsChar() );
    }
}

void CEdSceneExplorer::RefreshPresetComboBox()
{
    ASSERT( m_presetComboBox );

    m_presetComboBox->Freeze();
    m_presetComboBox->Clear();

	TDynArray< String > presetNames;
	m_presetManager.GetPresetNames( presetNames );
	
	for ( auto it=presetNames.Begin(); it != presetNames.End(); ++it )
	{
		m_presetComboBox->AppendString( (*it).AsChar() );
	}

    int index = m_presetComboBox->FindString( m_presetManager.GetActive().AsChar() );
    if ( index >= 0 )
    {
        m_presetComboBox->SetSelection( index );
    }
    else
    {
        m_presetComboBox->SetValue( PRESET_NONE );
		m_presetManager.SetActive( PRESET_NONE, false );
    }

    m_presetComboBox->Thaw();
}

void CEdSceneExplorer::OnSavePresets( wxCommandEvent& event )
{
    String activePreset = m_presetComboBox->GetValue().wc_str();
	m_presetManager.StoreGroupsTo( activePreset );
	m_presetManager.SetActive( activePreset, true );
	m_presetManager.Save();
	RefreshPresetComboBox();
}

void CEdSceneExplorer::OnRenamePreset( wxCommandEvent& event )
{
	String activePreset = m_presetComboBox->GetValue().wc_str();
	String newName;

	// Cannot rename 'None'
	if ( activePreset == PRESET_NONE )
	{
		wxMessageBox( wxT("Cannot rename the 'None' preset"), wxT("No Way"), wxOK|wxICON_ERROR );
		return;
	}

	// Ask for the new name
	newName = activePreset;
	if ( !InputBox( this, TXT("Rename Preset"), String::Printf( TXT("Enter a new name for '%s'"), activePreset.AsChar() ), newName, false ) )
	{
		return;
	}

	// Cannot use 'None' as the name
	if ( newName == PRESET_NONE )
	{
		wxMessageBox( wxT("Cannot use 'None' as a custom preset name"), wxT("No"), wxOK|wxICON_ERROR );
		return;
	}

	// Cancel if the new name is the same as the previous
	if ( newName == activePreset )
	{
		return;
	}

	// Make sure no existing preset has this name
	if ( m_presetManager.Has( newName ) )
	{
		wxMessageBox( wxString::Format( wxT("There already a preset named '%s'"), newName.AsChar() ), wxT("Name Conflict"), wxOK|wxICON_ERROR );
		return;
	}

	// Try to rename
	if ( !m_presetManager.Rename( activePreset, newName ) )
	{
		wxMessageBox( wxString::Format( wxT("Failed to rename preset '%s' to '%s'"), activePreset.AsChar(), newName.AsChar() ), wxT("Rename Failed"), wxOK|wxICON_ERROR );
		return;
	}

	// Save and update
	m_presetManager.Save();
	RefreshPresetComboBox();
}

void CEdSceneExplorer::OnDeletePreset( wxCommandEvent& event )
{
	String presetToDelete = m_presetComboBox->GetValue().wc_str();

	if ( presetToDelete == PRESET_NONE )
	{
		wxMessageBox( wxString::Format( wxT("The preset '%s' cannot be deleted."), presetToDelete.AsChar() ), wxT("Error"), wxICON_ERROR|wxOK );
		return;
	}

	// Check if the preset exists
	if ( !m_presetManager.Has( presetToDelete ) )
	{
		wxMessageBox( wxString::Format( wxT("The preset '%s' was not found."), presetToDelete.AsChar() ), wxT("Error"), wxICON_ERROR|wxOK );
		return;
	}

	// Confirm
	String message = String::Printf( TXT("Are you sure you want to remove preset '%s'?"), presetToDelete.AsChar() );
	String caption = TXT("Removing preset");
	if ( wxMessageBox( message.AsChar(), caption.AsChar(), wxYES_NO | wxCENTER, this ) == wxYES )
	{
		//m_recreateTreePending = presetToDelete == m_presetManager.GetActive();
		m_presetManager.Remove( presetToDelete );
		m_presetManager.Save();
		RefreshPresetComboBox();
	}
}

void CEdSceneExplorer::OnCopyPreset( wxCommandEvent& event )
{
	// Convert active preset to string
	String presetString = TXT("SPRESET:") + m_presetManager.GetPresetAsString( m_presetManager.GetActive() );

	// Copy the preset string to clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new wxTextDataObject( presetString.AsChar() ) );
		wxTheClipboard->Close();
		wxTheClipboard->Flush();
	}
}

void CEdSceneExplorer::OnPastePreset( wxCommandEvent& event )
{
	String presetString;

	// Try to paste preset string from clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTextDataObject data;
		if ( wxTheClipboard->GetData( data ) )
		{
			presetString = String( data.GetText().c_str() );
		}
		wxTheClipboard->Close();
	}

	// Make sure we've got a proper copied preset string
	presetString.Trim();
	if ( presetString.Empty() || !presetString.BeginsWith( TXT("SPRESET:") ) )
	{
		wxMessageBox( wxT("The clipboard does not seem to contain a valid preset."), wxT("No Preset To Paste"), wxICON_ERROR|wxOK );
		return;
	}

	// Try to put the pasted preset in the preset manager
	String name;
	presetString = presetString.MidString( 8, presetString.GetLength() );
	if ( !m_presetManager.SetPresetFromString( presetString, name ) )
	{
		wxMessageBox( wxT("The clipboard does not seem to contain a valid preset."), wxT("Invalid Preset String"), wxICON_ERROR|wxOK );
		return;
	}	

	// Update
	RefreshPresetComboBox();
}

void CEdSceneExplorer::OnDisplayLBTColors( wxCommandEvent& event )
{
	DrawLBTBackgrounds = event.IsChecked();
	m_sceneView->RefreshLater();
}

void CEdSceneExplorer::OnNoSorting( wxCommandEvent& event )
{
	SetSortingMode( SSM_NoSorting );
}

void CEdSceneExplorer::OnAlphabeticSorting( wxCommandEvent& event )
{
	SetSortingMode( SSM_Alphabetically );
}

void CEdSceneExplorer::OnRemoveObjectFilters( wxCommandEvent& event )
{
	ClearGlobalFilter();
	m_sceneView->RefreshLater();
}

void CEdSceneExplorer::OnLoadPresets( wxCommandEvent& event )
{
	m_presetManager.Load();
	m_presetManager.RestoreGroupsFrom( m_presetManager.GetActive() );
	RefreshPresetComboBox();
	//m_recreateTreePending = true;
}

void CEdSceneExplorer::OnPresetActions( wxCommandEvent& event )
{
	// Make sure we have a world out there
	if ( !GGame->GetActiveWorld() )
	{
		wxMessageBox( wxT("No world to use scene presets with"), wxT("World not found"), wxOK|wxICON_ERROR );
		return;
	}

	// Show menu with preset actions
	wxMenu menu;
	Bool customPreset = m_presetManager.GetActive() != PRESET_NONE;
	menu.Append( ID_MENU_PRESETACTION_SAVE, wxT("Save presets"), wxT("Save the presets"), false );
	if ( customPreset )
	{
		menu.AppendSeparator();
		menu.Append( ID_MENU_PRESETACTION_RENAME, wxT("Rename preset"), wxT("Rename the current preset"), false );
		menu.Append( ID_MENU_PRESETACTION_DELETE, wxT("Delete preset"), wxT("Delete the current preset"), false );
	}
	menu.AppendSeparator();
	if ( customPreset )
	{
		menu.Append( ID_MENU_PRESETACTION_COPY, wxT("Copy preset"), wxT("Copy the current preset to clipboard"), false );
	}
	menu.Append( ID_MENU_PRESETACTION_PASTE, wxT("Paste preset"), wxT("Paste the current preset from clipboard"), false );

	// Popup the menu below the actions button
	XRCCTRL( *this, "PresetActions", wxBitmapButton )->PopupMenu( &menu, wxPoint( 0, XRCCTRL( *this, "PresetActions", wxBitmapButton )->GetSize().GetHeight() ) );
}

void CEdSceneExplorer::OnDisplayOptions( wxCommandEvent& event )
{
	// Show menu with display options
	wxMenu menu;
	wxMenuItem* item = menu.Append( ID_MENU_DISPLAY_LBTCOLORS, wxT("Layer Background Tag Colors"), wxT("Toggle layer background tag colors"), wxITEM_CHECK );
	item->Check( DrawLBTBackgrounds );
	menu.AppendSeparator();
	item = menu.Append( ID_MENU_NO_SORTING, wxT("Do not sort"), wxT("Do not sort the objects"), wxITEM_RADIO );
	item->Check( GetSortingMode() == SSM_NoSorting );
	item = menu.Append( ID_MENU_ALPHABETIC_SORTING, wxT("Sort alphabetically"), wxT("Sort the objects alphabetically"), wxITEM_RADIO );
	item->Check( GetSortingMode() == SSM_Alphabetically );

	if ( !m_globalFilter.Empty() )
	{
		menu.AppendSeparator();
		item = menu.Append( ID_MENU_REMOVE_OBJECT_FILTERS, wxT("Remove object filters"), wxT("Remove any applied object filters"), wxITEM_NORMAL );
	}

	// Popup the menu below the actions button
	XRCCTRL( *this, "DisplayOptions", wxBitmapButton )->PopupMenu( &menu, wxPoint( 0, XRCCTRL( *this, "DisplayOptions", wxBitmapButton )->GetSize().GetHeight() ) );
}

void CEdSceneExplorer::OnFilterBoxText( wxCommandEvent& event )
{
	wxString filterString = XRCCTRL( *this, "FilterBox", wxTextCtrl )->GetValue().Trim();
	m_sceneView->SetFilterString( filterString );
}

void CEdSceneExplorer::SaveWorldModifiedOnly()
{
	if ( GGame->GetActiveWorld() == NULL ) return;

	TDynArray< CLayerGroup* > groups;
	groups.PushBack ( GGame->GetActiveWorld()->GetWorldLayers() );

	SaveLayerGroups( groups, true );
}

void CEdSceneExplorer::OnLoadLayerGroup( wxCommandEvent& event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Collect layers to load from selection
	TDynArray< CLayerInfo* > layersToLoad;
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;

		// Get the layers from the group to load
        const Bool recursive = event.GetId() == ID_MENU_LOAD_LAYER_GROUP_RECURSIVE;
        group->GetLayers( layersToLoad, false, recursive, true );
	}

	if ( !layersToLoad.Empty() )
	{
		// Load layers
		LayerGroupLoadingContext loadingContext;
		//loadingContext.m_dumpStats = true;
		LoadLayers( layersToLoad, loadingContext );

		// Flush
		GGame->GetActiveWorld()->UpdateLoadingState();
		GGame->GetActiveWorld()->DelayedActions();
		GGame->GetActiveWorld()->DelayedActions();

		// Flush collision cache
		GCollisionCache->Flush();
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnUnloadLayerGroup( wxCommandEvent& event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Unload selected layer groups
	// MattH: TTP Witcher 3, #4573. Only call garbage collector when all layers have unloaded, rather than one for each layer
	GObjectGC->DisableGC();
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		(*it)->SyncUnload();
	}
	GObjectGC->EnableGC();

	// Collect garbage
	GObjectGC->CollectNow();
}

void CEdSceneExplorer::OnRemoveLayerGroup( wxCommandEvent& event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelection( groups );

	// Unload selected layer groups
	TDynArray< CLayerInfo* > layersToLoad;
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		(*it)->SyncUnload();
	}

	// Update groups state
	GGame->GetActiveWorld()->UpdateLoadingState();
	GGame->GetActiveWorld()->DelayedActions();

	// Remove selected layer groups
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;

		// if a group has not been yet deleted
		if ( !group->IsSystemGroup() )
		{
			group->Remove();
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnGenerateResourceGraph( wxCommandEvent& event )
{
	if( GGame && GGame->GetActiveWorld() )
	{
		CDiskFile* worldFile = GGame->GetActiveWorld()->GetFile();

		CDirectory* worldDirectory = worldFile->GetDirectory();
		CDirectory* streamingDirectory = worldDirectory->CreateNewDirectory( TXT( "streaming" ) );
		CDirectory* ResourceGraphs = streamingDirectory->CreateNewDirectory( TXT( "resource_graphs" ) );

		String commandLine = String::Printf
		(
			TXT( "..\\dev\\internal\\ResourceGraphs\\BuildGraphs.bat %" ) RED_PRIWs,
			ResourceGraphs->GetAbsolutePath().AsChar()
		);

		// Run bundler and stop execution until it finishes
		wxExecute( commandLine.AsChar(), wxEXEC_SYNC );

		wxLaunchDefaultBrowser( ResourceGraphs->GetAbsolutePath().AsChar() );
	}
}

void CEdSceneExplorer::OnCollectEmptyEntities( wxCommandEvent& event )
{
	CEdEmptyEntityCollector* collector = new CEdEmptyEntityCollector( wxTheFrame );
	collector->Show();
}

void CEdSceneExplorer::OnCollectLightEntities( wxCommandEvent& event )
{
	CEdLightEntityCollector* collector = new CEdLightEntityCollector( wxTheFrame );
	collector->Show();
}


void CEdSceneExplorer::OnAddLayer( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > groups;
	if ( event.GetId() == ID_MENU_ADD_LAYER )
	{
		m_sceneView->GetSelectedLayerGroups( groups );
	}
	else // add layer to parent group case
	{
		TDynArray< CLayerInfo* > layers;
		m_sceneView->GetSelection( layers );

		for( const CLayerInfo* li : layers )
		{
			groups.PushBackUnique( li->GetLayerGroup() );
		}
	}

	// Make sure there is only one group selected
	if ( groups.Size() != 1 )
	{
		wxMessageBox( wxT("You need to select one (and only one) layer group to create a layer under"), wxT("Invalid selection"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}

	// Get the group
	CLayerGroup* group = groups[0];

	// Make sure we're not messing with system groups
	if ( group->IsSystemGroup() )
	{
		wxMessageBox( wxT("The selected layer group is a system group and cannot be manually edited"), wxT("Invalid group"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}

	// Make sure we're not trying to add a layer under a DLC group
	if ( group->GetName() == TXT("DLC") )
	{
		wxMessageBox( wxT("You cannot add a new layer directly under the DLC group."), wxT("Invalid group"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}
	if ( group->GetParentGroup() != nullptr && group->GetParentGroup()->GetName() == TXT("DLC") )
	{
		wxMessageBox( wxT("The selected layer group is a DLC group that you cannot add layers directly."), wxT("Invalid group"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}
	
	// Ask for layer name and properties
	String layerName;
	Int32 buildTagIndex = 1, staticIndex = 0;
	Int32 button = 0;

	CLayerInfo* copiedLayer = (CLayerInfo*)event.GetClientData();
	if ( copiedLayer )
	{
		buildTagIndex = copiedLayer->GetLayerBuildTag() - 1;
		staticIndex = copiedLayer->GetLayerType();
	}

	while ( layerName.Empty() && button == 0 )
	{
		button = FormattedDialogBox( wxTheFrame, wxT("Create Layer"), wxT("H{'Name:'|S*1}H{R'Build Tag'('Ignored''Outdoor''Indoor''Underground''Quest''Communities''Audio''Navigation''Gameplay')|V{R'Type'('Auto-static (static meshes, etc)''Non-static (dynamic entities)')~H{~B@'Create'|B'Cancel'}}"), &layerName, &buildTagIndex, &staticIndex );
	}

	if ( button == 0 )
	{
		// Convert layer name to lowercase
		layerName.MakeLower();

		// Create layer
		CLayerInfo* layer = group->CreateLayer( layerName );
		if ( layer )
		{
			// Activate layer
			ChangeActiveLayer( layer );

			// Set properties
			if ( group->IsDLC() )
			{
				layer->SetLayerBuildTag( LBT_DLC );
			}
			else
			{
				layer->SetLayerBuildTag( (ELayerBuildTag)( buildTagIndex + 1 ) );
			}
			layer->SetLayerType( ( ELayerType )staticIndex );
			
			if ( copiedLayer )
			{
				layer->SetTags( copiedLayer->GetTags() );
				layer->SetMergedContentMode( copiedLayer->GetMergedContentMode() );
			}

			// Make sure the file is in version control
			CDiskFile *file = GDepot->FindFile( layer->GetDepotPath() );
			if ( file )
			{
				file->Add();
			}
		}

		MarkTreeItemsForUpdate();
	}
}

void CEdSceneExplorer::OnAddLayerGroup( wxCommandEvent& event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Make sure there is only one group selected
	if ( groups.Size() != 1 )
	{
		wxMessageBox( wxT("You need to select one (and only one) layer group to create a subgroup under"), wxT("Invalid selection"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}

	// Get group
	CLayerGroup* group = groups[0];

	// Make sure we're not messing with system groups
	if ( group->IsSystemGroup() )
	{
		wxMessageBox( wxT("The selected layer group is a system group and cannot be manually edited"), wxT("Invalid group"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}
	
	// Make sure we're not trying to add a layer group under the DLC group
	if ( group->GetName() == TXT("DLC") )
	{
		wxMessageBox( wxT("You cannot add a new group directly under the DLC group."), wxT("Invalid group"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}

	// Ask for layer group name
	String layerGroupName;
	if ( InputBox( this, TXT("New layer group"), TXT("Enter layer group name"), layerGroupName ) )
	{
		// Convert layer group name to lowercase
		layerGroupName.MakeLower();

		// Create layer
		CLayerGroup* newGroupObject = group->CreateGroup( layerGroupName );
		if ( newGroupObject )
		{
			// Select group
			if ( CWorld *world = GGame->GetActiveWorld() )
			{
				world->GetSelectionManager()->DeselectAll();
				world->GetSelectionManager()->SelectLayer( newGroupObject );
			}
		}

		MarkTreeItemsForUpdate();
	}
}

void CEdSceneExplorer::UpdatePresetsUI()
{
    // Refresh presets combo pox
    if ( m_presetComboBox )
    {
        Bool isWorldOpen = (GGame->GetActiveWorld() != NULL);
        m_presetComboBox->Enable( isWorldOpen );

        if ( wxToolBar *toolBar = static_cast<wxToolBar*>( FindWindow(TXT("tlbTools")) ) )
        {
            toolBar->EnableTool( XRCID("SavePresets"), isWorldOpen );
            toolBar->EnableTool( XRCID("DeletePreset"), isWorldOpen );
        }
    }
}

void CEdSceneExplorer::ShowSceneMassActionsDialog( class IEdMassActionIterator* iterator )
{
	THashSet< CNode* > nodesToSelect;
	THashSet< CEntity* > entitiesToHide;
	THashSet< CEntity* > entitiesToReveal;

	CEdMassActionContext context( iterator );
	context.AddCommonActionAndConditionTypes();
	context.AddActionType( new CEdMassActionSelectNode( nodesToSelect ) );
	context.AddActionType( new CEdMassActionShowHideEntity( true, entitiesToHide ) );
	context.AddActionType( new CEdMassActionShowHideEntity( false, entitiesToReveal ) );
	context.AddActionType( new CEdMassActionAddEntityToList() );
	context.AddActionType( new CEdMassActionRemoveEntityFromList() );
	context.AddActionType( new CEdMassActionClearEntityList() );
	context.AddActionType( new CEdMassActionAddToSceneFilter() );
	context.AddActionType( new CEdMassActionChangeComponentClass() );
	context.AddActionType( new CEdMassActionNodeSpecific() );
	context.AddConditionType( new CEdMassActionContainsComponentCondition() );
	context.AddConditionType( new CEdMassActionNodeSpecificCondition() );

	// Construct the mass actions dialog
	CEdMassActionDialog* dialog = new CEdMassActionDialog( wxTheFrame, wxT("Scene Mass Actions"), &context );
	dialog->SetDefaults( TXT("sceneexplorer") );
	dialog->ShowModal();
	dialog->Destroy();
	
	// Select any nodes
	if ( !nodesToSelect.Empty() )
	{
		CWorld* world = (*nodesToSelect.Begin())->GetLayer()->GetWorld();
		world->GetSelectionManager()->Select( HashSetToDynArray( nodesToSelect ) );
	}
	// Hide any nodes
	if ( !entitiesToHide.Empty() )
	{
		wxTheFrame->GetWorldEditPanel()->HideEntities( HashSetToDynArray( entitiesToHide ) );
	}
	// Reveal any nodes
	if ( !entitiesToReveal.Empty() )
	{
		wxTheFrame->GetWorldEditPanel()->RevealEntities( HashSetToDynArray( entitiesToReveal ) );
	}
}

void CEdSceneExplorer::OnEntityListMassActions( class CEntityList*, const LayerEntitiesArray& entities )
{
	// Construct the mass actions context
	CEdEntitiesAndComponentsMassActionIterator iterator( entities );
	ShowSceneMassActionsDialog( &iterator );
	MarkTreeItemsForUpdate();
}

Bool CEdSceneExplorer::IsFilteredOut( ISerializable* serializable ) const
{
	return !m_globalFilter.Exist( serializable );
}

Bool CEdSceneExplorer::HasGlobalFilters() const
{
	return !m_globalFilter.Empty();
}

void CEdSceneExplorer::ClearGlobalFilter()
{
	m_globalFilter.Clear();
	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::AddToGlobalFilter( ISerializable* serializable )
{
	m_globalFilter.Insert( serializable );
	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::RemoveFromGlobalFilter( ISerializable* serializable )
{
	m_globalFilter.Erase( serializable );
	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::SetSortingMode( ESceneSortingMode mode )
{
	m_sortingMode = mode;
	m_sceneView->RefreshLater();
}

void CEdSceneExplorer::CheckLayersEntitiesCompatibility( const TDynArray< CLayerInfo* >& layers, Bool modifiedOnly ) const
{
	TDynArray< String > compatibilityErrors;
	CLayersEntityCompatibilityChecker::CheckLayersEntitiesCompatibility( layers, compatibilityErrors, modifiedOnly );
	if ( wxTheFrame )
	{
		wxTheFrame->DisplayErrorsList( TXT( "There are compatibility errors between some entities and layers:" ),
			TXT( "Please, change layers' build tag, mark entities as unsavable or remove them. Otherwise LISTED LAYERS WON'T BE SAVED." ),
			compatibilityErrors );
	}
}

void CEdSceneExplorer::OnCopy( ECopyMode copyMode /* = CM_Default */, Bool cut /* = false */ )
{
	if ( copyMode != CM_Entities )
	{
		// try to copy selected layers
		TDynArray<CLayerInfo*> layers;
		m_sceneView->GetSelection( layers );

		if ( !layers.Empty() )
		{
			RunLater( [this, layers, cut]() {
				wxTheFrame->GetWorldEditPanel()->CopyLayers( layers );
				if ( cut )
				{
					wxCommandEvent cmdEvent;
					Bool askForRemovalConfirmation = false;
					cmdEvent.SetClientData( &askForRemovalConfirmation );
					OnRemoveLayer( cmdEvent );
				}
			} );

			return;
		}
		else if ( copyMode == CM_Layers ) // if it's 'Paste layers' option show the warning, otherwise (Ctrl+C) try to copy selected entities
		{
			GFeedback->ShowWarn( TXT("No layers chosen.") );
			return;
		}
	}

	// copy selected entities
	RunLater( [this, cut]() {
		wxTheFrame->GetWorldEditPanel()->CopySelectedEntities();
		if ( cut )
		{
			wxCommandEvent cmdEvent;
			Bool askForRemovalConfirmation = false;
			cmdEvent.SetClientData( &askForRemovalConfirmation );
			OnRemoveNode( cmdEvent );
		}
	} );
}

void CEdSceneExplorer::OnPaste( ECopyMode copyMode, Bool pasteToParentGroup )
{
	if ( copyMode != CM_Entities )
	{
		TDynArray< CLayerGroup* > groups;
		if ( !pasteToParentGroup )
		{
			m_sceneView->GetSelectedLayerGroups( groups );
		}

		// if there are no selected groups and we are specifically pasting into parent group or the copy mode is default (ctrl+v), try pasting to selected layer's parent group
		if ( groups.Empty() && ( pasteToParentGroup || copyMode == CM_Default ) )
		{
			TDynArray< CLayerInfo* > layers;
			m_sceneView->GetSelection( layers );
			for ( const CLayerInfo* li : layers )
			{
				groups.PushBackUnique( li->GetLayerGroup() );
			}
		}

		Bool success = false;
		// Make sure there is only one group selected
		if ( groups.Size() == 1 )
		{
			// Get the group
			CLayerGroup* group = groups[0];

			// Make sure we're not messing with system groups
			if ( !group->IsSystemGroup() && group->GetName() != TXT("DLC") && ( group->GetParentGroup() == nullptr || group->GetParentGroup()->GetName() != TXT("DLC") ) )
			{
				success = wxTheFrame->GetWorldEditPanel()->PasteLayers( group );
			}
		}
		
		if ( success || copyMode == CM_Layers )
		{
			if ( !success )
			{
				GFeedback->ShowMsg( TXT("Paste failed"), TXT("Make sure you selected exactly one layer group that is neither system nor dlc group.") );
			}
			return;
		}
		
		//for ( THandle< CLayerInfo > layer : m_copiedLayers )
		//{
		//	if ( layer.IsValid() )
		//	{
		//		//event.SetClientData( layer.Get() );
		//		//OnAddLayer( event );
		//		//wxTheFrame->GetWorldEditPanel()->OnPaste( wxCommandEvent() );

		//	}
		//}
	}

	// paste entities
	wxTheFrame->GetWorldEditPanel()->PasteEntities( NULL, false );
}

void CEdSceneExplorer::OnSaveLayerGroupHierarchy( wxCommandEvent& event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelectedLayerGroups( groups );
	
	SaveLayerGroups( groups, event.GetId() == ID_MENU_SAVE_LAYER_GROUP_MODIFIED );
}

void CEdSceneExplorer::SaveLayerGroups( const TDynArray< CLayerGroup* >& groups, Bool modifiedOnly )
{

	TDynArray< CLayerInfo* > layers;
	for ( CLayerGroup* group : groups )
	{
		group->GetLayers( layers, false );
	}	

	CheckLayersEntitiesCompatibility( layers, modifiedOnly );

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if ( GVersionControl->IsSourceControlDisabled() )
	{
		TDynArray< CResource* > layerRes;
		for ( Uint32 i = 0; i < layers.Size(); ++i )
		{
			if ( layers[i]->IsLoaded() )
			{
				layerRes.PushBack( layers[i]->GetLayer() );
			}
		}

		CEdResourcesOverwriteChecker fileChecker( wxTheFrame, layerRes, modifiedOnly );
		if ( !fileChecker.Execute() )
		{
			return;
		}

		GFeedback->BeginTask( TXT("Saving"), true );
	}
#endif //NO_FILE_SOURCE_CONTROL_SUPPORT

	// Save the selected groups and their subgroups
	Uint32 i = 0;
	for ( auto it = groups.Begin(); it != groups.End(); ++it, ++i )
	{

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		if ( GVersionControl->IsSourceControlDisabled() )
		{
			//CDiskFile::SetForcedOverwriteFlag( String::EMPTY );
			GFeedback->UpdateTaskProgress( i, groups.Size() );
			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}
		}
#endif //NO_FILE_SOURCE_CONTROL_SUPPORT

		if ( CLayerGroup* group = *it )
		{
			group->Save( true, modifiedOnly );

			if ( group->IsRootGroup() )
			{
				CWorld* world = group->GetWorld();
				world->MarkModified();
				world->Save();

				world->GetFoliageEditionController().Save();
			}
		}
	}

	MarkTreeItemsForUpdate();

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if ( GVersionControl->IsSourceControlDisabled() )
	{
		GFeedback->EndTask();
	}
#endif //NO_FILE_SOURCE_CONTROL_SUPPORT
}

void CEdSceneExplorer::OnSaveLayerGroupThis( wxCommandEvent& event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Save only the selected groups
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;
		group->Save( false, false );
		if ( group->IsRootGroup() )
		{
			CWorld* world = group->GetWorld();
			world->MarkModified();
			world->Save();

			world->GetFoliageEditionController().Save();
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnSaveLayer( wxCommandEvent& event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	CheckLayersEntitiesCompatibility( layers, true );

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if ( GVersionControl->IsSourceControlDisabled() )
	{
		TDynArray< CResource* > layerRes;
		for ( Uint32 i = 0; i < layers.Size(); ++i )
		{
			if ( layers[i]->IsLoaded() )
			{
				layerRes.PushBack( layers[i]->GetLayer() );
			}
		}

		CEdResourcesOverwriteChecker fileChecker( wxTheFrame, layerRes, false );
		if ( !fileChecker.Execute() )
		{
			return;
		}
	}
#endif //NO_FILE_SOURCE_CONTROL_SUPPORT

	// Save selected layers
	Uint32 i = 0;
	for ( auto it=layers.Begin(); it != layers.End(); ++it, ++i )
	{
		GFeedback->UpdateTaskProgress( i, layers.Size() );
		(*it)->Save();
	}

	MarkTreeItemsForUpdate();

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if ( GVersionControl->IsSourceControlDisabled() )
	{
		GFeedback->EndTask();
	}
#endif //NO_FILE_SOURCE_CONTROL_SUPPORT
}

void CEdSceneExplorer::OnLoadLayer( wxCommandEvent& event )
{
}

void CEdSceneExplorer::OnSetActiveLayer( wxCommandEvent& event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Make sure there is only one layer selected
	if ( layers.Size() != 1 )
	{
		wxMessageBox( wxT("A single layer is needed to be set as the active one"), wxT("Invalid selection"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}

	ChangeActiveLayer( layers[0] );
}

void CEdSceneExplorer::OnLayerCheckOut( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Check out all selected layers
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layer = *it;
		CDiskFile* file = GDepot->FindFile( layer->GetDepotPath() );
		if ( file )
		{
			// Check if we need to re-add the layer's entities in the global filter
			Bool reAddEntitiesToGlobalFilter = false;
			if ( HasGlobalFilters() && layer->GetLayer() )
			{
				TDynArray< CEntity* > toRemove;
				for ( ISerializable* serializable : m_globalFilter )
				{
					CEntity* entity = Cast< CEntity >( serializable );
					if ( entity && entity->GetLayer() == layer->GetLayer() )
					{
						reAddEntitiesToGlobalFilter = true;
						toRemove.PushBack( entity );
					}
				}
				for ( CEntity* entity : toRemove )
				{
					m_globalFilter.Erase( entity );
				}
			}

			// Remove the current layer object from the filters
			Bool wasFiltered = m_globalFilter.Exist( layer->GetLayer() );
			if ( wasFiltered )
			{
				m_globalFilter.Erase( layer->GetLayer() );
			}

			file->CheckOut();
			layer->Refresh();

			// Re-add the layer into global filters if it was part of them
			if ( reAddEntitiesToGlobalFilter )
			{
				auto entities = layer->GetLayer()->GetEntities();
				for ( CEntity* entity : entities )
				{
					AddToGlobalFilter( entity );
				}
			}
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnLayerSubmit( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Submit all selected layers
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layer = *it;
		CDiskFile* file = GDepot->FindFile( layer->GetDepotPath() );
		if ( file )
		{
			// First save
			layer->Save();

			// Add the layer to version control if it isn't already there
			if ( file->IsLocal() )
			{
				file->Add();
			}

			// Perform submission
			if ( !file->Submit() )
			{
				file->Revert();
			}
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnLayerRevert( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Revert all selected layers
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layer = *it;
		CDiskFile* file = GDepot->FindFile( layer->GetDepotPath() );
		if ( file )
		{
			file->Revert();
		}

		// Refresh
		layer->Refresh( false );
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnLayerAdd( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Add all selected layers to version control
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layer = *it;
		CDiskFile* file = GDepot->FindFile( layer->GetDepotPath() );
		if ( file )
		{
			file->Add();
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnLayerSync( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Sync all selected layers
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		(*it)->GetLatest();
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnCopyLayerPath( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Get paths of all selected layers
	wxString allPaths;
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		// Separate multiple paths by EOL chars
		if ( !allPaths.IsEmpty() )
		{
			allPaths.Append( wxTextFile::GetEOL() );
		}

		allPaths.Append( (*it)->GetDepotPath().AsChar() );
	}

	// Copy the combined paths to the clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new wxTextDataObject( allPaths ) );
		wxTheClipboard->Close();
		wxTheClipboard->Flush();
	}
	else
	{
		wxMessageBox( wxT("Failed to open the clipboard"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
	}
}

void CEdSceneExplorer::OnRemoveLayer( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Remove selected layers
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layer = *it;

		// Make sure the user wants to remove the layer
		Bool remove = true;
		Bool askForRemovalConfirmation = true;
		if ( event.GetClientData() && (Bool*)event.GetClientData() )
		{
			askForRemovalConfirmation = *( (Bool*)event.GetClientData() );
		}

		if( askForRemovalConfirmation && layer->IsLoaded() && layer->GetLayer()->HasEntities() )
		{
			String message = String::Printf( TXT("Layer '%s' not empty. Are you sure to delete this layer?"), layer->GetLayer()->GetFriendlyName().AsChar() );
			wxMessageDialog dlg( this, message.AsChar(), TXT( "Warning!" ), wxOK | wxCANCEL | wxICON_QUESTION );
			remove = ( dlg.ShowModal() == wxID_OK );
		}

		// Do the remove
		if ( remove )
		{
			if ( layer == GetActiveLayerInfo() )
			{
				ChangeActiveLayer( NULL );
			}
			if ( layer->IsLoaded() )
			{
				layer->SyncUnload();
			}

			// The layerinfo needs to wait until all of the layer's attached
			// entities deattach themselves before removing (deleting) itself
			// which is why this loop that updates the world is needed
			bool destroying = true;
			while ( destroying )
			{
				destroying = false;

				// Update the world - this will cause entities to deattach
				// themselves from the layer and cause the layerinfo to stop
				// waiting for the layer to destroy itself and be removed from
				// the world's update list
				CWorldTickInfo tickinfo( GGame->GetActiveWorld(), 0.1f );
				GGame->GetActiveWorld()->Tick( tickinfo );

				// Attempt to remove the layerinfo objects
				if ( !layer->IsPendingDestroy() )
				{
					ASSERT( !layer->IsLoaded() );
					layer->Remove( askForRemovalConfirmation );
				}
				else
				{
					// if the layerinfo is still waiting for the layer's
					// attached components to deattach themselves, keep
					// looping
					destroying = true;
				}
			}
		}
	}
}

void CEdSceneExplorer::OnRenameLayer( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Make sure we have a single layer selected
	if ( layers.Size() != 1 )
	{
		wxMessageBox( wxT("Please select a single layer to rename"), wxT("Invalid selection"), wxOK|wxICON_ERROR|wxCENTRE, this );
		return;
	}

	// Get the layer info
	CLayerInfo* layer = layers[0];

	// Get the new name
	String name = layer->GetShortName();
	String prevName = name;
	if ( !InputBox( this, TXT("Rename layer '") + name + TXT("'"), TXT("Enter a new name for the layer"), name ) )
	{
		return;
	}
	if ( name == prevName ) 
	{
		return;
	}

	if ( !layer->Rename( name ) )
	{
		wxMessageBox( wxT("Failed to rename the layer."), wxT("Rename failed"), wxICON_ERROR|wxOK|wxCENTRE, this );
		return;
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnCopyLayer( wxCommandEvent& event )
{
	OnCopy( CM_Layers );
}

void CEdSceneExplorer::OnCutLayer( wxCommandEvent& event )
{
	// copy selected layers and remove them without asking for permission
	OnCopy( CM_Layers, true );
}

void CEdSceneExplorer::OnPasteLayer( wxCommandEvent& event )
{
	OnPaste( CM_Layers, event.GetId() == ID_MENU_PASTE_LAYER_TO_PARENT );
}

void CEdSceneExplorer::OnLayerShow( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	TDynArray<CLayerInfo*> layersToLoad;
	m_sceneView->GetSelection( layers );

	// Show selected layers
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layer = *it;
		layer->Show( true );
		ASSERT( layer->IsVisible(), TXT("Layer didn't became visible despite our request to do so. Debug") );

		// If the layer isn't loaded, add it to the list to load later
		if ( !layer->IsLoaded() )
		{
			layersToLoad.PushBack( layer );
		}
	}

	if ( !layersToLoad.Empty() )
	{
		// Setup loading params
		LayerGroupLoadingContext loadingContext;

		// Load layers with all that fancy precaching and stuff
		LoadLayers( layersToLoad, loadingContext );
	}

	// Scan for appearances, see comment in OnLayerHide for details
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layerInfo = *it;
		CLayer* layer = layerInfo->GetLayer();
		if ( layer != nullptr )
		{
			const LayerEntitiesArray& entities = layer->GetEntities();
			for ( auto it=entities.Begin(); it != entities.End(); ++it )
			{
				CAppearanceComponent* appearanceComponent = (*it)->FindComponent<CAppearanceComponent>();
				if ( appearanceComponent != nullptr )
				{
					CEntityTemplate* tpl = (*it)->GetEntityTemplate();
					CName currentAppearance = appearanceComponent->GetAppearance();
					if ( tpl != nullptr && !currentAppearance.Empty() )
					{
						auto app = tpl->GetAppearance( currentAppearance, true );
						if ( app != nullptr )
						{
							appearanceComponent->ApplyAppearance( *app );
						}
					}
				}
			}
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnLayerHide( wxCommandEvent &event )
{
	TDynArray<CLayerInfo*> layers;
	m_sceneView->GetSelection( layers );

	// Hide selected layers
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layerInfo = *it;
		CLayer* layer = layerInfo->GetLayer();

		// Remove appearances from entities with them (they do not handle being detached
		// well and at that point i do not want to change the appearance logic lest i
		// introduce bugs now that things seem to work ok - entities do not detach by
		// themselves in normal game and the issue only happens when an appearance
		// changes after it has been hidden and shown)
		if ( layer != nullptr )
		{
			const LayerEntitiesArray& entities = layer->GetEntities();
			for ( auto it=entities.Begin(); it != entities.End(); ++it )
			{
				CAppearanceComponent* appearanceComponent = (*it)->FindComponent<CAppearanceComponent>();
				if ( appearanceComponent != nullptr )
				{
					// Remove the appearance but do not reset the "appearance" property
					// so that when we get shown (or if the user saves the layer) it isn't lost
					appearanceComponent->RemoveCurrentAppearance( false );
				}
			}
		}

		layerInfo->Show( false );
		ASSERT( !layerInfo->IsVisible(), TXT("Layer didn't became invisible despite our request to do so. Debug") );
	}
}

void CEdSceneExplorer::OnGroupCheckOut( wxCommandEvent &event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Checkout the groups' directories
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;
		CDirectory* directory = group->GetDepotDirectory();

		if ( directory )
		{
			String path = directory->GetDepotPath();
			directory->Sync();

			// After sync the directory might have been deleted so make sure it exists
			directory = GDepot->FindPath( path.AsChar() );
			if ( directory )
			{
				directory->CheckOut();
			}
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnGroupSubmit( wxCommandEvent &event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Submit the selected groups
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;
		CDirectory* directory = group->GetDepotDirectory();

		// Save first
		group->Save( true, true );

		if ( directory )
		{
			directory->Add();
			if ( !directory->Submit() )
			{
				directory->Revert();
			}
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnGroupRevert( wxCommandEvent &event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Revert the selected groups
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;
		CDirectory* directory = group->GetDepotDirectory();

		if ( directory )
		{
			if ( directory->Revert() )
			{
				GGame->GetActiveWorld()->SynchronizeLayersAdd( group );
				group->Reload();
				GGame->GetActiveWorld()->DelayedActions();
			}
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnGroupAdd( wxCommandEvent &event )
{
	TDynArray<CLayerGroup*> groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Add the selected groups to version control
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;
		CDirectory* directory = group->GetDepotDirectory();

		if ( directory )
		{
			directory->Add();
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnGroupSync( wxCommandEvent &event )
{
	TDynArray< CLayerGroup* > groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Revert the selected groups
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;
		CDirectory* directory = group->GetDepotDirectory();

		if ( directory )
		{
			directory->Sync();
		}
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnRenameGroup( wxCommandEvent &event )
{
	TDynArray< CLayerGroup* > groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Make sure we have a single group selected
	if ( groups.Size() != 1 )
	{
		wxMessageBox( wxT("Please select a single group to rename"), wxT("Invalid selection"), wxOK|wxICON_ERROR|wxCENTRE, this );
		return;
	}

	// Get the layer info
	CLayerGroup* group = groups[0];

	// Get the new name
	String name = group->GetName();
	String prevName = name;

	if ( !InputBox( this, TXT("Rename group '") + name + TXT("'"), TXT("Enter a new name for the layer"), name ) )
	{
		return;
	}
	if ( name == prevName ) 
	{
		return;
	}

	if ( !group->Save( true, true ) || !group->Rename( name ) )
	{
		wxMessageBox( wxT("Failed to rename the group."), wxT("Rename failed"), wxICON_ERROR|wxOK|wxCENTRE, this );
		return;
	}

	MarkTreeItemsForUpdate();
}

namespace
{
	void ScanDirectoryForLayerGroups( CDirectory* dir, TDynArray< CDiskFile* >& outFiles )
	{
		const String layerGroupExt( TXT("w2lg") );

		for ( CDiskFile* file : dir->GetFiles() )
		{
			if ( file->GetFileName().EndsWith( layerGroupExt ) )
			{
				outFiles.PushBack( file );
			}
		}

		for ( CDirectory* cur : dir->GetDirectories() )
		{
			ScanDirectoryForLayerGroups( cur, outFiles );
		}
	}
}

void CEdSceneExplorer::OnWorldSaveDependencyFile( wxCommandEvent &event)
{
	String outputFolder = GFileManager->GetDataDirectory() +TXT("dep_files\\");
	CWorld* world = GGame->GetActiveWorld();
	switch ( event.GetId() )
	{
		case ID_MENU_SAVE_DEPENDENCY_FILE:
		{
			WorldSceneDependencyInfo::Start( world, outputFolder );
			break;
		}
		case ID_MENU_SAVE_FOLIAGE_DEPENDENCY_FILE:
		{
			WorldSceneDependencyInfo::Start( world, outputFolder, true );
			break;
		}
	}
}

void CEdSceneExplorer::OnWorldDeleteLayerGroupFiles( wxCommandEvent &event )
{
	// enumerate the layer group files
	TDynArray< CDiskFile* > layerGroupFiles;
	CDirectory* depotDir = nullptr;
	CWorld* world = GGame->GetActiveWorld();
    if ( world )
    {
		depotDir = GDepot->FindPath( world->GetDepotPath().AsChar() );
		if ( depotDir != nullptr )
		{
			ScanDirectoryForLayerGroups( depotDir, layerGroupFiles );
		}
	}

	// no files to delete
	if ( layerGroupFiles.Empty() )
	{
		wxMessageBox( wxT("No layer group files to delete"), wxT("Error"), wxOK | wxICON_ERROR, this );
		return;
	}

	// local mode
	if ( GVersionControl->IsSourceControlDisabled() )
	{
		if ( wxNO == wxMessageBox( wxT("Source control integration is disabled, you will have to submit the files manually. Proceed ?"), wxT("Warning"), wxYES_NO | wxICON_QUESTION, this ) )
		{
			return;
		}
	}

	// delete groups
	Uint32 numFailed = 0;
	Uint32 numLockedFiles = 0;
	if ( !layerGroupFiles.Empty() )
	{
		GFeedback->BeginTask( TXT("Deleting layer group files..."), true );

		for ( Uint32 i=0; i<layerGroupFiles.Size(); ++i )
		{
			CDiskFile* file = layerGroupFiles[i];

			GFeedback->UpdateTaskProgress( i, layerGroupFiles.Size() );

			if ( GFeedback->IsTaskCanceled() )
			{
				numFailed += layerGroupFiles.Size() - i;
				break;
			}

			// do not delete checked out files
			if ( !file->IsLocal() && (file->IsCheckedOut() || file->IsNotSynced()) )
			{
				numLockedFiles += 1;
				numFailed += 1;
				continue;
			}

			// do not confirm, do not auto submit
			if ( !file->Delete( false, false ) )
			{
				numFailed += 1;
			}
		}

		GFeedback->EndTask();
	}

	// Submit changes
	if ( depotDir != nullptr )
	{
		if ( !depotDir->Submit( String( TXT("Auto removed the layer group files") ) ) )
		{
			wxMessageBox( wxT("Failed to submit removed files to depot"), wxT("Error"), wxOK | wxICON_ERROR, this );
		}
	}

	// Final message
	if ( numFailed > 0 || numLockedFiles > 0 )
	{
		wxString msg = msg.Format( wxT("Failed to submit remove %d files (%d locked) out of %d files found.\n\nManual deletion in P4 is recommended."),
			numFailed, numLockedFiles, layerGroupFiles.Size() );

		wxMessageBox( msg, wxT("Error"), wxOK | wxICON_ERROR, this );
	}
	else
	{
		wxString msg = msg.Format( wxT("Removed %d layer group files. Please make sure this gets submitted to P4"),
			layerGroupFiles.Size() );

		wxMessageBox( msg, wxT("Successs"), wxOK | wxICON_INFORMATION, this );
	}
}

void CEdSceneExplorer::OnImportResource( wxCommandEvent& event )
{
	TDynArray< CResource* > resources;
	m_sceneView->GetSelection( resources );

	// Make sure we have a single resource selectde
	if ( resources.Size() != 1 )
	{
		wxMessageBox( wxT("To perform the import, a single resource must be selected."), wxT("Invalid selection"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}

	// Get resource
	CResource* resource = resources[0];
	
	// Get resource class
    CClass* resourceClass = resource->GetClass();

    // Find importer
    // Enumerate importable formats
    TDynArray< CFileFormat > formats;
    IImporter::EnumImportFormats( resourceClass, formats );

    if ( formats.Size() == 0 )
    {
        WARN_EDITOR( TXT("No valid importer for %s '%s'"), resourceClass->GetName().AsString().AsChar(), resource->GetFriendlyName().AsChar() );
        return;		
    }

    // Decompose to file path
    String name;
    if ( resource->GetFile() )
    {
        name = resource->GetFile()->GetFileName();
    }
    else 
    {
        name = resource->GetImportFile();
    }

    String defaultDir = String::EMPTY;
    String defaultFile = name;
    String wildCard;
    CFilePath loadPath;

    for ( Uint32 i=0; i<formats.Size(); i++ )
    {
        wildCard += String::Printf( TXT("%s (*.%s)|*.%s")
            , formats[i].GetDescription().AsChar()
            , formats[i].GetExtension().AsChar() 
            , formats[i].GetExtension().AsChar() );

        if ( i < formats.Size() - 1 )
        {
            wildCard += TXT("|");
        }
    }

    wxFileDialog loadFileDialog( this, TXT("Import file"), defaultDir.AsChar(), defaultFile.AsChar(), wildCard.AsChar(), wxFD_OPEN );
    if ( loadFileDialog.ShowModal() == wxID_OK )
    {
        loadPath = String( loadFileDialog.GetPath().wc_str() );
    }
    else 
    {
        return;
    }

    IImporter* importer = IImporter::FindImporter( resourceClass, loadPath.GetExtension() );

    if ( !importer )
    {
        WARN_EDITOR( TXT("No valid importer for extension '%s' for %s")
            , loadPath.GetExtension().AsChar()
            , resource->GetFriendlyName().AsChar() );
        return;		
    }

    IImporter::ImportOptions importOptions;
    importOptions.m_existingResource = resource;
    importOptions.m_parentObject = resource->GetParent();
    importOptions.m_sourceFilePath = loadPath.ToString();

    LOG_EDITOR( TXT("Importing resource %s as %s.")
        , importOptions.m_existingResource->GetFriendlyName().AsChar()
        , importOptions.m_sourceFilePath.AsChar() );

    importer->DoImport( importOptions );
}

void CEdSceneExplorer::OnExportResource( wxCommandEvent& event )
{
	TDynArray< CResource* > resources;
	m_sceneView->GetSelection( resources );

	// Make sure we have a single resource selectde
	if ( resources.Size() != 1 )
	{
		wxMessageBox( wxT("To perform the import, a single resource must be selected."), wxT("Invalid selection"), wxICON_ERROR|wxCENTRE|wxOK, this );
		return;
	}

	// Get resource
	CResource* resource = resources[0];

	if ( resource->IsA<CLayerGroup>() )
    {
        CLayerGroup *group = reinterpret_cast<CLayerGroup*>( resource );
        resource = group->GetWorld();
    } 

    if ( resource )
    {
        // Get resource class
        CClass* resourceClass = resource->GetClass();

        // Find exporter
        // Enumerate exportable formats
        TDynArray< CFileFormat > formats;
        IExporter::EnumExportFormats( resourceClass, formats );

        if ( formats.Size() == 0 )
        {
            WARN_EDITOR( TXT("No valid exporter for %s '%s'"), resourceClass->GetName().AsString().AsChar(), resource->GetFriendlyName().AsChar() );
            return;		
        }

        // Decompose to file path
        String name;
        if ( resource->GetFile() )
        {
            name = resource->GetFile()->GetFileName();
        }
        else 
        {
            name = resource->GetImportFile();
        }

        String defaultDir = String::EMPTY;
        String defaultFile = name;
        String wildCard;
        static CFilePath savePath;

        for ( Uint32 i=0; i<formats.Size(); i++ )
        {
            wildCard += String::Printf( TXT("%s (*.%s)|*.%s")
                , formats[i].GetDescription().AsChar()
                , formats[i].GetExtension().AsChar() 
                , formats[i].GetExtension().AsChar() );

            if ( i < formats.Size() - 1 )
            {
                wildCard += TXT("|");
            }
        }

        wxFileDialog saveFileDialog( this, TXT("Export as"), defaultDir.AsChar(), defaultFile.AsChar(), wildCard.AsChar(), wxFD_SAVE );
        if ( saveFileDialog.ShowModal() == wxID_OK )
        {
            savePath = String( saveFileDialog.GetPath().wc_str() );

            if ( saveFileDialog.GetFilterIndex() >= 0 && saveFileDialog.GetFilterIndex() < (Int32)formats.Size() )
            {
                savePath.SetExtension( formats[ saveFileDialog.GetFilterIndex() ].GetExtension() );
            }
        }
        else 
        {
            return;
        }

        IExporter* exporter = IExporter::FindExporter( resourceClass, savePath.GetExtension() );

        if ( !exporter )
        {
            WARN_EDITOR( TXT("No valid exporter for extension '%s' for %s")
                , savePath.GetExtension().AsChar()
                , resource->GetFriendlyName().AsChar() );
            return;		
        }

        IExporter::ExportOptions exportOptions;
        exportOptions.m_resource = resource;
        exportOptions.m_saveFileFormat = CFileFormat( savePath.GetExtension(), String::EMPTY );
        exportOptions.m_saveFilePath = savePath.ToString();

        LOG_EDITOR( TXT("Exporting resource %s as %s.")
            , exportOptions.m_resource->GetFriendlyName().AsChar()
            , exportOptions.m_saveFilePath.AsChar() );

        exporter->DoExport( exportOptions );
    }
}

void CEdSceneExplorer::SaveSession( CConfigurationManager &config )
{
    CWorld* world = GGame->GetActiveWorld();
    if ( world )
    {
        // Make sure a tree structure is up to date
        OnInternalIdle();

        LOG_EDITOR( TXT("Saving session...") );

		// Save active layer
        String activeLayerPath;
        if ( CLayer *layer = GetActiveLayer() )
        {
            activeLayerPath = layer->GetLayerInfo()->GetDepotPath();
        }
        config.Write( TXT("Scene/ActiveLayer"), activeLayerPath );
		
		// Save flag state
		config.Write( TXT("Scene/DrawLBTBackgrounds"), DrawLBTBackgrounds ? 1 : 0 );

        LOG_EDITOR( TXT("Session saved.") );
    }
}

void CEdSceneExplorer::RestoreSession( CConfigurationManager &config )
{
    CWorld* world = GGame->GetActiveWorld();
    if ( world )
    {
        LOG_EDITOR( TXT("Restoring session...") );

        // Load presets from config
        wxCommandEvent event;
        OnLoadPresets( event );

        // Restore active layer
        TDynArray< CLayerInfo* > worldLayers;
        world->GetWorldLayers()->GetLayers( worldLayers, false );
        String activeLayer = config.Read( TXT("Scene/ActiveLayer"), String::EMPTY );
        for( Uint32 i = 0; i < worldLayers.Size(); ++i )
        {				
            if ( worldLayers[i]->GetShortName() == activeLayer )
            {
                ChangeActiveLayer( worldLayers[i] );
            }
        }

		// Save flag state
		DrawLBTBackgrounds = config.Read( TXT("Scene/DrawLBTBackgrounds"), DrawLBTBackgrounds ? 1 : 0 ) != 0;

        //m_restoreTimer.Start( 500, true );
        LOG_EDITOR( TXT("Session restored.") );
    }
}

void CEdSceneExplorer::ClearPresets()
{
	m_presetManager.Reset();
    RefreshPresetComboBox();
}

void CEdSceneExplorer::OnMoveContentToLayer( wxCommandEvent& event )
{
#if 0
    if( !m_srcLayer || !m_dstLayer )
    {
        return;
    }

    // Load source layer
    if( !m_srcLayer->IsLoaded() )
    {
        LayerLoadingContext loadingContext;
        m_srcLayer->SyncLoad( loadingContext );
    }

    // Load destination layer
    if( !m_dstLayer->IsLoaded() )
    {
        LayerLoadingContext loadingContext;
        m_dstLayer->SyncLoad( loadingContext );
    }

    TDynArray< CEntity* > entities;
    m_srcLayer->GetLayer()->GetEntities( entities );
    for( TDynArray< CEntity* >::iterator it=entities.Begin(); it!=entities.End(); it++ )
    {
        m_srcLayer->GetLayer()->MoveEntity( *it, m_dstLayer->GetLayer() );
    }

    m_srcLayer = m_dstLayer = NULL;
#endif
}

void CEdSceneExplorer::OnMergeLayers( wxCommandEvent& event )
{
#if 0
    if ( !m_srcLayer || !m_dstLayer )
    {
        return;
    }
    CLayerInfo *src = m_srcLayer;
    OnMoveContentToLayer( event );
    if ( src->IsLoaded() )
    {
        src->SyncUnload();

        if ( GGame->GetActiveWorld() )
        {
            GGame->GetActiveWorld()->UpdateLoadingState();
        }
    }
    src->Remove();
#endif
}

#if 0
void CEdSceneExplorer::OnKeyDown( wxTreeEvent& event )
{
    switch ( event.GetKeyCode() )
    {
    case WXK_F2:
        {
            // Find selected element
            wxArrayTreeItemIds selectedIds;
            Uint32 selectedCount = m_sceneTree->GetSelections( selectedIds );
            if ( selectedCount > 0 )
            {
                // Edit label
                m_sceneTree->EditLabel( selectedIds[0] );
            }

            return;
        }
    case WXK_DELETE:
        {
            CWorld *world = GGame->GetActiveWorld();
            if ( world == NULL )
            {
                return;
            }

            SetCursor( wxCURSOR_ARROWWAIT );
            m_stopProcessingEvents = true;
            m_sceneTree->Freeze();
            wxArrayTreeItemIds selectedItems;
            Uint32 selectedCount = m_sceneTree->GetSelections( selectedItems );

            // Remove all selected nodes
            RemoveNodes( selectedItems );
            world->DelayedActions();
            RemoveLayers( selectedItems );
            world->DelayedActions();
            RemoveLayerGroups( selectedItems );
            world->DelayedActions();

            m_sceneTree->Thaw();
            m_stopProcessingEvents = false;
            m_recreateTreePending = true;
            SetCursor( wxCURSOR_ARROW );

            return;
        }
    }

    event.Skip();
}

void CEdSceneExplorer::OnBeginEditLabel( wxTreeEvent& event )
{
    wxTreeItemId itemId = event.GetItem();

    if (itemId.IsOk())
    {
        // Enable only for CNode
        NodeItemWrapper* item = static_cast< NodeItemWrapper* >( m_sceneTree->GetItemData( itemId ) );
        if( item && item->m_node->IsA< CNode >() )
            return;
    }

    event.Veto();
}

void CEdSceneExplorer::OnEndEditLabel( wxTreeEvent& event )
{
    NodeItemWrapper* item = static_cast< NodeItemWrapper* >( m_sceneTree->GetItemData( event.GetItem() ) );

    String newName = m_sceneTree->GetItemText( event.GetItem() );

    item->m_node->SetName( newName );

    m_updateTreeItemsPending = true;

    CWorld* world = GGame->GetActiveWorld();
    if ( world )
    {
        SEvents::GetInstance().QueueEvent( CNAME( SelectionPropertiesChanged ), CreateEventData( world ) );
    }
}
#endif

Bool CEdSceneExplorer::OnDropText( wxCoord x, wxCoord y, String &text )
{
    return true;
}

void CEdSceneExplorer::OnSelectByResource( wxCommandEvent& event )
{
#if 0
    CWorld *world = GGame->GetActiveWorld();

    // Get selected resources
    ResourceLayerWrapper *wrapper = static_cast< ResourceLayerWrapper * >( event.m_callbackUserData );
    CResource *res = wrapper ? wrapper->m_resource : NULL;
    CLayer *layer = wrapper ? wrapper->m_layer : NULL;

    if ( !world || !res || !res->GetFile() || !layer )
    {
        return;
    }

    CSelectionManager::CSelectionTransaction transaction(*world->GetSelectionManager());
    world->GetSelectionManager()->DeselectAll();

    const String &resourceFriendlyName = res->GetFriendlyName();
    TDynArray< CEntity * > entities;
    layer->GetEntities( entities );

    for ( Uint32 i = 0; i < entities.Size(); ++i )
    {
        CEntity *entity = entities[i];
        TDynArray< CResource * > usedResources;
        entity->CollectUsedResources( usedResources );

        for ( TDynArray< CResource * >::iterator it = usedResources.Begin(); it != usedResources.End(); ++it )
        {
            if ( (*it)->GetFriendlyName() == resourceFriendlyName )
            {
                world->GetSelectionManager()->Select( entity );
            }
        }
    }
#endif
}

void CEdSceneExplorer::SelectSpecifiedEntities( SceneView* sceneView, const CLayerInfo *layerInfo, const CClass* givenClass /* = nullptr */,  
											   const TDynArray< CClass* >& componentsClasses, const TDynArray< CClass* >& excludedComponents  )
{
	// selectes entities of a given class that don't contain excluded components or entities that contain given components

	if ( layerInfo->IsLoaded() )
	{
		CSelectionManager *selMan = layerInfo->GetWorld()->GetSelectionManager();
		ASSERT( selMan );

		CLayer *layer = layerInfo->GetLayer();
		TDynArray< CEntity* > entities; // en(.)(.)
		layer->GetEntities( entities );

		for ( Uint32 i = 0; i < entities.Size(); ++i )
		{
			CEntity* entity = entities[i];

			Bool hasGivenComponent = false, hasExcludedComponent = false;
			if ( !componentsClasses.Empty() || !excludedComponents.Empty() )
			{
				// check if has any components of componentsClasses
				SEntityStreamingState worldState;
				entity->PrepareStreamingComponentsEnumeration( worldState, true, SWN_DoNotNotifyWorld );
				entity->ForceFinishAsyncResourceLoads();

				const TDynArray< CComponent* >& components = entity->GetComponents();
				for ( Uint32 j = 0; j < components.Size(); ++j )
				{
					for ( Uint32 k = 0; k < componentsClasses.Size(); ++k )
					{
						if ( components[j]->IsA( componentsClasses[k] ) )
						{
							hasGivenComponent = true;
							break;
						}
					}

					for ( Uint32 k = 0; k < excludedComponents.Size(); ++k )
					{
						if ( components[j]->IsA( excludedComponents[k] ) )
						{
							hasExcludedComponent = true;
							break;
						}
					}
				}

				entity->FinishStreamingComponentsEnumeration( worldState );
			}

			Bool isOfGivenClass = givenClass != nullptr && entity->IsA( givenClass );
			if ( hasGivenComponent || ( isOfGivenClass && !hasExcludedComponent ) )
			{
				selMan->Select( entities[ i ] );
				sceneView->Select( entities[ i ] );
			}
		}
	}
}

static void SelectAllEntitiesOnLayer( SceneView* sceneView, const CLayerInfo *layerInfo )
{
    if ( layerInfo->IsLoaded() )
    {
        CSelectionManager *selMan = layerInfo->GetWorld()->GetSelectionManager();
        ASSERT( selMan );

        CLayer *layer = layerInfo->GetLayer();
        TDynArray< CEntity* > entities; // en(.)(.)
        layer->GetEntities( entities );

        for ( Uint32 i = 0; i < entities.Size(); ++i )
        {
			selMan->Select( entities[ i ] );
			sceneView->Select( entities[ i ] );
		}
    }
}

void CEdSceneExplorer::OnSelectAllEntities( wxCommandEvent& event )
{
	// Note: this is done with RunLater to avoid changing the selection from menu click-through
	TDynArray< ISerializable* > selection;
	m_sceneView->GetSelection( selection );
	GGame->GetActiveWorld()->GetSelectionManager()->DeselectAll();
	m_sceneView->ClearSelection();

	RunLater( [ this, selection ]() {
		for ( auto it=selection.Begin(); it != selection.End(); ++it )
		{
			ISerializable* obj = *it;
		
			if ( obj->IsA< CLayerInfo >() )
			{
				CSelectionManager::CSelectionTransaction transaction( *GGame->GetActiveWorld()->GetSelectionManager() );
				CLayerInfo *layer = Cast< CLayerInfo > ( obj );        
				SelectAllEntitiesOnLayer( m_sceneView, layer );
			}
			else if ( obj->IsA< CLayerGroup >() )
			{
				CSelectionManager::CSelectionTransaction transaction( *GGame->GetActiveWorld()->GetSelectionManager() );
				CLayerGroup *group = Cast< CLayerGroup > ( obj );
				TDynArray< CLayerInfo* > arr;
				group->GetLayers( arr, false );

				for ( Uint32 i = 0; i < arr.Size(); ++i )
				{
					SelectAllEntitiesOnLayer( m_sceneView, arr[ i ] );	
				}
			}
		}

		MarkTreeItemsForUpdate();
	} );

#if 0
    CWorld *world = GGame->GetActiveWorld();
    if ( world == NULL )
    {
        return;
    }

    if ( ISerializable* object = GetTreeItemObjectEx<ISerializable>( m_sceneTree, m_sceneTree->GetSelection() ) )
    {
        m_isSelectedChanging = true;
        if ( object->IsA< CLayerInfo >() )
        {
            CSelectionManager::CSelectionTransaction transaction(*world->GetSelectionManager());
            CLayerInfo *layer = Cast< CLayerInfo > ( object );        
            world->GetSelectionManager()->DeselectAll();
            SelectAllEntitiesOnLayer( layer );
        }
        else if ( object->IsA< CLayerGroup >() )
        {
            CSelectionManager::CSelectionTransaction transaction(*world->GetSelectionManager());
            CLayerGroup *group = Cast< CLayerGroup > ( object );
            TDynArray< CLayerInfo* > arr;
            group->GetLayers( arr, true );
            world->GetSelectionManager()->DeselectAll();

            for ( Uint32 i = 0; i < arr.Size(); ++i )
            {
                SelectAllEntitiesOnLayer( arr[ i ] );	
            }
        }
        m_isSelectedChanging = false;
    }
#endif
}

void CEdSceneExplorer::OnSelectFromSpecifiedLayers( wxCommandEvent& event )
{
	TDynArray< ISerializable* > selection;
	m_sceneView->GetSelection( selection );
	GGame->GetActiveWorld()->GetSelectionManager()->DeselectAll();
	m_sceneView->ClearSelection();

	Bool lights = false, containers = false, other = false;
	Int32 result = FormattedDialogBox( this, wxT("Selecting entities"), wxT("X'Lights' X'Containers' X'Other Gameplay Entities'|H{B@'OK' B'Cancel'}"), &lights, &containers, &other );
	if ( result == -1 || result == 1 || ( !lights && !containers && !other ) ) // X or Cancel
	{
		return;
	}

	Int32 eventId = event.GetId();
	RunLater( [ this, selection, eventId, lights, containers, other ]() {

		CClass* givenClass = other ? ClassID< CGameplayEntity >() : nullptr;

		TDynArray < CClass* > componentsClasses, excludedomponentsClasses;
		if ( lights )
		{
			componentsClasses.PushBack( ClassID< CLightComponent >() );
			componentsClasses.PushBack( ClassID< CGameplayLightComponent >() );
		}
		else
		{
			excludedomponentsClasses.PushBack( ClassID< CLightComponent >() );
			excludedomponentsClasses.PushBack( ClassID< CGameplayLightComponent >() );
		}

		if( containers )
		{
			componentsClasses.PushBack( ClassID< CInventoryComponent >() );
		}
		else
		{
			excludedomponentsClasses.PushBack( ClassID< CInventoryComponent >() );
		}

		Bool onlyEnvLayers = ( eventId == ID_MENU_SELECT_ALL_IN_ENV );

		for ( auto it=selection.Begin(); it != selection.End(); ++it )
		{
			ISerializable* obj = *it;
			if ( obj->IsA< CWorld >() )
			{
				obj = static_cast<CWorld*>( obj )->GetWorldLayers();
			}

			if ( obj->IsA< CLayerInfo >() )
			{
				CLayerInfo *layer = Cast< CLayerInfo > ( obj );        
				if ( layer && ( !onlyEnvLayers || layer->IsEnvironment() ) )
				{
					CSelectionManager::CSelectionTransaction transaction( *GGame->GetActiveWorld()->GetSelectionManager() );
					SelectSpecifiedEntities( m_sceneView, layer, givenClass, componentsClasses, excludedomponentsClasses );
				}
			}
			else if ( obj->IsA< CLayerGroup >() )
			{
				CSelectionManager::CSelectionTransaction transaction( *GGame->GetActiveWorld()->GetSelectionManager() );
				CLayerGroup *group = Cast< CLayerGroup > ( obj );
				TDynArray< CLayerInfo* > arr;
				group->GetLayers( arr, false );

				for ( Uint32 i = 0; i < arr.Size(); ++i )
				{
					if ( arr[i] && ( !onlyEnvLayers || arr[i]->IsEnvironment() ) )
					{
						SelectSpecifiedEntities( m_sceneView, arr[ i ], givenClass, componentsClasses, excludedomponentsClasses );	
					}
				}
			}
		}

		MarkTreeItemsForUpdate();
	} );
}

void CEdSceneExplorer::OnAddShadowsToGroup( wxCommandEvent& event )
{
	if( GFeedback->AskYesNo( TXT("Adding shadow casting to ALL MESHES in all layers. Are you sure?") ) )
	{
		TDynArray< CLayerGroup* > groups;
		m_sceneView->GetSelectedLayerGroups( groups );

		for ( auto it=groups.Begin(); it != groups.End(); ++it )
		{
			(*it)->AddShadowsToGroup();
		}

		MarkTreeItemsForUpdate();
#if 0
		CLayerGroup* group = GetTreeItemObjectEx< CLayerGroup >( m_sceneTree, m_sceneTree->GetSelection() );
		if ( group )
		{
			group->AddShadowsToGroup();
		}

		m_recreateTreePending = true;
#endif
	}
}

void CEdSceneExplorer::OnRemoveShadowsToGroup( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		(*it)->RemoveShadowsFromGroup();
	}

	MarkTreeItemsForUpdate();
#if 0
    CLayerGroup* group = GetTreeItemObjectEx< CLayerGroup >( m_sceneTree, m_sceneTree->GetSelection() );
    if ( group )
    {
        group->RemoveShadowsFromGroup();
    }

    m_recreateTreePending = true;
#endif
}

void CEdSceneExplorer::OnAddShadowsToLayer( wxCommandEvent& event )
{
	if( GFeedback->AskYesNo( TXT("Adding shadow casting to ALL MESHES in this layer(s). Are you sure?") ) )
	{	
		TDynArray< CLayerInfo* > layers;
		m_sceneView->GetSelection( layers );

		for ( auto it=layers.Begin(); it != layers.End(); ++it )
		{
			if ( (*it)->IsLoaded() )
			{
				(*it)->GetLayer()->AddShadowsToLayer();
			}
		}

		MarkTreeItemsForUpdate();
#if 0
		CLayerInfo* layer = GetTreeItemObjectEx< CLayerInfo >( m_sceneTree, m_sceneTree->GetSelection() );
		if ( layer && layer->IsLoaded() )
		{
			layer->GetLayer()->AddShadowsToLayer();
		}

		m_recreateTreePending = true;
#endif

	}
}

void CEdSceneExplorer::OnAddShadowsFromLocalLightsToLayer( wxCommandEvent& event )
{	
	TDynArray< CLayerInfo* > layers;
	m_sceneView->GetSelection( layers );

	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		if ( (*it)->IsLoaded() )
		{
			(*it)->GetLayer()->AddShadowsFromLocalLightsToLayer();
		}
	}
}

void CEdSceneExplorer::OnAddShadowsFromLocalLightsToGroup( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		(*it)->AddShadowsFromLocalLightsToGroup();
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnRemoveShadowsToLayer( wxCommandEvent& event )
{
	TDynArray< CLayerInfo* > layers;
	m_sceneView->GetSelection( layers );

	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		if ( (*it)->IsLoaded() )
		{
			(*it)->GetLayer()->RemoveShadowsFromLayer();
		}
	}

	MarkTreeItemsForUpdate();

#if 0
    CLayerInfo* layer = GetTreeItemObjectEx< CLayerInfo >( m_sceneTree, m_sceneTree->GetSelection() );
    if ( layer && layer->IsLoaded() )
    {
        layer->GetLayer()->RemoveShadowsFromLayer();
    }

    m_recreateTreePending = true;
#endif
}

void CEdSceneExplorer::OnConvertGroupToStreamedTemplatesOnly( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		(*it)->ConvertToStreamed( true );
	}

	MarkTreeItemsForUpdate();
#if 0
	CLayerGroup* group = GetTreeItemObjectEx< CLayerGroup >( m_sceneTree, m_sceneTree->GetSelection() );
	if ( group )
	{
		group->ConvertToStreamed(true);
	}

	m_recreateTreePending = true;
#endif
}

void CEdSceneExplorer::OnConvertGroupToStreamed( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		(*it)->ConvertToStreamed();
	}

	MarkTreeItemsForUpdate();
#if 0
	CLayerGroup* group = GetTreeItemObjectEx< CLayerGroup >( m_sceneTree, m_sceneTree->GetSelection() );
	if ( group )
	{
		group->ConvertToStreamed();
	}

	m_recreateTreePending = true;
#endif
}

void CEdSceneExplorer::OnConvertLayerToStreamed( wxCommandEvent& event )
{
	TDynArray< CLayerInfo* > layers;
	m_sceneView->GetSelection( layers );

	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		if ( (*it)->IsLoaded() )
		{
			(*it)->GetLayer()->ConvertToStreamed();
		}
	}

	MarkTreeItemsForUpdate();
#if 0
	CLayerInfo* layer = GetTreeItemObjectEx< CLayerInfo >( m_sceneTree, m_sceneTree->GetSelection() );
	if ( layer && layer->IsLoaded() )
	{
		layer->GetLayer()->ConvertToStreamed();
	}

	m_recreateTreePending = true;
#endif
}

static void SetTypeForLayers( TDynArray< CLayerInfo* >& layers, ELayerType layerType )
{
	Uint32 counter = 0;
	Uint32 max = layers.Size();
	GFeedback->BeginTask( TXT("Setting values"), false );
	GFeedback->UpdateTaskProgress( 0, max );
	for ( auto it = layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layerInfo = *it;
		if ( layerInfo )
		{
			layerInfo->MarkModified();
			CLayer* layer = layerInfo->GetLayer();
			if ( layer )
			{
				RED_VERIFY( layer->MarkModified() );
				layerInfo->SetLayerType( layerType );
			}			
		}
		GFeedback->UpdateTaskProgress( ++counter, max );
	}
	GFeedback->EndTask();
}

static void CollectLayers( CLayerGroup* layerGroup, TDynArray< CLayerInfo* >& layers )
{
	if ( !layerGroup )
	{
		return;
	}

	const CLayerGroup::TLayerList& layersInGroup = layerGroup->GetLayers();
	for ( auto it = layersInGroup.Begin(); it != layersInGroup.End(); ++it )
	{
		layers.PushBack( *it );
	}
	
	// recursive call
	TDynArray< CLayerGroup* > layerGroups;
	layerGroup->GetLayerGroups( layerGroups );
	for ( auto it = layerGroups.Begin(); it != layerGroups.End(); ++it )
	{
		CollectLayers( *it, layers );
	}
}

void CEdSceneExplorer::OnConvertGroupToNonStatic( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > layerGroups;
	m_sceneView->GetSelectedLayerGroups( layerGroups );

	GFeedback->BeginTask( TXT("Collecting layers"), false );
	TDynArray< CLayerInfo* > layers;
	for ( auto it = layerGroups.Begin(); it != layerGroups.End(); ++it )
	{
		CollectLayers( *it, layers );
	}
	GFeedback->EndTask();

	SetTypeForLayers( layers, LT_NonStatic );

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnConvertGroupToStatic( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > layerGroups;
	m_sceneView->GetSelectedLayerGroups( layerGroups );

	GFeedback->BeginTask( TXT("Collecting layers"), false );
	TDynArray< CLayerInfo* > layers;
	for ( auto it = layerGroups.Begin(); it != layerGroups.End(); ++it )
	{
		CollectLayers( *it, layers );
	}
	GFeedback->EndTask();

	SetTypeForLayers( layers, LT_AutoStatic );

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnConvertAllLayersTo( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > layerGroups;
	m_sceneView->GetSelectedLayerGroups( layerGroups );
	Int32 ca=-1;
	Int32 cb=-1;
	Int32 ret = FormattedDialogBox( this, wxT("Convert Layers to:"), wxT("'Select Group:'H{V{L('LT_AutoStatic''LT_NonStatic')}|V{L('LBT_None''LBT_Ignored' 'LBT_EnvOutdoor' 'LBT_EnvIndoor' 'LBT_EnvUnderground' 'LBT_Quest' 'LBT_Communities' 'LBT_Audio' 'LBT_Nav' 'LBT_Gameplay' 'LBT_Max')}|V{B@'OK'|B'Cancel'}}"), &ca, &cb );
	if( ret==0 )
	{
		if( ca>=0 )
		{
			GFeedback->BeginTask( TXT("Collecting layers"), false );
			TDynArray< CLayerInfo* > layers;
			for ( auto it = layerGroups.Begin(); it != layerGroups.End(); ++it )
			{
				CollectLayers( *it, layers );
			}
			GFeedback->EndTask();
			for( auto aa = layers.Begin(); aa!=layers.End(); ++aa )
			{
				CLayerInfo* lay = *aa;
				lay->SetLayerType( (ca==0 ? LT_AutoStatic : LT_NonStatic) );
			}
		}
		if( cb>=0 )
		{
			GFeedback->BeginTask( TXT("Collecting layers"), false );
			TDynArray< CLayerInfo* > layers;
			for ( auto it = layerGroups.Begin(); it != layerGroups.End(); ++it )
			{
				CollectLayers( *it, layers );
			}
			GFeedback->EndTask();
			for( auto aa = layers.Begin(); aa!=layers.End(); ++aa )
			{
				CLayerInfo* lay = *aa;
				lay->SetLayerBuildTag( ELayerBuildTag(cb) );
			}
		}
		MarkTreeItemsForUpdate();
	}
};

void CEdSceneExplorer::OnResetLightChannels( wxCommandEvent& event )
{
#if 0
	CLayerGroup* group = GetTreeItemObjectEx< CLayerGroup >( m_sceneTree, m_sceneTree->GetSelection() );
	if ( group )
	{
		TDynArray< CLayerInfo* > layers;
		group->GetLayers( layers, true, true );

		for ( Uint32 i = 0; i < layers.Size(); ++i )
		{
			TDynArray< CEntity* > entities;
			layers[ i ]->GetLayer()->GetEntities( entities );

			Bool layerModified     = false;
			Bool breakEntitiesLoop = false;

			for ( Uint32 j = 0; j < entities.Size(); ++j )
			{
				if ( entities[ j ]->GetTemplate() == NULL )
				{
					TDynArray< CDrawableComponent* > drawables;
					CollectEntityComponents( entities[ j ], drawables );

					for ( Uint32 k = 0; k < drawables.Size(); ++k )
					{
						if ( drawables[ k ]->GetLightChannels() != LC_Default )
						{
							if ( ! layerModified )
							{
								if ( ! layers[ i ]->GetLayer()->MarkModified() )
								{
									breakEntitiesLoop = true;
									break;
								}
								layerModified = true;
							}

							
							drawables[ k ]->SetLightChannels( LC_Default );
						}
					}

					if ( breakEntitiesLoop )
					{
						break;
					}
				}
			}
		}
	}
#endif
}

void CEdSceneExplorer::RemoveForceNoAutohide( CLayerGroup* group, TDynArray<String> &unableToCheckoutLayers, 
											 Uint32 &processedItemsAutohide, Uint32 &currentGroupNum, Uint32 &currentLayerNum )
{	
	Uint32 processedItemsHLODS = 0;

	const CLayerGroup::TGroupList& subgroups = group->GetSubGroups();
	
	if( subgroups.Size() > 0 )
	{
		for ( auto it=subgroups.Begin(); it != subgroups.End(); ++it )
		{
			++currentGroupNum;
			RemoveForceNoAutohide( (*it), unableToCheckoutLayers, processedItemsAutohide, currentGroupNum, currentLayerNum );
		}
	}
	
	Bool wasGroupLoaded = false;
	Bool wasLayerLoaded = false;

	LayerLoadingContext llc;
	llc.m_loadHidden = true;		
	
	wasGroupLoaded = group->IsLoaded();
	group->SyncLoad( llc );	

	TDynArray< CLayerInfo* > layers;
	group->GetLayers( layers, true, true );

	currentLayerNum += layers.Size();

	GFeedback->UpdateTaskName( group->GetDepotPath().AsChar() );	

	for ( Uint32 i = 0; i < layers.Size(); ++i )
	{		
		wasLayerLoaded = layers[i]->IsLoaded();
		layers[i]->SyncLoad(llc);

		TDynArray< CEntity* > entities;
		layers[ i ]->GetLayer()->GetEntities( entities );

		Bool layerModified     = false;
		Bool breakEntitiesLoop = false;

		for ( Uint32 j = 0; j < entities.Size(); ++j )
		{
			if ( entities[ j ]->GetTemplate() == NULL )
			{
				// force stream in
				entities[j]->CreateStreamedComponents( SWN_NotifyWorld );			
				entities[j]->ForceFinishAsyncResourceLoads();

				Bool shouleRecreateStreamingBuffers = false;

				TDynArray< CDrawableComponent* > drawables;
				CollectEntityComponents( entities[ j ], drawables );					

				for ( Uint32 k = 0; k < drawables.Size(); ++k )
				{
					Bool processForceAutohide = false;
					Bool processForceHighestLOD = false;

					if ( drawables[ k ]->GetDrawableFlags() & DF_ForceNoAutohide ) processForceAutohide = true;
					if ( drawables[ k ]->GetDrawableFlags() & DF_ForceHighestLOD ) processForceHighestLOD = true;

					if( processForceAutohide || processForceHighestLOD )
					{
						if ( ! layerModified )
						{
							// unable to check out																
							if( !layers[ i ]->GetLayer()->GetFile()->IsCheckedOut() )
							{
								if( !layers[ i ]->GetLayer()->GetFile()->SilentCheckOut() )
								{
									breakEntitiesLoop = true;									
									break;
								}								
							}
							layerModified = true;
						}
						if( processForceAutohide ) 
						{
							drawables[ k ]->SetForceNoAutohide( false );
							shouleRecreateStreamingBuffers = true;
							++processedItemsAutohide;
						}
						if( processForceHighestLOD ) 
						{
							drawables[ k ]->SetForcedHighestLOD( false );	
							++processedItemsHLODS;
						}							
					}
				}

				if ( breakEntitiesLoop )
				{
					break;
				}

				if( shouleRecreateStreamingBuffers ) entities[j]->UpdateStreamedComponentDataBuffers();
			}
		}

		const String layerDepotPath = layers[i]->GetLayer()->GetDepotPath();

		if( breakEntitiesLoop )
		{			
			unableToCheckoutLayers.PushBack( layerDepotPath );			
		}

		if( layerModified && !layers[i]->Save() ) GFeedback->ShowWarn( TXT("Unable to save layer: %s"), layerDepotPath.AsChar() );

		if( !wasLayerLoaded ) layers[i]->SyncUnload();
	}	

	if( !wasGroupLoaded ) group->SyncUnload();
}

void CEdSceneExplorer::OnRemoveForceNoAutohide( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > groups;
	m_sceneView->GetSelectedLayerGroups( groups );		
	
	Uint32 processedItemsAutohide = 0;	
	TDynArray<String> unableToCheckoutLayers;	

	Uint32 currentGroupNum = 0;
	Uint32 currentLayerNum = 0;

	GFeedback->BeginTask( TXT("Collecting layers"), false );
	GFeedback->UpdateTaskProgress( 0, groups.Size() );	

	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		RemoveForceNoAutohide( (*it), unableToCheckoutLayers, processedItemsAutohide, currentGroupNum, currentLayerNum );
	}

	GFeedback->EndTask();

	if( !unableToCheckoutLayers.Empty() )
	{
		GFeedback->ShowMsg( TXT("Unable to check out layers"), TXT("Unable to check out %d layer(s)"), unableToCheckoutLayers.Size() );
	}

	GFeedback->ShowMsg( TXT("Finished"), TXT("Checked, groups: %d, layers: %d, FORCE_NO_AUTOHIDE: %d"), currentGroupNum, currentLayerNum, processedItemsAutohide );
}

void CEdSceneExplorer::OnInspectObject( wxCommandEvent& event )
{
	TDynArray< ISerializable* > objects;
	m_sceneView->GetSelection( objects );

	if ( objects.Size() > 6 )
	{
		if ( wxMessageBox( wxT("Are you sure? This will open many object inspector windows"), wxT("Big Selection"), wxYES_NO|wxICON_QUESTION ) != wxYES )
		{
			return;
		}
	}

	for ( Uint32 i=0; i < objects.Size(); ++i )
	{
		InspectObject( objects[i], String::Printf( TXT("Scene Object %i of %i"), i + 1, objects.Size() ) );
	}
}

void CEdSceneExplorer::OnMouseCaptureLost( wxMouseCaptureLostEvent& WXUNUSED(event) )
{
    // don't call event.Skip() here...
}

static void ScanLayerGroupForEmptyLayerRemoval( CLayerGroup* group, TDynArray< CLayerInfo* >& emptyLayers, THashMap< CLayerInfo*, CLayerGroup* >& layerGroupMap )
{
	const CLayerGroup::TLayerList& layers = group->GetLayers();
	int emptyCount = 0;

	// Scan layers
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layer = (*it);
		if ( layer->IsLoaded() && layer->GetLayer()->GetEntities().Size() == 0 && layer->MarkModified() )
		{
			emptyLayers.PushBack( layer );
			layerGroupMap.Insert( layer, group );
			emptyCount++;
		}
	}

	// Scan subgroups
	const CLayerGroup::TGroupList& subgroups = group->GetSubGroups();
	for ( auto it=subgroups.Begin(); it != subgroups.End(); ++it )
	{
		ScanLayerGroupForEmptyLayerRemoval( *it, emptyLayers, layerGroupMap );
	}
}

static void ScanLayerGroupForEmptyGroupRemoval( CLayerGroup* group, TDynArray< CLayerGroup* >& emptyGroups )
{
	// Get all layers down to the bottom
	TDynArray< CLayerInfo* > layers;
	group->GetLayers( layers, false, true );

	// Empty group? Add it to emptyGroups
	if ( layers.Empty() )
	{
		emptyGroups.PushBack( group );
	}
	else // otherwise, scan subgroups
	{
		const CLayerGroup::TGroupList& subgroups = group->GetSubGroups();
		for ( auto it=subgroups.Begin(); it != subgroups.End(); ++it )
		{
			ScanLayerGroupForEmptyGroupRemoval( *it, emptyGroups );
		}
	}
}

void CEdSceneExplorer::OnRemoveEmptyLayers( wxCommandEvent& event )
{
	TDynArray< CLayerGroup* > groups;
	m_sceneView->GetSelectedLayerGroups( groups );

	// Collect all empty layers and groups
	TDynArray< CLayerInfo* > emptyLayers;
	THashMap< CLayerInfo*, CLayerGroup* > layerGroupMap;
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		ScanLayerGroupForEmptyLayerRemoval( *it, emptyLayers, layerGroupMap );
	}

	// Remove all empty layers
	for ( auto it=emptyLayers.Begin(); it != emptyLayers.End(); ++it )
	{
		(*it)->Remove();
	}

	// Find all empty groups
	TDynArray< CLayerGroup* > emptyGroups;
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		ScanLayerGroupForEmptyGroupRemoval( *it, emptyGroups );
	}

	// Remove all empty groups
	for ( auto it=emptyGroups.Begin(); it != emptyGroups.End(); ++it )
	{
		(*it)->Remove();
	}

	// Refresh scene view
	m_sceneView->ClearSelection();
	m_sceneView->RefreshLater();
}

void CEdSceneExplorer::MarkTreeItemsForUpdate()
{
	m_sceneView->RefreshLater();
}

void CEdSceneExplorer::LoadAllLayers( Bool quiet /*=false*/ )
{
	TDynArray<CLayerGroup*> groups;
	groups.PushBack( GGame->GetActiveWorld()->GetWorldLayers() );

	// Collect layers to load from selection
	TDynArray< CLayerInfo* > layersToLoad;
	for ( auto it=groups.Begin(); it != groups.End(); ++it )
	{
		CLayerGroup* group = *it;

		// Get the layers from the group to load
		group->GetLayers( layersToLoad, false, true, true );
	}

	if ( !layersToLoad.Empty() )
	{
		// Load layers
		LayerGroupLoadingContext loadingContext;
		//loadingContext.m_dumpStats = true;
		LoadLayers( layersToLoad, loadingContext, quiet );

		// Flush
		GGame->GetActiveWorld()->UpdateLoadingState();
		GGame->GetActiveWorld()->DelayedActions();
		GGame->GetActiveWorld()->DelayedActions();

		// Flush collision cache
		GCollisionCache->Flush();
	}

	MarkTreeItemsForUpdate();
}

void CEdSceneExplorer::OnGroupItems( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if( world == NULL )
	{
		return;
	}

	// Check if all selected entities belong to the same layer
	TDynArray< CEntity* > allEntities;
	TDynArray< CEntity* > skipEntities;
	CLayer* layer = NULL;
	world->GetSelectionManager()->GetSelectedEntities( allEntities );
	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if ( ( layer && layer != (*entityIter)->GetLayer() ) || world->GetSelectionManager()->CheckIfPartOfAnyGroup( *entityIter ) )
		{
			skipEntities.PushBackUnique( *entityIter );
		}
		layer = (*entityIter)->GetLayer();
	}

	// Confirm with the user that there might be entities that will be skipped
	if ( skipEntities.Size() > 0 )
	{
		GFeedback->ShowWarn( TXT("You cannot group entities that belong to existing groups or different layers. ") );
		return;
	}

	// Get selected entities
	TDynArray< CEntity* > entities;
	allEntities.Clear();
	world->GetSelectionManager()->GetSelectedEntities( allEntities );

	// Skip entities that belongs to other groups
	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( world->GetSelectionManager()->CheckIfPartOfAnyGroup( *entityIter ) == NULL )
		{
			entities.PushBack( *entityIter );
		}
	}

	// If nothing selected do nothing
	if( entities.Size() == 0 )
	{
		return;
	}

	// Ask for name
	String groupName = InputBox( this, TXT( "Group name" ), TXT( "Enter group name:" ), TXT( "NewGroup" ) );

	// Create new group
	EntitySpawnInfo info;
	info.m_entityClass = CEntityGroup::GetStaticClass();
	info.m_name = groupName;
	info.m_spawnPosition = entities[ 0 ]->GetPosition();
	info.m_spawnRotation = entities[ 0 ]->GetRotation();
	info.AddHandler( new TemplateInfo::CSpawnEventHandler() );

	// Add entities to the group
	CEntityGroup* newGroup = Cast< CEntityGroup >( layer->CreateEntitySync( info ) );
	if( newGroup == NULL )
	{
		wxMessageBox( TXT( "This name is already used" ), TXT( "Error" ) );
		return;
	}

	newGroup->AddEntities( entities );

	// create undo step
	TDynArray< CEntityGroup* > groups;
	groups.PushBack( newGroup );
	CUndoGroupObjects::CreateGroupStep( *wxTheFrame->GetUndoManager(), groups );

	// Add new group to the selection (so it can be copied/cloned with the selection)
	world->GetSelectionManager()->Select( newGroup, true );

	// Refresh
	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
}

void CEdSceneExplorer::OnUngroupItems( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if( world == nullptr )
	{
		return;
	}

	// Get selected entities
	TDynArray< CEntity* > allEntities;
	TDynArray< CEntityGroup* > groupEntities;
	m_sceneView->GetSelection( allEntities );

	// Skip non-group entities and groups that belongs to other groups
	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( !( *entityIter)->IsA< CEntityGroup >() )
		{
			continue;
		}

		if( world->GetSelectionManager()->CheckIfPartOfAnyGroup( *entityIter ) == nullptr )
		{
			groupEntities.PushBack( Cast< CEntityGroup >( *entityIter ) );
		}
	}

	// If nothing selected do nothing
	if( groupEntities.Size() == 0 )
	{
		return;
	}

	// create undo step
	CUndoGroupObjects::CreateUngroupStep( *wxTheFrame->GetUndoManager(), groupEntities );

	// Delete selected groups
	for( TDynArray< CEntityGroup* >::iterator entityIter = groupEntities.Begin();
		entityIter != groupEntities.End(); ++entityIter )
	{
		CEntityGroup* group = ( *entityIter );
		group->Destroy();
	}

	// deselect all
	m_sceneView->ClearSelection();
	world->GetSelectionManager()->DeselectAll();

	// Refresh
	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
}

void CEdSceneExplorer::OnLockGroup( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if( world == nullptr )
	{
		return;
	}

	CEntityGroup* entityGroup = nullptr;

	// Get selected entities
	TDynArray< CEntity* > allEntities;
	m_sceneView->GetSelection( allEntities );

	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( ( *entityIter )->IsA< CEntityGroup >() )
		{
			entityGroup = Cast< CEntityGroup >( *entityIter );
			break;
		}
		if( ( *entityIter )->GetPartOfAGroup() == true )
		{
			entityGroup = ( *entityIter )->GetContainingGroup();
			break;
		}
	}

	if( entityGroup != nullptr )
	{
		entityGroup->Lock();
		
		// create undo step
		CUndoLockGroupObjects::CreateStep( *wxTheFrame->GetUndoManager(), entityGroup, false );

		m_sceneView->Select( entityGroup );
	}	
}

void CEdSceneExplorer::OnUnlockGroup( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if( world == nullptr )
	{
		return;
	}

	CEntityGroup* entityGroup = nullptr;

	// Get selected entities
	TDynArray< CEntity* > allEntities;
	m_sceneView->GetSelection( allEntities );

	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( ( *entityIter )->IsA< CEntityGroup >() )
		{
			entityGroup = Cast< CEntityGroup >( *entityIter );
			break;
		}
		if( ( *entityIter )->GetPartOfAGroup() == true )
		{
			entityGroup = ( *entityIter )->GetContainingGroup();
			break;
		}
	}

	if( entityGroup != nullptr )
	{
		entityGroup->Unlock();

		// create undo step
		CUndoLockGroupObjects::CreateStep( *wxTheFrame->GetUndoManager(), entityGroup, true );

		m_sceneView->ClearSelection();
	}
}

void CEdSceneExplorer::OnRemoveFromGroup( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if( world == nullptr )
	{
		return;
	}

	CEntityGroup* entityGroup = nullptr;

	// Get selected entities
	TDynArray< CEntity* > allEntities;
	m_sceneView->GetSelection( allEntities );

	// create undo step
	CUndoRemoveObjectFromGroup::CreateStep( *wxTheFrame->GetUndoManager(), allEntities, true );

	for( TDynArray< CEntity* >::iterator entityIter = allEntities.Begin();
		entityIter != allEntities.End(); ++entityIter )
	{
		if( ( *entityIter )->GetPartOfAGroup() == true )
		{
			entityGroup = ( *entityIter )->GetContainingGroup();
			entityGroup->DeleteEntity( ( *entityIter ) );

			// group cannot be empty - if is empty should be removed
			if( entityGroup->IsEmpty() == true )
			{
				entityGroup->Destroy();
			}

			entityGroup = nullptr;
		}
	}

	// deselect all
	m_sceneView->ClearSelection();
	world->GetSelectionManager()->DeselectAll();

	// Refresh
	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
}

Bool LoadLayers( const TDynArray< CLayerInfo* >& layersToLoad, LayerGroupLoadingContext& loadingContext, Bool quiet /*=false*/ )
{
	// Check, if there are any missing entity templates on layersToLoad
	TSortedMap< String, TDynArray< String > > layersMissingTemplates;
	GFeedback->BeginTask( TXT("Validating layers..."), false );
	Uint32 i = 0;
	for ( auto it=layersToLoad.Begin(); it != layersToLoad.End(); ++it )
	{
		GFeedback->UpdateTaskProgress( i++, layersToLoad.Size() );

		CLayerInfo* layer = *it;
		CDiskFile* file = GDepot->FindFile( layer->GetDepotPath() );
		if ( file )
		{
			if ( IFile* reader = file->CreateReader() )
			{
				CDependencyLoader loader( *reader, NULL );
				TDynArray< String > missingTemplates;
				if ( loader.SearchForExportsMissingTemplates( missingTemplates ) )
				{
					layersMissingTemplates.Insert( file->GetDepotPath(), missingTemplates );
				}
				delete reader;
			}
		}
	}
	GFeedback->EndTask();

	if ( !( quiet || layersMissingTemplates.Empty() ) )
	{
		CEdMissingTemplatesErrorsDisplayer displayer( wxTheFrame );
		if ( !displayer.Execute( layersMissingTemplates ) )
		{
			return false;
		}
	}

	extern Bool LoadLayerList( const TDynArray< CLayerInfo* >& layersToLoad, LayerGroupLoadingContext& loadingContext );
	LoadLayerList( layersToLoad, loadingContext );
	
	GGame->GetActiveWorld()->UpdateWaterProxy();
	return true;
}
