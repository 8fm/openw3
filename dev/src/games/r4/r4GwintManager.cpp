#include "build.h"
#include "r4GwintManager.h"

IMPLEMENT_RTTI_ENUM( eGwintFaction ); 
IMPLEMENT_RTTI_ENUM( eGwintType );
IMPLEMENT_RTTI_ENUM( eGwintEffect );

IMPLEMENT_ENGINE_CLASS( SCardDefinition );
IMPLEMENT_ENGINE_CLASS( SDeckDefinition );
IMPLEMENT_ENGINE_CLASS( CollectionCard );

IMPLEMENT_ENGINE_CLASS( CR4GwintManager );

RED_DEFINE_STATIC_NAME(gwint_card_definitions_final);
RED_DEFINE_STATIC_NAME(gwint_battle_king_card_definitions);

RED_DEFINE_STATIC_NAME(index);
RED_DEFINE_STATIC_NAME(title);
RED_DEFINE_STATIC_NAME(power);
RED_DEFINE_STATIC_NAME(picture);
RED_DEFINE_STATIC_NAME(dlcPicture);
RED_DEFINE_STATIC_NAME(dlcPictureFlag);
RED_DEFINE_STATIC_NAME(dlcPictureFlag_name);
RED_DEFINE_STATIC_NAME(faction_index);

RED_DEFINE_STATIC_NAME(type_flags);
RED_DEFINE_STATIC_NAME(effect_flags);
RED_DEFINE_STATIC_NAME(summonFlags);

RED_DEFINE_STATIC_NAME(OnGwintSetupNewgame);
RED_DEFINE_STATIC_NAME(OnGwintSetupSkellige);

RED_DEFINE_STATIC_NAME(SBCollectionCardSize);
RED_DEFINE_STATIC_NAME(SBCollectionCard);
RED_DEFINE_STATIC_NAME(SBLeaderCollectionCardSize);
RED_DEFINE_STATIC_NAME(SBLeaderCollectionCard);
RED_DEFINE_STATIC_NAME(cardIndex);
RED_DEFINE_STATIC_NAME(numCopies);

RED_DEFINE_STATIC_NAME(SBSelectedDeckIndex);
RED_DEFINE_STATIC_NAME(deckIndex);

RED_DEFINE_STATIC_NAME(GwintDecks);
RED_DEFINE_STATIC_NAME(GwintDeck);
RED_DEFINE_STATIC_NAME(DeckCount);
RED_DEFINE_STATIC_NAME(CardCount);
RED_DEFINE_STATIC_NAME(GwintDeckCard);
RED_DEFINE_STATIC_NAME(LeaderIndex);
RED_DEFINE_STATIC_NAME(DeckUnlocked);

RED_DEFINE_STATIC_NAME(GwintTutorials);
RED_DEFINE_STATIC_NAME(GwintTutorialsDone);
RED_DEFINE_STATIC_NAME(GwintDeckTutorialsDone);

CR4GwintManager::CR4GwintManager()
: m_selectedDeckIndex(GwintFaction_Neutral)
, m_hasDoneTutorial(false)
, m_hasDoneDeckTutorial(false)
{
}

CR4GwintManager::~CR4GwintManager()
{

}

