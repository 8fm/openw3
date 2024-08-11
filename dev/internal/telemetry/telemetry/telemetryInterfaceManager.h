#pragma once

#include "../typeDefs.h"
#include "../memoryInterface.h"
#include "../fileInterface.h"
#include "../nbc/nonBlockingLIFOQueue.h"

//------------------ FORWARD DECLARATIONS -------------------------
namespace Telemetry
{
	enum EBackendTelemetry
	{
		BT_NONE				= 0,
		BT_LOCAL_STORAGE	= ( 1 << 0 ),
		BT_GOOGLE_ANALYTICS = ( 1 << 1 ),
		BT_RED_TEL_API		= ( 1 << 2 ),
		BT_DD_SERVICE_API	= ( 1 << 3 ),
		BT_XBOX_DATA_PLATFORM_API	= ( 1 << 4 ),
	};

	enum StateType
	{
		TIM_UNDEFINED_STATE = 0,
		TIM_COLLECTING,
		TIM_SESSION_COLLECTING,
		TIM_ENUM_COUNT
	};

	struct EventData;
	class ITelemetryInterface;
	class CTelemetryManagerInternal;

	//------------------ CLASS DECLARATION -------------------------

	//! This interface should be derived and implementations should write/read data from local storage
	//! Those methods are called on thread from Update method was invoked
	class ITelemetryManagerDelegate
	{
	public:
		virtual bool AppendDataToLocalStorage( const void* const dataBuffer, unsigned int bufferSize ) = 0;

		virtual unsigned int DataSize() = 0;
		virtual bool ReadDataToBuffer( void* dataBuffer ) = 0;
		virtual void DeleteCachedData() = 0;
	};

	class CTelemetryInterfaceManager
	{
	public:

		// this method is not thread-safe. It should be called exactly once and should exit before any other call to this library
		static void Init( IMemory* memoryAllocator, IFileReadCallBackManager* fileManager, TelemetryAssert assertFunc = nullptr, TelemetryDebug debugFunc = nullptr );

		CTelemetryInterfaceManager();
		virtual ~CTelemetryInterfaceManager();

		//! This methods is not thread safe.
		void AddBackend( EBackendTelemetry backend );

		void ConfigTelemetry( int gameId, int gameVersion, EPlatform platform, const_utf8 language, EUserCategory userCategory,
			int curlTimeout = DEFAULT_TIMEOUT_TIME, const_utf8 googleTrackingId = NULL );

		void ConfigTelemetry( int gameId, int gameVersion, EPlatform platform, const_utf8 deviceId,
			const_utf8 language, EUserCategory userCategory,
			int curlTimeout = DEFAULT_TIMEOUT_TIME, const_utf8 googleTrackingId = NULL );

		void ConfigTelemetryHTTPService( EBackendTelemetry backendName, const_utf8 serverAddress, unsigned int serverPort );

		inline void SetImmediatePost( bool immediate ) { m_immediatePostingEnabled = immediate; }

		//! Set the timeout between events batch transfer
		void SetBatchAggregationTime( int seconds );

		//! Maximum number of events that will be sent as batch
		//! Default value is 500
		inline void SetMaximumBatchCount( int count ) { m_batchEventCount = count; }

		void SetDelegate( ITelemetryManagerDelegate* del );

		//! This method is thread safe
		//! Before this call nothing will be collected
		bool StartCollecting();

		//! This method is thread safe 
		//! From this point collected data are connected with generated session id
		//! For HTTP base backends connection to webservice is established
		bool StartSession( const_utf8 playerId, const_utf8 parentSessionId, const_utf8 sessionTags );


