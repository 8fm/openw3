/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "skeletonPreview.h"
#include "behaviorEditorPanel.h"
#include "animationPreviewGhost.h"
#include "previewPanel.h"
#include "previewItem.h"


class CEdBehaviorPreviewPanel;
class CEdBehaviorEditor;


//////////////////////////////////////////////////////////////////////////

class CBehaviorVarItem : public IPreviewItem
{
public:
	CBehaviorVarItem( CEdBehaviorPreviewPanel* behPreview, Bool forInternalVariable );

	virtual Bool IsValid() const;

	virtual void Refresh();

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}

	virtual IPreviewItemContainer* GetItemContainer() const;

	Bool DoesMatchInternal(Bool forInternalVariable) const { return forInternalVariable == m_internalVariable; }

protected:
	CName GetVarName() const;

protected:
	CEdBehaviorPreviewPanel*	m_preview;
	Bool						m_internalVariable;
};

//////////////////////////////////////////////////////////////////////////

/// Preview panel that renders material preview
class CEdBehaviorPreviewPanel : public CEdPreviewPanel, 
								public IEdEventListener,
								public ISkeletonPreview,
								public CEdBehaviorEditorPanel,
								public IPreviewItemContainer
{
	DECLARE_EVENT_TABLE();
	friend class CEdBehaviorEditor;

	Bool							m_stickToEntity;

	CCamera*						m_previewCamera;

	CInputManager*					m_previewInputManager;
	
	Bool							m_isMovingCameraAlign;
	Vector							m_prevEntityPos;
	Bool							m_showEntityTransform;
	Vector							m_showEntityTransformPrevPos;
	EulerAngles						m_showEntityTransformPrevRot;
	Vector							m_cachedEntityTransformPos;
	EulerAngles						m_cachedEntityTransformRot;

	CPreviewHelperComponent*		m_dynamicTarget;
	
	PreviewGhostContainer			m_ghostContainer;

public:
	CEdBehaviorPreviewPanel( CEdBehaviorEditor* behaviorEditor );
	~CEdBehaviorPreviewPanel();

public: // CEdPreviewPanel
	virtual void HandleContextMenu( Int32 x, Int32 y );
	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& objects );

	virtual void OnViewportTick( IViewport* view, Float timeDelta );
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual Bool OnViewportMouseMove( const CMousePacket& packet );

public: // CEdBehaviorEditorPanel
	static wxString		GetPanelNameStatic()	{ return wxT("Preview"); }

	virtual wxWindow*	GetPanelWindow()		{ return this; }
	virtual wxString	GetPanelName() const	{ return wxT("Preview"); }
	virtual wxString	GetPanelCaption() const { return wxT("Preview"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void		OnReset();
	virtual void		OnLoadEntity();
	virtual void		OnUnloadEntity();

	virtual void		SaveSession( CConfigurationManager &config );
	virtual void		RestoreSession( CConfigurationManager &config );

public: // IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

public: // ISkeletonPreview
	virtual Bool IsSkeleton();
	virtual Bool IsBoneNames();
	virtual Bool IsBoneAxis();

public: // IPreviewItemContainer
	virtual CWorld* GetPreviewItemWorld() const { return m_previewWorld; }
	virtual Bool IsSelectionBoxDragging() { return m_selectionBoxDrag; }

public: // Ghosts
	void ShowGhosts( Uint32 number, PreviewGhostContainer::EGhostType type );
	void HideGhosts();
	Bool HasGhosts() const;

public:
	void EnumInputs( TDynArray< CName >& inputs ) const;

public:
	Bool HasHelper( const String& helperName, bool forInternalVariable ) const;
	void ToggleHelper( const String& helperName, Bool forInternalVariable );
	void CreateHelper( const String& helperName, bool forInternalVariable );
	void RemoveHelper( const String& helperName, bool forInternalVariable );
	void RecreateHelpers();

	void CreateDynamicTarget();
	void DestroyDynamicTarget();
	void MoveDynamicTarget( const Vector& pos );

	void OnDynTarget( Bool flag );

	Bool DynamicTargetInput( enum EInputKey key, enum EInputAction action, Float data, Vector& outMovement ) const;
	Bool DynamicTargetInput( const CMousePacket& packet, Vector& outMovement ) const;

protected:
	void OnLoadEntity( wxCommandEvent& event );
	void OnLoadEntityPlayer( wxCommandEvent& event );
	void OnLoadEntityIncludeTest( wxCommandEvent& event );

	void OnStickToEntity( wxCommandEvent& event );
	void OnSaveCameraOffsets( wxCommandEvent& event );
	void OnLoadCameraOffsets( wxCommandEvent& event );
	void OnHistory( wxCommandEvent& event );
	void OnTeleportToZero( wxCommandEvent& event );
	void OnShowTransform( wxCommandEvent& event );
	void OnCacheTransform( wxCommandEvent& event );

	Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources );

	void CreatePreviewCamera();
	void DestroyPreviewCamera();

	void AlignMovingCamera( const Vector& pos );

	IPreviewItem* GetHelper( const String& helperName, bool forInternalVariable ) const;
};
