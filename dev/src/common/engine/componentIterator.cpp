/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "componentIterator.h"
#include "component.h"
#include "entity.h"

BaseComponentIterator::BaseComponentIterator( const CEntity* entity, CClass* filterClass )
	: m_entity( entity )
	, m_components( entity->GetComponents() )
	, m_componentIndex( -1 )
	, m_class( filterClass )
{
	Next();
};

//! Copy constructor
BaseComponentIterator::BaseComponentIterator( const BaseComponentIterator& other )
	: m_entity( other.m_entity )
	, m_components( other.m_components )
	, m_componentIndex( other.m_componentIndex )
	, m_class( other.m_class )
{};

