/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CJobTree;
class CActor;
class CStorySceneDirector;

// DIALOG_TOMSIN_TODO - to jest jakas zena to przepisania w calosci
// mutable CEntity *	m_entity WTF?? co to jest za krap

//////////////////////////////////////////////////////////////////////////

class CStorySceneAction : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CStorySceneAction, CObject );

public:
	CStorySceneAction() : m_maxTime(0.f), m_timeStarted(0.f), m_entity(NULL), m_actor(NULL)
	{}
	//returns true when action finished or skipped false  when action in progress
	virtual Bool Perform( CStorySceneDirector* director, Float timePassed ) const;

protected:
	virtual Bool DoPerform()	const = 0;
	virtual Bool DoFail()		const { return true;}
	virtual Bool IsCompleted()	const = 0;

protected:
	Float				m_maxTime;
	
	//need to rethink this one
	mutable Float		m_timeStarted;


	//those are fine 
	mutable CEntity *	m_entity;
	mutable CActor	*	m_actor;

public:
	RED_INLINE void SetMaxTime( Float time ) { m_maxTime = time; }
};

BEGIN_ABSTRACT_CLASS_RTTI( CStorySceneAction );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT_RANGE( m_maxTime, TXT( "Maximum time for this action to last" ) , 0.f , 60.f);
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////

class CStorySceneActionTeleport : public CStorySceneAction
{
	DECLARE_ENGINE_CLASS( CStorySceneActionTeleport, CStorySceneAction, 0 )

public:
	CStorySceneActionTeleport() : m_allowedDistance(0.01f)
	{}
	
	virtual Bool Perform( CStorySceneDirector* director, Float timePassed ) const;

protected:
	mutable	EngineTransform m_targetSpot;
	Float					m_allowedDistance;

	virtual Bool DoPerform()	const;
	virtual Bool IsCompleted()	const;
	virtual Bool DoFail()		const;
};

BEGIN_CLASS_RTTI( CStorySceneActionTeleport );
	PARENT_CLASS( CStorySceneAction );
	PROPERTY_EDIT( m_allowedDistance, TXT( "Max allowed distance difference from destination position" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneActionMoveTo : public CStorySceneActionTeleport
{
	DECLARE_ENGINE_CLASS( CStorySceneActionMoveTo, CStorySceneActionTeleport, 0  );

public:
	CStorySceneActionMoveTo() 
	{
		m_allowedDistance = 0.1f;
	}

protected:
	virtual Bool DoPerform()	const;
};

BEGIN_CLASS_RTTI( CStorySceneActionMoveTo );
	PARENT_CLASS( CStorySceneActionTeleport );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneActionSlide : public CStorySceneActionTeleport
{
	DECLARE_ENGINE_CLASS( CStorySceneActionSlide, CStorySceneActionTeleport, 0  );

public:
	CStorySceneActionSlide() : m_slideTime(0.f)
	{}

	RED_INLINE void SetSlideTime(Float slideTime){m_slideTime = slideTime;}
protected:
	Float m_slideTime;

	virtual Bool DoPerform()	const;
};

BEGIN_CLASS_RTTI( CStorySceneActionSlide );
	PARENT_CLASS( CStorySceneActionTeleport );
	PROPERTY_EDIT_RANGE(m_slideTime, TXT( "Time of slide" ),0.f , 60.f )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneActionStopWork : public CStorySceneAction
{
	DECLARE_ENGINE_CLASS( CStorySceneActionStopWork, CStorySceneAction, 0 );
public:
	CStorySceneActionStopWork()
	{}

protected:
	virtual Bool DoPerform()	const;
	virtual Bool IsCompleted()	const;
	virtual Bool DoFail()		const;
};

BEGIN_CLASS_RTTI( CStorySceneActionStopWork );
	PARENT_CLASS( CStorySceneAction );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneActionStartWork : public CStorySceneAction
{
	DECLARE_ENGINE_CLASS( CStorySceneActionStartWork, CStorySceneAction, 0 );

public:
	CStorySceneActionStartWork() : m_jobTree(NULL)
	{}

protected:
	virtual Bool DoPerform()	const;
	virtual Bool IsCompleted()	const;
	virtual Bool DoFail()		const;

	THandle< CJobTree >		m_jobTree; 
	CName					m_category;
};

BEGIN_CLASS_RTTI( CStorySceneActionStartWork );
	PARENT_CLASS( CStorySceneAction );
	PROPERTY_EDIT(m_jobTree,TXT("Job Tree"))
	PROPERTY_CUSTOM_EDIT( m_category, TXT("Animation category name"), TXT("ActionCategorySelect") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneActionRotateToPlayer : public CStorySceneAction
{
	DECLARE_ENGINE_CLASS( CStorySceneActionRotateToPlayer, CStorySceneAction, 0 );

public:
	CStorySceneActionRotateToPlayer() : m_acceptableAngleDif(3.f)
	{}

	RED_INLINE void SetAcceptableAngleDif(Float accAngDif){m_acceptableAngleDif = accAngDif;}
protected:
	virtual Bool DoPerform()	const;
	virtual Bool IsCompleted()	const;
	virtual Bool DoFail()		const;

	Float m_acceptableAngleDif;
};

BEGIN_CLASS_RTTI( CStorySceneActionRotateToPlayer );
	PARENT_CLASS( CStorySceneAction );
	PROPERTY_EDIT_RANGE(m_acceptableAngleDif, TXT("Max acceptable angle difference to final rotation"), 0.f , 45.f)
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneActionEquipItem : public CStorySceneAction
{
	DECLARE_ENGINE_CLASS( CStorySceneActionEquipItem, CStorySceneAction, 0 );

public:
	CStorySceneActionEquipItem() : m_leftHandItem(CNAME(Any)) , m_rightHandItem(CNAME(Any))
	{}
	CStorySceneActionEquipItem( CName leftHandItem, CName rightHandItem)
		: m_leftHandItem(leftHandItem), m_rightHandItem(rightHandItem)
	{}

	CName m_leftHandItem;
	CName m_rightHandItem;

protected:
	virtual Bool DoPerform()	const;
	virtual Bool IsCompleted()	const;
	virtual Bool DoFail()		const;
};

BEGIN_CLASS_RTTI( CStorySceneActionEquipItem );
	PARENT_CLASS( CStorySceneAction );
	PROPERTY_CUSTOM_EDIT(  m_leftHandItem,   TXT( "Item to be held in left hand" ), TXT( "ChooseItem" ) );
	PROPERTY_CUSTOM_EDIT(  m_rightHandItem,  TXT( "Item to be held in right hand" ), TXT( "ChooseItem" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////


