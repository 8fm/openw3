#pragma once

#include "rewards.h"

class CRewardManager
{
	THashMap<CName, SReward> m_rewards;

	static const String	NODE_ROOT;
	static const String NODE_REWARD;
	static const String NODE_NAME;
	static const String NODE_EXPERIENCE;
	static const String NODE_GOLD;
	static const String NODE_ACHIEVEMENT;
	static const String NODE_SCRIPT;
	static const String NODE_ITEMS;
	static const String NODE_ITEM;
	static const String NODE_ITEM_NAME;
	static const String NODE_ITEM_AMOUNT;
	static const String NODE_LEVEL;

public:
	CRewardManager();
	~CRewardManager();

	const SReward* GetReward( CName name ) const;

	void GetRewards( TDynArray< CName >& rewardNames ) const;

	Bool LoadRewardsFromFile( const String& xmlFilePath, TDynArray< CName >* loadedRewardNames = nullptr );
	void RemoveRewards( const TDynArray< CName >& rewardNames );

private:
	void LoadRewards();
};
