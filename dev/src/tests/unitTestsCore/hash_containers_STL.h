#pragma once

#undef min
#undef max
#include <memory>
#include <cstdlib>
#include <iostream>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>

static Uint32 s_stdMemoryAllocated = 0;

// Hash functions

template<>
struct std::hash< String >
{
	RED_FORCE_INLINE size_t operator() (const String& value) const
	{
		return ( size_t ) value.CalcHash();
	}
};

template<>
struct std::hash< CGUID >
{
	RED_FORCE_INLINE size_t operator() (const CGUID& value) const
	{
		return ( size_t ) value.CalcHash();
	}
};

// STL allocator

template<typename T>
class my_allocator{
public :
	//    typedefs

	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

public :
	//    convert an allocator<T> to allocator<U>

	template<typename U>
	struct rebind {
		typedef my_allocator<U> other;
	};

public :
	inline explicit my_allocator() {}
	inline ~my_allocator() {}
	inline explicit my_allocator(my_allocator const&) {}
	template<typename U>
	inline explicit my_allocator(my_allocator<U> const&) {}

	//    address

	inline pointer address(reference r) { return &r; }
	inline const_pointer address(const_reference r) { return &r; }

	//    memory allocation

	inline pointer allocate(size_type count,
		typename std::allocator<void>::const_pointer = 0) {
			const size_t n = count * sizeof( T );
			pointer p = ( pointer ) malloc( n );
			s_stdMemoryAllocated += (Uint32) n;
			return p;
	}
	inline void deallocate(pointer p, size_type n) {
		free( p );
		s_stdMemoryAllocated -= (Uint32) ( n * sizeof(T) );
	}
	//    size
	inline size_type max_size() const {
		return std::numeric_limits<size_type>::max() / sizeof(T);
	}

	//    construction/destruction

	inline void construct(pointer p, const T& t) {
		new(p) T(t);
	}
	inline void destroy(pointer p) {
		p->~T();
	}

	inline bool operator==(my_allocator const&) { return true; }
	inline bool operator!=(my_allocator const& a) { return !operator==(a); }
};    //    end of class my_allocator

// Maps

template < typename K, typename V, typename STLMAP >
class STLBaseMap
{
private:
	STLMAP map;

public:
	RED_FORCE_INLINE typename STLMAP::iterator Begin()
	{
		return map.begin();
	}
	RED_FORCE_INLINE typename STLMAP::iterator End()
	{
		return map.end();
	}

	RED_FORCE_INLINE void Reserve( Uint32 capacity )
	{
		map.reserve( capacity );
	}

	RED_FORCE_INLINE TMemSize GetInternalMemSize() const
	{
		return s_stdMemoryAllocated;
	}

	RED_FORCE_INLINE Bool Insert( const K& key, const V& value )
	{
		auto it = map.find( key );
		if ( it == map.end() )
		{
			map.insert( std::pair< K, V >( key, value ) );
			return true;
		}
		return false;
	}

	RED_FORCE_INLINE Bool Erase( const K& key )
	{
		auto it = map.find( key );
		if ( it != map.end() )
		{
			map.erase( it );
			return true;
		}
		return false;
	}

	RED_FORCE_INLINE Bool KeyExist( const K& key ) const
	{
		auto it = map.find( key );
		return it != map.end();
	}
};

template < typename K, typename V >
class STLUnorderedMap : public STLBaseMap< K, V, std::unordered_map< K, V, std::hash< K >, std::equal_to< K >, my_allocator< std::pair< K, V > > > >
{
};

template < typename K, typename V >
class STLMap : public STLBaseMap< K, V, std::map< K, V, std::less< K >, my_allocator< std::pair< K, V > > > >
{
public: RED_FORCE_INLINE void Reserve( Uint32 capacity ) {}
};

// Set

template < typename K, typename STLSET >
class STLBaseSet
{
protected:
	STLSET set;

public:
	RED_FORCE_INLINE typename STLSET::iterator Begin()
	{
		return set.begin();
	}
	RED_FORCE_INLINE typename STLSET::iterator End()
	{
		return set.end();
	}

	RED_FORCE_INLINE void Reserve( Uint32 capacity )
	{
		set.reserve( capacity );
	}

	RED_FORCE_INLINE TMemSize GetInternalMemSize() const
	{
		return s_stdMemoryAllocated;
	}

	RED_FORCE_INLINE Bool Insert( const K& key )
	{
		auto it = set.find( key );
		if ( it == set.end() )
		{
			set.insert( key );
			return true;
		}
		return false;
	}

	RED_FORCE_INLINE Bool Erase( const K& key )
	{
		auto it = set.find( key );
		if ( it != set.end() )
		{
			set.erase( it );
			return true;
		}
		return false;
	}

	RED_FORCE_INLINE Bool Exist( const K& key ) const
	{
		auto it = set.find( key );
		return it != set.end();
	}
};

template < typename K >
class STLUnorderedSet : public STLBaseSet< K, std::unordered_set< K, std::hash< K >, std::equal_to< K >, my_allocator< K > > >
{
};

template < typename K >
class STLSet : public STLBaseSet< K, std::set< K, std::less< K >, my_allocator< K > > >
{
public: RED_FORCE_INLINE void Reserve( Uint32 capacity ) {}
};