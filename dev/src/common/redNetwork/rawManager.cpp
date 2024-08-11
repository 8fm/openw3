/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "rawManager.h"
#include <new>
#include "../redSystem/error.h"

namespace Red { namespace Network {

	RawManager::RawManager()
		: CThread( "RawNetworkThread" )
		, m_initializedFlag( false )
		, m_shutdownFlag( false )
		, m_idAllocator( 100 )
	{
	}

	void RawManager::Initialize()
	{
		// initialize only once
		if ( m_initializedFlag.Exchange( true ) )
			return;

		// start thread
		BindWakeSocket();

		// start thread
		InitThread();
	}

	void RawManager::Shutdown()
	{
		// do not shutdown if not yet initialized
		if ( !m_initializedFlag.GetValue() )
			return;

		// send shutdown notification, only once
		if ( m_shutdownFlag.Exchange( true ) )
			return;

		// wake up thread
		WakeThread();

		// wait for the thread to finish
		JoinThread();
	}

	TConnectionID RawManager::CreateConnection( const Address& address, IRawConnectionInterface* callbackInterface )
	{		
		// invalid interface
		if ( callbackInterface == nullptr )
			return 0;

		// allocate connection ID - this is here so we allocate even in case of a failed connection
		// this makes tracking failure a little bit easier
		const TConnectionID id = (m_idAllocator.Increment() << FLAG_SHIFT) | FLAG_CONNECTION;

		// Contention part
		{
			Threads::CScopedLock< Threads::CMutex > lock( m_lock );

			if( m_connections.GetSpaceRemaining() > 0 )
			{
				// connect to given address
				Socket socket;
				if ( !socket.Connect( address ) )
				{
					return 0; // connection failed
				}

				// create connection object, we should not run out of them
				auto* obj = m_connections.Add();
				if ( !obj )
				{
					socket.Close();
					return 0;
				}

				// setup connection object
				obj->m_assignedID = id;
				obj->m_owner = nullptr;
				obj->m_interface = callbackInterface;
				obj->m_socket = socket;
				obj->m_remove = false;
			}
		}

		// wake the thread so we can listen for data
		WakeThread();

		// return allocated ID
		return id;
	}

	TListenerID RawManager::CreateListener( const Uint16 localPort, IRawListenerInterface* callbackInterface )
	{
		// invalid interface
		if ( callbackInterface == nullptr )
			return 0;

		// allocate listener ID - this is here so we allocate even in case of a failed connection
		// this makes tracking failure a little bit easier
		const Uint32 id = (m_idAllocator.Increment() << FLAG_SHIFT) | FLAG_LISTENER;

		// Contention part
		{
			Threads::CScopedLock< Threads::CMutex > lock( m_lock );

			if( m_listeners.GetSpaceRemaining() > 0 )
			{
				// create socket
				Socket socket;
				if ( !socket.Create() )
					return 0; // create failed

				// bind it to local port
				if ( !socket.Bind( localPort ) )
				{
					socket.Close();
					return 0; // bind failed
				}

				// start listening on the socket
				if ( !socket.Listen() )
				{
					socket.Close();
					return 0; // listen failed
				}

				// create connection object, we may run out of them
				auto* obj = m_listeners.Add();
				if ( !obj )
				{
					socket.Close();
					return 0;
				}

				// setup connection object
				obj->m_assignedID = id;
				obj->m_interface = callbackInterface;
				obj->m_socket = socket;
				obj->m_remove = false;
			}
		}

		// wake the thread so we can listen for data
		WakeThread();

		// return allocated ID
		return id;
	}

	Bool RawManager::CloseConnection( const TConnectionID connectionID )
	{
		// not a connection ID
		if ( !(connectionID & FLAG_CONNECTION) )
			return false;

		// find and close matching connection
		Bool removed = false;
		{
			Threads::CScopedLock< Threads::CMutex > lock( m_lock );

			for ( auto it = m_connections.Begin(); it != m_connections.End(); ++it )
			{
				if ( it->m_assignedID == connectionID )
				{
					// close connection without notification since we are closing on user's request
					it->m_owner = nullptr;
					it->m_interface = nullptr;
					it->m_socket.Close();
					it->m_remove = true;

					// remove from connection list
					removed = true;
					break;
				}
			}
		}

		// wake up thread to refresh list of the connections
		if ( removed )
			WakeThread();

		// return true if removed
		return removed;
	}

	Bool RawManager::CloseListener( const TListenerID listenerID )
	{
		// not a listener ID
		if ( !(listenerID & FLAG_LISTENER) )
			return false;

		// find and close matching connection
		Bool removed = false;
		{
			Threads::CScopedLock< Threads::CMutex > lock( m_lock );

			for ( auto it = m_listeners.Begin(); it != m_listeners.End(); ++it )
			{
				if ( it->m_assignedID == listenerID )
				{
					// remove connections matching this listener
					for ( auto jt = m_connections.Begin(); jt != m_connections.End(); ++jt )
					{
						if ( jt->m_owner && jt->m_owner->m_assignedID == listenerID )
						{
							// cleanup connection
							jt->m_assignedID = 0;
							jt->m_interface = nullptr;
							jt->m_owner = nullptr;
							jt->m_remove = true;
							jt->m_socket.Close();
						}
					}

					// close connection without notification since we are closing on user's request
					it->m_interface = nullptr;
					it->m_remove = true;
					it->m_socket.Close();

					// remove from connection list
					removed = true;
					break;
				}
			}
		}

		// wake up thread to refresh list of the connections
		if ( removed )
			WakeThread();

		// return true if removed
		return removed;
	}

