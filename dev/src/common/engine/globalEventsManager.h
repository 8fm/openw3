/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../core/sortedmap.h"

//////////////////////////////////////////////////////////////////////////

enum EGlobalEventCategory
{
	GEC_Empty,
	GEC_Trigger,
	GEC_Tag,
	GEC_Fact,
	GEC_ScriptsCustom0,
	GEC_ScriptsCustom1,
	GEC_ScriptsCustom2,
	GEC_ScriptsCustom3,
	GEC_ScriptsCustom4,
	GEC_ScriptsCustom5,
	GEC_ScriptsCustom6,
	GEC_ScriptsCustom7,
	GEC_ScriptsCustom8,
	GEC_ScriptsCustom9,
	GEC_Last,
};

BEGIN_ENUM_RTTI( EGlobalEventCategory )
	ENUM_OPTION( GEC_Empty );
	ENUM_OPTION( GEC_Trigger );
	ENUM_OPTION( GEC_Tag );
	ENUM_OPTION( GEC_Fact );
	ENUM_OPTION( GEC_ScriptsCustom0 );
	ENUM_OPTION( GEC_ScriptsCustom1 );
	ENUM_OPTION( GEC_ScriptsCustom2 );
	ENUM_OPTION( GEC_ScriptsCustom3 );
	ENUM_OPTION( GEC_ScriptsCustom4 );
	ENUM_OPTION( GEC_ScriptsCustom5 );
	ENUM_OPTION( GEC_ScriptsCustom6 );
	ENUM_OPTION( GEC_ScriptsCustom7 );
	ENUM_OPTION( GEC_ScriptsCustom8 );
	ENUM_OPTION( GEC_ScriptsCustom9 );
	ENUM_OPTION( GEC_Last );
END_ENUM_RTTI()

enum EGlobalEventType
{
	GET_Unknown,
	GET_TriggerCreated,
	GET_TriggerRemoved,
	GET_TriggerActivatorCreated,
	GET_TriggerActivatorRemoved,
	GET_TagAdded,
	GET_TagRemoved,
	GET_StubTagAdded,
	GET_StubTagRemoved,
	GET_FactAdded,
	GET_FactRemoved,
	GET_ScriptsCustom0,
	GET_ScriptsCustom1,
	GET_ScriptsCustom2,
	GET_ScriptsCustom3,
};

BEGIN_ENUM_RTTI( EGlobalEventType )
	ENUM_OPTION( GET_Unknown );
	ENUM_OPTION( GET_TriggerCreated );
	ENUM_OPTION( GET_TriggerRemoved );
	ENUM_OPTION( GET_TriggerActivatorCreated );
	ENUM_OPTION( GET_TriggerActivatorRemoved );
	ENUM_OPTION( GET_TagAdded );
	ENUM_OPTION( GET_TagRemoved );
	ENUM_OPTION( GET_StubTagAdded );
	ENUM_OPTION( GET_StubTagRemoved );
	ENUM_OPTION( GET_FactAdded );
	ENUM_OPTION( GET_FactRemoved );
	ENUM_OPTION( GET_ScriptsCustom0 );
	ENUM_OPTION( GET_ScriptsCustom1 );
	ENUM_OPTION( GET_ScriptsCustom2 );
	ENUM_OPTION( GET_ScriptsCustom3 );
END_ENUM_RTTI()

//////////////////////////////////////////////////////////////////////////

struct SGlobalEventParam
{
protected:

	enum EGlobalEventParamType
	{
		GEPT_Unknown,
		GEPT_Entity,
		GEPT_Component,
		GEPT_Name,
		GEPT_String,
	};

	EGlobalEventParamType	m_type;

	SGlobalEventParam( EGlobalEventParamType type )
		: m_type( type )
	{}

	template < typename T >
	static EGlobalEventParamType GetParamType()
	{
		RED_ASSERT( false, TXT( "SGlobalEvenentParam not implemented in general case " ) );
		return GEPT_Unknown;
	}

public:

	virtual ~SGlobalEventParam()
	{}

	template < typename T >
	const T& Get() const
	{
		RED_ASSERT( false, TXT( "SGlobalEvenentParam not implemented in general case " ) );
		return T(0);
	}
};

template < typename T >
struct TGlobalEventParamImpl : public SGlobalEventParam
{
public:

	TGlobalEventParamImpl( const T& value )
		: SGlobalEventParam( GetParamType< T >() )
		, m_value( value )
	{}

private:

	T	m_value;

	const T& GetImpl() const
	{
		return m_value;
	}

	friend struct SGlobalEventParam;
};

