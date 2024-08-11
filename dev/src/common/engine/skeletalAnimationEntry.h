
#pragma once

#include "slotAnimationShiftingInterval.h"
#include "extAnimEvent.h"
#include "skeletalAnimation.h"

class CSkeletalAnimationSet;
enum ECompressedPoseBlend : CEnum::TValueType;

//////////////////////////////////////////////////////////////////////////

class ISkeletalAnimationSetEntryParam : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( ISkeletalAnimationSetEntryParam, MC_Animation );

public:
	virtual Bool EditorOnly() const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( ISkeletalAnimationSetEntryParam )
	PARENT_CLASS( ISerializable )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

struct SEventGroupsRanges 
{
	DECLARE_RTTI_STRUCT( SEventGroupsRanges );

	CName m_tag;
	Uint32 m_beginIndex;
	Uint32 m_endIndex;

	SEventGroupsRanges() : m_beginIndex( 0 ), m_endIndex( 0 ){}
	SEventGroupsRanges(	const CName& tag, Uint32 beginIndex, Uint32 endIndex )
		: m_tag( tag )
		, m_beginIndex( beginIndex )
		, m_endIndex( endIndex )
		{}
};

BEGIN_CLASS_RTTI( SEventGroupsRanges )
	PROPERTY( m_tag );
	PROPERTY( m_beginIndex );
	PROPERTY( m_endIndex );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////


/// Skeletal animation in the skeletal animation set
class CSkeletalAnimationSetEntry : public ISerializable, public IEventsContainer
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_POOL( CSkeletalAnimationSetEntry, MemoryPool_SmallObjects, MC_Animation );

	friend class CSkeletalAnimationSet;
	friend class CSkeletalAnimationContainer;
	friend class CAnimationMap;

protected:
	CSkeletalAnimationSet*				m_animSet;
	CSkeletalAnimation*					m_animation;
	ECompressedPoseBlend				m_compressedPoseBlend;

	typedef TDynArray< ISkeletalAnimationSetEntryParam* > TAnimSetEntryParams;
	TAnimSetEntryParams					m_params;

	// Contains animation events.
	typedef TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter > TAnimSetEntryEvents;
	TAnimSetEntryEvents					m_events;

	CSkeletalAnimationSetEntry*			m_nextInGlobalMapList; // Next on the list stored in CAnimationMap::m_animations

	typedef TDynArray< SEventGroupsRanges > TAnimSetEventsGroupsRanges;
	TAnimSetEventsGroupsRanges			m_eventsGroupsRanges;
	
public:
	//! Get animation set containing this entry
	RED_INLINE CSkeletalAnimationSet* GetAnimSet() const { return m_animSet; }

	//! Get contained animation
	RED_INLINE CSkeletalAnimation* GetAnimation() const { return m_animation; }

	//! Get contained animation's duration
	RED_INLINE Float GetDuration() const { return m_animation ? m_animation->GetDuration() : 1.f; }

	//! Get contained animation's name
	RED_INLINE const CName& GetName() const { return m_animation ? m_animation->GetName() : CName::NONE; }

	//! Get compressed pose blend type
	RED_INLINE ECompressedPoseBlend GetCompressedPoseBlend() const { return m_compressedPoseBlend; }

	//! Gets all events for animation
	void GetAllEvents( TDynArray< CExtAnimEvent* >& events ) const;

	//! A helper method that retrieves all trajectory blend events defined in the animation
	void GetTrajectoryBlendPoints( TSortedArray< CSlotAnimationShiftingInterval >& outIntervals ) const;

#ifndef NO_EDITOR
	virtual void OnPostLoad() override;
#endif

public:
	CSkeletalAnimationSetEntry();
	virtual ~CSkeletalAnimationSetEntry();

	//! Bind animation entry to animation set
	virtual void Bind( CSkeletalAnimationSet* animationSet );

	//! ISerializable serialization
	virtual void OnSerialize( IFile& file );

	//! Set new animation
	void SetAnimation( CSkeletalAnimation *anim );

public:
	/// IEventsContainer interface
	virtual void AddEvent( CExtAnimEvent* event );
	virtual void RemoveEvent( CExtAnimEvent* event );
	virtual void GetEventsForAnimation( const CName& animName, TDynArray< CExtAnimEvent* >& events );
	virtual const CResource* GetParentResource() const;
	void AddEventGroup( const CName& tagName, TAnimSetEntryEvents& events );

