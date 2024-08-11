/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkAudioLib.h
//
// AkAudioLib specific definitions
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_H_
#define _AUDIOLIB_H_

// Include exposed SDK interface.
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/Tools/Common/AkSyncCaller.h>

using namespace AK;

#include <AK/Tools/Common/AkObject.h>
#include "AkAction.h"		// enum AkActionType;

#ifdef WWISE_AUTHORING
#include <map>
#include <set>
#endif


class CAkIndexable;
namespace AkBank
{
	struct AKBKSubHircSection;
}
class CAkUsageSlot;

////////////////////////////////////////////////////////////////////////////
// Index ID to Ptr
////////////////////////////////////////////////////////////////////////////

enum AkIndexType
{
	AkIdxType_AudioNode,
	AkIdxType_BusNode,
	AkIdxType_CustomState,
	AkIdxType_Action,
	AkIdxType_Event,
	AkIdxType_DialogueEvent,
	AkIdxType_Layer,
	AkIdxType_Attenuation,
	AkIdxType_DynamicSequence,
	AkIdxType_FxShareSet,
	AkIdxType_FxCustom
};

////////////////////////////////////////////////////////////////////////////
// Behavioral extension type
////////////////////////////////////////////////////////////////////////////
typedef bool	(*AkExternalStateHandlerCallback)( AkStateGroupID in_stateGroupID, AkStateID in_stateID );
typedef AKRESULT	(*AkExternalBankHandlerCallback)( const AkBank::AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID );
typedef void	(*AkExternalProfileHandlerCallback)();

namespace AK
{
	class IALMonitor;

	namespace SoundEngine
	{
		// Sync class for bank loads.
		class AkBankSyncLoader : public AkSyncCaller
		{
		public:
			AkMemPoolId     memPoolId;
		};

		void DefaultBankCallbackFunc(
							AkBankID    in_bankID,
							const void* in_pInMemoryBankPtr,
							AKRESULT	in_eLoadResult,
							AkMemPoolId in_memPoolId,
							void *		in_pCookie );

		// Private, wwise-only methods

		extern AkPlayingID PostEvent(
	        AkUniqueID in_eventID,						///< Unique ID of the event
	        AkGameObjectID in_gameObjectID,				///< Associated game object ID
			AkUInt32 in_uFlags,							///< Bitmask: see \ref AkCallbackType
			AkCallbackFunc in_pfnCallback,				///< Callback function
			void * in_pCookie,							///< Callback cookie that will be sent to the callback function along with additional information
			AkCustomParamType * in_pCustomParam,		///< Optional custom parameter
			AkPlayingID	in_PlayingID = AK_INVALID_PLAYING_ID
	        );

		extern AKRESULT SetState( 
				AkStateGroupID in_StateGroup,
				AkStateID in_State,
				bool in_bSkipTransitionTime,
                bool in_bSkipExtension
				);

		extern AKRESULT ResetSwitches( AkGameObjectID in_GameObjID = AK_INVALID_GAME_OBJECT );
		extern AKRESULT ResetRTPC( AkGameObjectID in_GameObjID = AK_INVALID_GAME_OBJECT );

		enum LoadMediaFile_ActionType
		{
			LoadMediaFile_Load,
			LoadMediaFile_Unload,
			LoadMediaFile_Swap
		};

#ifdef AK_SUPPORT_WCHAR
		extern AKRESULT LoadMediaFileSync( AkUniqueID in_MediaID, const wchar_t* in_szFileName, LoadMediaFile_ActionType in_eActionType );
#endif //AK_SUPPORT_WCHAR

		/// AkCommandPriority
		/// Game should always only use only the flag AkCommandPriority_Game, 
		/// others values are reserved for internal usage.
		/// \sa
		/// - AK::SoundEngine::SetVolumeThreshold()
		/// - AK::SoundEngine::SetMaxNumVoicesLimit()
		enum AkCommandPriority 
		{
			AkCommandPriority_Game = 0, 		///< The game must always specify this flag
			AkCommandPriority_WwiseApp = 1, 	///< Reserved
			AkCommandPriority_InitDefault = 2, 	///< Reserved
			AkCommandPriority_None = 3 			///< Reserved
		};

		extern AKRESULT SetVolumeThresholdInternal( AkReal32 in_fVolumeThresholdDB, AkCommandPriority in_Priority );
		extern AKRESULT SetMaxNumVoicesLimitInternal( AkUInt16 in_maxNumberVoices, AkCommandPriority in_Priority );

		// This one cannot be set by the game or banks, it is baked inside conversion files.
#ifndef AK_OPTIMIZED
		extern AkLoudnessFrequencyWeighting GetLoudnessFrequencyWeighting();
		extern void SetLoudnessFrequencyWeighting( AkLoudnessFrequencyWeighting in_eLoudnessFrequencyWeighting );
#endif

		//////////////////////////////////////////////////////////////////////////////////
		//Monitoring
		//////////////////////////////////////////////////////////////////////////////////
		AK_EXTERNFUNC( IALMonitor*, GetMonitor )( void );

        ////////////////////////////////////////////////////////////////////////////
		// Behavioral extensions registration
		////////////////////////////////////////////////////////////////////////////
        extern AKSOUNDENGINE_API AKRESULT AddBehavioralExtension( 
            AkGlobalCallbackFunc in_pCallback
            );
        extern AKSOUNDENGINE_API AKRESULT RemoveBehavioralExtension( 
            AkGlobalCallbackFunc in_pCallback
            );
        extern void AddExternalStateHandler( 
            AkExternalStateHandlerCallback in_pCallback
            );
		extern void AddExternalBankHandler(
			AkExternalBankHandlerCallback in_pCallback
			);
		extern void AddExternalProfileHandler(
			AkExternalProfileHandlerCallback in_pCallback
			);