		//! Those methods below are thread safe and very lightweight. It can be used from any thread.
		void Log	( const_utf8 eventName, const_utf8 categoryName );
		void LogL	( const_utf8 eventName, const_utf8 categoryName, const_utf8 labelName );
		void LogV	( const_utf8 eventName, const_utf8 categoryName, int value );
		void LogV	( const_utf8 eventName, const_utf8 categoryName, const_utf8 value );
		void LogVL	( const_utf8 eventName, const_utf8 categoryName, int value, const_utf8 labelName );
		void LogVL	( const_utf8 eventName, const_utf8 categoryName, const_utf8 value, const_utf8 labelName );
		void LogV	( const_utf8 eventName, const_utf8 categoryName, void* data, size_t dataLength );
		void LogVL	( const_utf8 eventName, const_utf8 categoryName, void* data, size_t dataLength, const_utf8 labelName );

		void SetPlayerState( const_utf8 playerState );
		void SetWorldName( const_utf8 worldName );

		void SetCommonParam( const_utf8 paramName, int value );
		void SetCommonParam( const_utf8 paramName, float value );
		void SetCommonParam( const_utf8 paramName, const_utf8 value );

		//! This method immediatelly, synchronously posts data to telemetry server. 
		//! It can be used for posting errors, exceptions or crashes.
		//! It should not be used for normal events posting
		//! @note it uses system malloc / free functions
		//! @note this method is synchronous
		//! @note if not subscribed (during initialization), this call will be ignored!
		//! @param curlTimeout - timeout for curl in seconds. 0 - the default value is used
		void SendExceptionCategoryA( const_utf8 eventName, const_utf8 log, int curlTimeout = 0 );
		void SendExceptionCategoryW( const_utf8 eventName, const_utf16 log, int curlTimeout = 0 );

		//! Those methods should not be called on main/render/update thread.
		//! Those methods should be called only from one thread!
		//! One call can take a significant amount of time because sometimes it needs to flush data to local storage.
		//! This method can call HTTP request asynchronously.
		//! @param immediatePost Discard any configured batch count / batch time and dispatch cached events immediately
		void Update( bool immediatePost );

		//! This method is thread safe 
		//! From this point collected data is not connected with any session id
		//! For HTTP base backends connection to webservice is disabled
		bool StopSession();

		//! This method is thread safe 
		//! This method should be the last method to call (no events will be logged after calling this method!)
		//! This method might request to save data via ITelemetryManagerDelegate interface
		bool StopCollecting();

		Telemetry::StateType TelemetryState() const { return m_telemetryState; }

		//! Get number of events logged to given backend
		long LoggedEventsCountForBackend(EBackendTelemetry backend);

		//! Check if telemetry is doing some background work (i.e. HTTP req)
		//! If yes, it should be updated ASAP
		bool IsOperationInProgress();

		// session id from external service 
		// optional, depends on backend implementation
		void SetExternalSessionId( EBackendTelemetry backendName, const_utf8 extSessionId );

		// return session id subscribed by backend
		const_utf8 GetSessionId( EBackendTelemetry backendName );

		// return current timer value
		void GetTime( double& time, unsigned long long& qpf, unsigned long long& qpc );

	protected:

		CTelemetryInterfaceManager& operator= ( const CTelemetryInterfaceManager& ) { return *this; }
		CTelemetryInterfaceManager( const CTelemetryInterfaceManager& ) {}

		void SendEndSessionMarker();

		bool ProcessEvent( EventData* eventData );
		void ProcessAllEvents( bool lastUpdate, bool secondBuffer = false);

		void OnStartCollecting();
		void OnStartSession();
		void OnStopSession();
		void OnStopCollecting();

	protected:

		// This is needed to hide internal
		CTelemetryManagerInternal*			m_pImpl;

		bool								m_immediatePostingEnabled;
		int									m_backendEnums;

		int									m_batchEventCount;

		CNonBlockingLIFOQueue					m_eventsList[2];


		private:

		CNonBlockingLIFOQueue					m_commandBuffor;
		StateType								m_telemetryState;
		const_utf8								m_playerId;
		const_utf8								m_parentSessionId;
		const_utf8								m_sessionTags;

		void*									m_internalSync;

		static bool								s_initialized;		
	};
} // namespace Telemetry
