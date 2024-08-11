#pragma once

//---------------------------------------------------------------------------------
enum eGwintFaction
{
	GwintFaction_Neutral = 0,
	GwintFaction_NoMansLand,
	GwintFaction_Nilfgaard,
	GwintFaction_NothernKingdom,
	GwintFaction_Scoiatael,
	GwintFaction_Skellige,

	GwintFaction_Max
};

BEGIN_ENUM_RTTI( eGwintFaction )
	ENUM_OPTION( GwintFaction_Neutral )
	ENUM_OPTION( GwintFaction_NoMansLand )
	ENUM_OPTION( GwintFaction_Nilfgaard )
	ENUM_OPTION( GwintFaction_NothernKingdom )
	ENUM_OPTION( GwintFaction_Scoiatael )
	ENUM_OPTION( GwintFaction_Skellige )
END_ENUM_RTTI();

//---------------------------------------------------------------------------------
enum eGwintType
{
	GwintType_None = 0,
	GwintType_Melee = 1,
	GwintType_Ranged = 2,
	GwintType_Siege = 4,
	GwintType_Creature = 8,
	GwintType_Weather = 16,
	GwintType_Spell = 32,
	GwintType_RowModifier = 64,
	GwintType_Hero = 128,
	GwintType_Spy = 256,
	GwintType_FriendlyEffect = 512,
	GwintType_OffensiveEffect = 1024,
	GwintType_GlobalEffect = 2048
};

BEGIN_ENUM_RTTI( eGwintType )
	ENUM_OPTION( GwintType_None )
	ENUM_OPTION( GwintType_Melee )
	ENUM_OPTION( GwintType_Ranged )
	ENUM_OPTION( GwintType_Siege )
	ENUM_OPTION( GwintType_Creature )
	ENUM_OPTION( GwintType_Weather )
	ENUM_OPTION( GwintType_Spell )
	ENUM_OPTION( GwintType_RowModifier )
	ENUM_OPTION( GwintType_Hero )
	ENUM_OPTION( GwintType_Spy )
	ENUM_OPTION( GwintType_FriendlyEffect )
	ENUM_OPTION( GwintType_OffensiveEffect )
	ENUM_OPTION( GwintType_GlobalEffect )
END_ENUM_RTTI();

//---------------------------------------------------------------------------------
// #J The following effect flag ids must match the actionscript. 
// Some are deprecated and removed at the time of this refactor (ws to c++) (explaining # gaps)
enum eGwintEffect
{
	GwintEffect_None = 0,

	GwintEffect_Bin2 = 5,

	// Leader abilities
	GwintEffect_MeleeScorch = 7,
	GwintEffect_11thCard = 8,
	GwintEffect_ClearWeather = 9,
	GwintEffect_PickWeatherCard = 10,
	GwintEffect_PickRainCard = 11,
	GwintEffect_PickFogCard = 12,
	GwintEffect_PickFrostCard = 13,
	GwintEffect_View3EnemyCard = 14,
	GwintEffect_ResurectCard = 15,
	GwintEffect_ResurectFromEnemy = 16,
	GwintEffect_Bin2Pick1 = 17,
	GwintEffect_MeleeHorn = 18,
	GwintEffect_RangedHorn = 19,
	GwintEffect_SiegeHorn = 20,
	GwintEffect_SiegeScorch = 21,
	GwintEffect_CounterKingAbility = 22,

	// Regular Effects
	GwintEffect_Melee = 23,
	GwintEffect_Ranged = 24,
	GwintEffect_Siege = 25,
	GwintEffect_UnsummonDummy = 26,
	GwintEffect_Horn = 27,
	GwintEffect_Draw = 28,
	GwintEffect_Scorch = 29,
	GwintEffect_ClearSky = 30,
	GwintEffect_SummonClones = 31,
	GwintEffect_ImproveNeightbours = 32,
	GwintEffect_Nurse = 33,
	GwintEffect_Draw2 = 34,
	GwintEffect_SameTypeMorale = 35,

	// Episode 1 abilities
	GwintEffect_AgileReposition = 36,
	GwintEffect_RandomRessurect = 37,
	GwintEffect_DoubleSpy = 38,
	GwintEffect_RangedScorch = 39,
	GwintEffect_SuicideSummon = 40,

	// Episode 2 abilities
	GwintEffect_Mushroom = 41,
	GwintEffect_Morph = 42,
	GwintEffect_WeatherResistant = 43,
	GwintEffect_GraveyardShuffle = 44
};

