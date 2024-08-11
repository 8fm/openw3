/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "flashValueStorage.h"
#include "flashMovie.h"
#include "guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// CFlashValueBindingHandler
//////////////////////////////////////////////////////////////////////////
CFlashValueBindingHandler::CFlashValueBindingHandler( const String& key, CFlashValueStorage* flashValueStorage, Bool isGlobal )
	: m_key( key )
	, m_flashValueStorage( flashValueStorage )
	, m_isGlobal( isGlobal )
{
	ASSERT( m_flashValueStorage );
	if ( m_flashValueStorage )
	{
		m_flashValueStorage->AddRef();	
	}
}

CFlashValueBindingHandler::~CFlashValueBindingHandler()
{
	ASSERT( m_flashValueStorage );
	if ( m_flashValueStorage )
	{
		m_flashValueStorage->Release();
	}
}

//////////////////////////////////////////////////////////////////////////
// CFlashValueStorage
//////////////////////////////////////////////////////////////////////////
CFlashValueStorage::CFlashValueStorage( CFlashMovie* flashMovie )
	: m_isInTick( false )
{
	m_flashMovie = flashMovie;

	if ( m_flashMovie )
	{
		m_flashMovie->AddRef();
	}
}

CFlashValueStorage::~CFlashValueStorage()
{
	m_keyBindingMap.ClearFast();

	if ( m_flashMovie )
	{
		m_flashMovie->Release();
		m_flashMovie = nullptr;
	}
}

Bool CFlashValueStorage::RegisterFlashValueBindingHandler( const String& key, CFlashValueBindingHandler* flashValueBindingHandler )
{
	ASSERT( ! m_isInTick );
	if ( m_isInTick )
	{
		return false;
	}

	SValueBinding& valueBinding = m_keyBindingMap[ key ];
	if ( valueBinding.GetHandlers().PushBackUnique( flashValueBindingHandler ) )
	{
		flashValueBindingHandler->AddRef();

		if ( ! valueBinding.GetFlashValue().IsFlashUndefined() )
		{
			m_invalidatedKeys.PushBackUnique( key );
		}

		return true;
	}
	return false;
}

Bool CFlashValueStorage::UnregisterFlashValueBindingHandler( const String& key, CFlashValueBindingHandler* flashValueBindingHandler )
{
	ASSERT( ! m_isInTick );
	if ( m_isInTick )
	{
		return false;
	}

	if ( SValueBinding* valueBinding = m_keyBindingMap.FindPtr( key ) )
	{
		if ( valueBinding->GetHandlers().RemoveFast( flashValueBindingHandler ) )
		{
			flashValueBindingHandler->Release();
			return true;
		}
	}
	return false;
}

void CFlashValueStorage::CollectInvalidatedKeys( TKeyHandlerMap& keysMap )
{
	for ( TKeyList::const_iterator it = m_invalidatedKeys.Begin(); it != m_invalidatedKeys.End(); ++it )
	{
		const String key = *it;
		if ( !keysMap.KeyExist( key ) )
		{
			keysMap[ key ] = THandlerList();
		}
	}
}

void CFlashValueStorage::CollectHandlers( TKeyHandlerMap& keysMap )
{
	for ( TKeyHandlerMap::iterator itKey = keysMap.Begin(); itKey != keysMap.End(); ++itKey )
	{
		const String& key = itKey->m_first;

		SValueBinding* valueBinding = m_keyBindingMap.FindPtr( key );
		if ( valueBinding )
		{
			THandlerList& handlers = valueBinding->GetHandlers();
			for ( Uint32 i = 0; i < handlers.Size(); i++ )
			{
				// collect all global handlers and the 'local' one
				if ( handlers[ i ]->IsGlobal() || m_invalidatedKeys.Exist( key ))
				{
					itKey->m_second.PushBackUnique( handlers[ i ] );
				}
			}
		}
	}
}

void CFlashValueStorage::Process( const TKeyHandlerMap& keysMap )
{
	m_isInTick = true;

	for ( TKeyList::const_iterator it = m_invalidatedKeys.Begin(); it != m_invalidatedKeys.End(); ++it )
	{
		const String& key = *it;
		SValueBinding& valueBinding = m_keyBindingMap[ key ];
		const CFlashValue& changedFlashValue = valueBinding.GetFlashValue();
		const THandlerList* handlers = keysMap.FindPtr( key );
		if ( !handlers )
		{
			RED_HALT( "CFlashValueStorage::Process() - missing handlers for key [%s]", key.AsChar() );
			continue;
		}
		//TBD: split into separate lists with invalidated value.
		if ( valueBinding.IsAllInvalidated() )
		{
			ForEach( *handlers, [&]( CFlashValueBindingHandler* bindingHandler ) { bindingHandler->OnFlashValueChanged( changedFlashValue, -1 ); } );
		}
		else
		{
			const SValueBinding::TIndexList indices = valueBinding.GetInvalidatedArrayIndices();
			for ( SValueBinding::TIndexList::const_iterator indexIt = indices.Begin(); indexIt != indices.End(); ++indexIt )
			{
				const Int32 index = *indexIt;
				CFlashValue changedArrayElement;
				VERIFY( changedFlashValue.GetArrayElement( index, changedArrayElement ) );
				ForEach( *handlers, [&]( CFlashValueBindingHandler* bindingHandler ) { bindingHandler->OnFlashValueChanged( changedArrayElement, index ); } );
			}
		}
		valueBinding.ClearInvalidation();
	}
	m_invalidatedKeys.ClearFast();

	m_isInTick = false;
}

