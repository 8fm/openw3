/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "object.h"
#include "objectMap.h"
#include "objectIterator.h"

BaseObjectIterator::BaseObjectIterator()
	:	m_currentObject( nullptr )
	,	m_index( -1 )
	,	m_filterClass( nullptr )
	,	m_includeFlags( DEFAULT_INCLUDE_FLAGS )
	,	m_excludeFlags( DEFAULT_EXCLUDE_FLAGS )
{
	GObjectsMap->LockForIteration();

	FindNextObject();
}

BaseObjectIterator::BaseObjectIterator( CClass* filterClass )
	:	m_currentObject( nullptr )
	,	m_index( -1 )
	,	m_filterClass( filterClass )
	,	m_includeFlags( DEFAULT_INCLUDE_FLAGS )
	,	m_excludeFlags( DEFAULT_EXCLUDE_FLAGS )
{
	GObjectsMap->LockForIteration();

	FindNextObject();
}

BaseObjectIterator::BaseObjectIterator( const Uint32 includeFlags, const Uint32 excludeFlags )
	:	m_currentObject( nullptr )
	,	m_index( -1 )
	,	m_filterClass( nullptr )
	,	m_includeFlags( includeFlags )
	,	m_excludeFlags( excludeFlags )
{
	GObjectsMap->LockForIteration();

	FindNextObject();
}

BaseObjectIterator::~BaseObjectIterator()
{
	GObjectsMap->UnlockForIteration();
}

void BaseObjectIterator::FindNextObject()
{
	if ( !GObjectsMap->Iterate( m_index, m_currentObject, m_filterClass, m_includeFlags, m_excludeFlags ) )
		m_currentObject = nullptr;
}
