/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/newNpc.h"
#include "../../common/game/communitySystem.h"
#include "../../common/game/encounter.h"
#include "maraudersMapAIDebugCanvas.h"
#include "maraudersMapCanvas.h"

class CDoor;
class IMaraudersMapLogger;

enum EMaraudersMapItemType
{
	MMIT_Unknown,
	MMIT_ActionPoint,
	MMIT_NPC,
	MMIT_Player,
	MMIT_Mesh,
	MMIT_AgentStub,
	MMIT_WayPoint,
	MMIT_Sticker,
	MMIT_DeniedArea,
	MMIT_Encounter,
	MMIT_Door,
	MMIT_CommunityArea,
};

class CMaraudersMapItemHtmlBuilder
{
public:
	// links
	String MakeLinkVector( const Vector& vec ) const;
	String MakeLinkResourceGotoAndOpen( const CResource* res ) const;
	String MakeLinkResourceGoto( const CResource* res ) const;
	String MakeLinkMaraudersItemGotoActionPoint( const String& desc, TActionPointID apID ) const;
	String MakeLinkDeactivateAllCommunititesButThis( const CCommunity *community ) const;
	String MakeLinkOnlyOneStubMode( Int32 stubIdx ) const;
};

class CMaraudersMapItemBase
{
public:
	CMaraudersMapItemBase() : m_id( -1 ), m_typeId( -1 ), m_isVisible( true ), m_log( NULL ) {}
	virtual ~CMaraudersMapItemBase() {}

	virtual String GetShortDescription() const = 0;
	virtual String GetFullDescription() const = 0;
	virtual void GetLookAtInfo( String& info ) const {}
	virtual const TDynArray< SAIEvent >& GetAIHistory() const { return s_noHistory; }
	virtual const TDynArray< String >& GetAITrackNames() const { return s_noTracks; }
	virtual Vector GetWorldPosition() const = 0;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas ) = 0;
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const = 0;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const = 0;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const = 0;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const = 0;
	virtual Bool IsValid() const = 0;
	// If this method returns true, than the game will be paused and item will be selected
	virtual Bool Update( CMaraudersMapCanvasDrawing *canvas ) { return false; }
	
	// Layers filtering
	virtual Int32 GetLayerID() const { return m_id; }
	virtual void SetLayerID( int id ) { m_id = id; }
	
	// Type filtering
	virtual Int32 GetTypeID() const { return m_typeId; }
	virtual void SetTypeID( int id ) { m_typeId = id; }

	// Visibility
	virtual Bool IsVisible() const { return m_isVisible; }
	virtual void Hide() { m_isVisible = false; }
	virtual void Show() { m_isVisible = true; }

	// Dragging
	virtual Bool CanBeDragged() { return false; }
	virtual void SetDraggedPos( const Vector& worldPos ) {}

	// Tags
	virtual const TagList* GetTags() { return NULL; }

	// Context menu options
	virtual Int32 GetOptionsSize() { return 0; }
	virtual const TDynArray< String >* GetOptionsNames() { return NULL; }
	virtual Bool ExecuteOption( Int32 optionNum, TDynArray< String > &logOutput /* out */ ) { return false; }

	// Type info
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_Unknown; }

	// Logger
	void SetLogger( IMaraudersMapLogger *logger ) { m_log = logger; }

	// Fast update - if this method returns true, than item's Update() method will be called
	// very often (every frame if it is possible).
	virtual Bool DoesRequireFastUpdate() const { return false; }

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) const {}

	CMaraudersMapItemHtmlBuilder m_html;
	IMaraudersMapLogger *m_log;

protected:
	Int32 m_id;
	Int32 m_typeId;
	Bool m_isVisible;

private:
	static TDynArray< String >			s_noTracks;
	static TDynArray< SAIEvent >		s_noHistory;
};

