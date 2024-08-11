/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "previewItem.h"
#include "skeletonPreview.h"
#include "pathlibEditor.h"
#include "../../common/engine/soundAdditionalListener.h"

/// Graph editor hook
class EntityPreviewPanelHook
{
public:
	virtual ~EntityPreviewPanelHook() {};

	//! Selection has changed
	virtual void OnPreviewSelectionChanged()=0;
	virtual void OnPreviewWidgetModeChanged()=0;
	virtual void OnPreviewWidgetSpaceChanged()=0;
	virtual void OnPreviewItemTransformChanged()=0;
};

/// Preview panel for entity editor
class CEdEntityPreviewPanel : public CEdInteractivePreviewPanel
							, public IEdEventListener
							, public ISkeletonPreview
							, public IPreviewItemContainer
							, public CSoundAdditionalListener
							, protected CEdNavmeshEditor
{
	friend class CEdEntityEditor;

protected:
	CEdEntitySlotProperties*		m_activeSlotProperties;		//!< Active slot properties
	EntityPreviewPanelHook*			m_hook;						//!< Event hook
	CEntityTemplate*				m_template;					//!< Template
	CEntity*						m_entity;					//!< Entity to preview

	CBitmapTexture*					m_aimIcon;					//!< Sprite aim icon to diplay

	IPreviewItem::PreviewSize		m_itemPreviewSize;

	CName							m_entityAnimationName;
	TDynArray< CName >				m_entityPrevAnimationNames;
	
	Bool							m_isPaused;

	Bool							m_dispSkeleton;				//!< Show skeleton
	Bool							m_dispBoneNames;			//!< Show bone names
	Bool							m_dispBoneAxis;				//!< Show bone axes
	Bool							m_dispSlots;				//!< Show slots
	Bool							m_dispSlotNames;			//!< Show slot names
	Bool							m_dispSlotActive;			//!< Show active slot
	Bool							m_dispWounds;				//!< Show dismemberment wounds
	Bool							m_dispWoundNames;			//!< Show dismemberment wound names

	Bool							m_showWireframe;
	Bool							m_showCollision;
	Bool							m_showBoundingBox;
	Bool							m_showTBN;

	wxString						m_textureArraysDataSize;	//!< Displaying approx texture arrays data size
	wxString						m_textureDataSize;			//!< Displaying approx texture memory size
	wxString						m_meshDataSize;				//!< Displaying approx render data size
	Uint32							m_triangleCountOfSelection; //!< Displaying the selected components triangle count
	wxString						m_bbDiagonalInfo;				//!< Displaying half of the length of the bb diagonal

public:
	//! Get preview entity
	RED_INLINE CEntity* GetEntity() const { return m_entity; }

	//! Get edited template
	RED_INLINE CEntityTemplate* GetTemplate() const { return m_template; }

	//! Navmesh editor activation interface
	RED_INLINE Bool IsNavmeshEditorActive() const { return m_isNavmeshEditorActive; }
	RED_INLINE void SetNavmeshEditorActive( Bool b ) { CEdNavmeshEditor::SetNavmeshEditorActive( b ); }

	//! Should we show skeleton
	virtual Bool IsSkeleton();

	//! Should we show bone names
	virtual Bool IsBoneNames();

	//! Should we show bone axes
	virtual Bool IsBoneAxis();

	CEdEntityPreviewPanel( wxWindow* parent, CEntityTemplate* entityTemplate, EntityPreviewPanelHook* hook );
    ~CEdEntityPreviewPanel();

	//! Change entity template
	void SetTemplate( CEntityTemplate* entityTemplate );

	void ShowTBN( Bool val );
	void ShowWireframe( Bool val );
	void ShowBB( Bool val );
	void ShowCollision( Bool val );

	void SetEntitySlotProperties( CEdEntitySlotProperties* entSlotProperties );

	void SelectAll();
	void UnselectAll();
	void InvertSelection();

	void UpdateBBoxInfo();

protected:
    void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;
    void HandleSelection( const TDynArray< CHitProxyObject* >& objects ) override;
	void HandleContextMenu( Int32 x, Int32 y ) override;
	void OnViewportTick( IViewport* view, Float timeDelta ) override;
	void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) override;
	void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera ) override;
	Bool OnViewportMouseMove( const CMousePacket& packet ) override;
	Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) override;
	Bool OnViewportTrack( const CMousePacket& packet ) override;
	void RefreshPreviewSlotItem( const EntitySlot* selectedSlot );

	Bool EnableGraphEditing();
	void ShowStatsFromSelection();

public: // IPreviewItemContainer
	virtual CWorld* GetPreviewItemWorld() const { return m_previewWorld; }
	virtual Bool IsSelectionBoxDragging() { return m_selectionBoxDrag; }
	virtual void OnItemTransformChangedFromPreview( IPreviewItem* item );

	void RefreshPreviewSlotChanges();

	void RefreshPreviewWoundItem( const CDismembermentWound* selectedWound );
	void RefreshPreviewWoundChanges();
	void GenerateWoundFragments( IViewport *view, CRenderFrame *frame );
	void EnableManualSlotChange( Bool status );
	void ShowActiveSlot( Bool status );

public:
	void SetPreviewItemSize( IPreviewItem::PreviewSize size );
	RED_FORCE_INLINE IPreviewItem::PreviewSize GetPreviewItemSize() const { return m_itemPreviewSize; }

	void SetWidgetSpace( ERPWidgetSpace space );
	RED_FORCE_INLINE ERPWidgetSpace GetWidgetSpace() const { return m_widgetManager->GetWidgetSpace(); }

protected:
	void OnWidgetSelected( wxCommandEvent &event );
	void OnToggleWidgetSpace( wxCommandEvent& event );

	void OnSelectAll( wxCommandEvent& event );
	void OnUnselectAll( wxCommandEvent& event );
	void OnInvertSelection( wxCommandEvent& event );

	void OnContextMenuSelected( wxCommandEvent &event );
	void OnPlayAnimaiton( wxCommandEvent& event );
	void OnPlayMimicsAnimaitonHigh( wxCommandEvent& event );
	void OnPlayMimicsAnimaitonLow( wxCommandEvent& event );
	void OnPlayDefaultAnimaiton( wxCommandEvent& event );
	void OnStopAnimation( wxCommandEvent& event );
	void OnPauseAnimation( wxCommandEvent &event );
	void OnAnimationConfirmed( wxCommandEvent &event );
	void OnAnimationAbandoned( wxCommandEvent &event );
	void OnPause( wxCommandEvent &event );

	void StoreSessionCamera();
	void LoadSessionCamera();
	void StoreSessionAnimations();
	void LoadSessionAnimations();

	Bool PlayAnimation( const CName& animationName );
	Bool PlayMimicsAnimation( const CName& animationName, Bool high );

	//! Navmesh editor interface
	PathLib::CNavmesh* GetEditedNavmesh() override;
	PathLib::CNavmesh* FindNavmeshForEdition( const CMousePacket& packet ) override;
	CWorld* GetEditedWorld() override;
	CNavmeshComponent* GetEditedNavmeshComponent() override;
};