void CR4GwintManager::Initialize()
{
	LoadCardXMLs();
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CR4GwintManager::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

void CR4GwintManager::OnGameStart( const CGameInfo& gameInfo )
{
	if (gameInfo.IsNewGame())
	{
		m_playerCollection.ClearFast();
		m_playerLeaderCollection.ClearFast();

		m_hasDoneTutorial = false;
		m_hasDoneDeckTutorial = false;

		InitializeDecks();
	}
	else if (gameInfo.IsSavedGame())
	{
		OnLoadGame(gameInfo.m_gameLoadStream);

		// This currently happens when loading a save that did not have the new gwint manager
		if (m_playerDecks.Size() == 0)
		{
			InitializeDecks();
		}
		else if (m_playerDecks.Size() == 4) // for when skellige hasn't been initialized in the old save
		{
			InitializeSkellige();
		}
	}
}

bool CR4GwintManager::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, GetStaticClass()->GetName() );

	// Card Collection
	{
		CGameSaverBlock entriesBlock( saver, CNAME( SBCollectionCardSize ) );

		saver->WriteValue( CNAME( Size ), m_playerCollection.Size() );

		for( TDynArray<CollectionCard>::iterator iter = m_playerCollection.Begin(); iter != m_playerCollection.End(); ++iter )
		{
			CGameSaverBlock entriesBlock( saver, CNAME( SBCollectionCard ) );

			saver->WriteValue( CNAME( cardIndex ), iter->m_cardID );
			saver->WriteValue( CNAME( numCopies ), iter->m_numCopies );
		}
	}

	// Leader Card Collection
	{
		CGameSaverBlock entriesBlock( saver, CNAME( SBLeaderCollectionCardSize ) );

		saver->WriteValue( CNAME( Size ), m_playerLeaderCollection.Size() );

		for( TDynArray<CollectionCard>::iterator iter = m_playerLeaderCollection.Begin(); iter != m_playerLeaderCollection.End(); ++iter )
		{
			CGameSaverBlock entriesBlock( saver, CNAME( SBLeaderCollectionCard ) );

			saver->WriteValue( CNAME( cardIndex ), iter->m_cardID );
			saver->WriteValue( CNAME( numCopies ), iter->m_numCopies );
		}
	}

	// Selected Deck Index
	{
		CGameSaverBlock entriesBlock( saver, CNAME( SBSelectedDeckIndex ) );

		saver->WriteValue( CNAME( deckIndex ), m_selectedDeckIndex );
	}

	// Decks
	{
		CGameSaverBlock entriesBlock( saver, CNAME( GwintDecks ) );

		saver->WriteValue( CNAME( DeckCount ), m_playerDecks.Size() );

		for( TDynArray<SDeckDefinition>::iterator iter = m_playerDecks.Begin(); iter != m_playerDecks.End(); ++iter )
		{
			CGameSaverBlock entriesBlock( saver, CNAME( GwintDeck ) );

			saver->WriteValue( CNAME( DeckUnlocked ), iter->m_unlocked );

			saver->WriteValue( CNAME( LeaderIndex ), iter->m_leaderIndex );

			saver->WriteValue( CNAME( CardCount ), iter->m_cardIndices.Size() );

			for (TDynArray<Int32>::iterator card_iter = iter->m_cardIndices.Begin(); card_iter != iter->m_cardIndices.End(); ++card_iter)
			{
				CGameSaverBlock entriesBlock( saver, CNAME( GwintDeckCard ) );

				saver->WriteValue( CNAME( cardIndex ), *(card_iter) );
			}
		}
	}

	// Gwint Tutorials done
	{
		CGameSaverBlock entriesBlock( saver, CNAME( GwintTutorials ) );

		saver->WriteValue( CNAME( GwintTutorialsDone ), m_hasDoneTutorial );
		saver->WriteValue( CNAME( GwintDeckTutorialsDone ), m_hasDoneDeckTutorial );
	}

	END_TIMER_BLOCK( time )

	return true;
}
void CR4GwintManager::OnLoadGame( IGameLoader* loader )
{
	CGameSaverBlock block( loader, GetStaticClass()->GetName() );

	// Card Collection
	{
		CGameSaverBlock entriesBlock( loader, CNAME( SBCollectionCardSize ) );

		m_playerCollection.ClearFast();

		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for( Uint32 i = 0; i < size; ++i )
		{
			CGameSaverBlock entriesBlock( loader, CNAME( SBCollectionCard ) );

			Int32 cardIndex;
			Int32 cardCount;

			loader->ReadValue( CNAME( cardIndex ), cardIndex );
			loader->ReadValue( CNAME( numCopies ), cardCount );

			CollectionCard newEntry;
			newEntry.m_cardID = cardIndex;
			newEntry.m_numCopies = cardCount;

			m_playerCollection.PushBack(newEntry);
		}
	}

	// Leader Card Collection
	{
		CGameSaverBlock entriesBlock( loader, CNAME( SBLeaderCollectionCardSize ) );

		m_playerLeaderCollection.ClearFast();

		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

		for( Uint32 i = 0; i < size; ++i )
		{
			CGameSaverBlock entriesBlock( loader, CNAME( SBLeaderCollectionCard ) );

			Int32 cardIndex;
			Int32 cardCount;

			loader->ReadValue( CNAME( cardIndex ), cardIndex );
			loader->ReadValue( CNAME( numCopies ), cardCount );

			CollectionCard newEntry;
			newEntry.m_cardID = cardIndex;
			newEntry.m_numCopies = cardCount;

			m_playerLeaderCollection.PushBack(newEntry);
		}
	}

	// Selected Deck Index
	{
		CGameSaverBlock entriesBlock( loader, CNAME( SBSelectedDeckIndex ) );

		loader->ReadValue( CNAME( deckIndex ), m_selectedDeckIndex );

		if (m_selectedDeckIndex == GwintFaction_Neutral)
		{
			m_selectedDeckIndex = GwintFaction_NothernKingdom;
		}
	}

	// Decks
	{
		CGameSaverBlock entriesBlock( loader, CNAME( GwintDecks ) );

		Uint32 size = loader->ReadValue< Uint32 >( CNAME( DeckCount ) );

		m_playerDecks.ClearFast();

		for( Uint32 i = 0; i < size; ++i )
		{
			CGameSaverBlock entriesBlock( loader, CNAME( GwintDeck ) );

			SDeckDefinition currentDeck;
			currentDeck.m_specialCard = -1;


			loader->ReadValue( CNAME( DeckUnlocked ), currentDeck.m_unlocked );
			loader->ReadValue( CNAME( LeaderIndex ), currentDeck.m_leaderIndex );

			Uint32 numCards;
			loader->ReadValue( CNAME( CardCount ), numCards );

			for (Uint32 x = 0; x < numCards; ++x)
			{
				CGameSaverBlock entriesBlock( loader, CNAME( GwintDeckCard ) );

				Int32 cardIndex;
				loader->ReadValue( CNAME( cardIndex ), cardIndex );

				currentDeck.m_cardIndices.PushBack(cardIndex);
			}

			m_playerDecks.PushBack(currentDeck);
		}
	}

	// Gwint Tutorials done
	{
		CGameSaverBlock entriesBlock( loader, CNAME( GwintTutorials ) );

		loader->ReadValue( CNAME( GwintTutorialsDone ), m_hasDoneTutorial );
		loader->ReadValue( CNAME( GwintDeckTutorialsDone ), m_hasDoneDeckTutorial );
	}
}

