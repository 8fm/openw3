/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class IQuestTimeFunction;

class CQuestTimeManagementBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestTimeManagementBlock, CQuestGraphBlock, 0 )

private:
	IQuestTimeFunction*		m_function;

public:
	CQuestTimeManagementBlock() { m_name = TXT("Time management"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Octagon; }
	virtual Color GetClientColor() const { return Color( 192, 80, 77 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

};

BEGIN_CLASS_RTTI( CQuestTimeManagementBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_INLINED( m_function, TXT( "Time function" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class IQuestTimeFunction : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IQuestTimeFunction, CObject )

public:
	virtual ~IQuestTimeFunction() {}

	virtual void Execute() {}
};

BEGIN_ABSTRACT_CLASS_RTTI( IQuestTimeFunction )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CPauseTimeFunction : public IQuestTimeFunction
{
	DECLARE_ENGINE_CLASS( CPauseTimeFunction, IQuestTimeFunction, 0 )

private:
	Bool					m_pause;

public:
	void Execute();
};

BEGIN_CLASS_RTTI( CPauseTimeFunction )
	PARENT_CLASS( IQuestTimeFunction )
	PROPERTY_EDIT( m_pause, TXT( "Should the time be paused or started?" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CSetTimeFunction : public IQuestTimeFunction
{
	DECLARE_ENGINE_CLASS( CSetTimeFunction, IQuestTimeFunction, 0 )

private:
	GameTime				m_newTime;
	Bool					m_callEvents;

public:
	void Execute();
};

BEGIN_CLASS_RTTI( CSetTimeFunction )
	PARENT_CLASS( IQuestTimeFunction )
	PROPERTY_CUSTOM_EDIT( m_newTime, TXT( "New time that should be set" ), TXT( "DayTimeEditor" ) )
	PROPERTY_EDIT( m_callEvents, TXT( "Should the related events be sent out?" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CShiftTimeFunction : public IQuestTimeFunction
{
	DECLARE_ENGINE_CLASS( CShiftTimeFunction, IQuestTimeFunction, 0 )

private:
	GameTime				m_timeShift;
	Bool					m_callEvents;

public:
	void Execute();
};

BEGIN_CLASS_RTTI( CShiftTimeFunction )
	PARENT_CLASS( IQuestTimeFunction )
	PROPERTY_CUSTOM_EDIT( m_timeShift, TXT( "How much should the time be shifted" ), TXT( "DayTimeEditor" ) )
	PROPERTY_EDIT( m_callEvents, TXT( "Should the related events be sent out?" ) )
END_CLASS_RTTI()
