/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../../common/engine/flashMovie.h"
#include "../../common/engine/flashFunction.h"
#include "../../common/engine/flashValueObject.h"

#include "../../common/game/flashScriptSupport.h"
#include "../../common/game/flashScriptFunctionCalling.h"

#include "flashBindingAdapter.h"
#include "../engine/guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// CFlashBindingAdapter
//////////////////////////////////////////////////////////////////////////
CFlashBindingAdapter::CFlashBindingAdapter( const String& key, CFlashValueStorage* flashValueStorage, const CFlashValue& flashClosure, const CFlashValue& flashBoundObject, const CFlashValue& flashIsGlobal )
	: CFlashValueBindingHandler( key, flashValueStorage, flashIsGlobal.GetFlashBool() ) 
	, m_flashClosure( flashClosure )
	, m_flashBoundObject( flashBoundObject )
{
	ASSERT( m_flashClosure.IsFlashClosure() );
	ASSERT( m_flashBoundObject.IsFlashNull() || m_flashBoundObject.IsFlashObject() );
}

CFlashBindingAdapter::~CFlashBindingAdapter()
{
}

void CFlashBindingAdapter::OnFlashValueChanged( const CFlashValue& flashValue, Int32 index )
{
	ASSERT( m_flashClosure.IsFlashClosure() );
	if ( m_flashClosure.IsFlashClosure() )
	{
		const CFlashValue flashIndex( index );
		if ( ! m_flashClosure.InvokeSelf( flashValue, flashIndex ) )
		{
			GUI_ERROR( TXT("Failed to invoke Flash data binding handler for key '%ls'. Possibly an exception was thrown in Flash"), GetKey().AsChar() );
		}
	}
}