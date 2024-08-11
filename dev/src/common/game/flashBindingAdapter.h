/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/flashValueStorage.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashValueStorage;

//////////////////////////////////////////////////////////////////////////
// CFlashBindingAdapter
//////////////////////////////////////////////////////////////////////////
class CFlashBindingAdapter : public CFlashValueBindingHandler
{
private:
	CFlashValue	m_flashClosure;
	CFlashValue	m_flashBoundObject;

public:
	//! CFlashValueBindingHandler functions
	virtual void OnFlashValueChanged( const CFlashValue& flashValue, Int32 index ) override;

public:
	CFlashBindingAdapter( const String& key, CFlashValueStorage* flashValueStorage, const CFlashValue& flashClosure, const CFlashValue& flashBoundObject, const CFlashValue& flashIsGlobal );

public:
	RED_INLINE Bool operator==( const CFlashBindingAdapter& rhs ) const
	{
		return m_flashClosure == rhs.m_flashClosure && m_flashBoundObject == rhs.m_flashBoundObject;
	}

protected:
	~CFlashBindingAdapter();
};