public:
	Int32 FindEventGroupRangeIndex( const CName& tag );

	//! Gets event for given time range. It can fill CAnimationEventFired list or SBehaviorGraphOutput list or both
	void GetEventsByTime( Float prevTime, Float currTime, Int32 numLoops, Float alpha, TDynArray< CAnimationEventFired >* events, SBehaviorGraphOutput* output, const CName& tag = CName::NONE, Int32 eventGroupRangeIndex = -1 ) const;
	void GetEventsByTime( Float prevTime, Float currTime, Int32 numLoops, Float alpha, TDynArray< CAnimationEventFired >* events, SBehaviorGraphOutput* output, TAnimSetEntryEvents::const_iterator begin, TAnimSetEntryEvents::const_iterator end ) const;

	//! Get events of given type
	template < class EventType >
	void GetEventsOfType( TDynArray< EventType* >& list ) const;

	template < class EventType, class Collector >
	void GetEventsOfType( Collector& collector ) const;

	Uint32 GetNumEventsOfType( const CClass* c ) const;

public:
	void AddParam( ISkeletalAnimationSetEntryParam* param );

	Bool RemoveParam( const ISkeletalAnimationSetEntryParam* param );

	template< class T >
	Uint32 RemoveAllParamsByClass()
	{
		Uint32 count = 0;
		for ( Int32 i=m_params.SizeInt()-1; i>=0; --i )
		{
			const ISkeletalAnimationSetEntryParam* param = m_params[i];
			if ( param && param->GetClass()->IsA< T >() )
			{
				m_params.RemoveAt( i );
				count++;
			}
		}
		return count;
	}

	void ClearParams();

	const ISkeletalAnimationSetEntryParam* FindParamByClass( const CClass* c ) const;

	template< class T >
	const T* FindParam() const
	{
		const Uint32 size = m_params.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const ISkeletalAnimationSetEntryParam* param = m_params[i];
		
			if ( param && param->GetClass()->IsA< T >() )
			{
				return static_cast< const T* >( param );
			}
		}
		return NULL;
	}

	template< class T >
	class ParamsIterator
	{
		const CSkeletalAnimationSetEntry*	m_animation;
		Int32									m_index;

	public:
		ParamsIterator( const CSkeletalAnimationSetEntry* animation ) : m_index( -1 ), m_animation( animation )
		{
			Next();
		}

		ParamsIterator( const ParamsIterator& other ) : m_index( other.m_index ), m_animation( other.m_animation )
		{
			
		}

		RED_INLINE operator Bool() const
		{
			return ( m_index >= 0 ) && ( m_index < m_animation->m_params.SizeInt() );
		}

		RED_INLINE void operator++ ()
		{
			Next();
		}

		RED_INLINE T* operator*()
		{
			return static_cast< T* >( m_animation->m_params[ m_index ] );
		}

	private:
		RED_INLINE void Next()
		{
			while ( ++m_index < m_animation->m_params.SizeInt() )
			{
				if ( m_animation->m_params[ m_index ] && m_animation->m_params[ m_index ]->GetClass()->template IsA< T >() )
				{
					break;
				}
			}
		}
	};

	class EventsIterator
	{
		const CSkeletalAnimationSetEntry*	m_animation;

	public:
		TAnimSetEntryEvents::const_iterator	m_iterEvent;
		TAnimSetEntryEvents::const_iterator	m_iterEnd;

	public:
		EventsIterator( const CSkeletalAnimationSetEntry* animation ) : m_animation( animation ), m_iterEvent( m_animation->m_events.Begin() ), m_iterEnd( m_animation->m_events.End() )
		{
		}

		EventsIterator( const EventsIterator& other ) : m_animation( other.m_animation ), m_iterEvent( other.m_iterEvent ), m_iterEnd( other.m_iterEnd )
		{
		}

		RED_INLINE operator Bool() const
		{
			return m_iterEvent != m_iterEnd;
		}

		RED_INLINE void operator++ ()
		{
			if ( m_iterEvent != m_iterEnd )
			{
				++ m_iterEvent;
			}
		}

		RED_INLINE const CExtAnimEvent* operator*()
		{
			return *m_iterEvent;
		}
	};
};

BEGIN_CLASS_RTTI( CSkeletalAnimationSetEntry );
	PARENT_CLASS( ISerializable );
	PROPERTY_EDIT( m_animation, TXT("Animation") );
	PROPERTY_EDIT( m_compressedPoseBlend, TXT("") );
	PROPERTY( m_params );
	PROPERTY( m_eventsGroupsRanges );
END_CLASS_RTTI();
