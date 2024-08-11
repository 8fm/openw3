/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "flashReference.h"
#include "flashValue.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashMovie;

//////////////////////////////////////////////////////////////////////////
// CFlashObject
//////////////////////////////////////////////////////////////////////////
class CFlashObject : public IFlashReference
{
private:
	CFlashMovie*	m_flashMovie;

protected:
	CFlashValue		m_flashValue;

public:
	RED_INLINE const CFlashValue& AsFlashValue() const { return m_flashValue; }
	RED_INLINE CFlashValue& AsFlashValue() { return m_flashValue; }

protected:
	CFlashObject( CFlashMovie* flashMovie );
	virtual ~CFlashObject();
};

//////////////////////////////////////////////////////////////////////////
// CFlashArray
//////////////////////////////////////////////////////////////////////////
class CFlashArray : public IFlashReference
{
private:
	CFlashMovie*	m_flashMovie;

protected:
	CFlashValue		m_flashValue;

public:
	RED_INLINE const CFlashValue& AsFlashValue() const { return m_flashValue; }
	RED_INLINE CFlashValue& AsFlashValue() { return m_flashValue; }

protected:
	CFlashArray( CFlashMovie* flashMovie );
	virtual ~CFlashArray();
};

//////////////////////////////////////////////////////////////////////////
// CFlashString
//////////////////////////////////////////////////////////////////////////
class CFlashString : public IFlashReference
{
private:
	CFlashMovie*	m_flashMovie;

protected:
	CFlashValue		m_flashValue;

public:
	RED_INLINE const CFlashValue& AsFlashValue() const { return m_flashValue; }

protected:
	CFlashString( CFlashMovie* flashMovie );
	virtual ~CFlashString();
};