class CMaraudersMapItemNPC : public CMaraudersMapItemBase, public IAIDebugListener
{
public:
	CMaraudersMapItemNPC( CNewNPC *npc );
	~CMaraudersMapItemNPC();

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual void GetLookAtInfo( String& info ) const;
	virtual const TDynArray< SAIEvent >& GetAIHistory() const { return m_aiHistory; }
	virtual const TDynArray< String >& GetAITrackNames() const { return s_npcAITracks; }
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	virtual Bool CanBeDragged() { return true; }
	virtual void SetDraggedPos( const Vector& worldPos );
	virtual const TagList *GetTags() { return &m_npc.Get()->GetTags(); }
	virtual Int32 GetOptionsSize() { return s_optionsNames.Size(); }
	virtual const TDynArray< String >* GetOptionsNames() { return &s_optionsNames; }
	virtual Bool ExecuteOption( Int32 optionNum, TDynArray< String > &logOutput /* out */ );
	static void DescriptionInventory( String& result, CInventoryComponent* inventory );
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_NPC; }
	virtual Bool Update( CMaraudersMapCanvasDrawing *canvas );
	virtual Bool DoesRequireFastUpdate() const { return m_isFastUpdateRequired; }
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) const override;

	// ------------------------------------------------------------------------
	// IAIDebugListener implementation
	// ------------------------------------------------------------------------
	virtual void OnAIEvent( EAIEventType type, EAIEventResult result, const String& name, const String& description, Float time );
	virtual void OnAIEventStart( EAIEventType type, const String& name, const String& description, Float startTime );
	virtual void OnAIEventEnd( Float endTime, EAIEventType type, EAIEventResult result );

	static const String OPTION_BEHEAVIORDEBUG;
	static const String OPTION_BEHTREEDEBUG;
	static const String OPTION_STEERINGDEBUG;
	static const String OPTION_REACTIONSDEBUG;
	static const String OPTION_DESTROY;	
	static const String OPTION_KILL;
	static const String OPTION_STUN;
	static const String OPTION_LIST_APS;
	static const String OPTION_PAUSE_GAME_ON_AP_CHANGE;
	static const String OPTION_PAUSE_GAME_ON_NPC_STATE_CHANGE;
	static const String OPTION_PAUSE_GAME_ON_NPC_SCHEDULE_CHANGE;
	static const String OPTION_PAUSE_GAME_ON_BEHAVIOR_CHANGE;
	static const String OPTION_SKELETON;
	static const String OPTION_SET_ATTITUDE_HOSTILE;

private:
	struct SActionPointDebugData
	{
		Vector m_pos;
		Bool   m_isFree;
		SActionPointDebugData() {}
		SActionPointDebugData( const Vector &pos, Bool isFree ) : m_pos( pos ), m_isFree( isFree ) {}
	};

	void DrawNavigation( CMaraudersMapCanvasDrawing *canvas ) const;
	void DrawAvailableActionPoints( CMaraudersMapCanvasDrawing *canvas ) const;
	Int32 CollectAvailableActionPoints();
	void GetTimetableDescription( const CNewNPC * npc, String &result ) const;

	static CMaraudersNavigationCanvas	s_navigationCanvas;
	static TDynArray< String >			s_optionsNames;
	THandle< CNewNPC >					m_npc;	
	TDynArray< SActionPointDebugData >	m_availableAPs;
	
	// Game pause
	Bool								m_isFastUpdateRequired;
	// AP pause
	Bool								m_isPauseApEnabled;
	TActionPointID						m_pauseLastApID;
	TActionPointID						m_pauseCurrentApID;
	// NPC state pause
	CName								m_pauseLastNpcState;
	CName								m_pauseCurrentNpcState;
	Bool								m_isPauseNpcStateEnabled;
	// NPC schedule pause
	String								m_pauseLastNpcSchedule;
	String								m_pauseCurrentNpcSchedule;
	Bool								m_isPauseNpcScheduleEnabled;
	// NPC behavior pause
	String								m_pauseLastBehavior;
	String								m_pauseCurrentBehavior;
	Bool								m_isPauseBehaviorEnabled;
	// AI history
	TDynArray< SAIEvent >				m_aiHistory;
	Bool								m_debugAI;

	static TDynArray< String >			s_npcAITracks;
};

class CMaraudersMapItemPlayer : public CMaraudersMapItemBase
{
public:
	CMaraudersMapItemPlayer();

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;	
	virtual Int32 GetOptionsSize() { return m_optionsNames.Size(); }
	virtual const TDynArray< String >* GetOptionsNames() { return &m_optionsNames; }
	virtual Bool ExecuteOption( Int32 optionNum, TDynArray< String > &logOutput /* out */ );
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_Player; }
	void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) const override;

private:
	CPlayer* GetPlayer() const;
	static void DrawPlayer( CMaraudersMapCanvasDrawing *canvas, Bool selected );

	TDynArray< String >		m_optionsNames;
	Bool					m_debugAI;
};

class CMaraudersMapItemMesh : public CMaraudersMapItemBase
{
public:
	CMaraudersMapItemMesh( const CEntity *entity );
	~CMaraudersMapItemMesh();

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	virtual Bool Update( CMaraudersMapCanvasDrawing *canvas  );
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_Mesh; }
private:
	void GenerateFrame();
	void BoxToPointsClient( CMaraudersMapCanvasDrawing *canvas, const Box &box, wxPoint &pointBeg /* out */, wxPoint &pointEnd /* out */ ) const;
	Bool IsCross( const Vector &a, const Vector &b, Vector &crossPoint /* out */, Int32 &currRectI /* in out */, Int32 &currEdgeI /* out */ );
	Bool DoSectionsIntersect( const Vector &aBeg, const Vector &aEnd, const Vector &bBeg, const Vector &bEnd );

	THandle< CEntity > m_entity;
	TDynArray< Box > m_bboxes;
	String m_shortDescription;
	wxRect m_border;
	
	TDynArray< Vector > m_framePoints;
	struct SPoints { 
		Vector p[4]; 
		Vector& operator[](int i) { return p[i]; }
		Uint32 Size() const { return 4; }
	};
	TDynArray< SPoints > m_bboxesPoints;
};

