#pragma once

////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design 
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any 
//     purpose is hereby granted without fee, provided that the above copyright 
//     notice appear in all copies and that both that copyright notice and this 
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the 
//     suitability of this software for any purpose. It is provided "as is" 
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

/*
The code below is slightly modified version of the code from Loki library.
In order not to bloat the code with TypeLists and TypeTraits, there is no generic implementation of a functor with arbitrary number of parameters.
Instead, several template specifications (for arbitrary number of parameters) were implemented.
*/

// base implementation for functors
namespace Private
{
	template< typename R >
	struct FunctorImplBase
	{
		RED_INLINE FunctorImplBase()
			: m_refCount( 1 )
		{}

		RED_INLINE FunctorImplBase( const FunctorImplBase& other )
			: m_refCount( other.m_refCount )
		{}

		virtual ~FunctorImplBase()
		{
			ASSERT( m_refCount == 0 );
		}

		void AddRef()
		{
			++m_refCount;
		}

		void Release()
		{
			ASSERT( m_refCount > 0 );
			--m_refCount;
			if ( m_refCount == 0 )
			{
				DestroySelfIfRefCountZero();
			}
		}

		RED_INLINE Bool IsRefCountZero() const
		{
			return 0 == m_refCount;
		}

		void DestroySelfIfRefCountZero()
		{
			ASSERT( IsRefCountZero() );
			delete this;
		}

		typedef R ResultType;
		typedef FunctorImplBase< R > FunctorImplBaseType;

		virtual FunctorImplBase* DoClone() const = 0;

		template < class U >
		static U* Clone( U* obj )
		{
			if ( !obj )
			{
				return NULL;
			}
			U* clone = static_cast< U* >( obj->DoClone() );
			ASSERT( typeid( *clone ) == typeid( *obj ) );
			return clone;
		}

		virtual Bool operator== ( const FunctorImplBase& other ) const = 0;

		Uint32 m_refCount;
	};
}

#define DEFINE_CLONE_FUNCTORIMPL( ClassName ) \
	virtual ClassName* DoClone() const { return new ClassName( * this ); }

//////////////////////////////////////////////////////////////////////////
// Functor with 0 parameters
//////////////////////////////////////////////////////////////////////////

template< typename R >
class FunctorImpl0 : public Private::FunctorImplBase< R >
{
public:
	typedef R ResultType;
	virtual R operator()() = 0;
};

template < class ParentFunctor, typename Fun >
class FunctorHandler0 : public ParentFunctor::Impl
{
	typedef typename ParentFunctor::Impl	Base;

public:
	typedef typename Base::ResultType		ResultType;

public:
	FunctorHandler0( const Fun& fun ) : m_fun( fun ) {}

	DEFINE_CLONE_FUNCTORIMPL( FunctorHandler0 );

	Bool operator== ( const typename Base::FunctorImplBaseType& rhs ) const
	{
		if ( typeid( *this ) != typeid( rhs ) )
		{
			return false;
		}

		const FunctorHandler0& fh = static_cast< const FunctorHandler0& >( rhs );

		// if this line gives a compiler error, you are using a function object.
		// you need to implement bool MyFnObj::operator == (const MyFnObj&) const;
		return m_fun == fh.m_fun;
	}

	ResultType operator() ()						{ return m_fun();			}

private:
	Fun m_fun;
};

template< class ParentFunctor, typename PointerToObj, typename PointerToMemberFunction >
class MemberFunctionHandler0 : public ParentFunctor::Impl
{
	typedef typename ParentFunctor::Impl	Base;

public:
	typedef typename Base::ResultType		ResultType;

public:
	MemberFunctionHandler0( const PointerToObj& obj, PointerToMemberFunction memberFunction ) 
		: m_obj( obj )
		, m_memberFunction( memberFunction )
	{}

	DEFINE_CLONE_FUNCTORIMPL( MemberFunctionHandler0 );

	Bool operator==( const typename Base::FunctorImplBaseType& rhs ) const
	{
		if( typeid( *this ) != typeid( rhs ) )
		{
			return false;
		}

		const MemberFunctionHandler0& mfh = static_cast< const MemberFunctionHandler0& >( rhs );
		return m_obj == mfh.m_obj && m_memberFunction == mfh.m_memberFunction;
	}

