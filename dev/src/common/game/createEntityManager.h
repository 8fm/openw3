#pragma once
#include "../core/scriptable.h"

//////////////////////////////////////////
// CCreateEntityManager
// Interface for creating entities 
// Offers interface to scripts with CCreateEntityHelper
class CCreateEntityManager
{
private:

	TDynArray< THandle< CCreateEntityHelper > > m_createEntityHelperList;

public:
	CCreateEntityManager();
	virtual ~CCreateEntityManager();

	void Update();

	void OnWorldEnd();
	
	virtual Bool CreateEntityAsync( CCreateEntityHelper *const createEntityHelper, EntitySpawnInfo && entitySpawnInfo );
protected:
	void AddProcessingItem( CCreateEntityHelper *const createEntityHelper );
};