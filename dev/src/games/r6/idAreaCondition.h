/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idCondition.h"

class CIDAreaContition : public IIDContition
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CIDAreaContition, IIDContition )

protected:
	EntityHandle		m_area;
	Bool				m_checkOutside;

	Bool IsInArea( const Box& aabb ) const;
};

BEGIN_ABSTRACT_CLASS_RTTI( CIDAreaContition )
	PARENT_CLASS( IIDContition )
	PROPERTY_EDIT( m_area, TXT("An entity containing area component") )
	PROPERTY_EDIT( m_checkOutside, TXT("If true, will check if it is outside the area") )
END_CLASS_RTTI()

class CIDInterlocutorInAreaContition : public CIDAreaContition
{
	DECLARE_ENGINE_CLASS( CIDInterlocutorInAreaContition, CIDAreaContition, 0 )

protected:
	CName m_interlocutor;

public:
	virtual Bool IsFulfilled( Uint32 dialogId )	const;

};

BEGIN_CLASS_RTTI( CIDInterlocutorInAreaContition )
	PARENT_CLASS( CIDAreaContition )
	PROPERTY_CUSTOM_EDIT( m_interlocutor, TXT("An existing dialog interlocutor"), TXT("InterlocutorIDList") )	
END_CLASS_RTTI()

class CIDTaggedEntityInAreaContition : public CIDAreaContition
{
	DECLARE_ENGINE_CLASS( CIDTaggedEntityInAreaContition, CIDAreaContition, 0 )

protected:
	CName m_tagToMatch;

public:
	virtual Bool IsFulfilled( Uint32 dialogId )	const;
};

BEGIN_CLASS_RTTI( CIDTaggedEntityInAreaContition )
	PARENT_CLASS( CIDAreaContition )
	PROPERTY_EDIT( m_tagToMatch, TXT("Entity tag to look for") )
END_CLASS_RTTI()