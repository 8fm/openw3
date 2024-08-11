/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "shortcutsEditor.h"
#include "shapesPreviewItem.h"

RED_DECLARE_NAME( SetWorldEditorViewportCameraView );
RED_DECLARE_NAME( MoveWorldEditorViewportToOverviewPoint );

// Spawnable template info
class TemplateInfo
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
public:
	class CSpawnEventHandler : public ISpawnEventHandler
	{
	protected:
		void OnPostAttach( CEntity* entity ) override;
	};

	String		m_name;
	String		m_requiredExtension;
	String		m_templateName;
	String		m_group;
	String		m_templateInsertionClass;
	Bool		m_detachFromTemplate;

public:
	RED_INLINE TemplateInfo( const String& name, const String& requiredExtension, const String& templateName, const String& group, const String& templateInsertionClass, Bool detachFromTemplate );

	// Does this template given resource as a base
	RED_INLINE virtual Bool SupportsResource( const String& resName );

	// Create entity on layer
	virtual CEntity* Create( CLayer* layer, const Vector& spawnPosition, CResource* baseResource );

	void GetName( String &outName )const;
};

class ResourceWrapper : public wxObject
{
public:
	CResource*		m_resource;

public:
	ResourceWrapper( CResource* res )
		: m_resource( res )
	{};
};

//////////////////////////////////////////////////////////////////////////

#define MAX_WORLD_CAMERA_BOOKMARKS 10

struct CEdWorldCameraBookmark
{
	Vector position;
	EulerAngles rotation;
	Bool used;
};

class CEdWorldCameraBookmarks
{
	CEdWorldCameraBookmark m_bookmark[MAX_WORLD_CAMERA_BOOKMARKS];

public:
	CEdWorldCameraBookmarks();
	
	void SetBookmark( int num, const Vector& position, const EulerAngles& rotation );
	Bool GetBookmark( int num, Vector& position, EulerAngles& rotation ) const;
	Bool IsBookmarkSet( int num ) const;

	void SaveSession( CConfigurationManager &config );
	void RestoreSession( CConfigurationManager &config );
};

extern CEdWorldCameraBookmarks GWorldCameraBookmarks;

//////////////////////////////////////////////////////////////////////////

enum EWorldPickPointAction
{
	WPPA_SET,
	WPPA_CLEAR
};

class CEdWorldPickPointClient
{
public:
	virtual ~CEdWorldPickPointClient(){}

	//! Called when any world pick point action occurs
	virtual void OnWorldPickPointAction( EWorldPickPointAction action )=0;

	//! Called when an "extra" pick point command is selected
	virtual void OnWorldPickPointCommand( Int32 index ){}
};

//////////////////////////////////////////////////////////////////////////

class CMaraudersMap;
class CLocalizedStringsEditor;
class CEdScreenshotEditor;
class CEdEncounterEditor;

/// Rendering panel for editing the world
class CEdWorldEditPanel : public CEdRenderingPanel, public IEdEventListener, public Red::Network::ChannelListener
{
	friend class CEdFrame;
	friend class CEdSceneExplorer;
	friend class SceneView;

	DECLARE_EVENT_TABLE();

protected:
	CEdSceneExplorer*					m_scene;
	CMaraudersMap*						m_maraudersMap;
	CLocalizedStringsEditor*			m_locStringsEd;
	CEdScreenshotEditor*				m_screenshotEditor;

	TEdShortcutArray                    m_shortcuts;					//!< Defined shortcuts
	Vector								m_clickedWorldPos;				//!< Last world space point that has been clicked
	Vector								m_clickedWorldNormal;			//!< Last world space normal that has been clicked
	TDynArray< String >					m_stickersTexts;				//!< Sticker label
	TDynArray< TemplateInfo* >			m_templates;					//!< Templates that can be instanced
	THashMap< CEntity*, wxWindow* >			m_entityEditors;				//!< Entity editors
	THashMap< CObject*, CEdEncounterEditor* > m_spawnTreeEditors;
	Bool								m_forceOrthoCamera;				//!< Is orthogonal camera enabled
	Float								m_orthoZoom;					//!< Zoom for orthogonal projection
	Bool								m_enableSelectionUndo;
	THashSet< THandle< CEntity > >		m_hiddenEntities;				//!< Entities the user has explicitly hidden

