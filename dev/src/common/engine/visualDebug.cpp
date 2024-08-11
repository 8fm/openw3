/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "visualDebug.h"
#include "../core/scriptStackFrame.h"
#include "game.h"
#include "renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CVisualDebug );

const CVisualDebug::SDebugObject CVisualDebug::SDebugObject::EMPTY;

CVisualDebug::SDebugObject& CVisualDebug::GetDebugObject( TDynArray< SDebugObject >& array, CName name )
{
	Uint32 s = array.Size();
	
	// search
	if ( name != CName::NONE )
	{
		for( Uint32 i=0; i<s; i++ )
		{
			if( array[i].m_name == name )
			{
				return array[i];
			}
		}
	}

	// not found, add new	
	array.PushBack( SDebugObject::EMPTY );
	array[s].m_name = name;
	return array[s];
}

void CVisualDebug::RemoveDebugObject( TDynArray< SDebugObject >& array, CName name )
{
	Int32 s = Int32( array.Size() );
	for( Int32 i=s-1; i>=0; i-- )
	{
		if( array[i].m_name == name )
		{
			array.Erase( array.Begin() + i );
			return;
		}		
	}	
}

void CVisualDebug::RemoveAll()
{
	m_texts.ClearFast();
	m_spheres.ClearFast();
	m_boxes.ClearFast();
	m_axis.ClearFast();
	m_lines.ClearFast();
	m_capsules.ClearFast();
	m_lastTime = EngineTime::ZERO;

	m_customObjects.Clear();
}

void CVisualDebug::UpdateObjects( TDynArray< SDebugObject >& array, Float timeDelta )
{
	Int32 s = Int32( array.Size() );
	for( Int32 i=s-1; i>=0; i-- )
	{
		SDebugObject& obj = array[i];
		if( obj.m_flags & FLAG_TIMEOUT )
		{
			obj.m_timeout -= timeDelta;
			if( obj.m_timeout <= 0.0f )
			{
				array.Erase( array.Begin() + i );
			}
		}
	}
}

void CVisualDebug::AddText( CName name, const String& text, const Vector& position, Bool absolutePos, Uint8 line, Color color, Bool background, Float timeout )
{
	SDebugObject& obj = GetDebugObject( m_texts, name );
	obj.m_text = text;
	obj.m_position = position;
	obj.m_line = line;
	obj.m_color = color;
	obj.m_timeout = timeout;	
	obj.m_flags = 0;
	obj.m_flags |= absolutePos ? FLAG_POSITION_ABSOLUTE : 0;
	obj.m_flags |= background ? FLAG_BACKGROUND : 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
}

void CVisualDebug::AddSphere( CName name, Float radius, const Vector& position, Bool absolutePos, Color color, Float timeout )
{
	SDebugObject& obj = GetDebugObject( m_spheres, name );
	obj.m_size.X = radius;	
	obj.m_position = position;
	obj.m_color = color;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= absolutePos ? FLAG_POSITION_ABSOLUTE : 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
}

void CVisualDebug::AddBox( CName name, const Vector& size, const Vector& position, const EulerAngles& rotation, Bool absolutePos, Color color, Float timeout)
{
	SDebugObject& obj = GetDebugObject( m_boxes, name );
	obj.m_size = size*0.5f;
	obj.m_position = position;
	obj.m_rotation = rotation;
	obj.m_color = color;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= absolutePos ? FLAG_POSITION_ABSOLUTE : 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
}

void CVisualDebug::AddAxis( CName name, const Float scale /* = 1.f */, const Vector& position /* = Vector::ZEROS */, const EulerAngles& rotation /* = EulerAngles::ZEROS */, Bool absolutePos /* = false */, Float timeout /* = -1.0f  */)
{
	SDebugObject& obj = GetDebugObject( m_axis, name );
	obj.m_size.X = scale;
	obj.m_position = position;
	obj.m_rotation = rotation;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= absolutePos ? FLAG_POSITION_ABSOLUTE : 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
}

void CVisualDebug::AddLine( CName name, const Vector& startPos, const Vector& endPos /*= Vector::ZEROS*/, Bool absolutePos /*= false*/, Color color /*= Color::WHITE*/, Float timeout /*= -1.0f*/ )
{
	SDebugObject& obj = GetDebugObject( m_lines, name );
	obj.m_size = endPos;
	obj.m_color = color;
	obj.m_position = startPos;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= absolutePos ? FLAG_POSITION_ABSOLUTE : 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
}

