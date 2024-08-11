#pragma once

#include "../../common/game/journalBase.h"
#include "../../common/game/journalPath.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalQuestRoot : public CJournalBase
{
public:
	DECLARE_ENGINE_CLASS( CJournalQuestRoot, CJournalBase, 0 )

	CJournalQuestRoot();
	virtual ~CJournalQuestRoot();

	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalQuestRoot )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI();

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalQuestGroup : public CJournalBase
{
	DECLARE_ENGINE_CLASS( CJournalQuestGroup, CJournalBase, 0 )

public:
	CJournalQuestGroup();
	virtual ~CJournalQuestGroup();
	virtual Bool IsParentClass( CJournalBase* other ) const;

	RED_INLINE Uint32 GetTitleIndex() const { return m_title.GetIndex(); }

private:
	LocalizedString m_title;

private:
	void funcGetTitleStringId( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CJournalQuestGroup )
	PARENT_CLASS( CJournalBase )
	PROPERTY_EDIT( m_title, TXT( "Title" ) )
	
	NATIVE_FUNCTION( "GetTitleStringId", funcGetTitleStringId )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

enum eQuestType
{
	QuestType_Story = 0,
	QuestType_Chapter,
	QuestType_Side,
	QuestType_MonsterHunt,
	QuestType_TreasureHunt,

	QuestType_Max
};

BEGIN_ENUM_RTTI( eQuestType )
	ENUM_OPTION_DESC( TXT( "Story" ), QuestType_Story )
	ENUM_OPTION_DESC( TXT( "Chapter" ), QuestType_Chapter )
	ENUM_OPTION_DESC( TXT( "Side" ), QuestType_Side )
	ENUM_OPTION_DESC( TXT( "MonsterHunt" ), QuestType_MonsterHunt )
	ENUM_OPTION_DESC( TXT( "TreasureHunt" ), QuestType_TreasureHunt )
END_ENUM_RTTI();

enum EJournalContentType
{
	EJCT_Vanilla,
	EJCT_EP1,
	EJCT_EP2,
	EJCT_Count,
};

BEGIN_ENUM_RTTI( EJournalContentType )
	ENUM_OPTION( EJCT_Vanilla )
	ENUM_OPTION( EJCT_EP1 )
	ENUM_OPTION( EJCT_EP2 )
END_ENUM_RTTI();

class CJournalQuest : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalQuest, CJournalContainer, 0 )

public:
	CJournalQuest();
	virtual ~CJournalQuest();

	virtual Bool IsParentClass( CJournalBase* other ) const;

	RED_INLINE eQuestType GetType() const { return m_type; }
	RED_INLINE EJournalContentType GetContentType() const { return m_contentType; }
	RED_INLINE String GetTitle() const { return m_title.GetString(); }
	RED_INLINE Uint32 GetTitleIndex() const { return m_title.GetIndex(); }
	RED_INLINE Uint32 GetWorld() const  { return m_world; }
	RED_INLINE const String& GetQuestPhasePath() const { return m_questPhase.GetPath(); }

	Bool IsMonsterHuntQuest() const { return GetType() == QuestType_MonsterHunt; }
	THandle< CJournalPath > GetHuntingQuestCreaturePath() const { return m_huntingQuestPath; }
	const CJournalCreature* GetHuntingQuestCreature();

#ifdef FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS
	virtual String GetFriendlyName() const override;
#endif //FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS

protected:
	virtual void DefaultValues();

private:
	eQuestType m_type;
	EJournalContentType m_contentType;
	Uint32 m_world;
	THandle< CJournalPath > m_huntingQuestPath;
	LocalizedString m_title;
	TSoftHandle< CQuestPhase > m_questPhase;

