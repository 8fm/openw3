#pragma once

#include "questPhase.h"


///////////////////////////////////////////////////////////////////////////////

class CQuestStartBlock;
class CQuestGraph;

///////////////////////////////////////////////////////////////////////////////

class CQuest : public CQuestPhase
{
	DECLARE_ENGINE_RESOURCE_CLASS( CQuest, CQuestPhase, "w2quest", "Quest" );

public:
	CQuest();

	CQuestStartBlock* GetInput() const;

	String GetFileName() const;

};

BEGIN_CLASS_RTTI( CQuest )
	PARENT_CLASS( CQuestPhase )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