template<>
RED_INLINE CEntity* const & SGlobalEventParam::Get< CEntity* >() const
{
	RED_ASSERT( m_type == GEPT_Entity );
	return static_cast< const TGlobalEventParamImpl< CEntity* > & >( *this ).GetImpl();
}

template<>
RED_INLINE CComponent* const & SGlobalEventParam::Get< CComponent* >() const
{
	RED_ASSERT( m_type == GEPT_Component );
	return static_cast< const TGlobalEventParamImpl< CComponent* > & >( *this ).GetImpl();
}

template<>
RED_INLINE const CName& SGlobalEventParam::Get< CName >() const
{
	RED_ASSERT( m_type == GEPT_Name );
	return static_cast< const TGlobalEventParamImpl< CName > & >( *this ).GetImpl();
}

template<>
RED_INLINE const String& SGlobalEventParam::Get< String >() const
{
	RED_ASSERT( m_type == GEPT_String );
	return static_cast< const TGlobalEventParamImpl< String > & >( *this ).GetImpl();
}

template <>
RED_INLINE SGlobalEventParam::EGlobalEventParamType SGlobalEventParam::GetParamType< CEntity* >()
{
	return GEPT_Entity;
}

template <>
RED_INLINE SGlobalEventParam::EGlobalEventParamType SGlobalEventParam::GetParamType< CComponent* >()
{
	return GEPT_Component;
}

template <>
RED_INLINE SGlobalEventParam::EGlobalEventParamType SGlobalEventParam::GetParamType< CName >()
{
	return GEPT_Name;
}

template <>
RED_INLINE SGlobalEventParam::EGlobalEventParamType SGlobalEventParam::GetParamType< String >()
{
	return GEPT_String;
}

//////////////////////////////////////////////////////////////////////////

template < typename T >
class TGlobalEventCategoryFilter
{
	typedef TSortedMap< T, Uint32 >	TFilters;
	TFilters	m_filters;

public:

	void Add( const T& filter )
	{
		typename TFilters::iterator it = m_filters.Find( filter );
		if ( it != m_filters.End() )
		{
			it->m_second++;
		}
		else
		{
			m_filters.Insert( filter, 1 );
		}
	}

	void Add( const TDynArray< T > & filters )
	{
		typename TDynArray< T >::const_iterator itEnd = filters.End();
		for ( typename TDynArray< T >::const_iterator it = filters.Begin(); it != itEnd; ++it )
		{
			Add( *it );
		}
	}

	void Remove( const T& filter )
	{
		typename TFilters::iterator it = m_filters.Find( filter );
		if ( it != m_filters.End() )
		{
			if ( it->m_second > 1 )
			{
				it->m_second++;
			}
			else
			{
				m_filters.Erase( it );
			}
		}
	}

	void Remove( const TDynArray< T > & filters )
	{
		typename TDynArray< T >::const_iterator itEnd = filters.End();
		for ( typename TDynArray< T >::const_iterator it = filters.Begin(); it != itEnd; ++it )
		{
			Remove( *it );
		}
	}

	Bool Check( const T& filter )
	{
		return m_filters.KeyExist( filter );
	}

	void Clear()
	{
		return m_filters.ClearFast();
	}

	template < typename U >
	void Add( const U& )
	{
		RED_ASSERT( false, TXT( "You're trying to use global events filter with wrong data type" ) );
	}

	template < typename U >
	void Remove( const U& )
	{
		RED_ASSERT( false, TXT( "You're trying to use global events filter with wrong data type" ) );
	}

	template < typename U >
	Bool Check( const U& )
	{
		RED_ASSERT( false, TXT( "You're trying to use global events filter with wrong data type" ) );
		return false;
	}

	template < typename U >
	void Clear( const U& )
	{
		RED_ASSERT( false, TXT( "You're trying to use global events filter with wrong data type" ) );
	}
};

//////////////////////////////////////////////////////////////////////////

template < EGlobalEventCategory Category, typename TFilter, typename TBaseSelector >
class TGlobalEventFilterSelector : public TBaseSelector
{
	TGlobalEventCategoryFilter< TFilter > m_filter;

public:

	template < typename TResult, typename TCaller, typename T >
	RED_INLINE TResult Call( EGlobalEventCategory category, const T& param )
	{
		if ( category == Category )
		{
			return TCaller::template Call< TGlobalEventCategoryFilter< TFilter >, T >( m_filter, param );
		}
		else
		{
			return TBaseSelector::template Call< TResult, TCaller, T >( category, param );
		}
	}

