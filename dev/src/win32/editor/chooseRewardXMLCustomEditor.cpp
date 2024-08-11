#include "build.h"
#include "chooseRewardXMLCustomEditor.h"
#include "../../common/game/rewardManager.h"


void CEdChooseRewardXMLCustomEditor::FillChoices()
{
	const CRewardManager* defMgr = GCommonGame->GetRewardManager();
	TDynArray< CName > rewards;
	defMgr->GetRewards( rewards );

	Sort( rewards.Begin(), rewards.End(), [] ( CName a, CName b ) { return Red::StringCompareNoCase( a.AsChar(), b.AsChar() ) < 0; } );

	for ( auto reward : rewards )
	{
		m_ctrlChoice->AppendString( reward.AsChar() );
	}
}


CEdChooseRewardXMLCustomEditor::CEdChooseRewardXMLCustomEditor( CPropertyItem* propertyItem )
	: ISelectionEditor( propertyItem )
{
}