private:
	void funcGetTitleStringId( CScriptStackFrame& stack, void* result );
	void funcGetType( CScriptStackFrame& stack, void* result );
	void funcGetContentType( CScriptStackFrame& stack, void* result );
	void funcGetWorld( CScriptStackFrame& stack, void* result );
	void funcGetHuntingQuestCreatureTag( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CJournalQuest )
	PARENT_CLASS( CJournalContainer )
	PROPERTY_EDIT( m_type, TXT( "Type" ) )
	PROPERTY_EDIT( m_contentType, TXT( "Content type" ) )
	PROPERTY_CUSTOM_EDIT( m_world, TXT( "World" ), TXT("WorldSelection_Quest") )
	PROPERTY_CUSTOM_EDIT( m_huntingQuestPath, TXT( "Hunt Creature" ), TXT( "JournalPropertyBrowserCreature" ) )
	PROPERTY_EDIT( m_title, TXT( "Title" ) )
	PROPERTY_EDIT( m_questPhase, TXT( "Quest phase" ) )

	NATIVE_FUNCTION( "GetTitleStringId", funcGetTitleStringId )
	NATIVE_FUNCTION( "GetType", funcGetType )
	NATIVE_FUNCTION( "GetContentType", funcGetContentType )
	NATIVE_FUNCTION( "GetWorld", funcGetWorld )
	NATIVE_FUNCTION( "GetHuntingQuestCreatureTag", funcGetHuntingQuestCreatureTag )
END_CLASS_RTTI();

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalQuestPhase : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalQuestPhase, CJournalContainer, 0 )

public:
	CJournalQuestPhase();
	virtual ~CJournalQuestPhase();

	virtual Bool IsParentClass( CJournalBase* other ) const;
};

BEGIN_CLASS_RTTI( CJournalQuestPhase )
	PARENT_CLASS( CJournalContainer )
END_CLASS_RTTI();


class CJournalQuestDescriptionGroup : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalQuestDescriptionGroup, CJournalContainer, 0 )

public:
	CJournalQuestDescriptionGroup()
	{}
	virtual ~CJournalQuestDescriptionGroup()
	{}

	virtual Bool IsParentClass( CJournalBase* other ) const;
	void DefaultValues();
};

BEGIN_CLASS_RTTI( CJournalQuestDescriptionGroup )
	PARENT_CLASS( CJournalContainer )
END_CLASS_RTTI();

class CJournalQuestDescriptionEntry : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalQuestDescriptionEntry, CJournalContainerEntry, 0 )

public:
	CJournalQuestDescriptionEntry()
	{}
	virtual ~CJournalQuestDescriptionEntry()
	{}
	
	RED_INLINE Uint32 GetDescriptionIndex() const { return m_description.GetIndex(); }

	virtual Bool IsParentClass( CJournalBase* other ) const;
	void DefaultValues();
	void funcGetDescriptionStringId( CScriptStackFrame& stack, void* result );
	LocalizedString m_description;
};

BEGIN_CLASS_RTTI( CJournalQuestDescriptionEntry )
	PARENT_CLASS( CJournalContainerEntry )
	NATIVE_FUNCTION( "GetDescriptionStringId", funcGetDescriptionStringId )
	PROPERTY_EDIT( m_description, TXT( "Description" ) )
	PROPERTY_EDIT_NOSERIALIZE( m_active, TXT( "In journal at beginning" ) )
END_CLASS_RTTI();





//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

enum eQuestObjectiveType
{
	Type_Manual = 0,
	Type_KillCount,
	Type_InventoryCount,
	Type_HuntingList,

	Type_Max
};

BEGIN_ENUM_RTTI( eQuestObjectiveType )
	ENUM_OPTION_DESC( TXT( "Manual" ), Type_Manual )
	ENUM_OPTION_DESC( TXT( "Kill count" ), Type_KillCount )
	ENUM_OPTION_DESC( TXT( "Inventory Count" ), Type_InventoryCount )
	ENUM_OPTION_DESC( TXT( "Hunting List" ), Type_HuntingList )
END_ENUM_RTTI();

class CJournalQuestObjective : public CJournalContainer
{
	DECLARE_ENGINE_CLASS( CJournalQuestObjective, CJournalContainer, 0 )

public:

	CJournalQuestObjective();
	virtual ~CJournalQuestObjective();

	virtual Bool IsParentClass( CJournalBase* other ) const;

	RED_INLINE Uint32 GetTitleIndex() const { return m_title.GetIndex(); }
	RED_INLINE String GetTitle() const { return m_title.GetString(); }
	RED_INLINE const String& GetImage() const { return m_image; }
	Uint32 GetWorld() const;
	RED_INLINE Uint32 GetCounterType() const { return m_counterType; }
	RED_INLINE Uint32 GetCount() const { return m_count; }

