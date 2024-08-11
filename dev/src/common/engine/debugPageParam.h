/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/


#pragma once
#include "debugCheckBox.h"
#include "debugSlider.h"

#ifndef NO_DEBUG_PAGES

//////////////////////////////////////////////////////////////////////////

class CDebugCheckBoxParam : public IDebugCheckBox
{
	Bool*	m_paramValue;
	void (*callback)();

public:
	CDebugCheckBoxParam( IDebugCheckBox* parent, Bool& paramValue, const String& paramName, void (*_callback)() = 0 )
		: IDebugCheckBox( parent, paramName, false, true )
		, m_paramValue( &paramValue )
	{
		callback = _callback;
	};

	virtual Bool IsChecked() const
	{
		return *m_paramValue;
	}

	virtual void OnToggle()
	{
		*m_paramValue = !*m_paramValue;
		if( callback )
			callback();
	}
};

class CDebugSliderParam : public IDebugSlider
{
	Float *	m_paramValue;

	Float	m_paramMin;
	Float	m_paramMax;
	Float	m_paramStep;
	void (*callback)();

public:
	CDebugSliderParam( IDebugCheckBox* parent, Float& paramValue, const String& paramName, Float min, Float max, Float paramStep = 0.f, void (*_callback)() = 0 )
		: IDebugSlider( parent, paramName )
		, m_paramValue( &paramValue )
		, m_paramMin( min )
		, m_paramMax( max )
		, m_paramStep( paramStep )
	{
		if ( m_paramStep <= 0.f )
		{
			m_paramStep = ( GetMax() - GetMin() ) / 10.f;
		}
		callback = _callback;

	};

	virtual Float GetValue() const
	{
		return *m_paramValue;
	}

	virtual Float GetMin() const
	{
		return m_paramMin;
	}

	virtual Float GetMax() const
	{
		return m_paramMax;
	}

protected:
	virtual void OnValueInc()
	{
		*m_paramValue = Clamp( *m_paramValue + m_paramStep, GetMin(), GetMax() );
		if( callback )
			callback();
	}

	virtual void OnValueDec()
	{
		*m_paramValue = Clamp( *m_paramValue - m_paramStep, GetMin(), GetMax() ); 
		if( callback )
			callback();
	}
};

class CDebugSliderIntParam : public IDebugSlider
{
	Int32 *	m_paramValue;

	Int32	m_paramMin;
	Int32	m_paramMax;
	Int32	m_paramStep;
	void (*callback)();

public:
	CDebugSliderIntParam( IDebugCheckBox* parent, Int32& paramValue, const String& paramName, Int32 min, Int32 max, Int32 paramStep = 1, void (*_callback)() = 0 )
		: IDebugSlider( parent, paramName )
		, m_paramValue( &paramValue )
		, m_paramMin( min )
		, m_paramMax( max )
		, m_paramStep( paramStep )
	{
		callback = _callback;

	};

	virtual Float GetValue() const
	{
		return (Float)*m_paramValue;
	}

	virtual Float GetMin() const
	{
		return (Float)m_paramMin;
	}

	virtual Float GetMax() const
	{
		return (Float)m_paramMax;
	}

protected:
	virtual void OnValueInc()
	{
		*m_paramValue = Clamp( *m_paramValue + m_paramStep, m_paramMin, m_paramMax );
		if( callback )
			callback();
	}

	virtual void OnValueDec()
	{
		*m_paramValue = Clamp( *m_paramValue - m_paramStep, m_paramMin, m_paramMax ); 
		if( callback )
			callback();
	}
};

class CDebugSliderIntWithValParam : public IDebugSlider
{
	Int32	m_paramValue;

	Int32	m_paramMin;
	Int32	m_paramMax;
	Int32	m_paramStep;

public:
	CDebugSliderIntWithValParam( IDebugCheckBox* parent, const String& paramName, Int32 min, Int32 max, Int32 paramStep = 1 )
		: IDebugSlider( parent, paramName )
		, m_paramValue( 0 )
		, m_paramMin( min )
		, m_paramMax( max )
		, m_paramStep( paramStep )
	{

	};

	virtual Float GetValue() const
	{
		return (Float)m_paramValue;
	}

	virtual Float GetMin() const
	{
		return (Float)m_paramMin;
	}

	virtual Float GetMax() const
	{
		return (Float)m_paramMax;
	}

public:
	void SetMax( Int32 max )
	{
		m_paramMax = max;
	}

protected:
	virtual void OnValueInc()
	{
		m_paramValue = Clamp( m_paramValue + m_paramStep, m_paramMin, m_paramMax );
	}

	virtual void OnValueDec()
	{
		m_paramValue = Clamp( m_paramValue - m_paramStep, m_paramMin, m_paramMax ); 
	}
};

//////////////////////////////////////////////////////////////////////////

#endif
