/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once



#ifndef NO_DEBUG_PAGES

#include "debugChoice.h"
#include "debugPage.h"
#include "debugTextBox.h"

//////////////////////////////////////////////////////////////////////////
// Attribute provider used by the debug page to display changeable values
//////////////////////////////////////////////////////////////////////////

struct SAttribute
{
	CProperty*				m_property;
	THandle<IScriptable>	m_context;

	SAttribute( CProperty* prop = NULL, const THandle<IScriptable>& context = THandle<IScriptable>::Null() )
		: m_property( prop ), m_context( context ) {};

	RED_INLINE Bool IsValid() const { return m_property && m_context.IsValid(); }
};

class CAttributeProvider
{
public:
	CAttributeProvider() {};
	virtual ~CAttributeProvider() {};

	typedef THashMap< CName, SAttribute >	Attributes;

	RED_INLINE Bool AddAttribute( const CName& name, const SAttribute& attrib )
	{
		return m_attributes.Insert( name, attrib );
	}

	RED_INLINE const Attributes&	GetAttributes() const	{ return m_attributes; }

	RED_INLINE Attributes&		GetAttributes()			{ return m_attributes; }

	RED_INLINE void ClearAttributes() { m_attributes.Clear(); }

protected:
	Attributes	m_attributes;
};

//////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////// 
// Debug page used for changing registered attributes
//////////////////////////////////////////////////////////////////////////

template <typename T>
class IAttributeChanger
{
protected:
	IAttributeChanger( const SAttribute& attr ) : m_attribute( attr ) {};

	SAttribute	m_attribute;

	RED_INLINE void	SetValue( const T& value )
	{ 
		if( m_attribute.IsValid() )
		{
			m_attribute.m_property->Set( m_attribute.m_context.Get(), &value );
		}
	}

	RED_INLINE T		GetValue()
	{
		T value = (T)0;
		if( m_attribute.IsValid() )
		{
			m_attribute.m_property->Get( m_attribute.m_context.Get(), &value );
		}
		return value;
	}
};

template <typename T>
class CDebugTextBoxChanger : public IAttributeChanger<T>, public CDebugTextBox
{
protected:
	using IAttributeChanger<T>::GetValue;
	using IAttributeChanger<T>::SetValue;

public:
	CDebugTextBoxChanger( IDebugCheckBox* parent, const String& name, const SAttribute& attr ) : IAttributeChanger<T>( attr ), CDebugTextBox( parent, name )
	{
		SetSeparation( 200 );
		SetText( ToString( GetValue() ) );
	};

	//! Focus was lost, apply the changes from textBox to attribute
	virtual void OnLostFocus()
	{
		T value;
		// first try to parse the string to value
		FromString( m_text, value );

		// set the attribute according to the new value
		SetValue( value );

		// change the text to indicate the new value (if parse failed we will know about it)
		SetText( ToString( value ) );
	};
};

class CDebugBoolSelector : public IAttributeChanger<Bool>, public IDebugChoice
{
public:
	CDebugBoolSelector( IDebugCheckBox* parent, const String& name, const SAttribute& attr ) : IAttributeChanger( attr ), IDebugChoice( parent, name )
	{
		SetSeparation( 200 );
		m_currSelection = GetValue();
	};

	RED_INLINE virtual String GetSelection() const
	{
		return ToString( m_currSelection );
	}

	RED_INLINE virtual void OnLostFocus()
	{
		SetValue( m_currSelection );
	}

protected:
	Bool	m_currSelection;

	RED_INLINE virtual void OnNext() { m_currSelection = !m_currSelection; }
	RED_INLINE virtual void OnPrev() { m_currSelection = !m_currSelection; }
};

class CDebugEnumSelector : public IAttributeChanger<Int32>, public IDebugChoice
{
public:
	CDebugEnumSelector( IDebugCheckBox* parent, const String& name, const SAttribute& attr );

	virtual String GetSelection() const;

	RED_INLINE virtual void OnLostFocus()
	{
		Int32 value = 0;
		m_enum->FindValue( m_enum->GetOptions()[m_index], value );
		SetValue( value );
	}

protected:
	const CEnum*	m_enum;
	Uint32			m_index;

	virtual void OnNext();
	virtual void OnPrev();
};

class CDebugPageDynamicAttributes : public IDebugPage
{
	friend class CDebugAttributesManager;

public:
	CDebugPageDynamicAttributes();

	virtual void OnTick( Float timeDelta );

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	virtual void OnPageShown();

	virtual void OnPageHidden();

protected:
	CDebugOptionsTree*					m_tree;
	THashMap< CName, CAttributeProvider >	m_providers;
};

////////////////////////////////////////////////////////////////////////// 

#endif

////////////////////////////////////////////////////////////////////////// 
// Manager for creating and retrieving attribute debug pages
//////////////////////////////////////////////////////////////////////////
class CDebugAttributesManager : public CObject
{
	DECLARE_ENGINE_CLASS( CDebugAttributesManager, CObject, 0 )

#ifndef NO_DEBUG_PAGES
protected:
	CDebugPageDynamicAttributes* m_page;
#endif

public:
	CDebugAttributesManager() 
#ifndef NO_DEBUG_PAGES
		: m_page(NULL) 
#endif
	{};
	virtual ~CDebugAttributesManager() {};

	void Init();
	void ShutDown();

	// Clears all existing attributes
	void Clear();

private:
	void funcAddAttribute( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CDebugAttributesManager )
	PARENT_CLASS( CObject )
	NATIVE_FUNCTION( "AddAttribute",		funcAddAttribute );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////// 