
#pragma once

#include "animationPreview.h"

class CEdAnimationPreviewPostProcess;

class CEdAnimationTreeBrowserAnimFilter
{
public:
	virtual Bool Test( const CSkeletalAnimationSetEntry* animation ) = 0;
};

class CEdAnimationTreeBrowserPropSetter
{
public:
	virtual void Set( CSkeletalAnimationSetEntry* animation, CEdPropertiesPage* prop )
	{
		if ( animation )
		{
			prop->SetObject( animation );
		}
		else
		{
			prop->SetNoObject();
		}
	}
};

struct SEdAnimationTreeBrowserSettings
{
	Bool		m_verticalStyle;
	Bool		m_oneClickSelect;
	Bool		m_supportsDragAndDrop;

	CEdAnimationTreeBrowserAnimFilter*	m_filter;

	SEdAnimationTreeBrowserSettings( Bool vertical, CEdAnimationTreeBrowserAnimFilter* f )
		: m_verticalStyle( vertical )
		, m_filter( f )
		, m_oneClickSelect( false )
		, m_supportsDragAndDrop( true )
	{

	}

	SEdAnimationTreeBrowserSettings() 
		: m_verticalStyle( true )
		, m_filter( nullptr )
		, m_oneClickSelect( false )
		, m_supportsDragAndDrop( true )
	{

	}
};

class CEdAnimationTreeBrowser : public wxPanel
{
	DECLARE_EVENT_TABLE()

	wxTreeCtrl*							m_tree;
	CEdAnimationPreview*				m_preview;
	CEdPropertiesPage*					m_prop;
	CEdAnimationTreeBrowserAnimFilter*	m_filter;
	CEdAnimationTreeBrowserPropSetter*	m_propSetter;
	Bool								m_supportsDragAndDrop;
	Bool								m_oneClickSelect;

public:
	CEdAnimationTreeBrowser( wxWindow* parent, const SEdAnimationTreeBrowserSettings& settings );
	~CEdAnimationTreeBrowser();

	void LoadEntity( const String& fileName, const String& component = String::EMPTY );
	void UnloadEntity();

	void CloneAndUseAnimatedComponent( const CAnimatedComponent* animatedComponent );

	void SetFilter( CEdAnimationTreeBrowserAnimFilter* filter );
	void SetPropSetter( CEdAnimationTreeBrowserPropSetter* setter );

	void AddPreviewPostprocess( CEdAnimationPreviewPostProcess* postprocess );
	void SetBrowserStyle( Bool vertical );
	void SetAnimationGraphEnabled( Bool state );

	void CollectAllAnimations( TDynArray< const CSkeletalAnimationSetEntry* >& anims ) const;

	void SelectAnimation( const CName& animation );

	void Pause();
	void Unpause();

protected:
	void FillAnimationTree( const CAnimatedComponent* animatedComponent );
	void SelectAnimation( CSkeletalAnimationSetEntry* animation );
	void SelectAnimation( const wxTreeItemId& selectedId );

	wxTreeItemId FindAnimationTreeItem( CSkeletalAnimationSetEntry *anim );

protected:
	virtual void OnItemAdded( wxTreeItemId item, const CSkeletalAnimationSetEntry* animation ) {}
	virtual void OnItemAdded( wxTreeItemId item, const CSkeletalAnimationSet* set ) {}

	virtual void OnSelectAnimation( CSkeletalAnimationSetEntry* animation ) {}

protected:
	void OnTreeSelectionChanged( wxTreeEvent& event );
	void OnTreeItemActivated( wxTreeEvent& event );
	void OnTreeItemBeginDrag( wxTreeEvent &event );
};
