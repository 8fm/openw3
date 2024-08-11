/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#if !defined( NO_SECOND_SCREEN )

//#define SECOND_SCREEN_PROFILE_CODE

#include "../../common/core/jsonArray.h"
#include "../../common/core/jsonWriter.h"

#include "secondScreenDevice.h"
#include "secondScreenDeviceWatcher.h"


//! Fast travel pin point for second screen
struct  SSecondScreenMapPin
{
	Int32   m_id;
	CName	m_tag;
	CName   m_type;
	Vector	m_position;
	Bool	m_isDiscovered;
};

class ISecondScreenManagerDelegate
{
public:
	virtual Bool IsGUIActive() = 0;
	virtual void OnHandleFastTravel( const CName mapPinTag, Uint32 areaType, Bool currentWorld ) = 0;
	virtual void OnHandleTrackQuest( const CGUID questQuid ) = 0;
};

//! This class serves as access point to second screen devices.
class CSecondScreenManager: public ISecondScreenDeviceDelegate
{
private:

	class CSecondScreenJournalQuestEntry
	{
	public:
		enum EJournalQuestEntryType
		{
			JQE_QuestGroup,
			JQE_Quest,
			JQE_QuestPhase,
			JQE_QuestObjective,
			JQE_QuestItemTag,
			JQE_QuestEnemyTag,
			JQE_QuestMapPin,
			JQE_QuestDescriptionGroup,
			JQE_QuestDescriptionEntry
		};
	public:
		CSecondScreenJournalQuestEntry( const Int32 type, const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name ): m_type(type), m_guid(guid), m_parentGuid(parentGuid), m_tag(tag), m_name(name){}
		void ConstructJSON( CJSONObjectUTF16& jsonObject );

	public:
		static const Char* QuestEntryTypeToString( Int32 type );

	public:
		Int32  m_status;
		THashMap<String, String> m_properties;

	private:
		CGUID	m_guid;
		CGUID	m_parentGuid;
		String	m_name;
		CName	m_tag;
		Int32   m_type;
	};

	struct CSecondScreenReceiveMessage
	{
		const CSecondScreenDevice* mFromDevice;
		String mMessage;
	};

public:
	enum GAME_STATE
	{
		GS_NONE = 0,
		GS_MAIN_MENU,
		GS_GAME_SESSION
	};

public:
	CSecondScreenManager();
	~CSecondScreenManager();
	
	void Init();
	void Shutdown();

	const ISecondScreenManagerDelegate* GetDelegate() const { return m_delegate; }
	void SetDelegate(ISecondScreenManagerDelegate* val) { m_delegate = val; }

	//! ALL PUBLIC FUNCTIONS MUST BE CALLED FROM THE SAME THREAD 
	void Update( Float timeDelta );

	RED_INLINE GAME_STATE GetState() const { return m_gameState; }

	// Send game state (MainMenu/GameMenu/GameSession)
	void SendState( GAME_STATE state );

	void SendPlayerPosition( const Float x, const Float y, const Float z, const Float rotation );
	//! Send info about change actual local map (area)
	void SendChangeArea( const String& areaName );

	//! Send global static map points list
	void SendGlobalStaticMapPins( const TDynArray<SSecondScreenMapPin>& globalMapPins );
	//! Send area static map points list
	void SendAreaStaticMapPins( const Int32 areaType, const TDynArray<SSecondScreenMapPin>& areaMapPins );
	//! Send info about discovered map point (global/area/quests) 
	void SendStaticMapPinDiscovered( const CName& tag );
	//! Send dynamic map pin for actual area
	void SendActualAreaDynamicMapPin( const SSecondScreenMapPin& mapPin );
	//! Send remove dynamic map pin for actual area
	void SendRemoveActualAreaDynamicMapPin( const Int32 id );
	//! Send move dynamic map pin for actual area
	void SendMoveActualAreaDynamicMapPin( const Int32 id, const CName& type, const Vector& position );
	//! Send update dynamic map pin for actual area
	void SendUpdateActualAreaDynamicMapPin( const SSecondScreenMapPin& mapPin );

	//! Send quest journal related stuff
	void SendJournalQuestGroupStatus ( const CGUID& guid, const CName& tag, const String& name, const Uint32 stringIndex_title, const Int32 status );
	void SendJournalQuestStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 type, const Int32 area, const Uint32 stringIndex_title, const Int32 status, Bool silent ); //! silent - if false quest should not apera in quest journal  
	void SendJournalQuestPhaseStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 status );
	void SendJournalQuestObjectiveStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Uint32 stringIndex_title, const String& image, const Int32 area, const Int32 counterType, const Int32 count, const Int32 status );
	void SendJournalQuestItemTagStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const CName& item, const Int32 status );
	void SendJournalQuestEnemyTagStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const String& enemyTag, const Int32 status );
	void SendJournalQuestMapPinStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const CName& mapPinID, const Int32 status );
	void SendJournalQuestDescriptionGroupStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 status );
	void SendJournalQuestDescriptionEntryStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 stringIndex_description, const Int32 status );

	void SendJournalTrackedQuestUpdated( const CGUID& guid );
	void SendJournalTrackedQuestObjectiveUpdated( const CGUID& guid );
	
	void SendFastTravelEnable();
	void SendFastTravelDisable();

	void ClearFileNameToAreaMapping();
	void SetFileNameToAreaMapping( const String& fileName, const Int32 areaIndex );