void CVisualDebug::AddArrow( CName name, const Vector& start, const Vector& end, Float arrowPostionOnLine01, Float arrowSizeX, Float arrowSizeY, const Color& color, Bool absolutePos, Bool overlay, Float timeout )
{
	SDebugObject& obj = GetDebugObject( m_arrows, name );
	obj.m_size = end;
	obj.m_color = color;
	obj.m_position = start;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= absolutePos ? FLAG_POSITION_ABSOLUTE : 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
	obj.m_flags |= overlay ? FLAG_OVERLAY : 0;
	obj.m_rotation.Pitch = arrowPostionOnLine01;
	obj.m_rotation.Roll = arrowSizeX;
	obj.m_rotation.Yaw = arrowSizeY;
}

void CVisualDebug::AddCapsule( CName name, Float radius, Float height, const Vector& position, Bool absolutePos, Color color, Float timeout )
{
	SDebugObject& obj = GetDebugObject( m_capsules, name );
	obj.m_size.X = radius;
	obj.m_size.Y = height;
	obj.m_position = position;
	obj.m_color = color;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= absolutePos ? FLAG_POSITION_ABSOLUTE : 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
}

void CVisualDebug::AddBar( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progress, Color color, const String& text, Float timeout, Bool bg )
{
	SDebugObject& obj = GetDebugObject( m_bars, name );
	obj.m_position.X = (Float)x;
	obj.m_position.Y = (Float)y;
	obj.m_size.X = (Float)width;
	obj.m_size.Y = (Float)height;
	obj.m_size.Z = progress;
	obj.m_color = color;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
	obj.m_flags |= bg ? FLAG_BACKGROUND : 0;
	obj.m_text = text;
}

void CVisualDebug::AddBarColorSmooth( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progress, Color color, const String& text, Float timeout, Bool bg )
{
	SDebugObject& obj = GetDebugObject( m_bars, name );
	obj.m_position.X = (Float)x;
	obj.m_position.Y = (Float)y;
	obj.m_size.X = (Float)width;
	obj.m_size.Y = (Float)height;
	obj.m_size.Z = progress;
	obj.m_color = color;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
	obj.m_flags |= FLAG_BAR_SMOOTH;
	obj.m_flags |= bg ? FLAG_BACKGROUND : 0;
	obj.m_text = text;
}

void CVisualDebug::AddBarColorAreas( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progress, const String& text, Float timeout, Bool bg )
{
	SDebugObject& obj = GetDebugObject( m_bars, name );
	obj.m_position.X = (Float)x;
	obj.m_position.Y = (Float)y;
	obj.m_size.X = (Float)width;
	obj.m_size.Y = (Float)height;
	obj.m_size.Z = progress;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
	obj.m_flags |= FLAG_BAR_COLOR_AREAS;
	obj.m_flags |= bg ? FLAG_BACKGROUND : 0;
	obj.m_text = text;
}

void CVisualDebug::AddRangeBar( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progressStart, Float progressEnd, Color color, const String& text, Float timeout )
{
	SDebugObject& obj = GetDebugObject( m_bars, name );
	obj.m_position.X = (Float)x;
	obj.m_position.Y = (Float)y;
	obj.m_position.Z = progressEnd;
	obj.m_size.X = (Float)width;
	obj.m_size.Y = (Float)height;
	obj.m_size.Z = progressStart;
	obj.m_color = color;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
	obj.m_flags |= FLAG_BACKGROUND;
	obj.m_flags |= FLAG_RANGE_BAR;
	obj.m_text = text;
}

void CVisualDebug::AddRangeBarWithPointer( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progressStart, Float progressEnd, Float progressPointer, Color color, const String& text, Float timeout )
{
	SDebugObject& obj = GetDebugObject( m_bars, name );
	obj.m_position.X = (Float)x;
	obj.m_position.Y = (Float)y;
	obj.m_position.Z = progressEnd;
	obj.m_rotation.Roll = progressPointer;
	obj.m_rotation.Yaw = -1.f;
	obj.m_rotation.Pitch = -1.f;
	obj.m_size.X = (Float)width;
	obj.m_size.Y = (Float)height;
	obj.m_size.Z = progressStart;
	obj.m_color = color;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
	obj.m_flags |= FLAG_BACKGROUND;
	obj.m_flags |= FLAG_RANGE_BAR_PTR;
	obj.m_text = text;
}

