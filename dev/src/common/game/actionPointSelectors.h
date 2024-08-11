/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "2daProperties.h"
#include "behTreeVars.h"
#include "behTreeWorkData.h"

class CActionPointSelectorInstance;

class CActionPointSelector : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CActionPointSelector );

public:
	CActionPointSelector();

	virtual CActionPointSelectorInstance* SpawnInstance( CBehTreeSpawnContext& context );
};

BEGIN_CLASS_RTTI( CActionPointSelector )
	PARENT_CLASS( IScriptable )
END_CLASS_RTTI()

class CActionPointSelectorInstance
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

protected:
	static CName	SelectRandomSupportedCategory( const TDynArray< CName >& categoriesList, const CActionPointComponent*const actionPoint );

public:
	CActionPointSelectorInstance()											{}
	virtual ~CActionPointSelectorInstance()									{}

	virtual Bool	SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius = -1.0f ) = 0;
	virtual void	OnNodeActivated();
	virtual void	OnNodeDeactivated();
	virtual Bool	IsExternallyReservingAP() const;
};

///////////////////////////////////////////////////////////////////////////

class CCommunityActionPointSelector : public CActionPointSelector
{
	friend class CCommunityActionPointSelectorInstance;

	DECLARE_RTTI_SIMPLE_CLASS( CCommunityActionPointSelector )
public:

	CActionPointSelectorInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;
};

BEGIN_CLASS_RTTI( CCommunityActionPointSelector )
	PARENT_CLASS( CActionPointSelector )
END_CLASS_RTTI()

class CCommunityActionPointSelectorInstance : public CActionPointSelectorInstance
{
	typedef CActionPointSelectorInstance Super;
public:
	CCommunityActionPointSelectorInstance() {}

	Bool	SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius = -1.0f ) override;

	Bool	IsExternallyReservingAP() const override;

};

///////////////////////////////////////////////////////////////////////////

class CTimetableActionPointSelector : public CActionPointSelector
{
	friend class CTimetableActionPointSelectorInstance;

	DECLARE_RTTI_SIMPLE_CLASS( CTimetableActionPointSelector )
public:
	CActionPointSelectorInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;

protected:
	TTimetable m_timetable;
};

BEGIN_CLASS_RTTI( CTimetableActionPointSelector )
	PARENT_CLASS( CActionPointSelector )
	PROPERTY_INLINED( m_timetable, TXT( "Timetable definition" ) )
END_CLASS_RTTI()

class CTimetableActionPointSelectorInstance : public CActionPointSelectorInstance
{
	typedef CActionPointSelectorInstance Super;
public:
	CTimetableActionPointSelectorInstance( CTimetableActionPointSelector& def )
		: CActionPointSelectorInstance()
		, m_timetable( def.m_timetable ) {}

	Bool	SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius = -1.0f ) override;
protected:
	TTimetable	m_timetable;
};


//////////////////////////////////////////////////////////////////////////
// CSimpleActionPointSelector
class CSimpleActionPointSelector : public CActionPointSelector
#ifndef NO_EDITOR
	, public CActionPointCategories2dPropertyOwner
#endif
{
	friend class CSimpleActionPointSelectorInstance;
	DECLARE_RTTI_SIMPLE_CLASS( CSimpleActionPointSelector );

public:
	CActionPointSelectorInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;
	CSimpleActionPointSelector()
		: m_keepActionPointOnceSelected( false )
		, m_useNearestMatchingAP( false )
	{}
protected:
	TDynArray< CName >	m_categories;
	TagList				m_apTags;
	TagList				m_areaTags;
	CName				m_apAreaTag;
	Bool				m_keepActionPointOnceSelected;
	Bool				m_useNearestMatchingAP;
};

BEGIN_CLASS_RTTI( CSimpleActionPointSelector );
	PARENT_CLASS( CActionPointSelector );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_categories, TXT("Used action categories"), TXT("2daValueSelection") );
	PROPERTY_EDIT( m_apTags, TXT( "Optional list of required AP tags" ) );
	PROPERTY_EDIT( m_areaTags, TXT( "Working area tags" ) );
	PROPERTY_EDIT( m_apAreaTag, TXT( "Action point area tag" ) );
	PROPERTY_EDIT( m_keepActionPointOnceSelected,	TXT( "First used action point will be kept as long as it is available" ) );
	PROPERTY_EDIT( m_useNearestMatchingAP,	TXT( "If NPC should use nearest matching AP" ) );
