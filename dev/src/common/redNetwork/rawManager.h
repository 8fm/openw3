/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NETWORK_RAWMANAGER_H_
#define _RED_NETWORK_RAWMANAGER_H_

/// Raw Network is the proper network system with no BS wrapping around it

#include "../redSystem/types.h"
#include "../redThreads/redThreadsThread.h"
#include "../redThreads/redThreadsAtomic.h"

#include "fixedPoolList.h"
#include "list.h"

#include "network.h"

#include "address.h"
#include "socket.h"
#include "memory.h"

namespace Red
{
	namespace Network
	{
		// ID typedefs
		typedef Uint32 TConnectionID;
		typedef Uint32 TListenerID;

		// raw connection interface
		class IRawConnectionInterface
		{
		public:
			virtual ~IRawConnectionInterface() {};

			//! Called when connection is dropped, not called if that's us that closed the connection, not called on exit
			//! This callback is asynchronous and can happen at any time but is not reentrant
			virtual void OnDisconnected( const TConnectionID connectionID ) = 0;

			//! Called whenever we receive data, it's up to us to process it
			//! This callback is asynchronous and can happen at any time but is not reentrant
			virtual void OnData( const void* data, const Uint32 dataSize, const Address& incomingAddress, const TConnectionID connectionID ) = 0;
		};

		// raw listener interface
		class IRawListenerInterface
		{
		public:
			virtual ~IRawListenerInterface() {};

			//! Called when listener is closed due to some problems, not called when listener is closes by us or on exit
			//! This callback is asynchronous and can happen at any time but is not reentrant
			virtual void OnClosed( const TListenerID listenerID ) = 0;

			//! Called whenever we receive data, it's up to us to process it, returning FALSE will deny the connection
			//! This callback is asynchronous and can happen at any time but is not reentrant
			virtual bool OnConnection( const Address& incomingAddress, const TConnectionID connectionID, IRawConnectionInterface*& outConnectionInterface ) = 0;				
		};

		/// Manager for RAW network
		class RawManager : public Red::Threads::CThread
		{
		public:
			RawManager();

			// Initialize raw network
			void Initialize();

			// Shutdown raw network
			void Shutdown();

			//! Open a TCP connection to given address
			//! Specifying a callback interface is required
			//! This returns valid connection ID if successful or 0 if not
			//! This is a blocking call
			TConnectionID CreateConnection( const Address& address, IRawConnectionInterface* callbackInterface );

			//! Open a TCP listener at given port
			//! Specifying a callback interface is required
			//! This returns valid listener ID if successful or 0 if not
			//! This is a blocking call
			TListenerID CreateListener( const Uint16 localPort, IRawListenerInterface* callbackInterface );

			//! Close given connection, returns true if successful
			//! Requires valid connection to be specified
			//! This is a blocking call
			Bool CloseConnection( const TConnectionID connectionID );

			//! Close given listener, returns true if successful
			//! Requires valid listener to be specified
			//! This is a blocking call
			Bool CloseListener( const TListenerID listenerID );

			//! Get remove address of connection, returns true if successful
			//! Requires valid connection to be specified
			//! This is a non blocking call
			Bool GetConnectionAddress( const TConnectionID connectionID, Address& outAddress );

			//! Send data through connection, returns number of bytes sent
			//! Requires valid listener to be specified
			//! This can fail and call OnDisconnected on the connection
			//! This is a non blocking call
			Uint32 Send( const TConnectionID connectionID, const void* data, const Uint32 dataSize );

		private:
			// Create internal wake socket
			void BindWakeSocket();

			// Wake up the processing thread
			void WakeThread();

			// Process socket events
			void WaitForAction( fd_set& readSet, fd_set& writeSet, fd_set& errorSet );
			void ProcessNewConnections_NoLock( fd_set& readSet );
			void ProcessNewData_NoLock( fd_set& readSet );
			void RemovePendingObjects_NoLock();

			// Internal processing function
			virtual void ThreadFunc() override;

			static const Uint32 WAKE_THREAD_PORT_RANGE_START = 36150;
			static const Uint32 WAKE_THREAD_PORT_RANGE_END = 36250;

			static const Uint32 MAX_CONNECTIONS = 45;
			static const Uint32 MAX_LISTENERS = 16;

			static const Uint32 MAX_PACKET = 4096;

			static const Uint32 FLAG_SHIFT = 4;					// bit shift for flag mask vs ID
			static const Uint32 FLAG_MASK = 0xF;				// mask for flags in the ID
			static const Uint32 FLAG_CONNECTION = 0x1;			// connection flag
			static const Uint32 FLAG_LISTENER = 0x2;			// listener flag

			struct RawListener
			{
				Socket						m_socket;			// network socket
				TListenerID					m_assignedID;		// assigned ID, zero if not assigned
				IRawListenerInterface*		m_interface;		// communication interface
				Bool						m_remove;			// request to remove this listener from list
			};

			struct RawConnection
			{
				Socket						m_socket;			// connection
				TConnectionID				m_assignedID;		// assigned ID, zero if not assigned
				RawListener*				m_owner;			// null if manual connection
				IRawConnectionInterface*	m_interface;		// communication interface
				Bool						m_remove;			// request to remove this connection from list
			};

			//! Dummy socket used to trigger thread wakeup when we're sending data
			Socket m_threadWaker;
			Address m_threadWakerBoundAddress;

			//! Initialized/Shutdown flag 
			Red::Threads::CAtomic< Bool > m_initializedFlag;
			Red::Threads::CAtomic< Bool > m_shutdownFlag;

			//! ID allocator for connections and listeners
			Threads::CAtomic< Int32 > m_idAllocator;

			// For both connections and pending connections
			mutable Threads::CMutex m_lock;

			//! Active outgoing connections
			FixedPoolList< RawConnection, MAX_CONNECTIONS > m_connections;

			//! Active listeners
			FixedPoolList< RawListener, MAX_LISTENERS > m_listeners;
		};

	}
}

#endif