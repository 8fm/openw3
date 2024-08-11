/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "flashValue.h"
#include "flashReference.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashMovie;
class CFlashValueStorage;

//////////////////////////////////////////////////////////////////////////
// CFlashValueBindingHandler
//////////////////////////////////////////////////////////////////////////
class CFlashValueBindingHandler : public IFlashReference
{
private:
	String						m_key;
	CFlashValueStorage*			m_flashValueStorage;
	Bool						m_isGlobal;

public:
	RED_INLINE const String&	GetKey() const { return m_key; }
	RED_INLINE Bool			IsGlobal() const { return m_isGlobal; }

public:
	virtual void				OnFlashValueChanged( const CFlashValue& flashValue, Int32 index )=0;

protected:
								CFlashValueBindingHandler( const String& key, CFlashValueStorage* flashValueStorage, Bool isGlobal );
	virtual						~CFlashValueBindingHandler();
};

//////////////////////////////////////////////////////////////////////////
// CFlashValueStorage
//////////////////////////////////////////////////////////////////////////
class CFlashValueStorage : public IFlashReference
{
public:
	typedef TDynArray< CFlashValueBindingHandler* > THandlerList;
	typedef THashMap< String, THandlerList >		TKeyHandlerMap;

private:
	enum EFlashValueBindingResult
	{
		FVBR_Error,
		FVBR_NoChange,
		FVBR_Changed,
	};

private:
	struct SValueBinding
	{
		typedef TDynArray< Int32 >						TIndexList;

	private:
		mutable TIndexList			m_invalidatedArrayIndices;
		mutable Bool				m_allInvalidated;
		THandlerList				m_handlers;
		CFlashValue					m_value;

	public:
		Bool						IsAllInvalidated() const { return m_allInvalidated; }
		void						SetAllInvalidated();
		void						ClearInvalidation();

	public:
		EFlashValueBindingResult	SetFlashValue( const CFlashValue& value, Int32 index = -1 );
		CFlashValue&				GetFlashValue() { return m_value; }
		const CFlashValue&			GetFlashValue() const { return m_value; }

	public:
		THandlerList&				GetHandlers() { return m_handlers; }

	public:
		const TIndexList&			GetInvalidatedArrayIndices() const { return m_invalidatedArrayIndices; }

	public:
		SValueBinding();
		SValueBinding( const SValueBinding& other );
		~SValueBinding();

	private:
		EFlashValueBindingResult	UpdateFlashValue( const CFlashValue& value );
		EFlashValueBindingResult	UpdateFlashValueArrayElement( const CFlashValue& value, Int32 index );
	};

private:
	typedef THashMap< String, SValueBinding>		TKeyValueBindingMap;
	typedef	TDynArray< String >						TKeyList;

private:
	TKeyValueBindingMap			m_keyBindingMap;
	TKeyList					m_invalidatedKeys;
	CFlashMovie*				m_flashMovie;
	Bool						m_isInTick;

public:
	CFlashValueStorage( CFlashMovie* flashMovie );
	~CFlashValueStorage();

public:
	Bool					RegisterFlashValueBindingHandler( const String& key, CFlashValueBindingHandler* flashValueBindingHandler );
	Bool					UnregisterFlashValueBindingHandler( const String& key, CFlashValueBindingHandler* flashValueBindingHandler );

public:
	Bool					SetFlashValue( const String& key, const CFlashValue& value, Int32 index = -1 );

public:
	void					CollectInvalidatedKeys( THashMap< String, THandlerList >& keysMap );
	void					CollectHandlers( THashMap< String, THandlerList >& keysMap );
	void					Process( const THashMap< String, THandlerList >& keysMap );
};