void CR4GwintManager::InitializeDecks()
{
	m_playerDecks.ClearFast();
	m_playerDecks.PushBack(SDeckDefinition());
	m_playerDecks.PushBack(SDeckDefinition());
	m_playerDecks.PushBack(SDeckDefinition());
	m_playerDecks.PushBack(SDeckDefinition());
	m_playerDecks.PushBack(SDeckDefinition());

	this->CallEvent(CNAME(OnGwintSetupNewgame));
}

void CR4GwintManager::InitializeSkellige()
{
	m_playerDecks.PushBack(SDeckDefinition());

	this->CallEvent(CNAME(OnGwintSetupSkellige));
}

void CR4GwintManager::LoadCardXMLs()
{
	if (!GCommonGame || !(GCommonGame->GetDefinitionsManager()))
	{
		return;
	}

	m_cardDefs.Clear();
	m_kingDefs.Clear();

	LOG_GAME(TXT("Constructing Card information for gwint"));
	LoadCardsFromXML( CNAME(gwint_card_definitions_final), m_cardDefs );
	LOG_GAME(TXT("Constructing Leader Card information for gwint"));
	LoadCardsFromXML( CNAME(gwint_battle_king_card_definitions), m_kingDefs );
}

void CR4GwintManager::LoadCardsFromXML( CName nodeTag, TDynArray<SCardDefinition>& targetList )
{
	const SCustomNode* rootCardNode = GCommonGame->GetDefinitionsManager()->GetCustomNode( nodeTag );

	if (!rootCardNode)
	{
		ASSERT(rootCardNode, TXT("Failed to read gwint card data xml values") );
		return;
	}

	Uint32 i;
	Uint32 attr_it;
	Uint32 subNode_it;
	Uint32 subsubNode_it;

	Int32 currentFlag;

	for (i = 0; i < rootCardNode->m_subNodes.Size(); ++ i)
	{
		SCardDefinition cardDefinition;
		cardDefinition.m_typeFlags = 0;
		cardDefinition.m_dlcPicture = TXT("");
		cardDefinition.m_dlcPictureFlag = CName::NONE;

		const SCustomNode& currentNode = rootCardNode->m_subNodes[i];

		for (attr_it = 0; attr_it < currentNode.m_attributes.Size(); ++attr_it)
		{
			const SCustomNodeAttribute& currentAttr = currentNode.m_attributes[attr_it];

			if (currentAttr.m_attributeName == CNAME(index))
			{
				currentAttr.GetValueAsInt(cardDefinition.m_index);
			}
			else if (currentAttr.m_attributeName == CNAME(title))
			{
				cardDefinition.m_title = currentAttr.GetValueAsString();
			}
			else if (currentAttr.m_attributeName == CNAME(description))
			{
				cardDefinition.m_description = currentAttr.GetValueAsString();
			}
			else if (currentAttr.m_attributeName == CNAME(power))
			{
				currentAttr.GetValueAsInt(cardDefinition.m_power);
			}
			else if (currentAttr.m_attributeName == CNAME(picture))
			{
				cardDefinition.m_picture = currentAttr.GetValueAsString();
			}
			else if (currentAttr.m_attributeName == CNAME(dlcPictureFlag_name))
			{
				cardDefinition.m_dlcPictureFlag = currentAttr.GetValueAsCName();
			}
			else if (currentAttr.m_attributeName == CNAME(dlcPicture))
			{
				cardDefinition.m_dlcPicture = currentAttr.GetValueAsString();
			}
			else if (currentAttr.m_attributeName == CNAME(faction_index))
			{
				cardDefinition.m_faction = FactionFromString(currentAttr.GetValueAsString());
			}
		}

		for (subNode_it = 0; subNode_it < currentNode.m_subNodes.Size(); ++subNode_it)
		{
			const SCustomNode& currentSubNode = currentNode.m_subNodes[subNode_it];

			for (subsubNode_it = 0; subsubNode_it < currentSubNode.m_subNodes.Size(); ++subsubNode_it)
			{
				const SCustomNode& currentSubSubNode = currentSubNode.m_subNodes[subsubNode_it];

				// Currently they all have 1 attribute, making life easier by avoiding yet another for loop
				if (currentSubSubNode.m_attributes.Size() > 0)
				{
					const SCustomNodeAttribute& currentAttr = currentSubSubNode.m_attributes[0];

					if (currentSubNode.m_nodeName == CNAME(type_flags))
					{
						currentFlag = TypeFromString(currentAttr.GetValueAsString());
						cardDefinition.m_typeFlags |= currentFlag;
					}
					else if (currentSubNode.m_nodeName == CNAME(effect_flags))
					{
						cardDefinition.m_effectFlags.PushBack(EffectFromString(currentAttr.GetValueAsString()));
					}
					else if (currentSubNode.m_nodeName == CNAME(summonFlags))
					{
						currentAttr.GetValueAsInt(currentFlag);
						cardDefinition.m_summonFlags.PushBack(currentFlag);
					}
				}
			}
		}

		targetList.PushBack(cardDefinition);
	}
}

