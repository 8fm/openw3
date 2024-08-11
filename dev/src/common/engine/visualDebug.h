/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../core/engineTime.h"

class CRenderFrame;

class IVisualDebugInterface
{
public:
	virtual void Render( CRenderFrame* frame, const Matrix& matrix ) = 0;
};

// Retained debug visualization
class CVisualDebug : public CObject
{
	DECLARE_ENGINE_CLASS( CVisualDebug, CObject, 0 );
protected:
	struct SDebugObject
	{
		SDebugObject()
			: m_position( Vector::ZEROS )
			, m_size( Vector::ZEROS )
			, m_rotation( EulerAngles::ZEROS )
			, m_color( Color::WHITE )									
			, m_timeout( 0.0f )
			, m_flags( 0 )
			, m_line( 0 )
		{}
		
		Vector	m_position;
		Vector	m_size;
		EulerAngles m_rotation;
		Color	m_color;
		CName	m_name;
		String	m_text;						
		Float	m_timeout;
		Uint8	m_flags;
		Uint8	m_line;

		static const SDebugObject EMPTY;

		Vector CalcPos( const Vector& offset ) const
		{
			return ( m_flags & FLAG_POSITION_ABSOLUTE ) ? m_position : ( m_position + offset );
		}

		void CalcMatrix( const Matrix& matrix, Matrix& outMat ) const
		{
			Bool absolute = m_flags & FLAG_POSITION_ABSOLUTE;			
			if( absolute )
			{
				outMat = m_rotation.ToMatrix();
				outMat.SetTranslation( m_position );
			}
			else
			{				
				Matrix objMat = m_rotation.ToMatrix();
				objMat.SetTranslation( m_position );
				outMat = Matrix::Mul( matrix, objMat );
			}			
		};
	};

	static const Uint8 FLAG_POSITION_ABSOLUTE	= FLAG( 0 );
	static const Uint8 FLAG_TIMEOUT				= FLAG( 1 );
	static const Uint8 FLAG_BACKGROUND			= FLAG( 2 );
	static const Uint8 FLAG_BAR_SMOOTH			= FLAG( 3 );
	static const Uint8 FLAG_BAR_COLOR_AREAS		= FLAG( 4 );
	static const Uint8 FLAG_RANGE_BAR			= FLAG( 5 );
	static const Uint8 FLAG_RANGE_BAR_PTR		= FLAG( 6 );
	static const Uint8 FLAG_OVERLAY				= FLAG( 7 );

	TDynArray< SDebugObject >	m_texts;
	TDynArray< SDebugObject >	m_spheres;
	TDynArray< SDebugObject >	m_boxes;
	TDynArray< SDebugObject >	m_axis;
	TDynArray< SDebugObject >	m_lines;
	TDynArray< SDebugObject >	m_capsules;
	TDynArray< SDebugObject >	m_bars;
	TDynArray< SDebugObject >	m_arrows;
	EngineTime					m_lastTime;

	TDynArray< IVisualDebugInterface* > m_customObjects;

public:
	// Add debug text
	void AddText( CName name, const String& text, const Vector& position = Vector::ZEROS, Bool absolutePos = false, Uint8 line = 0, Color color = Color::WHITE, Bool background = false, Float timeout = -1.0f );

	// Add debug sphere
	void AddSphere( CName name, Float radius, const Vector& position = Vector::ZEROS, Bool absolutePos = false, Color color = Color::WHITE, Float timeout = -1.0f );

	// Add debug box
	void AddBox( CName name, const Vector& size, const Vector& position = Vector::ZEROS, const EulerAngles& rotation = EulerAngles::ZEROS, Bool absolutePos = false, Color color = Color::WHITE, Float timeout = -1.0f );

	// Add debug axis
	void AddAxis( CName name, const Float scale = 1.f, const Vector& position = Vector::ZEROS, const EulerAngles& rotation = EulerAngles::ZEROS, Bool absolutePos = false, Float timeout = -1.0f );

	// Add debug line
	void AddLine( CName name, const Vector& startPos, const Vector& endPos = Vector::ZEROS, Bool absolutePos = false, Color color = Color::WHITE, Float timeout = -1.0f );

	// Add debug capsule
	void AddCapsule( CName name, Float radius, Float height, const Vector& position  = Vector::ZEROS, Bool absolutePos = false, Color color = Color::WHITE, Float timeout = -1.0f );

	// Add debug bar
	void AddBar( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progress, Color color, const String& text = String::EMPTY, Float timeout = -1.0f, Bool bg = false );
	void AddBarColorSmooth( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progress, Color color, const String& text = String::EMPTY, Float timeout = -1.0f, Bool bg = false );
	void AddBarColorAreas( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progress, const String& text = String::EMPTY, Float timeout = -1.0f, Bool bg = false );