BEGIN_ENUM_RTTI( eGwintEffect )
	ENUM_OPTION( GwintEffect_None )
	ENUM_OPTION( GwintEffect_Bin2 )
	ENUM_OPTION( GwintEffect_MeleeScorch )
	ENUM_OPTION( GwintEffect_11thCard )
	ENUM_OPTION( GwintEffect_ClearWeather )
	ENUM_OPTION( GwintEffect_PickWeatherCard )
	ENUM_OPTION( GwintEffect_PickRainCard )
	ENUM_OPTION( GwintEffect_PickFogCard )
	ENUM_OPTION( GwintEffect_PickFrostCard )
	ENUM_OPTION( GwintEffect_View3EnemyCard )
	ENUM_OPTION( GwintEffect_ResurectCard )
	ENUM_OPTION( GwintEffect_ResurectFromEnemy )
	ENUM_OPTION( GwintEffect_Bin2Pick1 )
	ENUM_OPTION( GwintEffect_MeleeHorn )
	ENUM_OPTION( GwintEffect_RangedHorn )
	ENUM_OPTION( GwintEffect_SiegeHorn )
	ENUM_OPTION( GwintEffect_SiegeScorch )
	ENUM_OPTION( GwintEffect_CounterKingAbility )
	ENUM_OPTION( GwintEffect_Melee )
	ENUM_OPTION( GwintEffect_Ranged )
	ENUM_OPTION( GwintEffect_Siege )
	ENUM_OPTION( GwintEffect_UnsummonDummy )
	ENUM_OPTION( GwintEffect_Horn )
	ENUM_OPTION( GwintEffect_Draw )
	ENUM_OPTION( GwintEffect_Scorch )
	ENUM_OPTION( GwintEffect_ClearSky )
	ENUM_OPTION( GwintEffect_SummonClones )
	ENUM_OPTION( GwintEffect_ImproveNeightbours )
	ENUM_OPTION( GwintEffect_Nurse )
	ENUM_OPTION( GwintEffect_Draw2 )
	ENUM_OPTION( GwintEffect_SameTypeMorale )
	ENUM_OPTION( GwintEffect_Mushroom )
	ENUM_OPTION( GwintEffect_Morph )
	ENUM_OPTION( GwintEffect_WeatherResistant )
	ENUM_OPTION( GwintEffect_GraveyardShuffle )
END_ENUM_RTTI();

//---------------------------------------------------------------------------------
struct SCardDefinition
{
	DECLARE_RTTI_STRUCT( SCardDefinition );

	Int32					m_index;
	String					m_title;
	String					m_description;
	Int32					m_power;
	String					m_picture;
	eGwintFaction			m_faction;
	Int32					m_typeFlags;
	TDynArray<eGwintEffect>	m_effectFlags;
	TDynArray<Int32>		m_summonFlags;

	CName					m_dlcPictureFlag;
	String					m_dlcPicture;
};

BEGIN_CLASS_RTTI( SCardDefinition )
	PROPERTY( m_index )
	PROPERTY( m_title )
	PROPERTY( m_description )
	PROPERTY( m_power )
	PROPERTY( m_picture )
	PROPERTY( m_faction )
	PROPERTY( m_typeFlags )
	PROPERTY( m_effectFlags )
	PROPERTY( m_summonFlags )

	PROPERTY( m_dlcPictureFlag )
	PROPERTY( m_dlcPicture )
END_CLASS_RTTI();

//---------------------------------------------------------------------------------
struct SDeckDefinition
{
	DECLARE_RTTI_STRUCT( SDeckDefinition );

	TDynArray<Int32>	m_cardIndices;
	Int32				m_leaderIndex;
	Bool				m_unlocked;
	Int32				m_specialCard;
	TDynArray<Int32>	m_dynamicCardRequirements;
	TDynArray<Int32>	m_dynamicCards;
};

BEGIN_CLASS_RTTI( SDeckDefinition )
	PROPERTY( m_cardIndices )
	PROPERTY( m_leaderIndex )
	PROPERTY( m_unlocked )
	PROPERTY( m_specialCard )
	PROPERTY( m_dynamicCardRequirements )
	PROPERTY( m_dynamicCards )
END_CLASS_RTTI();

//---------------------------------------------------------------------------------
struct CollectionCard
{
	DECLARE_RTTI_STRUCT( CollectionCard );

	Int32	m_cardID;
	Int32	m_numCopies;
};

BEGIN_CLASS_RTTI( CollectionCard )
	PROPERTY( m_cardID )
	PROPERTY( m_numCopies )
END_CLASS_RTTI();

//---------------------------------------------------------------------------------
class CR4GwintManager : public IGameSystem, public IGameSaveSection
{
public:
	DECLARE_ENGINE_CLASS( CR4GwintManager, IGameSystem, 0);

public:
	CR4GwintManager();
	~CR4GwintManager();

	virtual void Initialize();
	virtual void Shutdown();

	virtual void OnGameStart( const CGameInfo& gameInfo );
	virtual bool OnSaveGame( IGameSaver* saver );

private:
	void OnLoadGame( IGameLoader* loader );
	void InitializeDecks();
	void InitializeSkellige();

	void LoadCardXMLs();
	void LoadCardsFromXML( CName xmlName, TDynArray<SCardDefinition>& targetList );

	eGwintFaction FactionFromString(const String& value);
	eGwintType TypeFromString(const String& value);
	eGwintEffect EffectFromString(const String& value);