		extern CAkIndexable* GetIndexable(
			AkUniqueID	in_IndexableID, // Indexable ID
			AkIndexType	in_eIndexType	// Index to look into
			);
		
		// Set the position of a game object.
		// This function is allowed to set the position of the omni game object
        extern AKSOUNDENGINE_API AKRESULT SetPositionInternal( 
			AkGameObjectID in_GameObjectID,			// Game object identifier
			const AkSoundPosition & in_Position		// Position to set
		    );

		extern AKSOUNDENGINE_API AkPlayingID PlaySourcePlugin( AkUInt32 in_plugInID, AkUInt32 in_CompanyID, AkGameObjectID in_GameObjID );
		extern AKSOUNDENGINE_API AKRESULT StopSourcePlugin( AkUInt32 in_plugInID, AkUInt32 in_CompanyID, AkPlayingID in_playingID );
	}

#ifdef WWISE_AUTHORING
	namespace WWISESOUNDENGINE_DLL
	{
		class AnalysisInfo;

		class IAnalysisObserver
		{
		public:
			virtual void OnAnalysisChanged( AK::WWISESOUNDENGINE_DLL::AnalysisInfo * in_pAnalysisInfo ) = 0;
		};

		class AnalysisInfo
		{
		public:
			AnalysisInfo() 
				: m_pEnvelope( NULL ) 
				, m_uNumPoints( 0 )
				, m_fMaxEnvValue( 0 )
				, m_bOwner( false )
			{}
			
			AnalysisInfo( AkFileParser::EnvelopePoint * in_pEnvelope, AkUInt32 in_uNumPoints, AkReal32 in_fMaxEnvValue ) 
				: m_pEnvelope( in_pEnvelope ) 
				, m_uNumPoints( in_uNumPoints )
				, m_fMaxEnvValue( in_fMaxEnvValue )
				, m_bOwner( false )	// envelope pointer assigned from outside; not owned by this object.
			{}

			// Deep copy.
			AnalysisInfo( const AnalysisInfo & other ) 
				: m_pEnvelope( NULL ) 
				, m_uNumPoints( 0 )
				, m_fMaxEnvValue( 0 )
				, m_bOwner( false )
			{
				if ( other.m_pEnvelope )
				{
					m_pEnvelope = (AkFileParser::EnvelopePoint*)malloc( sizeof(AkFileParser::EnvelopePoint) * other.m_uNumPoints );
					memcpy( m_pEnvelope, other.m_pEnvelope, sizeof(AkFileParser::EnvelopePoint) * other.m_uNumPoints );
					m_bOwner = true;
				}
				m_uNumPoints = other.m_uNumPoints;
				m_fMaxEnvValue = other.m_fMaxEnvValue;
				std::copy( other.m_observers.begin(), other.m_observers.end(), m_observers.begin() );
			}
			AnalysisInfo & operator = ( const AnalysisInfo & other )
			{
				if ( &other != this )
				{
					// Free envelope if we're its owner.
					if ( m_pEnvelope && m_bOwner )
						free( m_pEnvelope );
					m_pEnvelope = NULL;
					m_bOwner = false;

					if ( other.m_pEnvelope )
					{
						m_pEnvelope = (AkFileParser::EnvelopePoint*)malloc( sizeof(AkFileParser::EnvelopePoint) * other.m_uNumPoints );
						memcpy( m_pEnvelope, other.m_pEnvelope, sizeof(AkFileParser::EnvelopePoint) * other.m_uNumPoints );
						m_bOwner = true;
					}
					m_uNumPoints = other.m_uNumPoints;
					m_fMaxEnvValue = other.m_fMaxEnvValue;
					std::copy( other.m_observers.begin(), other.m_observers.end(), m_observers.begin() );
				}
				return *this;
			}
			~AnalysisInfo()
			{
				// Free envelope if we're its owner.
				if ( m_pEnvelope && m_bOwner )
					free( m_pEnvelope );
			}

			// Services for sources.
			void RegisterObserver( IAnalysisObserver * in_pObserver )
			{ 
				AKVERIFY( m_observers.insert( in_pObserver ).second ); 
			}
			void UnregisterObserver( IAnalysisObserver * in_pObserver ) 
			{ 
				std::set<IAnalysisObserver*>::iterator it = m_observers.find( in_pObserver ); 
				if ( it != m_observers.end() )
					it = m_observers.erase( it );
			}
			void NotifyObservers() 
			{
				for ( std::set<IAnalysisObserver*>::iterator it = m_observers.begin(); it != m_observers.end(); it++ )
				{
					(*it)->OnAnalysisChanged( this );
				}
			}

			inline AkFileParser::EnvelopePoint * GetEnvelope() const { return m_pEnvelope; }
			inline AkUInt32 GetNumPoints() const { return m_uNumPoints; }
			inline AkReal32 GetMaxEnvValue() const { return m_fMaxEnvValue; }

		private:
			std::set<IAnalysisObserver*> m_observers;
			AkFileParser::EnvelopePoint * m_pEnvelope;
			AkUInt32 m_uNumPoints;
			AkReal32 m_fMaxEnvValue;
			bool m_bOwner;
		};

		typedef std::map< AkUniqueID, AnalysisInfo > GlobalAnalysisSet;
	}
#endif
}

#endif