	ResultType operator() ()						{ return ( ( *m_obj ).*m_memberFunction )();			}

private:
	PointerToObj			m_obj;
	PointerToMemberFunction m_memberFunction;
};

template< typename R >
class Functor0
{
public:
	typedef FunctorImpl0< R > Impl;
	typedef R ResultType;

public:
	Functor0()
		: m_impl( NULL )
	{}

	Functor0( const Functor0& rhs )
		: m_impl( rhs.m_impl )
	{
		m_impl->AddRef();
	}

	Functor0( Impl* impl )
		: m_impl( impl )
	{
		m_impl->AddRef();
	}

	template <typename Fun>
	Functor0( Fun fun )
		: m_impl( new FunctorHandler0< Functor0, Fun >( fun ) )
	{}

	template < class PtrObj, typename MemberFunction >
	Functor0( const PtrObj& p, MemberFunction memberFunction )
		: m_impl( new MemberFunctionHandler0< Functor0, PtrObj, MemberFunction >( p, memberFunction ) )
	{}

	virtual ~Functor0()
	{
		ASSERT( m_impl );
		m_impl->Release();
	}

	Functor0& operator=( Functor0 const& rhs )
	{
		if ( this != &rhs )
		{
			m_impl = rhs.m_impl;
			m_impl->AddRef();
		}
		return *this;
	};

	Bool operator==( const Functor0& rhs ) const
	{
		if ( !m_impl && !rhs.m_impl )
		{
			return true;
		}
		if ( m_impl && rhs.m_impl )
		{
			return *m_impl == *rhs.m_impl;
		}
		else
		{
			return false;
		}
	}

	Bool operator!=( const Functor0& rhs ) const
	{
		return !( *this == rhs );
	}

	ResultType operator() () const { return ( *m_impl )(); }

	void AddRef()	{ if ( m_impl ) m_impl->AddRef();	}
	void Release()	{ if ( m_impl ) m_impl->Release();	}

private:
	Impl* m_impl;
};

//////////////////////////////////////////////////////////////////////////
// Functor with 1 parameter
//////////////////////////////////////////////////////////////////////////

template< typename R, typename P1 >
class FunctorImpl1 : public Private::FunctorImplBase< R >
{
public:
	typedef R ResultType;
	typedef P1 Param1;
	virtual R operator()( Param1 ) = 0;
};

template < class ParentFunctor, typename Fun >
class FunctorHandler1 : public ParentFunctor::Impl
{
	typedef typename ParentFunctor::Impl	Base;

public:
	typedef typename Base::ResultType		ResultType;
	typedef typename Base::Param1			Param1;

public:
	FunctorHandler1( const Fun& fun ) : m_fun( fun ) {}

	DEFINE_CLONE_FUNCTORIMPL( FunctorHandler1 );

	Bool operator== ( const typename Base::FunctorImplBaseType& rhs ) const
	{
		if ( typeid( *this ) != typeid( rhs ) )
		{
			return false;
		}

		const FunctorHandler1& fh = static_cast< const FunctorHandler1& >( rhs );

		// if this line gives a compiler error, you are using a function object.
		// you need to implement bool MyFnObj::operator == (const MyFnObj&) const;
		return m_fun == fh.m_fun;
	}

	ResultType operator() ( Param1 p1 )				{ return m_fun( p1 );		}

private:
	Fun m_fun;
};

template< class ParentFunctor, typename PointerToObj, typename PointerToMemberFunction >
class MemberFunctionHandler1 : public ParentFunctor::Impl
{
	typedef typename ParentFunctor::Impl	Base;

public:
	typedef typename Base::ResultType		ResultType;
	typedef typename Base::Param1			Param1;

public:
	MemberFunctionHandler1( const PointerToObj& obj, PointerToMemberFunction memberFunction ) 
		: m_obj( obj )
		, m_memberFunction( memberFunction )
	{}

	DEFINE_CLONE_FUNCTORIMPL( MemberFunctionHandler1 );