class CMaraudersMapItemActionPoint : public CMaraudersMapItemBase
{
public:
	CMaraudersMapItemActionPoint( const CEntity *entity );

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	virtual const TagList *GetTags();
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_ActionPoint; }

	Bool DoesMatchApID( TActionPointID apID ) const;
private:
	String GetStringFromArray( const TDynArray< CName > &names ) const;
	THandle< CEntity > m_entity;
	TDynArray< CActionPointComponent* > m_apComponents;
	TagList m_tagList;
};

class CMaraudersMapItemAgentStub : public CMaraudersMapItemBase
{
public:
	typedef SAgentStub SAgentStub;

	CMaraudersMapItemAgentStub( SAgentStub *agentStub ) : m_agentStub( agentStub ) {}

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	virtual Bool IsVisible() const;
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_AgentStub; }
private:
	String GetStringFromArray( const TDynArray< CName > &names ) const;
	SAgentStub *m_agentStub;
};

class CMaraudersMapItemWayPoint : public CMaraudersMapItemBase
{
public:
	CMaraudersMapItemWayPoint( CEntity *entity ) : m_entity( entity ) {}

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	const TagList *GetTags() { return &m_entity.Get()->GetTags(); };
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_WayPoint; }
private:
	THandle< CEntity > m_entity;
};

class CMaraudersMapItemSticker : public CMaraudersMapItemBase
{
public:
	CMaraudersMapItemSticker( CEntity *entity );

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_Sticker; }
private:
	THandle< CEntity > m_entity;
	CStickerComponent *m_sticker;
};

class CMaraudersMapItemDeniedArea : public CMaraudersMapItemBase
{
public:
	CMaraudersMapItemDeniedArea( const CDeniedAreaComponent* deniedArea );

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_DeniedArea; }

private:	
	THandle< CDeniedAreaComponent > m_deniedArea;
	wxColor m_color;
};

class CMaraudersMapItemCommunityArea : public CMaraudersMapItemBase
{
public:
	CMaraudersMapItemCommunityArea( const CCommunityArea* communityArea );

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_CommunityArea; }

private:
	void BoxToPointsClient( CMaraudersMapCanvasDrawing *canvas, const Box &box, wxPoint &pointBeg /* out */, wxPoint &pointEnd /* out */ ) const;

	THandle< CCommunityArea > m_communityArea;
	THandle< CAreaComponent > m_communityAreaComponent;
	wxColor m_color;
	wxColor m_activeColor;
	wxColor m_selectedColor;
};

class CMaraudersMapItemEncounter : public CMaraudersMapItemBase
{
public:
	CMaraudersMapItemEncounter( CEncounter *encounter );

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_Encounter; }
private:
	void BoxToPointsClient( CMaraudersMapCanvasDrawing *canvas, const Box &box, wxPoint &pointBeg /* out */, wxPoint &pointEnd /* out */ ) const;
	THandle< CEncounter > m_encounter;
	Box m_boundingBox;
};

class CMaraudersMapItemDoor : public CMaraudersMapItemBase
{
public:
	CMaraudersMapItemDoor( CDoorComponent *door );
	~CMaraudersMapItemDoor();

	virtual String GetShortDescription() const;
	virtual String GetFullDescription() const;
	virtual Vector GetWorldPosition() const;
	virtual wxRect GetClientBorder( CMaraudersMapCanvasDrawing *canvas );
	virtual void Draw( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawSelected( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual void DrawTooltip( CMaraudersMapCanvasDrawing *canvas ) const;
	virtual Bool operator==( const CMaraudersMapItemBase& item  ) const;
	virtual Bool IsValid() const;
	virtual Bool Update( CMaraudersMapCanvasDrawing *canvas  );
	virtual EMaraudersMapItemType GetMarItemType() const { return MMIT_Door; }

	// Context menu options
	virtual Int32 GetOptionsSize() { return s_optionsNames.Size(); }
	virtual const TDynArray< String >* GetOptionsNames() { return &s_optionsNames; }
	virtual Bool ExecuteOption( Int32 optionNum, TDynArray< String > &logOutput /* out */ );

private:
	void BoxToPointsClient( CMaraudersMapCanvasDrawing *canvas, const Box &box, wxPoint &pointBegOut, wxPoint &pointEndOut ) const;

	THandle< CDoorComponent > m_door;
	TDynArray< Box > m_bboxes;
	String m_shortDescription;
	wxRect m_border;

	static TDynArray< String > s_optionsNames;

	static const String OPTION_OPEN;
	static const String OPTION_CLOSE;
};

//////////////////////////////////////////////////////////////////////////