eGwintFaction CR4GwintManager::FactionFromString(const String& value)
{
	if (value == TXT("F_NO_MANS_LAND"))
	{
		return GwintFaction_NoMansLand;
	}
	else if (value == TXT("F_NILFGAARD"))
	{
		return GwintFaction_Nilfgaard;
	}
	else if (value == TXT("F_NORTHERN_KINGDOM"))
	{
		return GwintFaction_NothernKingdom;
	}
	else if (value == TXT("F_SCOIATAEL"))
	{
		return GwintFaction_Scoiatael;
	}
	else if (value == TXT("F_SKELLIGE"))
	{
		return GwintFaction_Skellige;
	}
	else // Default to Neutral
	{
		return GwintFaction_Neutral;
	}
}

eGwintType CR4GwintManager::TypeFromString(const String& value)
{
	if (value == TXT("TYPE_MELEE"))
	{
		return GwintType_Melee;
	}
	else if (value == TXT("TYPE_RANGED"))
	{
		return GwintType_Ranged;
	}
	else if (value == TXT("TYPE_SIEGE"))
	{
		return GwintType_Siege;
	}
	else if (value == TXT("TYPE_CREATURE"))
	{
		return GwintType_Creature;
	}
	else if (value == TXT("TYPE_WEATHER"))
	{
		return GwintType_Weather;
	}
	else if (value == TXT("TYPE_SPELL"))
	{
		return GwintType_Spell;
	}
	else if (value == TXT("TYPE_ROW_MODIFIER"))
	{
		return GwintType_RowModifier;
	}
	else if (value == TXT("TYPE_HERO"))
	{
		return GwintType_Hero;
	}
	else if (value == TXT("TYPE_SPY"))
	{
		return GwintType_Spy;
	}
	else if (value == TXT("TYPE_FRIENDLY_EFFECT"))
	{
		return GwintType_FriendlyEffect;
	}
	else if (value == TXT("TYPE_OFFENSIVE_EFFECT"))
	{
		return GwintType_OffensiveEffect;
	}
	else if (value == TXT("TYPE_GLOBAL_EFFECT"))
	{
		return GwintType_GlobalEffect;
	}
	else
	{
		return GwintType_None;
	}
}