void CVisualDebug::AddRangeBarWithPointer( CName name, Int32 x, Int32 y, Int32 width, Int32 height, Float progressStartA, Float progressEndA, Float progressStartB, Float progressEndB, Float progressPointer, Color color, const String& text, Float timeout )
{
	SDebugObject& obj = GetDebugObject( m_bars, name );
	obj.m_position.X = (Float)x;
	obj.m_position.Y = (Float)y;
	obj.m_position.Z = progressEndA;
	obj.m_rotation.Roll = progressPointer;
	obj.m_rotation.Yaw = progressStartB;
	obj.m_rotation.Pitch = progressEndB;
	obj.m_size.X = (Float)width;
	obj.m_size.Y = (Float)height;
	obj.m_size.Z = progressStartA;
	obj.m_color = color;
	obj.m_timeout = timeout;
	obj.m_flags = 0;
	obj.m_flags |= (timeout > 0.0f) ? FLAG_TIMEOUT : 0;
	obj.m_flags |= FLAG_BACKGROUND;
	obj.m_flags |= FLAG_RANGE_BAR_PTR;
	obj.m_text = text;
}

void CVisualDebug::AddObject( IVisualDebugInterface* object )
{
	ASSERT( !m_customObjects.Exist( object ) );
	m_customObjects.PushBack( object );
}

void CVisualDebug::RemoveObject( IVisualDebugInterface* object )
{
	VERIFY( m_customObjects.Remove( object ) );
}

