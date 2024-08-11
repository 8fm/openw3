#pragma once

#include "../core/hashmap.h"
#include "sectorData.h"

/// Entity pool for use with sector data streaming
class CSectorDataEntityPool
{
public:
	CSectorDataEntityPool( CDynamicLayer* layer );
	~CSectorDataEntityPool();

	// Allocate entity from pool
	// NOTE: entity is not attached
	CEntity* Allocate( const Matrix& referenceMatrix );

	// Release entity to pool
	void Release( CEntity* entity );

private:
	typedef TDynArray< CEntity* >		TFreeEntities;

	CDynamicLayer*			m_layer;
	TFreeEntities			m_entities;
};