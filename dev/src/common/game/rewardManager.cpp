#include "build.h"

#include "rewardManager.h"
#include "../core/depot.h"
#include "../core/xmlFileReader.h"

#define REWARDS_FILE TXT("gameplay\\rewards\\rewards.xml")

const String CRewardManager::NODE_ROOT = TXT( "redxml" );
const String CRewardManager::NODE_REWARD = TXT("reward");
const String CRewardManager::NODE_NAME = TXT("name");
const String CRewardManager::NODE_EXPERIENCE = TXT("experience");
const String CRewardManager::NODE_GOLD = TXT("gold");
const String CRewardManager::NODE_ACHIEVEMENT = TXT("achievement");
const String CRewardManager::NODE_SCRIPT = TXT("script");
const String CRewardManager::NODE_ITEMS = TXT("items");
const String CRewardManager::NODE_ITEM = TXT("item");
const String CRewardManager::NODE_ITEM_NAME = TXT("name");
const String CRewardManager::NODE_ITEM_AMOUNT = TXT("amount");
const String CRewardManager::NODE_LEVEL = TXT("level");

CRewardManager::CRewardManager()
{
	LoadRewards();
}

CRewardManager::~CRewardManager()
{
}

const SReward* CRewardManager::GetReward( CName name ) const
{
	return m_rewards.FindPtr( name );
}

void CRewardManager::GetRewards( TDynArray< CName >& rewardNames ) const
{
	m_rewards.GetKeys( rewardNames );
}

void CRewardManager::LoadRewards()
{
	LoadRewardsFromFile( REWARDS_FILE,  nullptr );
}

Bool CRewardManager::LoadRewardsFromFile( const String& xmlFilePath, TDynArray< CName >* loadedRewardNames /*= nullptr*/ )
{
	IFile* file = NULL;
	CDiskFile *diskFile = GDepot->FindFile( xmlFilePath );

	if( !diskFile )
	{
		return false;
	}

	file = diskFile->CreateReader();

	if( !file )
	{
		return false;
	}

	CXMLFileReader xmlReader( *file );
	if ( !xmlReader.GetChildCount() )
	{
		RED_WARNING( false, "Reward Manager: Invalid XML File: %ls", xmlFilePath.AsChar() );
	}

	String temp;
	if ( xmlReader.BeginNode( NODE_ROOT, true ) )
	{
		while ( xmlReader.BeginNode( NODE_REWARD, true ) )
		{
			SReward reward;
			if ( !xmlReader.Attribute( TXT("name"), temp ) )
			{
				continue;
			}
			reward.m_name = CName( temp );
			if ( xmlReader.Attribute( TXT("experience"), temp ) )
			{
				FromString( temp, reward.m_experience );
			}
			if ( xmlReader.Attribute( TXT("gold"), temp ) )
			{
				FromString( temp, reward.m_gold );
			}
			if ( xmlReader.Attribute( TXT("achievement"), temp ) )
			{
				FromString( temp, reward.m_achievement );
			}
			if ( xmlReader.Attribute( TXT("level"), temp ) )
			{
				FromString( temp, reward.m_level );
			}
			if ( xmlReader.Attribute( TXT("script"), temp ) )
			{
				reward.m_script = CName( temp );
			}
			if ( xmlReader.Attribute( TXT("comment"), temp ) )
			{
				reward.m_comment = temp;
			}

			if ( xmlReader.BeginNode( NODE_ITEMS, true ) )
			{
				while ( xmlReader.BeginNode( NODE_ITEM, true ) )
				{
					SItemReward rewardItem;
					if ( !xmlReader.Attribute( TXT("name"), temp ) )
					{
						continue;
					}
					rewardItem.m_item = CName( temp );

					if ( xmlReader.Attribute( TXT("amount"), temp ) )
					{
						FromString( temp, rewardItem.m_amount );
					}

					reward.m_items.PushBack( rewardItem );
					

					xmlReader.EndNode();
				}
				xmlReader.EndNode();
			}
			xmlReader.EndNode();
						
			if( !m_rewards.KeyExist( reward.m_name) )
			{
				m_rewards.Insert( reward.m_name, reward );
				if( loadedRewardNames )
				{
					loadedRewardNames->PushBackUnique( reward.m_name );
				}
			}
			else
			{
				RED_WARNING( false, "Reward Manager: %ls reward already defined", reward.m_name.AsChar() );
			}
			
		}
		xmlReader.EndNode();
	}
	return true;
}

void CRewardManager::RemoveRewards( const TDynArray< CName >& rewardNames )
{
	for( auto& rewardName : rewardNames )
	{
		m_rewards.Erase( rewardName );
	}
}
