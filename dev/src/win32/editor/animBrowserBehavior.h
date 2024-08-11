/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/behaviorGraphAnimationSlotNode.h"

#include "animBrowser.h"

//////////////////////////////////////////////////////////////////////////

class CAnimBrowserBehaviorPage;

class CLookAtTargetItem : public IPreviewItem
{
public:
	CLookAtTargetItem( CAnimBrowserBehaviorPage* page, Bool isStatic = true );

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) {}
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}

	virtual IPreviewItemContainer* GetItemContainer() const;
	virtual Bool HasCaption() const;

	Bool IsStatic() const;

protected:
	CAnimBrowserBehaviorPage*	m_page;
	Bool						m_static;
};

//////////////////////////////////////////////////////////////////////////

class CAnimBrowserBehaviorPage	: public wxPanel
								, public IAnimBrowserPageInterface
								, public IPreviewItemContainer
{
	DECLARE_EVENT_TABLE()

	static const unsigned int		PAGE_SLOT = 0;
	static const unsigned int		PAGE_ITEM = 1;
	static const unsigned int		PAGE_GAMEPLAY_MIMIC = 2;
	static const unsigned int		PAGE_LAST = 3;

	class SlotPageListener : public ISlotAnimationListener
	{
	public:
		CAnimBrowserBehaviorPage* m_page;

		virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status );
		virtual String GetListenerName() const { return TXT("CAnimBrowserBehaviorPage_Slot"); }
	};

	class GameplayMimicPageListener : public ISlotAnimationListener
	{
	public:
		CAnimBrowserBehaviorPage* m_page;

		virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status );
		virtual String GetListenerName() const { return TXT("CAnimBrowserBehaviorPage_Mimic"); }
	};

protected:
	CEdAnimBrowser*							m_browser;
	Bool									m_active;

	Bool									m_lookAtTool;
	String									m_targetName;
	CName									m_instName;
	String									m_animSlotName;

	TDynArray< CName >						m_slotAnimations;
	TDynArray< CName >						m_localSlotAnimation;

	SlotPageListener						m_slotListener;
	GameplayMimicPageListener				m_gameplayMimicListener;

	CBehaviorGraphAnimationBaseSlotNode*	m_slot;
	IBehaviorGraphSlotInterface*			m_gameplayMimicPlayer;

	static const Vector						DEFAULT_TARGET;

protected:
	Int32					m_itemLatentAction;

protected:
	CName				m_cachedInstanceName;
	String				m_cachedCategory;
	String				m_cachedCategory2;

protected:
	wxTreeCtrl*			m_animTree;
	wxListBox*			m_animList;
	wxStaticText*		m_selAnimName;

	wxNotebook*			m_notebook;

	wxStaticText*		m_dispDuration;
	wxStaticText*		m_dispProgress;
	wxStaticText*		m_dispTime;
	wxStaticText*		m_dispBlendIn;
	wxStaticText*		m_dispBlendOut;
	wxCheckBox*			m_dispBlendInCheck;
	wxCheckBox*			m_dispBlendOutCheck;

	CEdPropertiesPage*	m_gmAnimProp;
	CEdUndoManager*		m_undoManager;

public:
	CAnimBrowserBehaviorPage( wxWindow* parent, CEdAnimBrowser* browser, CEdUndoManager* undoManager );

public: // IAnimBrowserPageInterface
	void DestroyPanel();
	void EnablePanel( Bool flag );
	void OnSelectedAnimation();
	void OnRefreshAnimation();
	void Tick( Float timeDelta );
	void HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual void Save( CUserConfigurationManager& config );
	virtual void Load( CUserConfigurationManager& config );
	virtual void Reset( CUserConfigurationManager& config );

public:
	void LookAtEnabled( Bool flag );
	Vector GetLookAtTarget() const;
	virtual CWorld* GetPreviewItemWorld() const { return m_browser->m_previewPanel->GetPreviewWorld(); }
	virtual void OnSelectItem( IPreviewItem* item );

protected:
	void OnPageChanged( wxNotebookEvent& event );
	void OnLookAtTool( wxCommandEvent& event );
	void OnBehInstanceSelected( wxCommandEvent& event );
	void OnAnimSlotSelected( wxCommandEvent& event );
	void OnAddAnim( wxCommandEvent& event );
	void OnRemoveAnim( wxCommandEvent& event );
	void OnAnimListClick( wxCommandEvent& event );
	void OnAnimListDClick( wxCommandEvent& event );
	void OnPlay( wxCommandEvent& event );
	void OnStop( wxCommandEvent& event );
	void OnItemAction( wxCommandEvent& event );
	void OnItemCategory( wxCommandEvent& event );
	void OnItemCategory2( wxCommandEvent& event );
	void OnGMAnimPrev( wxCommandEvent& event );
	void OnGMAnimNext( wxCommandEvent& event );
	void OnGMAnimChoice( wxCommandEvent& event );

protected:
	void ShowItems( Bool flag );
	void UpdateLookAtTargets();
	void CreateLookAtTargets();
	void CreateLookAtTarget( Uint32 num, const Vector& pos );
	CLookAtTargetItem* FindItemByNode( const CNode* node );
	CLookAtTargetItem* FindItemByName( const String& name );
	CBehaviorGraphInstance* GetBehaviorGraphInstance() const;
	void RestoreCachedBehaviorInstance();

	void FillAnimationTree();
	void FillBehaviorInstances();
	void FillBehaviorSlots();
	void UpdateLookAtToolIcon();
	void ActivateBehaviorStack( Bool flag );
	void ActivateBehaviorInstance( const CName& instName );
	void SelectBehaviorSlot( const String& slotName );
	void UpdateAnimList( Int32 sel = -1 );
	void SetFreezeUserElem( Bool flag );
	void UpdateSlotDisp();

	Bool PlayNextAnim();
	void StopAnim();
	void FinishAnim();

	Bool IsLooped()		const;
	Float GetBlendIn()	const;
	Float GetBlendOut() const;
	CName GetSlotName() const;

	CActor* GetPreviewActor();
	void FillItemList();
	void StartItemLatentAction( EItemLatentAction action );
	void EndItemLatentAction();
	void ShowItemActionResult( Bool isOk );
	Bool IsWaitingForFinished() const;

	void PlayAnimationForMimicPage( const CName& animation );
	void PlayGameplayMimicAnimation( const CName& animation );
	void StopAnimationForMimicPage();

private:
	void ChangePage( const unsigned int page );

	void ShowPageSlot();
	void ShowPageItem();
	void ShowPageGameplayMimic();

	void HidePageSlot();
	void HidePageItem();
	void HidePageGameplayMimic();

	void TickPageSlot();
	void TickPageItem();
	void TickPageGameplayMimic();
};
