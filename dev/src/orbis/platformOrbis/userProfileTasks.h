/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef __USER_PROFILE_TASKS_H__
#define __USER_PROFILE_TASKS_H__

#include <user_service.h>
#include <np/np_trophy.h>
#include <np/np_webapi.h>

class COrbisUserProfileTask : public CTask
{
public:
	COrbisUserProfileTask();
	virtual ~COrbisUserProfileTask();

	virtual void Run() override final;

private:
	virtual Bool Initialize() = 0;
	virtual Bool Main() = 0;
	virtual Bool Shutdown() = 0;

#ifndef NO_DEBUG_PAGES
private:
	virtual const Char* GetDebugName() const override;
	virtual Uint32 GetDebugColor() const override;
#endif // NO_DEBUG_PAGES
};

//////////////////////////////////////////////////////////////////////////

class COrbisTrophyTask : public COrbisUserProfileTask
{
public:
	COrbisTrophyTask( SceUserServiceUserId userId, SceNpTrophyContext& trophyContext );
	virtual ~COrbisTrophyTask() override;

private:
	virtual Bool Initialize() override final;
	virtual Bool Shutdown() override final;

protected:
	SceUserServiceUserId m_userId;
	SceNpTrophyContext& m_context;
	SceNpTrophyHandle m_handle;
};

//////////////////////////////////////////////////////////////////////////

class COrbisTrophyContextRegistrationTask : public COrbisTrophyTask
{
public:
	COrbisTrophyContextRegistrationTask( SceUserServiceUserId userId, SceNpTrophyContext& trophyContext );
	virtual ~COrbisTrophyContextRegistrationTask() override;

private:
	virtual Bool Main() override final;
};

//////////////////////////////////////////////////////////////////////////

class COrbisTrophyUnlockTask : public COrbisTrophyTask
{
public:
	COrbisTrophyUnlockTask( SceUserServiceUserId userId, SceNpTrophyContext& trophyContext, SceNpTrophyId trophyId );
	virtual ~COrbisTrophyUnlockTask() override;

private:
	virtual Bool Main() override final;

	SceNpTrophyId m_trophyId;
};

//////////////////////////////////////////////////////////////////////////

class COrbisPresenceTask : public COrbisUserProfileTask
{
public:
	COrbisPresenceTask( SceUserServiceUserId userId, Int32 webApiContextId, Uint32 localizedStringId );
	virtual ~COrbisPresenceTask();

private:
	virtual Bool Initialize() override final;
	virtual Bool Main() override final;
	virtual Bool Shutdown() override final;

	Uint32 CreateBody( AnsiChar* body, const Uint32 size );
	Bool CreateRequest( Uint32 contentSize );
	Bool VerifyRequestResponse( const SceNpWebApiResponseInformationOption& responseInfo );

	Bool UpdateScreenshotsDataWithPresence( const AnsiChar* presenceInUTF8 );

	enum class EInitStage
	{
		Start,
		OnlineId,
		WebApiContext,
		CreateRequest,
	};

	static const Uint32 MAX_PRESENCE_LENGTH = 128;

	Int64 m_requestId;
	const Uint32 m_localizedStringId;
	const SceUserServiceUserId m_userId;
	const Int32 m_webApiContextId;
	Int32 m_webApiUserContextId;
	SceNpOnlineId m_onlineId;
	EInitStage m_stage;
};

//////////////////////////////////////////////////////////////////////////

class COrbisPermissionCheckTask : public COrbisUserProfileTask
{
public:
	COrbisPermissionCheckTask( SceUserServiceUserId userId, Red::Threads::CAtomic< Bool >& canUseNetworkLibrary );
	virtual ~COrbisPermissionCheckTask();

private:
	virtual Bool Initialize() override final;
	virtual Bool Main() override final;
	virtual Bool Shutdown() override final;

	SceUserServiceUserId m_userId;
	Red::Threads::CAtomic< Bool >& m_canUseNetworkLibrary;
	
	Int32 m_requestId;
};

#endif // __USER_PROFILE_TASKS_H__