	Bool operator==( const typename Base::FunctorImplBaseType& rhs ) const
	{
		if( typeid( *this ) != typeid( rhs ) )
		{
			return false;
		}

		const MemberFunctionHandler1& mfh = static_cast< const MemberFunctionHandler1& >( rhs );
		return m_obj == mfh.m_obj && m_memberFunction == mfh.m_memberFunction;
	}

	ResultType operator() ( Param1 p1 )				{ return ( ( *m_obj ).*m_memberFunction )( p1 );		}

private:
	PointerToObj			m_obj;
	PointerToMemberFunction m_memberFunction;
};

template< typename R, typename P1 >
class Functor1
{
public:
	typedef FunctorImpl1< R, P1 >	Impl;
	typedef R						ResultType;
	typedef typename Impl::Param1	Param1;

public:
	Functor1()
		: m_impl( NULL )
	{}

	Functor1( const Functor1& rhs )
		: m_impl( rhs.m_impl )
	{
		m_impl->AddRef();
	}

	Functor1( Impl* impl )
		: m_impl( impl )
	{
		m_impl->AddRef();
	}

	template <typename Fun>
	Functor1( Fun fun )
		: m_impl( new FunctorHandler1< Functor1, Fun >( fun ) )
	{}

	template < class PtrObj, typename MemberFunction >
	Functor1( const PtrObj& p, MemberFunction memberFunction )
		: m_impl( new MemberFunctionHandler1< Functor1, PtrObj, MemberFunction >( p, memberFunction ) )
	{}

	virtual ~Functor1()
	{
		ASSERT( m_impl );
		m_impl->Release();
	}

	Functor1& operator=( Functor1 const& rhs )
	{
		if ( this != &rhs )
		{
			m_impl = rhs.m_impl;
			m_impl->AddRef();
		}
		return *this;
	};

	Bool operator==( const Functor1& rhs ) const
	{
		if ( !m_impl && !rhs.m_impl )
		{
			return true;
		}
		if ( m_impl && rhs.m_impl )
		{
			return *m_impl == *rhs.m_impl;
		}
		else
		{
			return false;
		}
	}

	Bool operator!=( const Functor1& rhs ) const
	{
		return !( *this == rhs );
	}

	ResultType operator() ( Param1 p1 )	const { return ( *m_impl )( p1 ); }

	void AddRef()	{ if ( m_impl ) m_impl->AddRef();	}
	void Release()	{ if ( m_impl ) m_impl->Release();	}

private:
	Impl* m_impl;
};

//////////////////////////////////////////////////////////////////////////
// Functor with 2 parameters
//////////////////////////////////////////////////////////////////////////

template< typename R, typename P1, typename P2 >
class FunctorImpl2 : public Private::FunctorImplBase< R >
{
public:
	typedef R ResultType;
	typedef P1 Param1;
	typedef P2 Param2;
	virtual R operator()( Param1, Param2 ) = 0;
};

template < class ParentFunctor, typename Fun >
class FunctorHandler2 : public ParentFunctor::Impl
{
	typedef typename ParentFunctor::Impl	Base;

public:
	typedef typename Base::ResultType		ResultType;
	typedef typename Base::Param1			Param1;
	typedef typename Base::Param2			Param2;

public:
	FunctorHandler2( const Fun& fun ) : m_fun( fun ) {}

	DEFINE_CLONE_FUNCTORIMPL( FunctorHandler2 );

	Bool operator== ( const typename Base::FunctorImplBaseType& rhs ) const
	{
		if ( typeid( *this ) != typeid( rhs ) )
		{
			return false;
		}

		const FunctorHandler2& fh = static_cast< const FunctorHandler2& >( rhs );

		// if this line gives a compiler error, you are using a function object.
		// you need to implement bool MyFnObj::operator == (const MyFnObj&) const;
		return m_fun == fh.m_fun;
	}

	ResultType operator() ( Param1 p1, Param2 p2 )	{ return m_fun( p1, p2 );	}

private:
	Fun m_fun;
};