	void AddCardToCollection(Int32 cardIndex);
	void RemoveCardFromCollection(Int32 cardIndex);
	bool HasCardInCollection(Int32 cardIndex);

	void RemoveFromDeckIfInvalid(SDeckDefinition& deckRef, Int32 cardIndex, Int32 numCopies);

	void AddCardToDeck(eGwintFaction factionIndex, Int32 cardIndex);
	void RemoveCardFromDeck(eGwintFaction factionIndex, Int32 cardIndex);

private:
	void funcGetCardDefs( CScriptStackFrame& stack, void* result );
	void funcGetLeaderDefs( CScriptStackFrame& stack, void* result );
	void funcGetFactionDeck( CScriptStackFrame& stack, void* result );
	void funcSetFactionDeck( CScriptStackFrame& stack, void* result );
	void funcGetPlayerCollection( CScriptStackFrame& stack, void* result );
	void funcGetPlayerLeaderCollection( CScriptStackFrame& stack, void* result );
	void funcGetSelectedPlayerDeck( CScriptStackFrame& stack, void* result );
	void funcSetSelectedPlayerDeck( CScriptStackFrame& stack, void* result );
	void funcUnlockDeck( CScriptStackFrame& stack, void* result );
	void funcIsDeckUnlocked( CScriptStackFrame& stack, void* result );
	void funcAddCardToCollection( CScriptStackFrame& stack, void* result );
	void funcRemoveCardFromCollection( CScriptStackFrame& stack, void* result );
	void funcHasCardInCollection( CScriptStackFrame& stack, void* result );
	void funcHasCardsOfFactionInCollection( CScriptStackFrame& stack, void* result );
	void funcAddCardToDeck( CScriptStackFrame& stack, void* result );
	void funcRemoveCardFromDeck( CScriptStackFrame& stack, void* result );
	void funcGetHasDoneTutorial( CScriptStackFrame& stack, void* result );
	void funcSetHasDoneTutorial( CScriptStackFrame& stack, void* result );
	void funcGetHasDoneDeckTutorial( CScriptStackFrame& stack, void* result );
	void funcSetHasDoneDeckTutorial( CScriptStackFrame& stack, void* result );

private:
	TDynArray<SCardDefinition>		m_cardDefs;
	TDynArray<SCardDefinition>		m_kingDefs;

	TDynArray<SDeckDefinition>		m_playerDecks;
	eGwintFaction					m_selectedDeckIndex;

	TDynArray<CollectionCard>		m_playerCollection;
	TDynArray<CollectionCard>		m_playerLeaderCollection;

	Bool							m_hasDoneTutorial;
	Bool							m_hasDoneDeckTutorial;

	ASSIGN_GAME_SYSTEM_ID( GS_Gwint );
};

BEGIN_CLASS_RTTI( CR4GwintManager )
	PARENT_CLASS( IGameSystem )
	NATIVE_FUNCTION( "GetCardDefs", funcGetCardDefs );
	NATIVE_FUNCTION( "GetLeaderDefs", funcGetLeaderDefs );
	NATIVE_FUNCTION( "GetFactionDeck", funcGetFactionDeck );
	NATIVE_FUNCTION( "SetFactionDeck", funcSetFactionDeck );
	NATIVE_FUNCTION( "GetPlayerCollection", funcGetPlayerCollection );
	NATIVE_FUNCTION( "GetPlayerLeaderCollection", funcGetPlayerLeaderCollection );
	NATIVE_FUNCTION( "GetSelectedPlayerDeck", funcGetSelectedPlayerDeck );
	NATIVE_FUNCTION( "SetSelectedPlayerDeck", funcSetSelectedPlayerDeck );
	NATIVE_FUNCTION( "UnlockDeck", funcUnlockDeck );
	NATIVE_FUNCTION( "IsDeckUnlocked", funcIsDeckUnlocked );
	NATIVE_FUNCTION( "AddCardToCollection", funcAddCardToCollection );
	NATIVE_FUNCTION( "RemoveCardFromCollection", funcRemoveCardFromCollection );
	NATIVE_FUNCTION( "HasCardInCollection", funcHasCardInCollection );
	NATIVE_FUNCTION( "HasCardsOfFactionInCollection", funcHasCardsOfFactionInCollection );
	NATIVE_FUNCTION( "AddCardToDeck", funcAddCardToDeck );
	NATIVE_FUNCTION( "RemoveCardFromDeck", funcRemoveCardFromDeck );
	NATIVE_FUNCTION( "GetHasDoneTutorial", funcGetHasDoneTutorial );
	NATIVE_FUNCTION( "SetHasDoneTutorial", funcSetHasDoneTutorial );
	NATIVE_FUNCTION( "GetHasDoneDeckTutorial", funcGetHasDoneDeckTutorial );
	NATIVE_FUNCTION( "SetHasDoneDeckTutorial", funcSetHasDoneDeckTutorial );
END_CLASS_RTTI()