	bool								m_cameraRotating;				//!< If true the camera is in rotation mode around a point
	POINT								m_cameraRotationCenter2D;		//!< The camera rotation point in widget space
	Vector								m_cameraRotationCenter3D;		//!< The camera rotation point in 3D space
	Vector								m_cameraRotationInitialPosition;//!< The initial camera position
	EulerAngles							m_cameraRotationInitialAngles;	//!< The initial camera rotation
	POINT								m_cameraRotationInitialCursorPosition; //<! The initial cursor position before rotating the camera
	Float								m_cameraRotationDistance;		//!< The distance between the camera origin and the rotation point

	wxMilliClock_t						m_lastClickTime;				//!< Time that the left mouse button was pressed last time
	wxMilliClock_t						m_previousLastClickTime;		//!< Previous m_lastClickTime value
	Int32								m_lastClickX;					//!< Last click coordinates
	Int32								m_lastClickY;					//!< Last click coordinates

	bool								m_polygonDrawing;				//!< If true, the editor is in polygon drawing mode
	TDynArray<Vector>					m_polygonPoints;				//!< Polygon points for the polygon drawing mode

	Bool								m_hasPickPoint;					//!< We have a pick point
	Vector								m_pickPoint;					//!< World pick point position
	Vector								m_pickPointNormal;				//!< Normal at pick point position
	CEdWorldPickPointClient*			m_pickPointClient;				//!< World pick point client
	TDynArray<String>					m_pickPointCommands;			//!< Extra commands to be added for the pick point

	ShapesPreviewContainer*				m_shapesContainer;				//!< container used to display and edit shapes from objects in editor

	Bool								m_selFreeSpaceShow;				//!< The free space bounds visualization is active
	TDynArray< DebugVertex >			m_selFreeSpaceMesh;				//!< Mesh used to visualize the selection's "free space" bounds for leaving enough space for player 
	TDynArray< Uint16 >					m_selFreeSpaceMeshIndices;
	Bool								m_selFreeSpaceLock;				//!< The free space bounds visualization is locked

private:
	CNodeTransformManager*				m_transformManager;
	TDynArray< THandle< CLayerInfo > >	m_copiedLayers;
	CDirectory*							m_copiesDirectory;

public:
	RED_INLINE const Vector& GetLastClickedPosition() const { return m_clickedWorldPos; };

public:
	CEdWorldEditPanel( wxWindow* parent, CEdSceneExplorer *scene );
	~CEdWorldEditPanel();

	// Get viewed world
	virtual CWorld* GetWorld() override;
	virtual CSelectionManager* GetSelectionManager() override;
	virtual CNodeTransformManager* GetTransformManager() override;

	virtual CEdSceneExplorer* GetSceneExplorer() override { return m_scene; }
	TEdShortcutArray *GetAccelerators();

	void DeleteEntities( const TDynArray< CEntity* >& entities, Bool requireUserConfirmation = true );
	void ReplaceEntitiesWithMesh( const TDynArray< CEntity* >& entities, Bool createStaticMesh );
	void ReplaceEntitiesWithEntity( const TDynArray< CEntity* >& entities, Bool copyComponents );
	void ReplaceEntitiesWithEntity( const String& resourcePath, const TDynArray< CEntity* >& entities, Bool copyComponents, TDynArray< CEntity* >& createdEntities );

	void SetMaraudersMap( CMaraudersMap *maraudersMap ) { m_maraudersMap = maraudersMap; }
	CMaraudersMap *GetMaraudersMap() { return m_maraudersMap; }

	void SetLocStringsEditor( CLocalizedStringsEditor *locStringsEd ) { m_locStringsEd = locStringsEd; }
	CLocalizedStringsEditor *GetLocStrindEditor() { return m_locStringsEd; }