	template < typename TResult, typename TCaller >
	RED_INLINE TResult Call( EGlobalEventCategory category )
	{
		if ( category == Category )
		{
			return TCaller::template Call< TGlobalEventCategoryFilter< TFilter > >( m_filter );
		}
		else
		{
			return TBaseSelector::template Call< TResult, TCaller >( category );
		}
	}

	template < typename TResult, typename TCaller, typename T >
	RED_INLINE TResult CallAll( const T& param )
	{
		TCaller::template Call< TGlobalEventCategoryFilter< TFilter >, T >( m_filter, param );
		TBaseSelector::template CallAll< TResult, TCaller, T >( param );
	}

	template < typename TResult, typename TCaller >
	RED_INLINE TResult CallAll()
	{
		TCaller::template Call< TGlobalEventCategoryFilter< TFilter > >( m_filter );
		TBaseSelector::template CallAll< TResult, TCaller >();
	}
};

class CDummyFilterSelector
{
public:

	template < typename TResult, typename TCaller, typename T >
	RED_INLINE TResult Call( EGlobalEventCategory category, const T& param )
	{
		return TResult( 0 );
	}

	template < typename TResult, typename TCaller >
	RED_INLINE TResult Call( EGlobalEventCategory category )
	{
		return TResult( 0 );
	}

	template < typename TResult, typename TCaller, typename T >
	RED_INLINE TResult CallAll( const T& param )
	{
		return TResult( 0 );
	}

	template < typename TResult, typename TCaller >
	RED_INLINE TResult CallAll()
	{
		return TResult( 0 );
	}
};

//////////////////////////////////////////////////////////////////////////

class IGlobalEventsListener
{
public:

	virtual ~IGlobalEventsListener() { }
	virtual void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) = 0;
};

//////////////////////////////////////////////////////////////////////////

class CGlobalEventsReporter;

class CGlobalEventsManager
{
	struct CallAdd
	{
		template < typename TFilter, typename T >
		static void Call( TFilter& filter, const T& t )
		{
			filter.Add( t );
		}
	};

	struct CallRemove
	{
		template < typename TFilter, typename T >
		static void Call( TFilter& filter, const T& t )
		{
			filter.Remove( t );
		}
	};

	struct CallCheck
	{
		template < typename TFilter, typename T >
		static Bool Call( TFilter& filter, const T& t )
		{
			return filter.Check( t );
		}
	};

	struct CallClear
	{
		template < typename TFilter >
		static void Call( TFilter& filter )
		{
			return filter.Clear();
		}
	};

	class CListeners
	{
		typedef TDynArray< IGlobalEventsListener* >	TListeners;

		TListeners	m_listeners;
		TListeners	m_listenersToAdd;
		TListeners  m_listenersToRemove;
		Bool		m_areListening;

	public:

		typedef TDynArray< IGlobalEventsListener* >::iterator iterator;

		CListeners();
		~CListeners();

		//! Add listener (or cache for the future addition)
		Bool Add( IGlobalEventsListener* listener );

		//! Remove listener (or cache for the future removal)
		Bool Remove( IGlobalEventsListener* listener );

		//! Process cached additions/removals.
		Bool ProcessAddAndRemove();

		//! Mark listeners as "listening right now", and cannot be changed during that process
		void SetAreListening( Bool areListening );

		//! Clear listeners
		void Clear();

		//! Return the number of listeners, if "all" is true, cached listeners are also counted
		Uint32 Size( Bool all ) const;

		//! Check if listener is present on the list, if "all" is true, cached listeners are also checked
		Bool Exist( IGlobalEventsListener* listener, Bool all ) const;

		//! Return begin iterator
		RED_INLINE iterator Begin() { return m_listeners.Begin(); }

		//! Return end iterator
		RED_INLINE iterator End() { return m_listeners.End(); }
	};

public:

	CGlobalEventsManager();
	~CGlobalEventsManager();

	template < typename T >
	Bool SendEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const T& value );
	
	Bool AddListener( EGlobalEventCategory eventCategory, IGlobalEventsListener* listener );
	Bool RemoveListener( EGlobalEventCategory eventCategory, IGlobalEventsListener* listener );
	
	template < typename T >
	Bool AddFilteredListener( EGlobalEventCategory eventCategory, IGlobalEventsListener* listener, const T& filter );
	template < typename T >
	Bool RemoveFilteredListener( EGlobalEventCategory eventCategory, IGlobalEventsListener* listener, const T& filter );

	template < typename T >
	Bool AddFilterForListener( EGlobalEventCategory eventCategory, IGlobalEventsListener* listener, const T& filter );
	template < typename T >
	Bool RemoveFilterFromListener( EGlobalEventCategory eventCategory, IGlobalEventsListener* listener, const T& filter );

	Bool AddReporter( CGlobalEventsReporter* reporter );
	Bool RemoveReporter( CGlobalEventsReporter* reporter );

	TGlobalEventFilterSelector< GEC_Tag, CName, TGlobalEventFilterSelector< GEC_Fact, String, CDummyFilterSelector > >	m_filterSelector;

