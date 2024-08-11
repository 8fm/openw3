/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CQuestTestBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CQuestTestBlock, CQuestGraphBlock );

protected:
	CQuestTestBlock() {}

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 240, 17, 17 ); }
	virtual String GetBlockCategory() const { return TXT( "Tests" ); }
#endif
};

BEGIN_CLASS_RTTI( CQuestTestBlock )
	PARENT_CLASS( CQuestGraphBlock )
	END_CLASS_RTTI()