END_CLASS_RTTI();

class CSimpleActionPointSelectorInstance : public CActionPointSelectorInstance
{
	typedef CActionPointSelectorInstance Super;
public:
	CSimpleActionPointSelectorInstance( CSimpleActionPointSelector& def, CBehTreeSpawnContext& context );

	Bool	SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP , Float forceRadius = -1.0f ) override;
protected:
	TDynArray< CName >			m_categories;
	TagList						m_areaTags;
	TagList						m_apTags;
	THandle< CAreaComponent >	m_apArea;
	Bool						m_keepActionPointOnceSelected;
	Bool						m_useNearestMatchingAP;

	CName						SelectRandomSupportedCategory( const CActionPointComponent*const actionPoint )const;
};

//////////////////////////////////////////////////////////////////////////
// CWanderActionPointSelector
struct SEncounterActionPointSelectorPair
{
	DECLARE_RTTI_STRUCT( SEncounterActionPointSelectorPair );

	CName	m_name;
	Float	m_chance;	
};

BEGIN_CLASS_RTTI( SEncounterActionPointSelectorPair );	
	PROPERTY_CUSTOM_EDIT( m_name, TXT("The name of the category"), TXT("2daValueSelection") );
	PROPERTY_EDIT( m_chance, TXT("The chance of the category to be selected in %") );
END_CLASS_RTTI();

class CWanderActionPointSelector : public CActionPointSelector
#ifndef NO_EDITOR
	, public CActionPointCategories2dPropertyOwner
#endif
{
	friend class CWanderActionPointSelectorInstance;
	DECLARE_RTTI_SIMPLE_CLASS( CWanderActionPointSelector );

public:
	CWanderActionPointSelector();
	CActionPointSelectorInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;

protected:
	TDynArray< SEncounterActionPointSelectorPair >		m_categories;
	TagList												m_areaTags;
	Float												m_delay;
	Float												m_radius;
	TagList												m_apTags;
	CName												m_apAreaTag;	
	Bool												m_chooseClosestAP;
};

BEGIN_CLASS_RTTI( CWanderActionPointSelector )
	PARENT_CLASS( CActionPointSelector )
	PROPERTY_EDIT( m_categories,		TXT( "This AP selector gives each category of AP a chance to be selected every m_delay seconds. m_categories allows to pair each category with a chance to be selected in %" ) )
	PROPERTY_EDIT( m_delay,				TXT( "( s ) Frequency at which the chances are refreshed" ) )
	PROPERTY_EDIT( m_radius,			TXT( "Radius that limits search for action points" ) )
	PROPERTY_EDIT( m_apTags,			TXT( "Action point tags" ) )
	PROPERTY_EDIT( m_areaTags,			TXT( "Working area tags" ) )
	PROPERTY_EDIT( m_apAreaTag,			TXT( "Action point area tag" ) )
	PROPERTY_EDIT( m_chooseClosestAP,	TXT( "If npc should choose closest matching AP" ) )
END_CLASS_RTTI()

class CWanderActionPointSelectorInstance : public CActionPointSelectorInstance
{
	typedef CActionPointSelectorInstance Super;
public:
	CWanderActionPointSelectorInstance( CWanderActionPointSelector& def, CBehTreeSpawnContext& context );

	Bool	SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius = -1.0f ) override;
	void	OnNodeActivated()override;
	void	OnNodeDeactivated()override;

protected:
	TDynArray< SEncounterActionPointSelectorPair >		m_categories;
	TagList												m_apTags;
	TagList												m_areaTags;
	Float												m_delay;
	Float												m_delayTimer;
	Float												m_radius;
	Bool												m_reset;
	TDynArray< CName >									m_elligibleCategories;
	THandle< CAreaComponent >							m_apArea;
	Bool												m_ignoreDirection;
	Bool												m_chooseClosestAP;
};