	void SetScreenshotEditor( CEdScreenshotEditor *screenshotEditor ) { m_screenshotEditor = screenshotEditor; }
	CEdScreenshotEditor *GetScreenshotEditor() { return m_screenshotEditor; }

	void OpenSpawnTreeEditor( CObject* ownerObject );
	Bool IsSpawnTreeBeingEdited( CObject* ownerObject );

	Bool GetSelectionTracking() const { return m_enableSelectionUndo; }
	void SetSelectionTracking( Bool enable ) { m_enableSelectionUndo = enable; }

	//! Calculates the free space visualization mesh of the selection in the background
	void RefreshSelectionFreeSpaceMesh();

	//! Set the visibility (and recalculation) of free space visualization
	void ShowFreeSpaceVisualization( Bool show );

	//! Lock the free space visualization so that it is not recalculated when the selection changes
	void LockFreeSpaceVisualization( Bool lock );

	//! Returns true if we have a pick point in the world
	RED_INLINE Bool HasPickPoint() const { return m_hasPickPoint; }

	//! Returns the position of the pick point
	RED_INLINE Vector GetPickPoint() const { return m_pickPoint; }

	//! Returns the normal at the pick point (may not be accurate)
	RED_INLINE Vector GetPickPointNormal() const { return m_pickPointNormal; }

	//! Set the pick point client - this is needed to move the pick point around, display the relevant commands, etc. Note that
	//! this will remove the previous pick point (if any)
	void SetPickPointClient( CEdWorldPickPointClient* client, const String& addCommand, const String& removeCommand, const TDynArray<String>* extraCommands );

	//! Clear the pick point client - only given one if specified, otherwise any client that is currently set
	void ClearPickPointClient( CEdWorldPickPointClient* client = nullptr );

	//! Returns true if we have a pick point client
	RED_INLINE Bool HasPickPointClient() const { return m_pickPointClient != nullptr; }

	void CopySelectedEntities();
	void CopyLayers( TDynArray< CLayerInfo* > layers );
	Bool PasteLayers( CLayerGroup* parentGroup );

public: 
	void OnCopy( wxCommandEvent& event );
	void OnCut( wxCommandEvent& event );
	void OnPaste( wxCommandEvent& event );
	void OnDelete( wxCommandEvent& event );
	void OnCopyCameraView( wxCommandEvent& event );
	void OnPasteCameraView( wxCommandEvent& event );
	void OnPasteHere( wxCommandEvent& event );
	void OnPasteAlignedHere( wxCommandEvent& event );
	void OnSpawnHere( wxCommandEvent& event );
	void OnSpawnClass( wxCommandEvent& event );
	void OnChangeLayer( wxCommandEvent& event );
	void OnEditEntity( wxCommandEvent& event );
	void OnActivateEntityLayer( wxCommandEvent& event );
	void OnLookAtSelected( wxCommandEvent& event );
	void OnRemoveCollisions( wxCommandEvent& event );
	void OnRebuildCollisions( wxCommandEvent& event );
	void OnConvertToRigidMeshes( wxCommandEvent& event );
	void OnNavigationConvert( wxCommandEvent& event, EPathLibCollision collisionGroup );
	void OnShadowCasting(Bool isLocal);
	void OnNavigationToStaticWalkable( wxCommandEvent& event );
	void OnNavigationToWalkable( wxCommandEvent& event );
	void OnNavigationToStatic( wxCommandEvent& event );
	void OnNavigationToDynamic( wxCommandEvent& event );
	void OnPreviewAPMan( wxCommandEvent& event );
	void OnPreviewAPBigMan( wxCommandEvent& event );
	void OnPreviewAPWoman( wxCommandEvent& event );
	void OnPreviewAPDwarf( wxCommandEvent& event );
	void OnPreviewAPChild( wxCommandEvent& event );
	void OnPreviewAPSelectedEntity( wxCommandEvent& event );
	void PreviewActionPoint( const String& entityTemplatePath, wxCommandEvent& event );
	void OnGenerateCubemap( wxCommandEvent& event );
	void OnNavmeshAddGenerationRoot( wxCommandEvent& event );
	void OnNavmeshMoveGenerationRoot( wxCommandEvent& event );
	void OnNavmeshDelGenerationRoot( wxCommandEvent& event );
	void OnNavmeshGenerate( wxCommandEvent& event );
	void OnPickPointMenu( wxCommandEvent& event );

