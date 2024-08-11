/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include <algorithm>

#include "types.h"
#include "../redThreads/redThreadsThread.h"

//////////////////////////////////////////////////////////////////////////

// Moved here from system.h (as CSystem is no more!)
namespace CSystem
{
	const Uint32 DEFAULT_ALIGNMENT = 16;
}

typedef void (*AtExitFnType)();

void AtExitFn();

void* PolicyMemoryAllocate( size_t size, size_t align );
void PolicyMemoryFree( void* mem );

class CLifetimeTracker;

class CTrackerArray 
{
public:
	typedef CLifetimeTracker** CTrackers;

	static const Uint32	NUM_TRACKED_SINGLETONS = 32;

	CLifetimeTracker*	m_trackers[NUM_TRACKED_SINGLETONS];
	Uint32				m_elements;

	CTrackerArray()
		: m_elements(0)
	{
		Red::System::MemorySet(m_trackers, 0, sizeof(m_trackers));
	}
};

extern CTrackerArray* GTrackerArray;

class CLifetimeTracker
{
private:
	Uint32	m_longevity;

public:
	CLifetimeTracker(Uint32 longevity)
		: m_longevity(longevity)
	{
	}

	virtual ~CLifetimeTracker() = 0;

	static Bool Compare(const CLifetimeTracker* tracker1, const CLifetimeTracker* tracker2)
	{
		return tracker1->m_longevity > tracker2->m_longevity;
	}
};

RED_INLINE CLifetimeTracker::~CLifetimeTracker()
{
}

template <typename T, typename Destroyer>
class CConcreteLifetimeTracker : public CLifetimeTracker
{
public:
	CConcreteLifetimeTracker(T* p,unsigned int longevity, Destroyer d)
		: CLifetimeTracker(longevity)
		, m_tracked(p)
		, m_destroyer(d)
	{
	}

	~CConcreteLifetimeTracker()
	{ 
		m_destroyer(m_tracked); 
	}

private:
	T*			m_tracked;
	Destroyer	m_destroyer;
};
/************************************************************************/
/* threading policy                                                     */
/************************************************************************/
template<class T>
struct TSingleThreaded
{
	typedef T VolatileType;

	struct Lock
	{
		Lock() {}
		explicit Lock(const TSingleThreaded&) {}
		explicit Lock(const TSingleThreaded*) {}
	};
};

template<class T>
struct TMultiThreadedClassLvl
{
	typedef volatile T VolatileType;

	struct Initializer
	{  
		bool					m_initialized;
		Red::Threads::CMutex	m_mutex;

		Initializer() : m_initialized(false)
		{
			m_initialized = true;
		}

		~Initializer()
		{
			RED_ASSERT(m_initialized);
		}
	};

	static struct Initializer &initializer()
	{
		static struct Initializer i;
		return i;
	}

	struct Lock
	{
		Lock() 
		{
			RED_ASSERT(initializer().m_initialized);
			initializer().m_mutex.Acquire();
		}

		explicit Lock(const TMultiThreadedClassLvl&) 
		{
			RED_ASSERT(initializer().m_initialized);
			initializer().m_mutex.Acquire();
		}

		explicit Lock(const TMultiThreadedClassLvl*) 
		{
			RED_ASSERT(initializer().m_initialized);
			initializer().m_mutex.Acquire();		
		}

		~Lock()
		{
			RED_ASSERT(initializer().m_initialized);
			initializer().m_mutex.Release();
		}
	};
};

/************************************************************************/
/* lifetime policy                                                      */
/************************************************************************/
template<class T>
struct TDefaultLifetime
{
	static void ScheduleDestruction(T*, AtExitFnType atExitFn)
	{
		atexit(atExitFn);
	}

	static void OnDeadReference()
	{
		RED_HALT("Dead reference detected!");
	}
};

template<class T>
struct TNoDestructionLifetime
{
	static void ScheduleDestruction(T*, AtExitFnType atExitFn)
	{
		RED_UNUSED( atExitFn );
	}

	static void OnDeadReference()
	{
	}
};

template<class T>
struct TPheonixLifetime
{
	static void ScheduleDestruction(T*, AtExitFnType atExitFn)
	{
		if (!m_destructionScheduled)
			atexit(atExitFn);
	}

	static void OnDeadReference()
	{
	}

private:
	static Bool	m_destructionScheduled;
};

template<class T> Bool TPheonixLifetime<T>::m_destructionScheduled = false;

template <class T>
struct Adapter
{
	void operator()(T*) { return atExitFn(); }
	AtExitFnType atExitFn;
};

template<Uint32 Longevity, class T>
struct TLongevityLifetime
{
	virtual ~TLongevityLifetime() {}

	static void ScheduleDestruction(T* p, AtExitFnType atExitFn)
	{
		Adapter<T> adapter = { atExitFn };
		SetLongevity(p, Longevity, adapter);
	}

	static void OnDeadReference()
	{
		RED_HALT("Dead reference detected!");
	}
};

template<class T>
struct TDieFirstLifetime : public TLongevityLifetime<0, T>
{
};

template<class T>
struct TDieBeforeLastLifetime : public TLongevityLifetime<Uint32(~0-1), T>
{
};

template<class T>
struct TDieLastLifetime : public TLongevityLifetime<Uint32(~0), T>
{
};

