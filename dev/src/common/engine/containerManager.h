#pragma once

#include "..\core\hashmap.h"
#include "..\engine\idTag.h"




class CContainerManager
{
public:

	struct SContainerData
	{
		Uint16 m_lootBitmask;

		SContainerData();

		void StreamLoad( ISaveFile* loader, Uint32 version );
		void StreamSave( ISaveFile* saver );
	};


private:
	THashMap< IdTag, SContainerData > m_containerData;

public:
	void Reset();

	Bool WasItemLooted( IdTag idTag, Uint32 itemID ) const;
	void NotifyItemLooted( IdTag idTag, Uint32 itemID, Bool looted = true );
	void NotifyQuestItemLooted( IdTag idTag, Uint32 itemID );

	void ResetContainerData( IdTag idTag );

private:
	void StreamLoad( ISaveFile* loader, Uint32 version );
	void StreamSave( ISaveFile* saver );

public:

	void Load( IGameLoader* loader );
	void Save( IGameSaver* saver );
};