	void OnReplaceWithEntity( wxCommandEvent& event );
	void OnReplaceWithEntityCopyComponents( wxCommandEvent& event );
	void OnReplaceWithMesh( wxCommandEvent& event );
	void OnReplaceWithMeshStatic( wxCommandEvent& event );
	void OnGroupItems( wxCommandEvent& event );
	void OnUngroupItems( wxCommandEvent& event );
	void OnLockGroup( wxCommandEvent& event );
	void OnUnlockGroup( wxCommandEvent& event );
	void OnRemoveFromGroup( wxCommandEvent& event );
	void OnShowResource( wxCommandEvent& event );
	void OnDetachTemplates( wxCommandEvent& event );
	void OnAddStickerFromContextMenu( wxCommandEvent& event );
	void OnAddStickerInCurrentMousePos( wxCommandEvent& event );
	void OnShowInSceneExplorer( wxCommandEvent& event );
	void Debug_AddShadowsToAllMeshes( wxCommandEvent& event );
	void OnAlignPivot( wxCommandEvent& event );
	void OnCreateEntityTemplate( wxCommandEvent& event );
	void OnCreateMeshResource( wxCommandEvent& event );
	void OnCreateShadowMesh( wxCommandEvent& event );
	void OnExtractMeshesFromEntities( wxCommandEvent& event );
	void OnExtractComponentsFromEntities( wxCommandEvent& event );
	void OnHideSelectedEntities( wxCommandEvent& event );
	void OnIsolateSelectedEntities( wxCommandEvent& event );
	void OnListHiddenEntities( wxCommandEvent& event );
	void OnRevealHiddenEntities( wxCommandEvent& event );
	void OnEncounterEditor( wxCommandEvent& event );
	void OnChangeAppearance( wxCommandEvent& event );
	void OnImportEntitiesFromOldTiles( wxCommandEvent& event );
	void OnForceStreamInInsideArea( wxCommandEvent& event );

protected:
	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual void HandleContextMenu( Int32 x, Int32 y );
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
	virtual void OnViewportTick( IViewport* view, Float timeDelta );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	virtual void OnViewportSetDimensions ( IViewport* view );

protected:
	virtual void OnCameraMoved();

	virtual void OnPacketReceived( const AnsiChar* channelName, Red::Network::IncomingPacket& packet );

protected:
	void UpdateViewportWidgets();
	void UpdateSelectionSensitiveTools();
	void PasteEntities( const Vector* relativePosition, Bool align );
	void SpawnEntityEditor( CEntity* entity );
	void CloseEntityEditors( CEntity* entity );
	void OnEntityEditorClosed( wxCloseEvent& event );
	void OnEncounterEditorClosed( wxWindowDestroyEvent& event );
	CLayer* CheckActiveLayer();

	bool CalcClickedPosition( Int32 x, Int32 y );
	Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources );

	void InitStickers();

	Bool CanSpawnEntity( CEntityTemplate* entityTemplate );

	void CreateAreaFromPolygon();
	Bool SpawnHereWithTemplateInsertionClasss( const TemplateInfo *const templateInfo, CLayer* layer, CResource* baseResource );
	void ClearCopiedLayers();

public:
	void HideEntities( const TDynArray< CEntity* >& entities );
	void IsolateEntities( const TDynArray< CEntity* >& entities );
	void RevealEntities( const TDynArray< CEntity* >& entities );
	void RevealEntities();

	Bool AlignEntitiesToPositionFromViewportXY( Int32 x, Int32 y, const TDynArray< CEntity* >& entities, Bool applyRotation, Bool addUndoSteps );
	Bool GetVisibleTerrainBounds( Float& minX, Float& minY, Float& maxX, Float& maxY );
};