template <typename T, typename Destroyer>
void SetLongevity(T* pDynObject, Uint32 longevity, Destroyer d)
{
	typedef CConcreteLifetimeTracker<T, Destroyer> tLifetimeTracker;

	void* ptr = PolicyMemoryAllocate( sizeof(tLifetimeTracker), CSystem::DEFAULT_ALIGNMENT );

	CLifetimeTracker* p = new (ptr) tLifetimeTracker(pDynObject, longevity, d);

	if (GTrackerArray == NULL)
	{
		void* ptr = PolicyMemoryAllocate( sizeof(CTrackerArray), CSystem::DEFAULT_ALIGNMENT );

		GTrackerArray = new (ptr) CTrackerArray;
	}

	CTrackerArray::CTrackers trackers = GTrackerArray->m_trackers;
	Uint32 &elems = GTrackerArray->m_elements;

	// These uses of the standard library need to be fixed - do not reference the standard library directly (this is why _SCL_SECURE_NO_WARNINGS is defined in core.h
	// Insert a pointer to the object into the queue
	CTrackerArray::CTrackers pos = std::upper_bound
		( trackers
		, trackers + elems
		, p
		, CLifetimeTracker::Compare);

	std::copy_backward
		( pos
		, trackers + elems
		, trackers + elems + 1);

	*pos = p;
	++elems;

	// Register a call to AtExitFn
	atexit(AtExitFn);
}

template <typename T>
struct TDeleter
{
	typedef void (*Type)(T*);
	static void Delete(T* pObj)
	{ 
		delete pObj; 
	}
};

template <typename T>
void SetLongevity(T* pDynObject, unsigned int longevity, typename TDeleter<T>::Type d = TDeleter<T>::Delete)
{
	SetLongevity<T, typename TDeleter<T>::Type>(pDynObject, longevity, d);
}

/************************************************************************/
/* creation policy                                                      */
/************************************************************************/
template<class T>
struct TCreateUsingNew
{
	static T* Create()
	{
		return new T;
	}

	static void Destroy(T* p)
	{
		delete p;
	}
};

template<class T> 
struct TCreateUsingMalloc
{
	static T* Create()
	{
		void* p = PolicyMemoryAllocate( sizeof(T), CSystem::DEFAULT_ALIGNMENT );
		if (!p) return 0;
		return new(p) T;
	}

	static void Destroy(T* p)
	{
		p->~T();
		PolicyMemoryFree(p);
	}
};

template < typename SingletonType > struct TCreateStatic
{
#ifdef _MSC_VER
RED_WARNING_PUSH() 
RED_DISABLE_WARNING_MSC( 4121 )// alignment of a member was sensitive to packing
#endif // _MSC_VER

	union MaxAlign
	{
		char t_[ sizeof( SingletonType ) ];
		short int shortInt_;
		int int_;
		long int longInt_;
		float float_;
		double double_;
		long double longDouble_;
		struct Test;
		int Test::* pMember_;
		int (Test::*pMemberFn_)(int);
	};

#ifdef _MSC_VER
RED_WARNING_POP()
#endif // _MSC_VER

	static SingletonType* Create()
	{
		static MaxAlign staticT;
		return new(&staticT) SingletonType;
	}

	// If you've got a warning about "p" being unused, you've forgotten to define a destructor in your class
	static void Destroy(SingletonType* p)
	{
		p->~SingletonType();
	}
};
/************************************************************************/
/* singleton                                                            */
/************************************************************************/
template
	< typename ObjectType
	, template <class> class LifetimePolicy = TDefaultLifetime
	, template <class> class CreationPolicy = TCreateStatic
	, template <class> class ThreadingModel = TMultiThreadedClassLvl>
class TSingleton
{
public:
	RED_INLINE static ObjectType& GetInstance()
	{
		if (!s_instance)
		{
			OnCreate();
		}

		return *s_instance;
	}

private:
	// private constructor
	TSingleton();

	static void OnCreate()
	{
		typename ThreadingModel<ObjectType>::Lock guard;

		if (!s_instance)
		{
			if (s_destroyed)
			{
				s_destroyed = false;
				LifetimePolicy<ObjectType>::OnDeadReference();
			}

			s_instance = CreationPolicy<ObjectType>::Create();

			LifetimePolicy<ObjectType>::ScheduleDestruction(s_instance, OnDestroy);
		}
	}

	static void OnDestroy()
	{
		RED_ASSERT(s_instance);

		CreationPolicy<ObjectType>::Destroy(s_instance);

		s_instance	= NULL;
		s_destroyed	= true;
	}

	typedef typename ThreadingModel<ObjectType*>::VolatileType InstancePtrType;

	static InstancePtrType	s_instance;
	static Bool				s_destroyed;
};

template
< typename ObjectType
, template <class> class LifetimePolicy
, template <class> class CreationPolicy
, template <class> class ThreadingModel>
typename TSingleton<ObjectType, LifetimePolicy, CreationPolicy, ThreadingModel>::InstancePtrType
TSingleton<ObjectType, LifetimePolicy, CreationPolicy, ThreadingModel>::s_instance;

template
< typename ObjectType
, template <class> class LifetimePolicy
, template <class> class CreationPolicy
, template <class> class ThreadingModel>
Bool 
TSingleton<ObjectType, LifetimePolicy, CreationPolicy, ThreadingModel>::s_destroyed;