Bool CFlashValueStorage::SetFlashValue( const String& key, const CFlashValue& value, Int32 index /*=-1*/ )
{
	if ( m_isInTick )
	{
		GUI_ERROR(TXT("CFlashValueStorage::SetFlashValue: trying to set key '%ls' during tick. Does your binding handler try to modify Flash value storage?"), key.AsChar() );

		return false;
	}

	SValueBinding& valueBinding = m_keyBindingMap[ key ];

#ifdef USE_SCALEFORM
	//ASSERT ( ! value.m_gfxValue.GetMovie() || value.m_gfxValue.GetMovie() == static_cast< CFlashMovieScaleform* >( m_flashMovie ).m_gfxMovie );
#endif

	const Bool wasAlreadyInvalidated = valueBinding.IsAllInvalidated();
	const EFlashValueBindingResult result = valueBinding.SetFlashValue( value, index );
	if ( result == FVBR_Changed && ! wasAlreadyInvalidated )
	{
		//!FIXME: Just for this handler
		valueBinding.SetAllInvalidated();
		m_invalidatedKeys.PushBackUnique( key );
	}

	return result != FVBR_Error;
}

CFlashValueStorage::EFlashValueBindingResult CFlashValueStorage::SValueBinding::SetFlashValue( const CFlashValue& value, Int32 index /*=-1*/ )
{
	if ( ! m_value.IsFlashUndefined() && m_value.GetFlashValueType() != value.GetFlashValueType() )
	{
		HALT( "Cannot change the Flash value binding type" );
		return FVBR_Error;
	}

	EFlashValueBindingResult result = FVBR_Error;

	if ( index >= 0 )
	{
		result = UpdateFlashValueArrayElement( value, index );
		(void)m_invalidatedArrayIndices.PushBackUnique( index );

		// Don't redundantly invalidate an index if the entire array is already invalidated
		if ( result == FVBR_Changed && ! m_allInvalidated )
		{
			m_invalidatedArrayIndices.PushBackUnique( index );
		}
		return result;
	}

	result = UpdateFlashValue( value );
	if ( result == FVBR_Changed )
	{
		SetAllInvalidated();
	}

	return result;
}

CFlashValueStorage::EFlashValueBindingResult CFlashValueStorage::SValueBinding::UpdateFlashValue( const CFlashValue& value )
{
	// Can't prevent all redundant updates without keeping a copy of the last frames value,
	// which could be more of a problem with arrays and objects.
	if ( m_value == value )
	{
		return FVBR_NoChange;
	}

	m_value = value;

	return FVBR_Changed;
}

CFlashValueStorage::EFlashValueBindingResult CFlashValueStorage::SValueBinding::UpdateFlashValueArrayElement( const CFlashValue& value, Int32 index )
{
	ASSERT( index >= 0 && ( m_value.IsFlashArray() || m_value.IsFlashNull() ) );

	if ( m_value.IsFlashNull() )
	{
		GUI_ERROR( TXT("Flash array is null"));
		return FVBR_Error;
	}
	else if ( ! m_value.IsFlashArray() )
	{
		GUI_ERROR( TXT("Not a Flash array!") );
		return FVBR_Error;
	}

	// TBD: For now, just sets the value without retrieving current value to compare
	// Also allows you to change the type.
	if ( ! m_value.SetArrayElementFlashValue( index, value ) )
	{
		GUI_ERROR( TXT("Failed to set array element at index '%d'"), index);
		return FVBR_Error;
	}

	return FVBR_Changed;
}

void CFlashValueStorage::SValueBinding::SetAllInvalidated()
{
	m_allInvalidated = true;
	m_invalidatedArrayIndices.ClearFast();
}

void CFlashValueStorage::SValueBinding::ClearInvalidation()
{
	m_allInvalidated = false;
	m_invalidatedArrayIndices.ClearFast();
}

CFlashValueStorage::SValueBinding::SValueBinding()
	: m_allInvalidated( false )
{
}

CFlashValueStorage::SValueBinding::SValueBinding( const SValueBinding& other )
{
	*this = other;

	for ( THandlerList::const_iterator it = m_handlers.Begin(); it != m_handlers.End(); ++it )
	{
		CFlashValueBindingHandler* flashValueBindingHandler = *it;
		flashValueBindingHandler->AddRef();
	}
}

CFlashValueStorage::SValueBinding::~SValueBinding()
{
	// Smart pointers...
	for ( THandlerList::const_iterator it = m_handlers.Begin(); it != m_handlers.End(); ++it )
	{
		CFlashValueBindingHandler* flashValueBindingHandler = *it;
		flashValueBindingHandler->Release();
	}
}


