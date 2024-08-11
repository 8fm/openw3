
#include "build.h"
#include "waterDebug.h"
#include "../../common/engine/world.h"
#include "../engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CWaterDebug );

CWaterDebug::CWaterDebug() 
	: m_isAdded( false ) 
{

}

Float CWaterDebug::GetWaterLevel( Float x, Float y ) const
{
	Vector trace;
	Float depth = 100.0f;
	Vector intersection;
	Vector normal;
	CWorld *world = GGame->GetActiveWorld();
	Vector v( x, y, 50 );	
	return world->GetWaterLevel( v, false );
}

void CWaterDebug::Init( Float length, Int32 divides, CActor* center )
{
	m_length = length;
	m_divides = divides;
	m_center = center;

	if ( m_isAdded )
	{
		if ( m_center && m_center->GetVisualDebug() )
		{
			m_center->GetVisualDebug()->RemoveObject( this );
		}
	}
	if ( !m_isAdded && m_center && m_center->GetVisualDebug() )
	{
		m_center->GetVisualDebug()->AddObject( this );

		m_isAdded = true;
	}
}
void CWaterDebug::Reset()
{
	if ( m_isAdded && m_center && m_center->GetVisualDebug() )
	{
		m_center->GetVisualDebug()->RemoveObject( this );
	}
}
void CWaterDebug::funcReset( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Reset();
}
void CWaterDebug::funcInit( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, length, 0.0f );
	GET_PARAMETER( Int32, divides, 0 );
	GET_PARAMETER( THandle<CActor>, center, THandle<CActor>(NULL) );
	FINISH_PARAMETERS;

	Init( length, divides, center.Get() );
}
void CWaterDebug::Render( CRenderFrame* frame, const Matrix& matrix )
{
	Vector pos = m_center->GetWorldPosition();
	Float d = m_length / m_divides;
	Vector v = pos + Vector( - m_length * 0.5f, - m_length * 0.5f, 0 );
	v.Z = GetWaterLevel( v.X, v.Y );
	Vector v2 = v + Vector( d, 0 , 0 );

	for ( Int32 x = 0; x < m_divides; ++x )
	{
		v2.Z = GetWaterLevel( v2.X, v2.Y );
		frame->AddDebugLine(v,v2,Color::LIGHT_BLUE);
		v = v2;
		v2.X += d;
	}

	v = pos + Vector( - m_length * 0.5f, - m_length * 0.5f, 0 );
	v.Z = GetWaterLevel( v.X, v.Y );
	v2 = v + Vector( 0, d , 0 );

	for ( Int32 x = 0; x < m_divides; ++x )
	{
		v2.Z = GetWaterLevel( v2.X, v2.Y );
		frame->AddDebugLine(v,v2,Color::LIGHT_BLUE);
		v = v2;
		v2.Y += d;
	}

	for ( Int32 y = 1; y <= m_divides; ++y )
	{
		Vector v11 = pos + Vector( - m_length * 0.5f, - m_length * 0.5f + d * ( y - 1 ), 0 );
		v11.Z = GetWaterLevel( v11.X, v11.Y );
		Vector v21 = pos + Vector( - m_length * 0.5f + d, - m_length * 0.5f + d * ( y - 1 ), 0 );
		v21.Z = GetWaterLevel( v21.X, v21.Y );
		Vector v12 = pos + Vector( - m_length * 0.5f, - m_length * 0.5f + d * y, 0 );
		v12.Z = GetWaterLevel( v12.X, v12.Y );
		Vector v22 = pos + Vector( - m_length * 0.5f + d, - m_length * 0.5f + d * y, 0 );
		v22.Z = GetWaterLevel( v22.X, v22.Y );
		for ( int x = 1; x <= m_divides; ++x )
		{
			frame->AddDebugLine(v22,v12,Color::LIGHT_BLUE);
			frame->AddDebugLine(v22,v21,Color::LIGHT_BLUE);
			frame->AddDebugLine(v22,v11,Color::LIGHT_BLUE);
			v11 = v21;
			v12 = v22;
			v21.X += d;
			v22.X += d;
			v21.Z = GetWaterLevel( v21.X, v21.Y );
			v22.Z = GetWaterLevel( v22.X, v22.Y );
		}
	}
}