template< class ParentFunctor, typename PointerToObj, typename PointerToMemberFunction >
class MemberFunctionHandler2 : public ParentFunctor::Impl
{
	typedef typename ParentFunctor::Impl	Base;

public:
	typedef typename Base::ResultType		ResultType;
	typedef typename Base::Param1			Param1;
	typedef typename Base::Param2			Param2;

public:
	MemberFunctionHandler2( const PointerToObj& obj, PointerToMemberFunction memberFunction ) 
		: m_obj( obj )
		, m_memberFunction( memberFunction )
	{}

	DEFINE_CLONE_FUNCTORIMPL( MemberFunctionHandler2 );

	Bool operator==( const typename Base::FunctorImplBaseType& rhs ) const
	{
		if( typeid( *this ) != typeid( rhs ) )
		{
			return false;
		}

		const MemberFunctionHandler2& mfh = static_cast< const MemberFunctionHandler2& >( rhs );
		return m_obj == mfh.m_obj && m_memberFunction == mfh.m_memberFunction;
	}

	ResultType operator() ( Param1 p1, Param2 p2 )	{ return ( ( *m_obj ).*m_memberFunction )( p1, p2 );	}

private:
	PointerToObj			m_obj;
	PointerToMemberFunction m_memberFunction;
};

template< typename R, typename P1, typename P2 >
class Functor2
{
public:
	typedef FunctorImpl2< R, P1, P2 >	Impl;
	typedef R							ResultType;
	typedef typename Impl::Param1		Param1;
	typedef typename Impl::Param2		Param2;

public:
	Functor2()
		: m_impl( NULL )
	{}

	Functor2( const Functor2& rhs )
		: m_impl( rhs.m_impl )
	{
		m_impl->AddRef();
	}

	Functor2( Impl* impl )
		: m_impl( impl )
	{
		m_impl->AddRef();
	}

	template <typename Fun>
	Functor2( Fun fun )
		: m_impl( new FunctorHandler2< Functor2, Fun >( fun ) )
	{}

	template < class PtrObj, typename MemberFunction >
	Functor2( const PtrObj& p, MemberFunction memberFunction )
		: m_impl( new MemberFunctionHandler2< Functor2, PtrObj, MemberFunction >( p, memberFunction ) )
	{}

	virtual ~Functor2()
	{
		ASSERT( m_impl );
		m_impl->Release();
	}

	Functor2& operator=( Functor2 const& rhs )
	{
		if ( this != &rhs )
		{
			m_impl = rhs.m_impl;
			m_impl->AddRef();
		}
		return *this;
	};

	Bool operator==( const Functor2& rhs ) const
	{
		if ( !m_impl && !rhs.m_impl )
		{
			return true;
		}
		if ( m_impl && rhs.m_impl )
		{
			return *m_impl == *rhs.m_impl;
		}
		else
		{
			return false;
		}
	}

	Bool operator!=( const Functor2& rhs ) const
	{
		return !( *this == rhs );
	}

	ResultType operator() ( Param1 p1, Param2 p2 )	const { return ( *m_impl )( p1, p2 ); }

	void AddRef()	{ if ( m_impl ) m_impl->AddRef();	}
	void Release()	{ if ( m_impl ) m_impl->Release();	}

private:
	Impl* m_impl;
};

//////////////////////////////////////////////////////////////////////////
// Functor with 4 parameters
//////////////////////////////////////////////////////////////////////////

template< typename R, typename P1, typename P2, typename P3, typename P4 >
class FunctorImpl4 : public Private::FunctorImplBase< R >
{
public:
	typedef R ResultType;
	typedef P1 Param1;
	typedef P2 Param2;
	typedef P3 Param3;
	typedef P4 Param4;
	virtual R operator()( Param1, Param2, Param3, Param4 ) = 0;
};

template < class ParentFunctor, typename Fun >
class FunctorHandler4 : public ParentFunctor::Impl
{
	typedef typename ParentFunctor::Impl	Base;

public:
	typedef typename Base::ResultType		ResultType;
	typedef typename Base::Param1			Param1;
	typedef typename Base::Param2			Param2;
	typedef typename Base::Param3			Param3;
	typedef typename Base::Param4			Param4;

public:
	FunctorHandler4( const Fun& fun ) : m_fun( fun ) {}

	DEFINE_CLONE_FUNCTORIMPL( FunctorHandler4 );

