/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifdef USE_PHYSX

#include "../physics/physicsIncludes.h"
#include "../core/task.h"

class CPhysXCpuDispatcher : public physx::PxCpuDispatcher, private Red::System::NonCopyable
{
private:
	class CPhysXTask : public CTask
	{
		friend class CPhysXCpuDispatcher;
	private:
		physx::PxBaseTask& m_physXTask;
#ifndef NO_DEBUG_PAGES
		const char* m_namePtr;
#endif

	public:
#ifndef NO_DEBUG_PAGES
		CPhysXTask( physx::PxBaseTask& physXTask, Bool isCritical );
#else
		CPhysXTask( physx::PxBaseTask& physXTask );
#endif

		~CPhysXTask();
	public:
		void				Run() override;

	public:
#ifndef NO_DEBUG_PAGES
		Bool				m_isCritical;
	public:
		virtual const Char* GetDebugName() const override { return ANSI_TO_UNICODE( m_namePtr ); }
		virtual Uint32		GetDebugColor() const override { return m_isCritical ? Color::LIGHT_YELLOW.ToUint32() : Color::LIGHT_GREEN.ToUint32(); }

#endif
	};

	ETaskSchedulePriority m_priority;
	Red::Threads::CAtomic< CPhysXTask* > m_collectedTask;
	Red::Threads::CAtomic< CPhysXTask* > m_beginTask;

public:
	CPhysXCpuDispatcher( ETaskSchedulePriority priority ) : m_priority( priority ), m_beginTask( nullptr ) {}
	virtual void			submitTask( physx::PxBaseTask& task ) override;
	virtual physx::PxU32	getWorkerCount() const override;

	void					ProcessCollectedTask();

	void					StartBeginTask();

};

#endif // USE_PHYSX