eGwintEffect CR4GwintManager::EffectFromString(const String& value)
{
	if (value == TXT("CP_BIN2"))
	{
		return GwintEffect_Bin2;
	}
	else if (value == TXT("CP_MELEE_SCORCH"))
	{
		return GwintEffect_MeleeScorch;
	}
	else if (value == TXT("CP_11TH_CARD"))
	{
		return GwintEffect_11thCard;
	}
	else if (value == TXT("CP_CLEAR_WEATHER"))
	{
		return GwintEffect_ClearWeather;
	}
	else if (value == TXT("CP_PICK_WEATHER_CARD"))
	{
		return GwintEffect_PickWeatherCard;
	}
	else if (value == TXT("CP_PICK_RAIN_CARD"))
	{
		return GwintEffect_PickRainCard;
	}
	else if (value == TXT("CP_PICK_FOG_CARD"))
	{
		return GwintEffect_PickFogCard;
	}
	else if (value == TXT("CP_PICK_FROST_CARD"))
	{
		return GwintEffect_PickFrostCard;
	}
	else if (value == TXT("CP_VIEW_3_ENEMY_CARDS"))
	{
		return GwintEffect_View3EnemyCard;
	}
	else if (value == TXT("CP_RESURECT_CARD"))
	{
		return GwintEffect_ResurectCard;
	}
	else if (value == TXT("CP_RESURECT_FROM_ENEMY"))
	{
		return GwintEffect_ResurectFromEnemy;
	}
	else if (value == TXT("CP_BIN2_PICK1"))
	{
		return GwintEffect_Bin2Pick1;
	}
	else if (value == TXT("CP_MELEE_HORN"))
	{
		return GwintEffect_MeleeHorn;
	}
	else if (value == TXT("CP_RANGE_HORN"))
	{
		return GwintEffect_RangedHorn;
	}
	else if (value == TXT("CP_SIEGE_HORN"))
	{
		return GwintEffect_SiegeHorn;
	}
	else if (value == TXT("CP_SIEGE_SCORCH"))
	{
		return GwintEffect_SiegeScorch;
	}
	else if (value == TXT("CP_COUNTER_KING_ABLILITY"))
	{
		return GwintEffect_CounterKingAbility;
	}
	else if (value == TXT("EFFECT_MELEE"))
	{
		return GwintEffect_Melee;
	}
	else if (value == TXT("EFFECT_RANGED"))
	{
		return GwintEffect_Ranged;
	}
	else if (value == TXT("EFFECT_SIEGE"))
	{
		return GwintEffect_Siege;
	}
	else if (value == TXT("EFFECT_UNSUMMON_DUMMY"))
	{
		return GwintEffect_UnsummonDummy;
	}
	else if (value == TXT("EFFECT_HORN"))
	{
		return GwintEffect_Horn;
	}
	else if (value == TXT("EFFECT_DRAW"))
	{
		return GwintEffect_Draw;
	}
	else if (value == TXT("EFFECT_SCORCH"))
	{
		return GwintEffect_Scorch;
	}
	else if (value == TXT("EFFECT_CLEAR_SKY"))
	{
		return GwintEffect_ClearSky;
	}
	else if (value == TXT("EFFECT_SUMMON_CLONES"))
	{
		return GwintEffect_SummonClones;
	}
	else if (value == TXT("EFFECT_IMPROVE_NEIGHBOURS"))
	{
		return GwintEffect_ImproveNeightbours;
	}
	else if (value == TXT("EFFECT_NURSE"))
	{
		return GwintEffect_Nurse;
	}
	else if (value == TXT("EFFECT_DRAW_X2"))
	{
		return GwintEffect_Draw2;
	}
	else if (value == TXT("EFFECT_SAME_TYPE_MORALE"))
	{
		return GwintEffect_SameTypeMorale;
	}
	else if (value == TXT("EFFECT_AGILE_REPOSITION"))
	{
		return GwintEffect_AgileReposition;
	}
	else if (value == TXT("EFFECT_RANDOM_RESSURECT"))
	{
		return GwintEffect_RandomRessurect;
	}
	else if (value == TXT("EFFECT_DOUBLE_SPY"))
	{
		return GwintEffect_DoubleSpy;
	}
	else if (value == TXT("EFFECT_RANGED_SCORCH"))
	{
		return GwintEffect_RangedScorch;
	}
	else if (value == TXT("EFFECT_SUICIDE_SUMMON"))
	{
		return GwintEffect_SuicideSummon;
	}
	else if (value == TXT("EFFECT_MUSHROOM"))
	{
		return GwintEffect_Mushroom;
	}
	else if (value == TXT("EFFECT_MORPH"))
	{
		return GwintEffect_Morph;
	}
	else if (value == TXT("EFFECT_WEATHER_RESISTANT"))
	{
		return GwintEffect_WeatherResistant;
	}
	else if (value == TXT("EFFECT_GRAVEYARD_SHUFFLE"))
	{
		return GwintEffect_GraveyardShuffle;
	}
	else
	{
		return GwintEffect_None;
	}
}

