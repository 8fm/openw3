/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/behaviorGraphAnimationSlotNode.h"

struct SJobTreeExecutionContext;
class CEdJobTreeEditor;
class CJobTreeNode;

/// Preview panel that renders scene preview
class CEdJobTreePreviewPanel : public CEdPreviewPanel, public ISlotAnimationListener, public IEventHandler< CAnimationEventFired >
{
	DECLARE_EVENT_TABLE();
	friend class CEdJobTreeEditor;

private:
	void OnLoadEp2Man( wxCommandEvent& event );
	void OnLoadEp2Woman( wxCommandEvent& event );
	void OnLoadMan( wxCommandEvent& event );
	void OnLoadBigMan( wxCommandEvent& event );
	void OnLoadWoman( wxCommandEvent& event );
	void OnLoadDwarf( wxCommandEvent& event );
	void OnLoadChild( wxCommandEvent& event );

protected:

	static const String			CONFIG_PATH;
	static const String			CONFIG_ACTOR;

	CEdJobTreeEditor*			m_jobTreeEditor;
	CJobTreeNode*				m_jobTreeNode;
	
	CName						m_currentActionItemName;
	CEntity*					m_currentItemEntity;

	CActor*						m_currentActor;
	SJobTreeExecutionContext*	m_jobContext;

	CAnimatedComponent*			m_animated;		//!< actor's animated component

	const CJobActionBase*		m_currentlyPreviewedAction;

public:
	static const CName			JTP_SLOT_NAME;

	CEdJobTreePreviewPanel( wxWindow* parent, CEdJobTreeEditor* editor );
	~CEdJobTreePreviewPanel();
	void Init();

public:
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	CEntity* LoadEntity( CEntityTemplate* entityTemplate );
	CEntity* LoadEntity( const String& entityTemplatePath );
	void UnloadEntity();
	void SetJobTree( CJobTreeNode* jobTree );

	void Play( CName category );
	void PlayAnimation( CName animationName, CName itemName = CName::NONE );
	void Stop();

	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status );
	virtual String GetListenerName() const { return TXT("CEdJobTreePreviewPanel"); }

	virtual void OnViewportTick( IViewport* view, Float timeDelta );

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	void OnLoadEntity( wxCommandEvent& event );
	virtual void HandleContextMenu( Int32 x, Int32 y );

	RED_INLINE CActor* GetActor()
	{ return m_currentActor; }
	 
	//! Register this as anim event handler
	RED_INLINE void RegisterAnimEventHandler();

	//! Unregister this as anim event handler
	RED_INLINE void UnregisterAnimEventHandler();

	//! Handle animation event
	void HandleEvent( const CAnimationEventFired &event );

	//! Fake pick up
	void PerformPickUp();

	//! Fake put
	void PerformPut();
};