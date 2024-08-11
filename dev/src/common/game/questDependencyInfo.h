#pragma once

namespace QuestDependencyInfo
{
	void MakeNiceString( String& myString );
	TDynArray< StringAnsi > MakeDBString( TDynArray< String > dbStrings );
	void Start( CQuestGraph* questGraph, TDynArray< CQuestGraphBlock* > blocks, String folderToDepFiles );
};