	RED_INLINE const CJournalQuest* GetParentQuest() const
	{
		const CJournalQuestPhase* phase	= GetParentAs< CJournalQuestPhase >();
		const CJournalQuest* quest		= phase->GetParentAs< CJournalQuest >();

		return quest;
	}

	RED_INLINE CJournalQuest* GetParentQuest()
	{
		return const_cast< CJournalQuest* >( static_cast< const CJournalQuestObjective* >( this )->GetParentQuest() );
	}

	//
	// !!! this function is called only when duplicating entry !!!
	// !!! do not use it for any other purposes !!!
	//
	RED_INLINE void Reset()
	{
		RecreateGUID();
		m_title.SetIndex( 0 );
	}

#ifdef FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS
	virtual String GetFriendlyName() const override;
#endif //FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS

protected:
	void DefaultValues();

private:
	LocalizedString m_title;
	String m_image;
	Uint32 m_world;
	eQuestObjectiveType m_counterType;
	Uint32 m_count;
	CName m_bookShortcut;
	CName m_itemShortcut;
	CName m_recipeShortcut;
	THandle< CJournalPath > m_monsterShortcut;
	Bool m_mutuallyExclusive;

private:
	void funcGetTitleStringId( CScriptStackFrame& stack, void* result );
	void funcGetTitle( CScriptStackFrame& stack, void* result );
	void funcGetWorld( CScriptStackFrame& stack, void* result );
	void funcGetCount( CScriptStackFrame& stack, void* result );
	void funcGetCounterType( CScriptStackFrame& stack, void* result );
	void funcIsMutuallyExclusive( CScriptStackFrame& stack, void* result );
	void funcGetBookShortcut( CScriptStackFrame& stack, void* result );
	void funcGetItemShortcut( CScriptStackFrame& stack, void* result );
	void funcGetRecipeShortcut( CScriptStackFrame& stack, void* result );
	void funcGetMonsterShortcut( CScriptStackFrame& stack, void* result );
	void funcGetParentQuest( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CJournalQuestObjective )
	PARENT_CLASS( CJournalContainer )
	PROPERTY_EDIT( m_title, TXT( "Description" ) )
	PROPERTY_EDIT( m_image, TXT( "Image" ) )
	PROPERTY_CUSTOM_EDIT( m_world, TXT( "World" ), TXT( "WorldSelection_Objective" ) )
	PROPERTY_EDIT( m_counterType, TXT( "Counter Type" ) )
	PROPERTY_EDIT( m_count, TXT( "Count" ) )
	PROPERTY_EDIT( m_mutuallyExclusive, TXT( "Mutually exclusive" ) )
	PROPERTY_CUSTOM_EDIT( m_bookShortcut, TXT( "Shortcut - book name" ), TXT( "ChooseItem" ) )
	PROPERTY_CUSTOM_EDIT( m_itemShortcut, TXT( "Shortcut - item name" ), TXT( "ChooseItem" ) )
	PROPERTY_CUSTOM_EDIT( m_recipeShortcut, TXT( "Shortcut - recipe name" ), TXT( "ChooseItem" ) )
	PROPERTY_CUSTOM_EDIT( m_monsterShortcut, TXT( "Shortcut - monster path" ), TXT( "JournalPropertyBrowserCreature" ) )

	NATIVE_FUNCTION( "GetTitleStringId", funcGetTitleStringId )
	NATIVE_FUNCTION( "GetTitle", funcGetTitle )
	NATIVE_FUNCTION( "GetWorld", funcGetWorld )
	NATIVE_FUNCTION( "GetCount", funcGetCount )
	NATIVE_FUNCTION( "IsMutuallyExclusive", funcIsMutuallyExclusive )
	NATIVE_FUNCTION( "GetCounterType", funcGetCounterType )
	NATIVE_FUNCTION( "GetBookShortcut", funcGetBookShortcut )
	NATIVE_FUNCTION( "GetItemShortcut", funcGetItemShortcut )
	NATIVE_FUNCTION( "GetRecipeShortcut", funcGetRecipeShortcut )
	NATIVE_FUNCTION( "GetMonsterShortcut", funcGetMonsterShortcut )
	NATIVE_FUNCTION( "GetParentQuest", funcGetParentQuest )
END_CLASS_RTTI();

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalQuestItemTag : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalQuestItemTag, CJournalContainerEntry, 0 )

