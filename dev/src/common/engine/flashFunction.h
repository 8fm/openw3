/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "flashReference.h"

#ifdef USE_SCALEFORM
#include "flashValue.h"
#include "flashMovieScaleform.h"
#else
#include "flashMovie.h"
#endif

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashValue;

namespace Flash
{
	template< typename T >
	struct TFlashFuncPtr
	{
		typedef void (T::*Type )( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	};
}

#ifdef USE_SCALEFORM

class CFlashFunctionHandler : public GFx::FunctionHandler
{
	virtual void Call(const Params& params) = 0;
};

template < typename T > class CFlashFunctionHandlerT : public CFlashFunctionHandler
{
private:
	T*											m_context;
	typename Flash::TFlashFuncPtr< T >::Type	m_flashFunc;

	virtual void Call(const Params& params) override
	{
		TDynArray< CFlashValue > flashValueParams;

		flashValueParams.Reserve( params.ArgCount );
		for ( Uint32 i = 0; i < params.ArgCount; ++i )
		{
			flashValueParams.PushBack( CFlashValue( params.pArgs[ i ] ) );
		}

		ASSERT( params.pMovie && params.pMovie->GetUserData() );
		if ( ! params.pMovie || ! params.pMovie->GetUserData() )
		{
			return;
		}

		CFlashMovieScaleform* flashMovieScaleform = reinterpret_cast< CFlashMovieScaleform* >( params.pMovie->GetUserData() );
		CFlashValue flashThis( params.pThis ? *params.pThis : CFlashValue() );

		(m_context->*m_flashFunc)( flashMovieScaleform, flashThis, flashValueParams );
	}

public:
	CFlashFunctionHandlerT( T* context, typename Flash::TFlashFuncPtr< T >::Type flashFunc )
		: m_context( context )
		, m_flashFunc( flashFunc )
	{
	}
};

#else

class CFlashFunctionHandler : public IFlashReference
{
};

typedef CFlashFunctionHandler CFlashFunctionHandlerNull;

#endif // USE_SCALEFORM

namespace Flash
{
	template < typename T > inline CFlashFunctionHandler* CreateFunctionHandler( T* pContext, typename TFlashFuncPtr< T >::Type flashFunc )
	{
#ifdef USE_SCALEFORM
		return SF_HEAP_NEW( SF::Memory::GetGlobalHeap() ) CFlashFunctionHandlerT< T >( pContext, flashFunc );
#else
		return new CFlashFunctionHandlerNull;
#endif // USE_SCALEFORM
	}
}

//////////////////////////////////////////////////////////////////////////
// CFlashFunction
//////////////////////////////////////////////////////////////////////////
class CFlashFunction : public IFlashReference
{
	friend class CFlashPlayer;

private:
	CFlashFunctionHandler*			m_flashFunctionHandler;

private:
	void*							m_userData;

private:
	CFlashMovie*					m_flashMovie;	

protected:
	CFlashFunction( CFlashMovie* flashMovie, CFlashFunctionHandler* flashFunctionHandler );
	virtual ~CFlashFunction();
};