void CR4GwintManager::AddCardToCollection(Int32 cardIndex)
{
	TDynArray<CollectionCard>* listReference;

	if (cardIndex >= 1000)
	{
		listReference = &m_playerLeaderCollection;
	}
	else
	{
		listReference = &m_playerCollection;
	}

	for (Uint32 i = 0; i < listReference->Size(); ++i)
	{
		if ((*listReference)[i].m_cardID == cardIndex)
		{
			(*listReference)[i].m_numCopies += 1;

			return;
		}
	}

	CollectionCard newEntry;
	newEntry.m_cardID = cardIndex;
	newEntry.m_numCopies = 1;
	listReference->PushBack(newEntry);
}

void CR4GwintManager::RemoveCardFromCollection(Int32 cardIndex)
{
	TDynArray<CollectionCard>* listReference;

	if (cardIndex >= 1000)
	{
		listReference = &m_playerLeaderCollection;
	}
	else
	{
		listReference = &m_playerCollection;
	}

	int numCopiesLeft = 0;

	for (Uint32 i = 0; i < listReference->Size(); ++i)
	{
		if ((*listReference)[i].m_cardID == cardIndex && (*listReference)[i].m_numCopies > 0)
		{
			(*listReference)[i].m_numCopies -= 1;
			numCopiesLeft = (*listReference)[i].m_numCopies;
		}
	}

	// Make sure none of the player decks have more copies of this card than is now valid
	for (Uint32 x = 0; x < m_playerDecks.Size(); ++x)
	{
		RemoveFromDeckIfInvalid(m_playerDecks[x], cardIndex, numCopiesLeft);
	}
}

bool CR4GwintManager::HasCardInCollection(Int32 cardIndex)
{
	TDynArray<CollectionCard>* listReference;

	if (cardIndex >= 1000)
	{
		listReference = &m_playerLeaderCollection;
	}
	else
	{
		listReference = &m_playerCollection;
	}

	for (Uint32 i = 0; i < listReference->Size(); ++i)
	{
		if ((*listReference)[i].m_cardID == cardIndex)
		{
			return true;
		}
	}

	return false;
}