void CVisualDebug::Render( CRenderFrame* frame, const Matrix& matrix )
{	
	EngineTime time = GGame->GetEngineTime();
	float timeDelta = time - m_lastTime;

	// if last time is zero this is first render after construction
	if( m_lastTime != EngineTime::ZERO )
	{	
		UpdateObjects( m_texts, timeDelta );
		UpdateObjects( m_spheres, timeDelta );
		UpdateObjects( m_boxes, timeDelta );
		UpdateObjects( m_axis, timeDelta );
		UpdateObjects( m_lines, timeDelta );
		UpdateObjects( m_capsules, timeDelta );
		UpdateObjects( m_boxes, timeDelta );
		UpdateObjects( m_arrows, timeDelta );
	}

	m_lastTime = time;

	for( Uint32 i=0; i<m_texts.Size(); i++ )
	{
		const SDebugObject& obj = m_texts[i];
		Bool background = ( obj.m_flags & FLAG_BACKGROUND ) != 0;
		frame->AddDebugText( obj.CalcPos( matrix.GetTranslation() ), obj.m_text, 0, obj.m_line, background, obj.m_color );
	}

	for( Uint32 i=0; i<m_spheres.Size(); i++ )
	{
		const SDebugObject& obj = m_spheres[i];
		frame->AddDebugSphere( obj.CalcPos( matrix.GetTranslation() ), obj.m_size.X, Matrix::IDENTITY, obj.m_color );
	}
		
	Box box;
	Matrix mat;	
	for( Uint32 i=0; i<m_boxes.Size(); i++ )
	{
		const SDebugObject& obj = m_boxes[i];		
		obj.CalcMatrix( matrix, mat );
		box.Min = -obj.m_size;
		box.Max = obj.m_size;
		frame->AddDebugBox( box, mat, obj.m_color );		
	}

	for( Uint32 i=0; i<m_axis.Size(); i++ )
	{
		const SDebugObject& obj = m_axis[i];
		obj.CalcMatrix( matrix, mat );
		frame->AddDebugAxis( mat.GetTranslation(), mat, obj.m_size.X );
	}

	for( Uint32 i=0; i<m_lines.Size(); i++ )
	{
		const SDebugObject& obj = m_lines[i];
		Bool absolute = obj.m_flags & FLAG_POSITION_ABSOLUTE;
		if( absolute )
		{
			frame->AddDebugLine( obj.m_position, obj.m_size, obj.m_color );
		}
		else
		{			
			Vector a = matrix.TransformPoint( obj.m_position );
			Vector b = matrix.TransformPoint( obj.m_size );
			frame->AddDebugLine( a, b, obj.m_color );
		}
	}

	for( Uint32 i=0; i<m_capsules.Size(); i++ )
	{
		const SDebugObject& obj = m_capsules[i];
		FixedCapsule capsule( obj.CalcPos( matrix.GetTranslation() ), obj.m_size.X, obj.m_size.Y );
		frame->AddDebugCapsule( capsule, Matrix::IDENTITY, obj.m_color );
	}

	for ( Uint32 i=0; i<m_arrows.Size(); ++i )
	{
		SDebugObject& obj = m_arrows[i];

		Bool absolute = obj.m_flags & FLAG_POSITION_ABSOLUTE;
		if( absolute )
		{
			frame->AddDebugLineWithArrow( obj.m_position, obj.m_size, obj.m_rotation.Pitch, obj.m_rotation.Roll, obj.m_rotation.Yaw, obj.m_color, ( obj.m_flags & FLAG_OVERLAY ) > 0 );
		}
		else
		{			
			Vector a = matrix.TransformPoint( obj.m_position );
			Vector b = matrix.TransformPoint( obj.m_size );
			frame->AddDebugLineWithArrow( a, b, obj.m_rotation.Pitch, obj.m_rotation.Roll, obj.m_rotation.Yaw, obj.m_color, ( obj.m_flags & FLAG_OVERLAY ) > 0 );
		}
	}

	for ( Uint32 i=0; i<m_bars.Size(); ++i )
	{
		const SDebugObject& obj = m_bars[i];

		const Bool smooth = ( obj.m_flags & FLAG_BAR_SMOOTH ) > 0;
		const Bool areas = ( obj.m_flags & FLAG_BAR_COLOR_AREAS ) > 0;
		const Bool range = ( obj.m_flags & FLAG_RANGE_BAR ) > 0;
		const Bool rangePtr = ( obj.m_flags & FLAG_RANGE_BAR_PTR ) > 0;
		const Bool bg = ( obj.m_flags & FLAG_BACKGROUND ) > 0;

		ASSERT( !( smooth && areas ) );

		const Int32 x = (Int32)obj.m_position.X;
		const Int32 y = (Int32)obj.m_position.Y;
		const Int32 barWidth = (Int32)obj.m_size.X;
		const Int32 barHeight = (Int32)obj.m_size.Y;
		const Float progress = obj.m_size.Z;

		static const Color bgColor( 0,0,0 );
		Color fColor = obj.m_color;

		if ( smooth )
		{
			fColor.Mul3( progress );
		}
		else if ( areas )
		{

			static Color fColor1( 0, 255, 0 );
			static Color fColor2( 255, 255, 0 );
			static Color fColor3( 255, 0, 0 );
			static Color fColor4( 255, 255, 255 );

			if ( progress <= 0.25f )
			{
				fColor = fColor1;
			}
			else if ( progress <= 0.5f )
			{
				fColor = fColor2;
			}
			else if ( progress <= 0.75f )
			{
				fColor = fColor3;
			}
			else
			{
				fColor = fColor4;
			}
		}
		
		if ( bg )
		{
			frame->AddDebugRect( x - 1, y - 1, barWidth + 2, barHeight + 2, bgColor );
		}

		if ( !range && !rangePtr )
		{
			const Int32 progressWidth = Clamp< Int32 >( (Int32)( progress * barWidth ), 0, barWidth );
			frame->AddDebugRect( x, y,  progressWidth, barHeight, fColor );
		}
		else
		{
			const Float progressEnd = obj.m_position.Z;

			const Int32 progressOffset = Clamp< Int32 >( (Int32)( progress * barWidth ), 0, barWidth );
			const Int32 progressWidth = Clamp< Int32 >( (Int32)( (progressEnd-progress) * barWidth ), 0, barWidth );

			frame->AddDebugRect( x+progressOffset, y,  progressWidth, barHeight, fColor );

			const Float progressStartB = obj.m_rotation.Yaw;
			const Float progressEndB = obj.m_rotation.Pitch;

			if ( progressStartB > 0.f )
			{
				const Int32 progressOffsetB = Clamp< Int32 >( (Int32)( progressStartB * barWidth ), 0, barWidth );
				const Int32 progressWidthB = Clamp< Int32 >( (Int32)( (progressEndB-progressStartB) * barWidth ), 0, barWidth );

				frame->AddDebugRect( x+progressOffsetB, y,  progressWidthB, barHeight, fColor );
			}

			if ( rangePtr )
			{
				const Float progressPtr = obj.m_rotation.Roll;

				const Int32 ptrOffset = Clamp< Int32 >( (Int32)( progressPtr * barWidth ), 0, barWidth );
				const Int32 ptrWidth = Clamp< Int32 >( (Int32)( 0.01f * barWidth ), 1, barWidth );

				const Bool insideA = progressPtr >= progress && progressPtr <= progressEnd;
				const Bool insideB = progressStartB > 0.f && progressPtr >= progressStartB && progressPtr <= progressEndB;

				Color ptrColor = insideA || insideB ? Color( 255, 128, 128 ) : Color( 128, 128, 128 );

				frame->AddDebugRect( x+ptrOffset, y,  ptrWidth, barHeight, ptrColor );
			}
		}

		if ( !obj.m_text.Empty() )
		{
			frame->AddDebugScreenText( x + 100, y + 10, obj.m_text.AsChar() );
		}
	}

	const Uint32 coSize = m_customObjects.Size();
	for ( Uint32 i=0; i<coSize; i++ )
	{
		m_customObjects[ i ]->Render( frame, matrix );
	}
}

