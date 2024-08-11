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


/*	
	----------------------------------------------------------------------------------------
	Common file used by the communication as a cross platform wrapper for the BSD sockets API.
	Location: \Wwise\Communication\Remote\CodeBase
	Implementation location: In the platform folders: Wwise\Communication\Remote\{Platform}
	
	See more information in the implementation file.
	-----------------------------------------------------------------------------------------
*/

#pragma once

#include "AkPrivateTypes.h"
#include "NetworkTypes.h"

class GameSocketAddr;

class GameSocket
{
public:
	enum PollType
	{
		PollRead,
		PollWrite
	};

	GameSocket();
	~GameSocket();

	bool Create( AkInt32 in_type, AkInt32 in_protocol, bool in_bRequiresAccumulator = false );
	
	void ReuseAddress();
	void NoDelay();

	AkInt32 Connect( const GameSocketAddr& in_rAddr );

	AkInt32 Bind( const GameSocketAddr& in_rAddr );
	AkInt32 Listen( AkInt32 in_backlog ) const;
	void Accept( GameSocketAddr& out_rAddr, GameSocket & out_targetSocket );

	AkInt32 Send( const void* in_pBuf, AkInt32 in_length, AkInt32 in_flags ) const;
	AkInt32 Recv( void* in_pBuf, AkInt32 in_length, AkInt32 in_flags ) const;

	AkInt32 SendTo( const void* in_pBuf, AkInt32 in_length, AkInt32 in_flags, const GameSocketAddr& in_rAddr ) const;
	AkInt32 RecvFrom( void* in_pBuf, AkInt32 in_length, AkInt32 in_flags, GameSocketAddr& out_rAddr ) const;

	AkInt32 Poll( PollType in_ePollType, AkUInt32 in_timeout ) const;

	AkInt32 Shutdown( AkInt32 in_how ) const;
	AkInt32 Close();

	bool IsValid() const;

	AkUInt16 GetPort() const;
//	static AkInt32 Select( fd_set* in_readfds, fd_set* in_writefds, fd_set* exceptfds, const timeval* in_timeout );

private:
	SOCKET m_socket;
};