	Bool RawManager::GetConnectionAddress( const TConnectionID connectionID, Address& outAddress )
	{
		// not a connection ID
		if ( !(connectionID & FLAG_CONNECTION) )
			return false;

		// contention region
		{
			Threads::CScopedLock< Threads::CMutex > lock( m_lock );

			for ( auto it = m_connections.Begin(); it != m_connections.End(); ++it )
			{
				if ( it->m_assignedID == connectionID )
				{
					outAddress = it->m_socket.GetPeer();
					return true;
				}
			}
		}

		// connection not found
		return false;
	}

	Uint32 RawManager::Send( const TConnectionID connectionID, const void* data, const Uint32 dataSize )
	{
		// not a connection ID
		if ( !(connectionID & FLAG_CONNECTION) )
			return false;

		// contention region
		{
			Threads::CScopedLock< Threads::CMutex > lock( m_lock );

			for ( auto it = m_connections.Begin(); it != m_connections.End(); ++it )
			{
				if ( it->m_assignedID == connectionID )
				{
					return it->m_socket.Send( data, dataSize );
				}
			}
		}

		// not found or nothing sent
		return 0;
	}

	void RawManager::BindWakeSocket()
	{
		// Threadwaker "dummy" UDP socket, that will send dummy data to wake the
		// network thread when there's something to be sent
		m_threadWaker.Create( Socket::UDP );

		System::Uint16 port = WAKE_THREAD_PORT_RANGE_START;
		m_threadWakerBoundAddress = Address( "127.0.0.1", port );

		// Search for available port
		while( m_threadWaker.IsUnbound() && !m_threadWaker.Bind( m_threadWakerBoundAddress ) && port <= WAKE_THREAD_PORT_RANGE_END )
		{
			++port;
			m_threadWakerBoundAddress.SetPort( port );
		}
	}

	void RawManager::WakeThread()
	{
		System::Uint32 buffer = 0xCA11;
		m_threadWaker.SendTo( &buffer, sizeof( System::Uint32 ), m_threadWakerBoundAddress );
	}

	void RawManager::WaitForAction( fd_set& readSet, fd_set& writeSet, fd_set& errorSet )
	{
		FD_ZERO( &writeSet );
		FD_ZERO( &readSet );
		FD_ZERO( &errorSet );

		// add thread waker to the set
		SocketId highestSocketId = m_threadWaker.GetRawDescriptor();
		FD_SET( m_threadWaker.GetRawDescriptor(), &readSet );

		// contention point
		{
			Threads::CScopedLock< Threads::CMutex > lock( m_lock );

			// listeners
			for ( auto it = m_listeners.Begin(); it != m_listeners.End(); ++it )
			{
				if ( !it->m_remove && it->m_socket.IsListening() )
				{
					FD_SET( it->m_socket.GetRawDescriptor(), &readSet );

					auto socketId = it->m_socket.GetRawDescriptor();
					if ( socketId > highestSocketId )
						highestSocketId = socketId;
				}
			}

			// connections
			for ( auto it = m_connections.Begin(); it != m_connections.End(); ++it )
			{
				if ( !it->m_remove )
				{
					auto socketId = it->m_socket.GetRawDescriptor();
					if ( socketId > highestSocketId )
						highestSocketId = socketId;

					if ( it->m_socket.IsConnecting() )
					{
						FD_SET( it->m_socket.GetRawDescriptor(), &writeSet );
						FD_SET( it->m_socket.GetRawDescriptor(), &errorSet );
					}
					else if( it->m_socket.IsConnected() )
					{
						FD_SET( it->m_socket.GetRawDescriptor(), &readSet );
					}
				}
			}
		}

		// Has to be the value of the highest socket descriptor + 1
		++highestSocketId;

		// Wait for sockets
		select( (int)highestSocketId, &readSet, &writeSet, &errorSet, nullptr );
	}

