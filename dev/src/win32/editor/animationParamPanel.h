
#pragma once

#include "animationTreeBrowser.h"
#include "animationTrajectoryParamInitializer.h"
#include "../../common/engine/animationGameParams.h"

class CEdAnimationParamPanelListener
{
public:
	virtual void OnAnimationParamSelectedAnimation( const CSkeletalAnimationSetEntry* animation ) {}
	virtual void OnAnimationParamAddedToAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param ) {}
	virtual void OnAnimationParamRemovedFromAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param ) {}
};

class CEdAnimationTreeBrowserParamSetter : public CEdAnimationTreeBrowserPropSetter
{
	CClass*	m_paramClass;

public:
	CEdAnimationTreeBrowserParamSetter( CClass* paramClass )
		: m_paramClass( paramClass )
	{

	}

	virtual void Set( CSkeletalAnimationSetEntry* animation, CEdPropertiesPage* prop )
	{
		if ( animation )
		{
			const ISkeletalAnimationSetEntryParam* param = animation->FindParamByClass( m_paramClass );
			prop->SetObject( const_cast< ISkeletalAnimationSetEntryParam* >( param ) );
		}
		else
		{
			prop->SetNoObject();
		}
	}
};

class CEdAnimationParamPanel : public CEdAnimationTreeBrowser
{
	DECLARE_EVENT_TABLE()

	CClass*									m_paramClass;
	const CEdAnimationParamInitializer*		m_initializer;
	CEdAnimationParamPanelListener*			m_listener;

public:
	CEdAnimationParamPanel( wxWindow* parent, CClass* paramClass, const CEdAnimationParamInitializer* initializer, CEdAnimationParamPanelListener* listener = NULL, Bool verticalStyle = true );
	~CEdAnimationParamPanel();

protected:
	void RefreshParamPanel();

protected:
	virtual void OnItemAdded( wxTreeItemId item, const CSkeletalAnimationSetEntry* animation );

	virtual void OnSelectAnimation( CSkeletalAnimationSetEntry* animation );

protected:
	void OnTreeItemMenu( wxTreeEvent& event );
	void OnRemoveParam( wxCommandEvent& event );
	void OnAddParam( wxCommandEvent& event );
};