	void AddRangeBar( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progressStart, Float progressEnd, Color color, const String& text = String::EMPTY, Float timeout = -1.0f );
	void AddRangeBarWithPointer( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progressStart, Float progressEnd, Float progressPointer, Color color, const String& text = String::EMPTY, Float timeout = -1.0f );
	void AddRangeBarWithPointer( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progressStartA, Float progressEndA, Float progressStartB, Float progressEndB, Float progressPointer, Color color, const String& text = String::EMPTY, Float timeout = -1.0f );

	// Add debug arrow
	void AddArrow( CName name, const Vector& start, const Vector& end, Float arrowPostionOnLine01, Float arrowSizeX, Float arrowSizeY, const Color& color, Bool absolutePos = false, Bool overlay=false, Float timeout = -1.0f );

	// Add object
	void AddObject( IVisualDebugInterface* object );

	// Remove debug text
	void RemoveText( CName name ) { RemoveDebugObject( m_texts, name ); }

	// Remove debug sphere
	void RemoveSphere( CName name ) { RemoveDebugObject( m_spheres, name ); }

	// Remove debug box
	void RemoveBox( CName name ) { RemoveDebugObject( m_boxes, name ); }

	// Remove debug axis
	void RemoveAxis( CName name ) { RemoveDebugObject( m_axis, name ); }
	
	// Remove debug axis
	void RemoveLine( CName name ) { RemoveDebugObject( m_lines, name ); }

	// Remove debug capsule
	void RemoveCapsule( CName name ) { RemoveDebugObject( m_capsules, name ); }

	// Remove debug bar
	void RemoveBar( CName name ) { RemoveDebugObject( m_bars, name ); }

	// Remove debug arrow
	void RemoveArrow( CName name ) { RemoveDebugObject( m_arrows, name ); }

	// Remove object
	void RemoveObject( IVisualDebugInterface* object );

	// Remove all objects
	void RemoveAll();

	// Render contents
	void Render( CRenderFrame* frame, const Matrix& matrix );

protected:
	SDebugObject& GetDebugObject( TDynArray< SDebugObject >& array, CName name );
	void RemoveDebugObject( TDynArray< SDebugObject >& array, CName name );
	void UpdateObjects( TDynArray< SDebugObject >& array, Float timeDelta );

private:
	void funcAddText( CScriptStackFrame& stack, void* result );
	void funcAddSphere( CScriptStackFrame& stack, void* result );
	void funcAddBox( CScriptStackFrame& stack, void* result );
	void funcAddAxis( CScriptStackFrame& stack, void* result );
	void funcAddLine( CScriptStackFrame& stack, void* result );
	void funcAddBar( CScriptStackFrame& stack, void* result );
	void funcAddBarColorSmooth( CScriptStackFrame& stack, void* result );
	void funcAddBarColorAreas( CScriptStackFrame& stack, void* result );
	void funcAddArrow( CScriptStackFrame& stack, void* result );

	void funcRemoveText( CScriptStackFrame& stack, void* result );
	void funcRemoveSphere( CScriptStackFrame& stack, void* result );
	void funcRemoveBox( CScriptStackFrame& stack, void* result );
	void funcRemoveAxis( CScriptStackFrame& stack, void* result );
	void funcRemoveLine( CScriptStackFrame& stack, void* result );
	void funcRemoveBar( CScriptStackFrame& stack, void* result );
	void funcRemoveArrow( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CVisualDebug );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "AddText", funcAddText );
	NATIVE_FUNCTION( "AddSphere", funcAddSphere );
	NATIVE_FUNCTION( "AddBox", funcAddBox );
	NATIVE_FUNCTION( "AddAxis", funcAddAxis );
	NATIVE_FUNCTION( "AddLine", funcAddLine );
	NATIVE_FUNCTION( "AddBar", funcAddBar );
	NATIVE_FUNCTION( "AddBarColorSmooth", funcAddBarColorSmooth );
	NATIVE_FUNCTION( "AddBarColorAreas", funcAddBarColorAreas );
	NATIVE_FUNCTION( "AddArrow", funcAddArrow );
	NATIVE_FUNCTION( "RemoveText", funcRemoveText );
	NATIVE_FUNCTION( "RemoveSphere", funcRemoveSphere );
	NATIVE_FUNCTION( "RemoveBox", funcRemoveBox );
	NATIVE_FUNCTION( "RemoveAxis", funcRemoveAxis );
	NATIVE_FUNCTION( "RemoveLine", funcRemoveLine );
	NATIVE_FUNCTION( "RemoveBar", funcRemoveBar );
	NATIVE_FUNCTION( "RemoveArrow", funcRemoveArrow );
END_CLASS_RTTI();
