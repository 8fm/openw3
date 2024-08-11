/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "userProfileTasks.h"
#include "orbisApiCall.h"

#include <np/np_webapi.h>

// For conversion of presence string to UTF-8
#include <ces.h>
#pragma comment( lib, "libSceCes.a" )

#include "../../common/engine/localizedContent.h"

// For the screenshot library
#include <screenshot.h>

COrbisUserProfileTask::COrbisUserProfileTask()
{

}

COrbisUserProfileTask::~COrbisUserProfileTask()
{

}

void COrbisUserProfileTask::Run()
{
	if( Initialize() )
	{
		Main();
	}

	Shutdown();
}

#ifndef NO_DEBUG_PAGES
const Char* COrbisUserProfileTask::GetDebugName() const
{
	return TXT( "CTrophyContextRegistrationTask" );
}

Uint32 COrbisUserProfileTask::GetDebugColor() const
{
	return Color::LIGHT_BLUE.ToUint32();
}

#endif // NO_DEBUG_PAGES

//////////////////////////////////////////////////////////////////////////

COrbisTrophyTask::COrbisTrophyTask( SceUserServiceUserId userId, SceNpTrophyContext& trophyContext )
:	m_userId( userId )
,	m_context( trophyContext )
,	m_handle( SCE_NP_TROPHY_INVALID_HANDLE )
{

}

COrbisTrophyTask::~COrbisTrophyTask()
{

}

Bool COrbisTrophyTask::Initialize()
{
	RED_FATAL_ASSERT( m_handle == SCE_NP_TROPHY_INVALID_HANDLE, "Trophy task handle already initialised" );

	ORBIS_SYS_CALL_RET( sceNpTrophyCreateHandle( &m_handle ) );

	return true;
}