void CR4GwintManager::RemoveFromDeckIfInvalid(SDeckDefinition& deckRef, Int32 cardIndex, Int32 numCopies)
{
	Uint32 currentIndex = 0;
	Int32 numCopiesFound = 0;

	while (currentIndex < deckRef.m_cardIndices.Size())
	{
		if (deckRef.m_cardIndices[currentIndex] == cardIndex)
		{
			++numCopiesFound;

			if (numCopiesFound > numCopies)
			{
				deckRef.m_cardIndices.RemoveAt(currentIndex);
				continue;
			}
		}

		++currentIndex;
	}
}

void CR4GwintManager::AddCardToDeck(eGwintFaction factionIndex, Int32 cardIndex)
{
	if (factionIndex > GwintFaction_Neutral && factionIndex < GwintFaction_Max)
	{
		Int32 index = factionIndex - 1;

		if ((Int32)(m_playerDecks.Size()) > index)
		{
			m_playerDecks[index].m_cardIndices.PushBack(cardIndex);
		}
	}
}

void CR4GwintManager::RemoveCardFromDeck(eGwintFaction factionIndex, Int32 cardIndex)
{
	if (factionIndex > GwintFaction_Neutral && factionIndex < GwintFaction_Max)
	{
		Int32 index = factionIndex - 1;

		if ((Int32)(m_playerDecks.Size()) > index)
		{
			for (Uint32 i = 0; i < m_playerDecks[index].m_cardIndices.Size(); ++i)
			{
				if (m_playerDecks[index].m_cardIndices[i] == cardIndex)
				{
					m_playerDecks[index].m_cardIndices.RemoveAt(i);
					break;
				}
			}
		}
	}
}

void CR4GwintManager::funcGetCardDefs( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if( result )
	{
		TDynArray< SCardDefinition > & resultArr = *(TDynArray< SCardDefinition >*) result;
		resultArr = m_cardDefs;
	}
}

void CR4GwintManager::funcGetLeaderDefs( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if( result )
	{
		TDynArray< SCardDefinition > & resultArr = *(TDynArray< SCardDefinition >*) result;
		resultArr = m_kingDefs;
	}
}

void CR4GwintManager::funcGetFactionDeck( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( eGwintFaction, factionIndex, GwintFaction_NothernKingdom );
	GET_PARAMETER_REF( SDeckDefinition, data, SDeckDefinition() );
	FINISH_PARAMETERS;

	Bool foundDeck = false;
	
	if (factionIndex > GwintFaction_Neutral && factionIndex < GwintFaction_Max)
	{
		Int32 index = factionIndex - 1;

		if ((Int32)(m_playerDecks.Size()) > index)
		{
			data = m_playerDecks[index];
			foundDeck = true;
		}
	}

	RETURN_BOOL(foundDeck);
}


void CR4GwintManager::funcSetFactionDeck( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( eGwintFaction, factionIndex, GwintFaction_Neutral );
	GET_PARAMETER( SDeckDefinition, deck, SDeckDefinition() );
	FINISH_PARAMETERS;

	if (factionIndex > GwintFaction_Neutral && factionIndex < GwintFaction_Max)
	{
		Int32 index = factionIndex - 1;

		if ((Int32)(m_playerDecks.Size()) > index)
		{
			m_playerDecks[index] = deck;
		}
	}
}

void CR4GwintManager::funcGetPlayerCollection( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if( result )
	{
		TDynArray< CollectionCard > & resultArr = *(TDynArray< CollectionCard >*) result;
		resultArr = m_playerCollection;
	}
}

void CR4GwintManager::funcGetPlayerLeaderCollection( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if( result )
	{
		TDynArray< CollectionCard > & resultArr = *(TDynArray< CollectionCard >*) result;
		resultArr = m_playerLeaderCollection;
	}
}

void CR4GwintManager::funcGetSelectedPlayerDeck( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( result ) *(eGwintFaction*)result = m_selectedDeckIndex;
}

void CR4GwintManager::funcSetSelectedPlayerDeck( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(eGwintFaction, targetFaction, GwintFaction_Neutral);
	FINISH_PARAMETERS;

	m_selectedDeckIndex = targetFaction;
}