	Bool operator== ( const typename Base::FunctorImplBaseType& rhs ) const
	{
		if ( typeid( *this ) != typeid( rhs ) )
		{
			return false;
		}

		const FunctorHandler4& fh = static_cast< const FunctorHandler4& >( rhs );

		// if this line gives a compiler error, you are using a function object.
		// you need to implement bool MyFnObj::operator == (const MyFnObj&) const;
		return m_fun == fh.m_fun;
	}

	ResultType operator() ( Param1 p1, Param2 p2, Param3 p3, Param4 p4 )	{ return m_fun( p1, p2, p3, p4 );	}

private:
	Fun m_fun;
};

template< class ParentFunctor, typename PointerToObj, typename PointerToMemberFunction >
class MemberFunctionHandler4 : public ParentFunctor::Impl
{
	typedef typename ParentFunctor::Impl	Base;

public:
	typedef typename Base::ResultType		ResultType;
	typedef typename Base::Param1			Param1;
	typedef typename Base::Param2			Param2;
	typedef typename Base::Param3			Param3;
	typedef typename Base::Param4			Param4;

public:
	MemberFunctionHandler4( const PointerToObj& obj, PointerToMemberFunction memberFunction ) 
		: m_obj( obj )
		, m_memberFunction( memberFunction )
	{}

	DEFINE_CLONE_FUNCTORIMPL( MemberFunctionHandler4 );

	Bool operator==( const typename Base::FunctorImplBaseType& rhs ) const
	{
		if( typeid( *this ) != typeid( rhs ) )
		{
			return false;
		}

		const MemberFunctionHandler4& mfh = static_cast< const MemberFunctionHandler4& >( rhs );
		return m_obj == mfh.m_obj && m_memberFunction == mfh.m_memberFunction;
	}

	ResultType operator() ( Param1 p1, Param2 p2, Param3 p3, Param4 p4 ) { return ( ( *m_obj ).*m_memberFunction )( p1, p2, p3, p4 ); }

private:
	PointerToObj			m_obj;
	PointerToMemberFunction m_memberFunction;
};

template< typename R, typename P1, typename P2, typename P3, typename P4 >
class Functor4
{
public:
	typedef FunctorImpl4< R, P1, P2, P3, P4 >	Impl;
	typedef R									ResultType;
	typedef typename Impl::Param1				Param1;
	typedef typename Impl::Param2				Param2;
	typedef typename Impl::Param3				Param3;
	typedef typename Impl::Param4				Param4;

public:
	Functor4()
		: m_impl( NULL )
	{}

	Functor4( const Functor4& rhs )
		: m_impl( rhs.m_impl )
	{
		m_impl->AddRef();
	}

	Functor4( Impl* impl )
		: m_impl( impl )
	{
		m_impl->AddRef();
	}

	template <typename Fun>
	Functor4( Fun fun )
		: m_impl( new FunctorHandler4< Functor4, Fun >( fun ) )
	{}

	template < class PtrObj, typename MemberFunction >
	Functor4( const PtrObj& p, MemberFunction memberFunction )
		: m_impl( new MemberFunctionHandler4< Functor4, PtrObj, MemberFunction >( p, memberFunction ) )
	{}

	virtual ~Functor4()
	{
		ASSERT( m_impl );
		m_impl->Release();
	}

	Functor4& operator=( Functor4 const& rhs )
	{
		if ( this != &rhs )
		{
			m_impl = rhs.m_impl;
			m_impl->AddRef();
		}
		return *this;
	};

	Bool operator==( const Functor4& rhs ) const
	{
		if ( !m_impl && !rhs.m_impl )
		{
			return true;
		}
		if ( m_impl && rhs.m_impl )
		{
			return *m_impl == *rhs.m_impl;
		}
		else
		{
			return false;
		}
	}

	Bool operator!=( const Functor4& rhs ) const
	{
		return !( *this == rhs );
	}

	ResultType operator() ( Param1 p1, Param2 p2, Param3 p3, Param4 p4 ) const { return ( *m_impl )( p1, p2, p3, p4 ); }

	void AddRef()	{ if ( m_impl ) m_impl->AddRef();	}
	void Release()	{ if ( m_impl ) m_impl->Release();	}

private:
	Impl* m_impl;
};