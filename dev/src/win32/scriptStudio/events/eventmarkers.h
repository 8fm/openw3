/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef _SS_EVENT_MARKERS_H_
#define _SS_EVENT_MARKERS_H_

#include "../solution/slnDeclarations.h"

enum EMarkerState
{
	Marker_Enabled = 0,
	Marker_Disabled,
	Marker_Deleted
};

class CMarkerToggledEvent : public wxEvent
{
public:
	CMarkerToggledEvent( wxEventType commandType, int winid );
	CMarkerToggledEvent( wxEventType type, const SolutionFilePtr& file, Red::System::Int32 line, EMarkerState state );
	virtual ~CMarkerToggledEvent();

	inline SolutionFilePtr& GetFile() { return m_file; }
	inline const SolutionFilePtr& GetFile() const { return m_file; }

	inline Red::System::Int32 GetLine() const { return m_line; }
	inline EMarkerState GetState() const { return m_state; }

protected:
	SolutionFilePtr m_file;
	Red::System::Int32 m_line;
	EMarkerState m_state;

	wxDECLARE_ABSTRACT_CLASS( CMarkerToggledEvent );
};

#define DECLARE_MARKER_EVENT( MarkerEventClass, MarkerEventId )													\
class MarkerEventClass : public CMarkerToggledEvent																\
{																												\
public:																											\
	MarkerEventClass( wxEventType commandType = wxEVT_NULL, int winid = 0 );									\
	MarkerEventClass( const SolutionFilePtr& file, Red::System::Int32 line, EMarkerState state );				\
	virtual ~MarkerEventClass();																				\
																												\
private:																										\
	virtual wxEvent* Clone() const override final { return new MarkerEventClass( m_file, m_line, m_state ); }	\
																												\
	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( MarkerEventClass );														\
};																												\
wxDECLARE_EVENT( MarkerEventId, MarkerEventClass )

#define IMPLEMENT_MARKER_EVENT( MarkerEventClass, MarkerEventId )												\
MarkerEventClass::MarkerEventClass( wxEventType commandType, int winid )										\
:	CMarkerToggledEvent( commandType, winid ) {}																\
MarkerEventClass::MarkerEventClass( const SolutionFilePtr& file, Red::System::Int32 line, EMarkerState state )	\
:	CMarkerToggledEvent( MarkerEventId, file, line, state ) {}													\
MarkerEventClass::~MarkerEventClass() {}																		\
wxDEFINE_EVENT( MarkerEventId, MarkerEventClass );																\
wxIMPLEMENT_DYNAMIC_CLASS( MarkerEventClass, CMarkerToggledEvent );

#endif // _SS_EVENT_MARKERS_H_
