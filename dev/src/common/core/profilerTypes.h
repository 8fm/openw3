#pragma once

/// Simple time counter
class CTimeCounter
{
protected:
	Double		m_startTime;		//!< Time at which this counter started

public:
	//! Constructor, start time counting
	RED_INLINE CTimeCounter()
	{
		m_startTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	}

	//! Get time elapsed from the start
	RED_INLINE Float GetTimePeriod() const
	{
		Double diff = Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_startTime;
		return ( Float ) diff;
	}

	//! Get time elapsed from the start in MS
	RED_INLINE Double GetTimePeriodMS() const
	{
		Double diff = ( Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_startTime );
		return diff * 1000.0;
	}

	RED_INLINE void ResetTimer()
	{
		m_startTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	}

	//! Get the start time
	RED_INLINE Double GetStartTime() const
	{
		return m_startTime;
	}
};

template<class TArgs0, class TArgs1=void>
class CProfilerDelegate
{
public:
	CProfilerDelegate()
		: object_ptr( nullptr )
		, stub_ptr( nullptr )
	{ /*Intentionally Empty */ }

	RED_INLINE void Send( TArgs0 args, TArgs1 args1 ) const
	{
		return (*stub_ptr)(object_ptr, args);
	}

	RED_INLINE void operator()( TArgs0 args0, TArgs1 args1 ) const
	{
		return (*stub_ptr)(object_ptr, args0, args1);
	}

	template<class T, void (T::*TMethod)( TArgs0, TArgs1 )>
	static CProfilerDelegate Create( T* objectPtr )
	{
		CProfilerDelegate d;
		d.object_ptr = objectPtr;
		d.stub_ptr = &MethodStub<T, TMethod>;
		return d;
	}

	template<class T, void (T::*TMethod)( TArgs0, TArgs1 )>
	void Set( T* listener )
	{
		this->object_ptr = listener;
		this->stub_ptr = &MethodStub<T, TMethod>;
	}

	void Clear()
	{
		this->object_ptr = nullptr;
		this->stub_ptr = nullptr;
	}

	bool IsSet()
	{
		if( this->object_ptr == nullptr || this->stub_ptr == nullptr )
			return false;

		return true;
	}

	bool operator==( CProfilerDelegate<TArgs0, TArgs1> other )
	{
		if( this->object_ptr == other.object_ptr && this->stub_ptr == other.stub_ptr )
			return true;

		return false;
	}

private:
	typedef void (*stub_type)(void* object_ptr, TArgs0, TArgs1);

	void* object_ptr;
	stub_type stub_ptr;

	template<class T, void (T::*TMethod)( TArgs0, TArgs1 )>
	static void MethodStub(void* object_ptr, TArgs0 args0, TArgs1 args1)
	{
		T* ptr = static_cast<T*>( object_ptr );
		return (ptr->*TMethod)( args0, args1 );
	}
};

template<class TArgs0>
class CProfilerDelegate< TArgs0, void >
{
public:
	CProfilerDelegate()
		: object_ptr( nullptr )
		, stub_ptr( nullptr )
	{ /*Intentionally Empty */ }

	RED_INLINE void Send( TArgs0 args ) const
	{
		return (*stub_ptr)(object_ptr, args);
	}

	RED_INLINE void operator()( TArgs0 args ) const
	{
		return (*stub_ptr)(object_ptr, args);
	}

	template<class T, void (T::*TMethod)( TArgs0 )>
	static CProfilerDelegate Create( T* objectPtr )
	{
		CProfilerDelegate d;
		d.object_ptr = objectPtr;
		d.stub_ptr = &MethodStub<T, TMethod>;
		return d;
	}

	template<class T, void (T::*TMethod)( TArgs0 )>
	void Set( T* listener )
	{
		this->object_ptr = listener;
		this->stub_ptr = &MethodStub<T, TMethod>;
	}

	void Clear()
	{
		this->object_ptr = nullptr;
		this->stub_ptr = nullptr;
	}

	bool IsSet()
	{
		if( this->object_ptr == nullptr || this->stub_ptr == nullptr )
			return false;

		return true;
	}

	bool operator==( CProfilerDelegate<TArgs0> other )
	{
		if( this->object_ptr == other.object_ptr && this->stub_ptr == other.stub_ptr )
			return true;

		return false;
	}

private:
	typedef void (*stub_type)(void* object_ptr, TArgs0);

	void* object_ptr;
	stub_type stub_ptr;

	template<class T, void (T::*TMethod)( TArgs0 )>
	static void MethodStub(void* object_ptr, TArgs0 args)
	{
		T* ptr = static_cast<T*>( object_ptr );
		return (ptr->*TMethod)( args );
	}
};

class CProfilerBlock;

struct SProfilerFuncPackage
{
	CProfilerDelegate< CProfilerBlock* > begin;
	CProfilerDelegate< CProfilerBlock* > end;
	CProfilerDelegate< CProfilerBlock*, const Char* > message;

	SProfilerFuncPackage()
	{
		/* Intentionally Empty */
	}

	SProfilerFuncPackage( CProfilerDelegate< CProfilerBlock* > begin, CProfilerDelegate< CProfilerBlock* > end, CProfilerDelegate< CProfilerBlock*, const Char* > message )
		: begin( begin )
		, end( end )
		, message( message )
	{
		/* Intentionally Empty */
	}
};

struct SProfilerFuncPackageTable
{
	SProfilerFuncPackage* funcs;
	Uint32 size;
	Red::Threads::CAtomic<Int32> refcount;

	SProfilerFuncPackageTable();
	SProfilerFuncPackageTable( Uint32 size );
	~SProfilerFuncPackageTable();

	void Initialize();
};
