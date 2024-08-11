/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "functionCalling.h"

RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool >( this, eventName, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0 >( this, eventName, data0, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0, class T1 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0, const T1& data1 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0, T1 >( this, eventName, data0, data1, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0, class T1, class T2 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0, T1, T2 >( this, eventName, data0, data1, data2, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0, class T1, class T2, class T3 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0, T1, T2, T3 >( this, eventName, data0, data1, data2, data3, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0, class T1, class T2, class T3, class T4 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0, T1, T2, T3, T4 >( this, eventName, data0, data1, data2, data3, data4, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0, class T1, class T2, class T3, class T4, class T5 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0, T1, T2, T3, T4, T5 >( this, eventName, data0, data1, data2, data3, data4, data5, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0, class T1, class T2, class T3, class T4, class T5, class T6 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0, T1, T2, T3, T4, T5, T6 >( this, eventName, data0, data1, data2, data3, data4, data5, data6, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, const T7& data7 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0, T1, T2, T3, T4, T5, T6, T7 >( this, eventName, data0, data1, data2, data3, data4, data5, data6, data7, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, const T7& data7, const T8& data8 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0, T1, T2, T3, T4, T5, T6, T7, T8 >( this, eventName, data0, data1, data2, data3, data4, data5, data6, data7, data8, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}

template< class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 > RED_INLINE EEventCallResult IScriptable::CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, const T7& data7, const T8& data8, const T9& data9 )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CallEvent can only be called from Main Thread." );

	Bool result = false;
	Bool found = CallFunctionRet< Bool, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 >( this, eventName, data0, data1, data2, data3, data4, data5, data6, data7, data8, data9, result );
	return found ? ( result ? CR_EventSucceeded : CR_EventFailed ) : CR_EventNotFound;
}