Bool COrbisTrophyTask::Shutdown()
{
	if( m_handle != SCE_NP_TROPHY_INVALID_HANDLE )
	{
		ORBIS_SYS_CALL_RET( sceNpTrophyDestroyHandle( m_handle ) );
		m_handle = SCE_NP_TROPHY_INVALID_HANDLE;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

COrbisTrophyContextRegistrationTask::COrbisTrophyContextRegistrationTask( SceUserServiceUserId userId, SceNpTrophyContext& trophyContext )
:	COrbisTrophyTask( userId, trophyContext )
{

}

COrbisTrophyContextRegistrationTask::~COrbisTrophyContextRegistrationTask()
{

}

Bool COrbisTrophyContextRegistrationTask::Main()  
{
	ORBIS_SYS_CALL_RET( sceNpTrophyRegisterContext( m_context, m_handle, 0 ) );

	return true;
}

//////////////////////////////////////////////////////////////////////////

COrbisTrophyUnlockTask::COrbisTrophyUnlockTask( SceUserServiceUserId userId, SceNpTrophyContext& trophyContext, SceNpTrophyId trophyId )
:	COrbisTrophyTask( userId, trophyContext )
,	m_trophyId( trophyId )
{

}

COrbisTrophyUnlockTask::~COrbisTrophyUnlockTask()
{

}

Bool COrbisTrophyUnlockTask::Main()
{
	SceNpTrophyId platinumTrophyId;
	ORBIS_SYS_CALL_RET( sceNpTrophyUnlockTrophy( m_context, m_handle, m_trophyId, &platinumTrophyId ) );

	return true;
}

//////////////////////////////////////////////////////////////////////////

COrbisPresenceTask::COrbisPresenceTask( SceUserServiceUserId userId, Int32 webApiContextId, Uint32 localizedStringId )
:	m_requestId( 0 )
,	m_localizedStringId( localizedStringId )
,	m_userId( userId )
,	m_webApiContextId( webApiContextId )
,	m_webApiUserContextId( -1 )
,	m_stage( EInitStage::Start )
{
}

COrbisPresenceTask::~COrbisPresenceTask()
{

}

Bool COrbisPresenceTask::Initialize()
{
	ORBIS_SYS_CALL_RET( sceNpGetOnlineId( m_userId, &m_onlineId ) );
	m_stage = EInitStage::OnlineId;

	ORBIS_SYS_CALL_RET( m_webApiUserContextId = sceNpWebApiCreateContext( m_webApiContextId, &m_onlineId ) );
	m_stage = EInitStage::WebApiContext;

	return true;
}

Bool COrbisPresenceTask::Main()
{
	AnsiChar body[ MAX_PRESENCE_LENGTH ];
	const Uint32 size = CreateBody( body, MAX_PRESENCE_LENGTH );

	if( !CreateRequest( size ) )
	{
		return false;
	}

	SceNpWebApiResponseInformationOption responseInfo;
	ORBIS_SYS_CALL_RET( sceNpWebApiSendRequest2( m_requestId, body, size, &responseInfo ) );

	return VerifyRequestResponse( responseInfo );
}

Uint32 COrbisPresenceTask::CreateBody( AnsiChar* body, const Uint32 size )
{
#define PRESENCE_BODY_START	"{\"gameStatus\":\""
#define PRESENCE_BODY_END	"\"}"

	Red::System::StringCopy( body, PRESENCE_BODY_START, size );

	// Grab the presence string from the string database
	LocalizedString localizedPresenceString;
	localizedPresenceString.SetIndex( m_localizedStringId );
	String presenceString = localizedPresenceString.GetString();

	// Convert our UCS2 string to UTF8
	SceCesUcsContext utf8ConversionContext;
	ORBIS_SYS_CALL_RET( sceCesUcsContextInit( &utf8ConversionContext ) );

	const Uint32 startLength = Red::System::StringLengthCompileTime( PRESENCE_BODY_START );
	const Uint32 endLength = Red::System::StringLengthCompileTime( PRESENCE_BODY_END );

	Uint32 ucsLen = 0;
	Uint32 utf8Len = 0;
	ORBIS_SYS_CALL_RET
	(
		sceCesUcs2StrToUtf8Str
		(
			&utf8ConversionContext,
			
			reinterpret_cast< const Uint16* >( presenceString.AsChar() ),
			presenceString.Size(),
			&ucsLen,

			reinterpret_cast< Uint8* >( body + startLength ),
			MAX_PRESENCE_LENGTH - startLength,
			&utf8Len
		)
	);

	UpdateScreenshotsDataWithPresence( body + startLength );

	Red::System::StringConcatenate( body, PRESENCE_BODY_END, MAX_PRESENCE_LENGTH );

	return startLength + utf8Len + endLength + 1;
}

Bool COrbisPresenceTask::CreateRequest( Uint32 contentSize )
{
	// Create path string
	AnsiChar requestPath[ MAX_PRESENCE_LENGTH ];
	Red::System::SNPrintF( requestPath, MAX_PRESENCE_LENGTH, "/v1/users/%hs/presence/gameStatus", m_onlineId.data );

	// Fill in configuration parameters
	SceNpWebApiContentParameter	contentParam;
	Red::System::MemoryZero( &contentParam, sizeof( contentParam ) );
	contentParam.contentLength	= contentSize;
	contentParam.pContentType	= SCE_NP_WEBAPI_CONTENT_TYPE_APPLICATION_JSON_UTF8;

	// Make request
	ORBIS_SYS_CALL_RET( sceNpWebApiCreateRequest( m_webApiUserContextId, "userProfile", requestPath, SCE_NP_WEBAPI_HTTP_METHOD_PUT, &contentParam, &m_requestId ) );
	m_stage = EInitStage::CreateRequest;

	return true;
}

Bool COrbisPresenceTask::VerifyRequestResponse( const SceNpWebApiResponseInformationOption& responseInfo )
{
	const Int32 httpStatusCode = responseInfo.httpStatus;

	Int32 totalRead = 0;
	Int32 amountRead = 0;
	AnsiChar buf[ 2 * 1024 ];

	do
	{
		ORBIS_SYS_CALL_RET( amountRead = sceNpWebApiReadData( m_requestId, buf + totalRead, sizeof( buf ) - totalRead ) );

		totalRead += amountRead; 
	} while ( amountRead > 0 );

	// Let's assume we can receive any of the 2XX HTTP codes and treat them as a success
	return httpStatusCode >= 200 && httpStatusCode < 300;
}

Bool COrbisPresenceTask::Shutdown()
{
	switch( m_stage )
	{
	case EInitStage::CreateRequest:
		ORBIS_SYS_CALL( sceNpWebApiDeleteRequest( m_requestId ) );

	case EInitStage::WebApiContext:
		ORBIS_SYS_CALL( sceNpWebApiDeleteContext( m_webApiUserContextId ) );

	case EInitStage::OnlineId:
	case EInitStage::Start:
	default:
		{
		}
	}

	return true;
}

Bool COrbisPresenceTask::UpdateScreenshotsDataWithPresence( const AnsiChar* presenceInUTF8 )
{
	SceScreenShotParam param;

	param.thisSize = sizeof( SceScreenShotParam );

	param.photoTitle = presenceInUTF8;

	// Should come from param.sfo
	param.gameTitle = nullptr;

	// Can we get the current quest?
	param.gameComment = nullptr;

	// Not used
	param.reserved = nullptr;

	ORBIS_SYS_CALL_RET( sceScreenShotSetParam( &param ) );

	return true;
}

//////////////////////////////////////////////////////////////////////////

COrbisPermissionCheckTask::COrbisPermissionCheckTask( SceUserServiceUserId userId, Red::Threads::CAtomic< Bool >& canUseNetworkLibrary )
:	m_userId( userId )
,	m_canUseNetworkLibrary( canUseNetworkLibrary )
,	m_requestId( -1 )
{

}

COrbisPermissionCheckTask::~COrbisPermissionCheckTask()
{

}

Bool COrbisPermissionCheckTask::Initialize()
{
	 ORBIS_SYS_CALL_RET( m_requestId = sceNpCreateRequest() );

	 return true;
}

Bool COrbisPermissionCheckTask::Main()
{
	SceNpOnlineId onlineId;
	ORBIS_SYS_CALL_RET( sceNpGetOnlineId( m_userId, &onlineId ) );

	Int32 success = 0;
	ORBIS_SYS_CALL_RET( success = sceNpCheckNpAvailability( m_requestId, &onlineId, nullptr ) );

	m_canUseNetworkLibrary.SetValue( success == 0 );

	return true;
}

Bool COrbisPermissionCheckTask::Shutdown()
{
	ORBIS_SYS_CALL_RET( sceNpDeleteRequest( m_requestId ) );

	return true;
}