private:

	typedef TDynArray< CListeners >				TListenersByCategory;
	typedef TDynArray< CGlobalEventsReporter* >	TReporters;

	TListenersByCategory	m_listenersByCategory;
	TListenersByCategory	m_filteredListenersByCategory;
	TReporters				m_reporters;

	void ReportCategoryActivated( EGlobalEventCategory eventCategory );
	void ReportCategoryDeactivated( EGlobalEventCategory eventCategory );
};

//////////////////////////////////////////////////////////////////////////

class CGlobalEventsReporter
{
public:

	virtual ~CGlobalEventsReporter();

protected:

	CGlobalEventsReporter( EGlobalEventCategory eventsCategory );

	EGlobalEventCategory	m_category;
	Bool					m_registered;
	Bool					m_active;

	void Register();
	void OnCategoryActivated( EGlobalEventCategory eventCategory );
	void OnCategoryDeactivated( EGlobalEventCategory eventCategory );
	CGlobalEventsManager* GetGlobalEventsManager() const;

	friend class CGlobalEventsManager;
};

//////////////////////////////////////////////////////////////////////////

template < typename T >
struct TGlobalEventsReporterDefaultStorage
{
	typedef	T	TParamType;
	typedef T	TStorageType;

	static const TStorageType& ToStorageType( const TParamType& param )
	{
		return param;
	}

	static const TParamType& ToParamType( const TStorageType& storage )
	{
		return storage;
	}
};

template < typename T >
struct TGlobalEventsReporterStorageByHandle
{
	typedef	T*				TParamType;
	typedef THandle< T > 	TStorageType;

	static const TStorageType& ToStorageType( const TParamType& param )
	{
		static TStorageType storage;
		storage = param;
		return storage;
	}

	static const TParamType& ToParamType( const TStorageType& storage )
	{
		static TParamType param;
		param = storage.Get();
		return param;
	}
};

template < typename T, typename TStorageStrategy = TGlobalEventsReporterDefaultStorage< T > >
class TGlobalEventsReporterImpl : public CGlobalEventsReporter
{
public:

	TGlobalEventsReporterImpl( EGlobalEventCategory eventsCategory )
		: CGlobalEventsReporter( eventsCategory )
	{
	}

	~TGlobalEventsReporterImpl()
	{
		m_events.ClearFast();
	}

	Bool AddEvent( EGlobalEventType eventType, const T& param, Bool immediate = false );
	Bool ReportEvents();

private:

	typedef typename TStorageStrategy::TStorageType	TStorageType;

	typedef TPair< EGlobalEventType, TStorageType >	TEvent;
	typedef TDynArray< TEvent >						TEvents;
	TEvents		m_events;
};

//////////////////////////////////////////////////////////////////////////

template < typename T >
Bool CGlobalEventsManager::AddFilteredListener( EGlobalEventCategory eventCategory,
												IGlobalEventsListener* listener,
												const T& filter )
{
	Int32 index = static_cast< Int32 >( eventCategory );
	if ( m_filteredListenersByCategory[ index ].Add( listener ) )
	{
		m_filterSelector.Call< void, CallAdd >( eventCategory, filter );
		// if we're adding the first listener -> update reporters state for this category
		if ( m_filteredListenersByCategory[ index ].Size( true ) == 1 && m_listenersByCategory[ index ].Size( true ) == 0 )
		{
			ReportCategoryActivated( eventCategory );
		}
		return true;
	}
	return false;
}

template < typename T >
Bool CGlobalEventsManager::RemoveFilteredListener( EGlobalEventCategory eventCategory,
												   IGlobalEventsListener* listener,
												   const T& filter )
{
	Int32 index = static_cast< Int32 >( eventCategory );
	if ( m_filteredListenersByCategory[ index ].Remove( listener ) )
	{
		m_filterSelector.Call< void, CallRemove >( eventCategory, filter );
		// if we're removing the last listener -> update reporters state for this category
		if ( m_filteredListenersByCategory[ index ].Size( true ) == 0 && m_listenersByCategory[ index ].Size( true ) == 0  )
		{
			ReportCategoryDeactivated( eventCategory );
		}
		if ( m_filteredListenersByCategory[ index ].Size( true ) == 0 )
		{
			m_filterSelector.Call< void, CallClear >( eventCategory );
		}
		return true;
	}
	return false;
}