private:

	//! THREAD-SAFE
	//ISecondScreenDeviceDelegate
	void OnDeviceAdded( CSecondScreenDeviceWatcher* sender, CSecondScreenDevice* device );
	void OnDeviceRemoved(CSecondScreenDeviceWatcher* sender, const CSecondScreenDevice& device);
	void OnMessageReceived( const CSecondScreenDevice& device, const Char* message );

private:

	Bool GameStateToString( String& gameState ) const;
	static const Char* JournalQuesStatusToString( Int32 status );

	CSecondScreenJournalQuestEntry* GetJournalQuestEntry( const Int32 type, const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 status );

	//! add also mandatory fields to message
	//! return message as a string
	String JsonValueToMessage( CJSONObjectUTF16& jsonObject ) const;	

	//! add also mandatory fields to message
	//! arrayName - name of array
	//! messageProperties - if single message has to have any other properties then <arrayName> 
	//! messages - return messages as array of strings 
	void JsonArrayToMessageArray( const CJSONArrayUTF16& jsonArray, const String arrayName, const THashMap<String, String>& messageProperties, TDynArray<String>& messages ) const;

	void ConstructGlobalStaticMapPinsJsonMessage( TDynArray<String>& messages ) const;
	void ConstructAreaMapPinJsonObject( CJSONObjectUTF16& jsonObject, const SSecondScreenMapPin& mapPin ) const;
	void ConstructAreaMapPinsJsonArray( CJSONArrayUTF16& jsonArray, const TDynArray<SSecondScreenMapPin>& mapPinsArray  ) const;
	void ConstructAreaStaticMapPinsJsonMessage( TDynArray<String>& messages, const Int32 areaType ) const;
	void ConstructActualAreaDynamicMapPinsJsonMessage( TDynArray<String>& messages ) const;
	void ConstructQuestsStatusJsonMessage( TDynArray<String>& messages ) const;

	void ConstructTrackedQuestUpdatedJsonMessage( String& message ) const;
	void ConstructTrackedQuestObjectiveUpdatedJsonMessage( String& message ) const;

	void ConstructFastTravelEnableJsonMessage( String& message ) const;

	void ConstructTextFromGameJsonMessage( const Uint32 reqId, const Int32 stringId, const String& stringKey, TDynArray<String>& messages ) const;
	void ConstructJsonMessage( const String& text, Uint32 reqId, Int32 stringId, const String& stringKey, TDynArray<String>& messages ) const;
	void ConstructEmptyJsonMessage( const Uint32 reqId, TDynArray<String>& messages ) const;
	void SplitToParts( const Uint32 messageHeaderSize, const String& encodedString, TDynArray<String>& stringBase64Parts ) const;

	void SendMessages( CSecondScreenDevice* device, TDynArray<String> messages );
	void ClearGameData();
		
	void OnHandleRecievedMessages();
	void OnHandleFastTravelMessage( const CJSONObjectRefUTF16& jsonObject );
	void OnHandleSetTrackedQuestMessage( const CJSONObjectRefUTF16& jsonObject );
	void OnHandleGetTextMessage( const CJSONObjectRefUTF16& jsonObject, const CSecondScreenDevice* toDevice );
	
	MemSize EncodeBinaryArray( AnsiChar *dest, const AnsiChar *src, MemSize srcLen ) const;
	MemSize GetEncodeBinaryArrayLen( MemSize srcLen ) const;

private:
	Bool							m_Initialized;
	CSecondScreenDeviceWatcher*		m_deviceWatcher;
	TList< CSecondScreenDevice* >	m_devicesList;
		
	GAME_STATE							m_gameState;
	Float								m_playerPositionX;
	Float								m_playerPositionY;
	Float								m_playerPositionZ;
	Float								m_playerRotation;
	Int32								m_actualArea;
	String								m_actualAreaFileName;
	TDynArray<SSecondScreenMapPin>					  m_globalStaticMapPins;
	TDynArray<SSecondScreenMapPin>					  m_actualAreaDynamicMapPins;
	THashMap<Int32, TDynArray<SSecondScreenMapPin>>	  m_areasStaticMapPins;
	THashMap<String, Int32>							  m_fileNameToAreaMapping;
	THashMap< CGUID, CSecondScreenJournalQuestEntry*> m_journalQuestEntrys;
	CGUID											  m_trackedQuest;
	CGUID											  m_trackedQuestObjective;
	Bool								m_fastTravelEnabled;
	TDynArray<CSecondScreenReceiveMessage>	m_receivedMessages;

	Red::Threads::CMutex				m_mutex;
	Red::Threads::CMutex				m_mutexReceive;
	ISecondScreenManagerDelegate*		m_delegate;

	static const Uint32 GAME_VERSION;
};

typedef TSingleton< CSecondScreenManager, TDefaultLifetime, TCreateUsingNew > SCSecondScreenManager;
#endif