public:

	CJournalQuestItemTag();
	virtual ~CJournalQuestItemTag();

	RED_INLINE const CName& GetItem() const { return m_item; }

	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	CName m_item;
};

BEGIN_CLASS_RTTI( CJournalQuestItemTag )
	PARENT_CLASS( CJournalContainerEntry )
	PROPERTY_CUSTOM_EDIT( m_item, TXT( "Item" ), TXT( "ChooseItem" ) )
END_CLASS_RTTI();

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalQuestEnemyTag : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalQuestEnemyTag, CJournalContainerEntry, 0 )

public:

	CJournalQuestEnemyTag();
	virtual ~CJournalQuestEnemyTag();

	RED_INLINE const String& GetTag() const { return m_tag; }

	virtual Bool IsParentClass( CJournalBase* other ) const;

private:
	String m_tag;
};

BEGIN_CLASS_RTTI( CJournalQuestEnemyTag )
	PARENT_CLASS( CJournalContainerEntry )
	PROPERTY_EDIT( m_tag, TXT( "Enemy Tag" ) )
END_CLASS_RTTI();

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

enum EJournalMapPinType
{
	EJMPT_Default,
	EJMPT_QuestReturn,
	EJMPT_HorseRace,
	EJMPT_BoatRace,
	EJMPT_QuestBelgard,
	EJMPT_QuestCoronata,
	EJMPT_QuestVermentino,
};

BEGIN_ENUM_RTTI( EJournalMapPinType )
	ENUM_OPTION( EJMPT_Default )
	ENUM_OPTION( EJMPT_QuestReturn )
	ENUM_OPTION( EJMPT_HorseRace )
	ENUM_OPTION( EJMPT_BoatRace )
	ENUM_OPTION( EJMPT_QuestBelgard )
	ENUM_OPTION( EJMPT_QuestCoronata )
	ENUM_OPTION( EJMPT_QuestVermentino )
END_ENUM_RTTI();

class CJournalQuestMapPin : public CJournalContainerEntry
{
	DECLARE_ENGINE_CLASS( CJournalQuestMapPin, CJournalContainerEntry, 0 )

public:

	CJournalQuestMapPin();
	virtual ~CJournalQuestMapPin();

	virtual Bool IsParentClass( CJournalBase* other ) const;

public:
	RED_INLINE const CName& GetMapPinID() const { return m_mapPinID; }
	RED_INLINE const Float  GetRadius() const { return m_radius; }
	RED_INLINE EJournalMapPinType GetType() const { return m_type; }
    RED_INLINE Bool IsEnabledAtStartup() const { return m_enabledAtStartup; }

	void funcGetMapPinID( CScriptStackFrame& stack, void* result );
	void funcGetRadius( CScriptStackFrame& stack, void* result );

#ifdef FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS
	virtual String GetFriendlyName() const override;
#endif //FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS

private:
	Float m_radius;
	CName m_mapPinID;
	EJournalMapPinType m_type;
    Bool  m_enabledAtStartup;
};

BEGIN_CLASS_RTTI( CJournalQuestMapPin )
	PARENT_CLASS( CJournalContainerEntry )
	NATIVE_FUNCTION( "GetMapPinID", funcGetMapPinID )
	NATIVE_FUNCTION( "GetRadius", funcGetRadius )

	PROPERTY_EDIT( m_radius , TXT("Radius of minimap pin.") )
	PROPERTY_CUSTOM_EDIT( m_mapPinID, TXT( "Map Pin ID" ), TXT( "MapPin" ) )
	PROPERTY_EDIT( m_type , TXT("Type of map pin") )
	PROPERTY_EDIT( m_enabledAtStartup, TXT( "Enabled" ) )
END_CLASS_RTTI();
