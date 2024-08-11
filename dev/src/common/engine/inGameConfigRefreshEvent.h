#pragma once

namespace InGameConfig
{
	class IRefreshEventListener
	{
	public:
		virtual ~IRefreshEventListener() { /* Intentionally Empty */ }
		virtual void OnRequestRefresh( const CName& eventName ) = 0;
	};

	class CRefreshEventDispatcher
	{
	public:
		void DispatchIfExists( const CName& eventName );
		void RegisterForEvent( const CName& eventName, IRefreshEventListener* listener );
		void UnregisterFromAllEvents( IRefreshEventListener* listener );
		void UnregisterFromEvent( const CName& eventName, IRefreshEventListener* listener );

	private:
		typedef TDynArray<IRefreshEventListener*> ListenerBucket;
		typedef THashMap<CName, ListenerBucket> ListenerBucketMap;

		void Dispatch( const CName& eventName );
		Bool DoesExist( const CName& eventName ) const;

		ListenerBucket& GetOrCreateListenerBucket( const CName& eventName );

	private:
		ListenerBucketMap m_listenerBuckets;

	};

	typedef TSingleton< CRefreshEventDispatcher > GRefreshEventDispatcher;
}