void CR4GwintManager::funcUnlockDeck( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(eGwintFaction, factionIndex, GwintFaction_Neutral);
	FINISH_PARAMETERS;

	if (factionIndex > GwintFaction_Neutral && factionIndex < GwintFaction_Max)
	{
		Int32 index = factionIndex - 1;

		if ((Int32)(m_playerDecks.Size()) > index)
		{
			SDeckDefinition& deck = m_playerDecks[index];

			if (deck.m_unlocked)
			{
				return;
			}

			m_playerDecks[index].m_unlocked = true;

			Uint32 numCards = deck.m_cardIndices.Size();
			for (Uint32 i = 0; i < numCards; ++i)
			{
				AddCardToCollection(deck.m_cardIndices[i]);
			}

			AddCardToCollection(deck.m_leaderIndex);
		}
	}
}

void CR4GwintManager::funcIsDeckUnlocked( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(eGwintFaction, factionIndex, GwintFaction_Neutral);
	FINISH_PARAMETERS;

	Bool returnVal = false;

	if (factionIndex > GwintFaction_Neutral && factionIndex < GwintFaction_Max)
	{
		Int32 index = factionIndex - 1;

		if ((Int32)(m_playerDecks.Size()) > index)
		{
			returnVal = m_playerDecks[index].m_unlocked;
		}
	}

	RETURN_BOOL(returnVal);
}

void CR4GwintManager::funcAddCardToCollection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(Int32, cardIndex, -1);
	FINISH_PARAMETERS;

	AddCardToCollection(cardIndex);
}

void CR4GwintManager::funcRemoveCardFromCollection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(Int32, cardIndex, -1);
	FINISH_PARAMETERS;

	RemoveCardFromCollection(cardIndex);
}

void CR4GwintManager::funcHasCardInCollection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(Int32, cardIndex, -1);
	FINISH_PARAMETERS;

	RETURN_BOOL(HasCardInCollection(cardIndex));
}

void CR4GwintManager::funcHasCardsOfFactionInCollection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(Int32, faction, -1);
	GET_PARAMETER_OPT(Bool, includeLeaders, false);
	FINISH_PARAMETERS;

	for( const auto& card : m_playerCollection )
	{
		for( const auto& cardDef : m_cardDefs )
		{
			if( cardDef.m_index == card.m_cardID )
			{
				if( cardDef.m_faction == faction )
				{
					RETURN_BOOL( true );
					return;
				}
				break;
			}
		}
	}

	if( includeLeaders )
	{
		for( const auto& card : m_playerLeaderCollection )
		{
			for( const auto& cardDef : m_kingDefs )
			{
				if( cardDef.m_index == card.m_cardID )
				{
					if( cardDef.m_faction == faction )
					{
						RETURN_BOOL( true );
						return;
					}
					break;
				}
			}
		}
	}

	RETURN_BOOL( false );
}

void CR4GwintManager::funcAddCardToDeck( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(eGwintFaction, factionIndex, GwintFaction_Neutral);
	GET_PARAMETER(Int32, cardIndex, -1);
	FINISH_PARAMETERS;

	AddCardToDeck(factionIndex, cardIndex);
}

void CR4GwintManager::funcRemoveCardFromDeck( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(eGwintFaction, factionIndex, GwintFaction_Neutral);
	GET_PARAMETER(Int32, cardIndex, -1);
	FINISH_PARAMETERS;

	RemoveCardFromDeck(factionIndex, cardIndex);
}

void CR4GwintManager::funcGetHasDoneTutorial( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL(m_hasDoneTutorial);
}

void CR4GwintManager::funcSetHasDoneTutorial( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(Bool, hasDoneTutorial, false);
	FINISH_PARAMETERS;

	m_hasDoneTutorial = hasDoneTutorial;
}

void CR4GwintManager::funcGetHasDoneDeckTutorial( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL(m_hasDoneDeckTutorial);
}

void CR4GwintManager::funcSetHasDoneDeckTutorial( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER(Bool, hasDoneTutorial, false);
	FINISH_PARAMETERS;

	m_hasDoneDeckTutorial = hasDoneTutorial;
}