void CVisualDebug::funcAddText( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( String, text, String::EMPTY );
	GET_PARAMETER_OPT( Vector, pos, Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, absolute, false );
	GET_PARAMETER_OPT( Uint8, line, 0 );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	GET_PARAMETER_OPT( Bool, background, false );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;
	AddText( name, text, pos, absolute, line, color, background, timeout);
}

void CVisualDebug::funcAddSphere( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Float, radius, 0.0f );
	GET_PARAMETER_OPT( Vector, pos, Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, absolute, false );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;
	AddSphere( name, radius, pos, absolute, color, timeout);
}

void CVisualDebug::funcAddBox( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Vector, size, Vector::ZEROS );
	GET_PARAMETER_OPT( Vector, pos, Vector::ZEROS );
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( Bool, absolute, false );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;
	AddBox( name, size, pos, rot, absolute, color, timeout);
}

void CVisualDebug::funcAddAxis( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Float, scale, 1.0f );
	GET_PARAMETER_OPT( Vector, pos, Vector::ZEROS );
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( Bool, absolute, false );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;
	AddAxis( name, scale, pos, rot, absolute, timeout );
}

void CVisualDebug::funcAddLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Vector, posS, Vector::ZEROS );
	GET_PARAMETER_OPT( Vector, posE, Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, absolute, false );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;
	AddLine( name, posS, posE, absolute, color, timeout );
}

void CVisualDebug::funcAddArrow( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Vector, start, Vector::ZEROS );
	GET_PARAMETER_OPT( Vector, end, Vector::EY );
	GET_PARAMETER_OPT( Float, arrowPostionOnLine01, 1.0f );
	GET_PARAMETER_OPT( Float, arrowSizeX, 1.0f );
	GET_PARAMETER_OPT( Float, arrowSizeY, 1.0f );
	GET_PARAMETER_OPT( Bool, absolute, false );
	GET_PARAMETER_OPT( Color, color, Color::WHITE );
	GET_PARAMETER_OPT( Bool, overlay, false );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;

	AddArrow( name, start, end, arrowPostionOnLine01, arrowSizeX, arrowSizeY, color, absolute, overlay, timeout );
}

void CVisualDebug::funcAddBar( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Int32, x, 0 );
	GET_PARAMETER( Int32, y, 0 );
	GET_PARAMETER( Int32, width, 0 );
	GET_PARAMETER( Int32, height, 0 );
	GET_PARAMETER( Float, progress, 0.f );
	GET_PARAMETER( Color, color, Color::WHITE );
	GET_PARAMETER_OPT( String, text, String::EMPTY );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;

	AddBar( name, x, y, width, height, progress, color, text, timeout );
}

void CVisualDebug::funcAddBarColorSmooth( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Int32, x, 0 );
	GET_PARAMETER( Int32, y, 0 );
	GET_PARAMETER( Int32, width, 0 );
	GET_PARAMETER( Int32, height, 0 );
	GET_PARAMETER( Float, progress, 0.f );
	GET_PARAMETER( Color, color, Color::WHITE );
	GET_PARAMETER_OPT( String, text, String::EMPTY );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;

	AddBarColorSmooth( name, x, y, width, height, progress, color, text, timeout );
}

void CVisualDebug::funcAddBarColorAreas( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Int32, x, 0 );
	GET_PARAMETER( Int32, y, 0 );
	GET_PARAMETER( Int32, width, 0 );
	GET_PARAMETER( Int32, height, 0 );
	GET_PARAMETER( Float, progress, 0.f );
	GET_PARAMETER_OPT( String, text, String::EMPTY );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;

	AddBarColorAreas( name, x, y, width, height, progress, text, timeout );
}

void CVisualDebug::funcRemoveText( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RemoveText( name );
}

void CVisualDebug::funcRemoveSphere( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RemoveSphere( name );
}

void CVisualDebug::funcRemoveBox( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RemoveBox( name );
}

void CVisualDebug::funcRemoveAxis( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RemoveAxis( name );
}

void CVisualDebug::funcRemoveLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RemoveLine( name );
}

void CVisualDebug::funcRemoveArrow( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RemoveArrow( name );
}

void CVisualDebug::funcRemoveBar( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RemoveBar( name );
}