template < typename T >
Bool CGlobalEventsManager::AddFilterForListener( EGlobalEventCategory eventCategory,
											     IGlobalEventsListener* listener,
												 const T& filter )
{
	Int32 index = static_cast< Int32 >( eventCategory );
	if ( m_filteredListenersByCategory[ index ].Exist( listener, true ) )
	{
		m_filterSelector.Call< void, CallAdd >( eventCategory, filter );
		return true;
	}
	return false;
}

template < typename T >
Bool CGlobalEventsManager::RemoveFilterFromListener( EGlobalEventCategory eventCategory,
											  	     IGlobalEventsListener* listener,
												     const T& filter )
{
	Int32 index = static_cast< Int32 >( eventCategory );
	if ( m_filteredListenersByCategory[ index ].Exist( listener, true ) )
	{
		m_filterSelector.Call< void, CallRemove >( eventCategory, filter );
		return true;
	}
	return false;
}

template < typename T >
Bool CGlobalEventsManager::SendEvent( EGlobalEventCategory eventCategory,
									  EGlobalEventType eventType,
									  const T& value )
{
	PC_SCOPE_PIX( GlobalEvenetsManager_SendEvent );

	Bool res = false;
	Int32 index = static_cast< Int32 >( eventCategory );
	if ( m_listenersByCategory[ index ].Size( false ) > 0 )
	{
		CListeners& listeners = m_listenersByCategory[ index ];
		listeners.SetAreListening( true );

		TGlobalEventParamImpl< T > param = TGlobalEventParamImpl< T >( value );
		CListeners::iterator itEnd = listeners.End();
		for ( CListeners::iterator it = listeners.Begin(); it != itEnd; ++it )
		{
			(*it)->OnGlobalEvent( eventCategory, eventType, param );
		}

		listeners.SetAreListening( false );
		listeners.ProcessAddAndRemove();

		res = true;
	}

	if ( m_filteredListenersByCategory[ index ].Size( false ) > 0 )
	{
		if ( m_filterSelector.Call< Bool, CallCheck >( eventCategory, value ) )
		{
			CListeners& listeners = m_filteredListenersByCategory[ index ];
			listeners.SetAreListening( true );

			TGlobalEventParamImpl< T > param = TGlobalEventParamImpl< T >( value );
			CListeners::iterator itEnd = listeners.End();
			for ( CListeners::iterator it = listeners.Begin(); it != itEnd; ++it )
			{
				(*it)->OnGlobalEvent( eventCategory, eventType, param );
			}

			listeners.SetAreListening( false );
			listeners.ProcessAddAndRemove();

		}
		res = true;
	}

	return res;
}

//////////////////////////////////////////////////////////////////////////

template < typename T, typename TStorageStrategy >
Bool TGlobalEventsReporterImpl< T, TStorageStrategy >::AddEvent( EGlobalEventType eventType,
																 const T& param,
																 Bool immediate /* = false */ )
{
	if ( !m_registered )
	{
		Register();
	}
	if ( !m_active )
	{
		return false;
	}
	if ( immediate )
	{
		PC_SCOPE( SendGlobalEvents );
		CGlobalEventsManager* globalEventsManager = GetGlobalEventsManager();
		RED_ASSERT( globalEventsManager != nullptr );
		if ( globalEventsManager != nullptr )
		{
			globalEventsManager->SendEvent( m_category, eventType, param );
		}
	}
	else
	{
		m_events.PushBack( TEvent( eventType, TStorageStrategy::ToStorageType( param ) ) );
	}
	return true;
}

template < typename T, typename TStorageStrategy >
Bool TGlobalEventsReporterImpl< T, TStorageStrategy >::ReportEvents()
{
	if ( m_active && m_events.Size() > 0 )
	{
		CGlobalEventsManager* globalEventsManager = GetGlobalEventsManager();
		RED_ASSERT( globalEventsManager != nullptr );
		if ( globalEventsManager != nullptr )
		{
			PC_SCOPE( SendGlobalEvents );
			typename TEvents::iterator itEnd = m_events.End();
			for ( typename TEvents::iterator it = m_events.Begin(); it != itEnd; ++it )
			{				
				// if false is returned stop sending events
				if ( !globalEventsManager->SendEvent( m_category, it->m_first, TStorageStrategy::ToParamType( it->m_second ) ) )
				{
					break;
				}
			}
			m_events.ClearFast();
			return true;
		}
	}
	m_events.ClearFast();
	return false;
}
