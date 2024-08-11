/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

struct SActionPointId
{
	DECLARE_RTTI_STRUCT( SActionPointId );

public:
	CGUID	m_component;
	CGUID	m_entity;

	SActionPointId();
	SActionPointId( const CGUID& component, const CGUID& entity );

	RED_INLINE Bool operator==( const SActionPointId& rhs ) const
	{
		return m_component == rhs.m_component && m_entity == rhs.m_entity;
	}

	RED_INLINE Bool operator!=( const SActionPointId& rhs ) const
	{
		return m_component != rhs.m_component || m_entity != rhs.m_entity;
	}

	RED_INLINE Bool operator<( const SActionPointId& rhs ) const
	{
		return m_entity != rhs.m_entity ? m_entity < rhs.m_entity : m_component < rhs.m_component;
	}

	String	ToString() const;
	Bool	FromString( const String &text );

	RED_FORCE_INLINE Uint32 CalcHash() const
	{
		return m_component.CalcHash() ^ m_entity.CalcHash();
	}
};

BEGIN_CLASS_RTTI( SActionPointId );	
	PROPERTY( m_component );
	PROPERTY( m_entity );
END_CLASS_RTTI();

typedef SActionPointId TActionPointID;									//!< Unique action point identifier
extern const TActionPointID ActionPointBadID;							//!< Invalid action point identifier