	void RawManager::ProcessNewConnections_NoLock( fd_set& readSet )
	{
		for ( auto it = m_listeners.Begin(); it != m_listeners.End(); ++it )
		{
			// removed listener
			if ( it->m_remove )
				continue;

			// something new on this socket ?
			if ( !FD_ISSET( it->m_socket.GetRawDescriptor(), &readSet ) )
				continue;

			// broken ?
			if ( !it->m_socket.IsListening() )
			{
				RED_LOG_SPAM( RED_LOG_CHANNEL( Net ), TXT( "RawConnection Listener Dropped, ListenerID=%d," ), it->m_assignedID );

				// remove from list of listeners
				// NOTE: this will not break the iteration
				auto callback = it->m_interface;
				auto assignedId = it->m_assignedID;

				// cleanup
				it->m_interface = nullptr;
				it->m_assignedID = 0;
				it->m_remove = true;

				// notify that the listener got closed
				RED_FATAL_ASSERT( callback != nullptr, "Lost listener interface, ID=%d", assignedId );
				callback->OnClosed( assignedId );				
				continue;
			}

			// accept the connection
			Socket newSocket = it->m_socket.Accept();
			if ( !newSocket.IsConnected() )
				continue;

			// allocate ID
			const TConnectionID connectionID = (m_idAllocator.Increment() << FLAG_MASK) | FLAG_CONNECTION;

			// ask the interface if it allows such connection
			IRawConnectionInterface* connectionInterface = nullptr;
			if ( !it->m_interface->OnConnection( newSocket.GetPeer(), connectionID, connectionInterface ) || !connectionInterface )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL( Net ), TXT( "RawConnection Refused: listener refused connection" ) );
				newSocket.Close();
				continue;
			}

			// create connection 
			RawConnection* connection = m_connections.Add();
			if ( !connection )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL( Net ), TXT( "RawConnection Refused: not enough connection objects" ) );
				newSocket.Close();
				continue;
			}

			// setup object
			connection->m_assignedID = connectionID;
			connection->m_interface = connectionInterface;
			connection->m_owner = &*it;
			connection->m_socket = newSocket;
			connection->m_remove = false;

			// connection accepted
			RED_LOG_SPAM( RED_LOG_CHANNEL( Net ), TXT( "RawConnection Established, ListenerID=%d, ConnectionID=%d" ), it->m_assignedID, connectionID );
		}
	}

	void RawManager::ProcessNewData_NoLock( fd_set& readSet )
	{
		for ( auto it = m_connections.Begin(); it != m_connections.End(); ++it )
		{
			// connection is about to be removed
			if ( it->m_remove )
				continue;

			// prevent from checking dead sockets
			if ( it->m_socket.IsConnected() )
			{
				// something new on this socket ?
				if ( !FD_ISSET( it->m_socket.GetRawDescriptor(), &readSet ) )
					continue;

				// try to read data
				Uint8 buffer[ MAX_PACKET+10 ];
				Uint32 size = it->m_socket.Receive( buffer, MAX_PACKET );
				while ( it->m_socket.IsConnected() && size > 0 )
				{
					// process data
					if ( it->m_interface != nullptr )
					{
						it->m_interface->OnData( buffer, size, it->m_socket.GetPeer(), it->m_assignedID );
					}

					// try to read next packet
					size = it->m_socket.Receive( buffer, MAX_PACKET );
				}
			}

			// lost ? - we need to recheck because we may have been closed while in reading
			if ( !it->m_socket.IsConnected() )
			{
				RED_LOG_SPAM( RED_LOG_CHANNEL( Net ), TXT( "RawConnection Connection Dropped, ConnectionID=%d," ), it->m_assignedID );

				// remove from list of connection
				// NOTE: this will not break the iteration
				auto callback = it->m_interface;
				auto assignedId = it->m_assignedID;

				// cleanup
				it->m_assignedID = 0;
				it->m_interface = nullptr;
				it->m_owner = nullptr;
				it->m_remove = true;

				// notify that the listener got closed
				RED_FATAL_ASSERT( callback != nullptr, "Lost connection interface, ID=%d", assignedId );
				callback->OnDisconnected( assignedId );
				continue;
			}
		}
	}

	void RawManager::RemovePendingObjects_NoLock()
	{
		// cleanup connections
		for ( auto it = m_connections.Begin(); it != m_connections.End(); ++it )
		{
			if ( it->m_remove )
			{
				RED_FATAL_ASSERT( it->m_interface == nullptr, "Connection removed in invalid state" );
				it->m_socket.Close();

				// remove from connection list, safe
				m_connections.Remove( it );
			}
		}

		// cleanup listeners
		for ( auto it = m_listeners.Begin(); it != m_listeners.End(); ++it )
		{
			if ( it->m_remove )
			{
				RED_FATAL_ASSERT( it->m_interface == nullptr, "Listener removed in invalid state" );
				it->m_socket.Close();

				// remove from connection list, safe
				m_listeners.Remove( it );
			}
		}
	}

	void RawManager::ThreadFunc()
	{		
		while ( !m_shutdownFlag.GetValue() )
		{
			// Wait for the sockets
			fd_set readSet, writeSet, errorSet;
			WaitForAction( readSet, writeSet, errorSet );

			// Purge waker buffer
			if ( FD_ISSET( m_threadWaker.GetRawDescriptor(), &readSet ) )
			{
				Address localhost;
				System::Uint8 buffer[ 32 ];
				m_threadWaker.ReceiveFrom( buffer, sizeof( buffer ), localhost );
			}

			// Contention block
			{
				Threads::CScopedLock< Threads::CMutex > lock( m_lock );

				// Process incoming connections
				ProcessNewConnections_NoLock( readSet );

				// Process incoming data
				ProcessNewData_NoLock( readSet );

				// Remove connections and listeners
				RemovePendingObjects_NoLock();
			}
		}
	}


} } // Red::Network

