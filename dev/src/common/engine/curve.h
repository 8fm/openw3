/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../core/curveBase.h"

class CCurve : public CObject, public SCurveBase
{
	DECLARE_ENGINE_CLASS( CCurve, CObject, 0 );

protected:
	Color	m_color;

public:
	CCurve( const Color& color = Color( 255, 255, 255 ) );
	CCurve( const Float value, const Color& color = Color( 255, 255, 255 ) );
	virtual ~CCurve();

	virtual void	OnSerialize( IFile& file );
	virtual void	OnSerializeXML( IXMLFile& file );

	Color	GetColor() const { return m_color; }
	void	SetColor( const Color& color ) { m_color = color; }

protected:
	void funcGetValue( CScriptStackFrame& stack, void* result );
	void funcGetDuration( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCurve );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_color, TXT("Curve color") );
	PROPERTY_NAME( m_data.m_baseType, TXT("dataBaseType") );
	PROPERTY_EDIT( m_data.m_loop, TXT("Loop") );

	// TODO: Scripting interface has not been updated yet.
	NATIVE_FUNCTION( "GetValue", funcGetValue );
	NATIVE_FUNCTION( "GetDuration", funcGetDuration );
END_CLASS_RTTI();


namespace
{
	RED_INLINE void GetLowerIndex( Float t, Uint32& index, Uint32 num, Float& frac )
	{
		const Float indexWithFrac = t * ( num - 1 );
		const Float fIndex = MFloor( indexWithFrac );
		frac = indexWithFrac - fIndex;
		index = ::Clamp< Uint32 >( (Uint32) fIndex, 0, num - 2 );
	}
}

///--
// BIG TODO: the curves should be VECTORIZED!!! (instead of having 4 curves of floats we should have one curve of Vector4)
///--
class CurveApproximation
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_CurveParam );

public:
	enum ESampleCount
	{
		NUM_SAMPLES = 16 // can be changed without recooking
	};

	typedef Float Sample;

	CurveApproximation();	

	void BuildFrom( const CCurve* sourceData );
	void Serialize( IFile& ar );

	RED_FORCE_INLINE const Sample GetFloatValue( Float time ) const
	{
		Uint32 index;
		Float frac;
		Float normalizedTime = Clamp< Float >( time, 0.0f, 1.0f );
		GetLowerIndex( normalizedTime, index, NUM_SAMPLES, frac );

		return Red::Math::NumericalUtils::Lerp< Sample >( frac, m_samples[ index ], m_samples[ index+1 ] );
	}

	friend IFile& operator<<( IFile& ar, CurveApproximation& data )
	{
		data.Serialize( ar );
		return ar;
	}

private:
	Sample		m_samples[ NUM_SAMPLES ];
};

class CurveParameter
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_CurveParam );

	static const Uint32		MAX_CURVES = 4;

	CName 	m_name;
	Uint8	m_counts; // 0-3
	Uint8	m_isCooked; // 0 or 1

	// TODO: make an union
	CCurve*					m_curves[4];  // only valid when m_isCooked = false
	CurveApproximation*		m_approx[4];  // only valid when m_isCooked = true

#ifndef NO_EDITOR
	Color	m_color;
#endif

public:
	RED_INLINE CurveParameter()
		: m_counts( 0 )
		, m_isCooked( 0 )
	{
		ZeroData();
	}

	RED_INLINE CurveParameter( const CurveParameter& other )
		: m_counts( 0 )
		, m_isCooked( 0 )
	{
		ZeroData();
		CopyData( other );
	}

	RED_INLINE CurveParameter( const CName& paramName, const Int32 curveCount )
		: m_name( paramName )
		, m_counts( (Uint8)curveCount )
		, m_isCooked( 0 )
	{
		ZeroData();
	}

	~CurveParameter()
	{
		ClearData();
	}

	RED_INLINE Uint32			GetCurveCount() const { return m_counts; }
	RED_INLINE CName			GetName() const { return m_name; }
	RED_INLINE void			SetName( const CName& paramName ) { m_name = paramName; }

#ifndef NO_EDITOR
	RED_INLINE void			SetColor( const Color& curveColor ) { m_color = curveColor; }
	RED_INLINE Color			GetColor() const { return m_color; }
#endif

	RED_INLINE void			SetCurve( const Int32 curveIndex, CCurve* curve ) { m_curves[curveIndex] = curve; }
	RED_INLINE void			SetCurveCount( Uint32 count ) { RED_FATAL_ASSERT( count < MAX_CURVES, "Invalid curve count" ); m_counts = (Uint8)count; }
	RED_INLINE CCurve*		GetCurve( const Int32 curveIndex ) { return m_curves[curveIndex]; }
	RED_INLINE const CCurve*	GetCurve( const Int32 curveIndex ) const { return m_curves[curveIndex]; }

	RED_INLINE Float			GetCurveValue( Uint32 curveIdx, Float time ) const
	{
		// nasty fail save - happening to often ATM :(
		RED_ASSERT( curveIdx < m_counts, TXT( "Out of Bound curve id! FIXME!" ) );
		if ( curveIdx >= m_counts )
			return 0.0f;

#ifndef RED_FINAL_BUILD
		if ( !m_isCooked )
		{
			RED_FATAL_ASSERT( m_curves[ curveIdx ], "Missing curve data. Expect crash." );
			return m_curves[ curveIdx ]->GetFloatValue( time );
		}
		else
#endif
		{
			RED_FATAL_ASSERT( m_approx[ curveIdx ], "Missing curve data. Expect crash." );
			return m_approx[ curveIdx ]->GetFloatValue( time );
		}
	}

	void CreateCurves( const CName& paramName, const Uint32 curveCount, CObject* curveParent );
	void DiscardCurves();
	void ChangeCurveCount( const Uint32 curveCount, CObject* curveParent );

	friend IFile& operator<<( IFile& file, CurveParameter& param )
	{
#ifndef RED_FINAL_BUILD
		if ( file.GetVersion() < VER_CURVE_COOKING )
		{
			param.SerializeLegacy( file );
		}
		else
#endif
		{
			param.Serialize( file );
		}

		return file;
	}

private:
	RED_INLINE void ZeroData()
	{
		for ( Uint32 i=0; i<MAX_CURVES; ++i )
		{
			m_approx[i] = NULL;
			m_curves[i] = NULL;
		}
	}

	void Serialize( IFile& file );

#ifndef RED_FINAL_BUILD
	void SerializeLegacy( IFile& file );
	void CookOnSerialize( IFile& file );
#endif

	void CopyData( const CurveParameter& other );
	void ClearData();		
};

class CurveGroup
{
	CName									m_groupName;
	TDynArray< CurveParameter, MC_Engine >	m_curveParams;

public:
	CurveGroup( const CName& groupName )
		: m_groupName( groupName )
	{
	}

	RED_INLINE void		Clear() { m_curveParams.Clear(); }
	RED_INLINE Uint32		GetCount() const { return m_curveParams.Size(); }
	RED_INLINE CName		GetGroupName() { return m_groupName; }
	RED_INLINE void		SetGroupName( const CName& groupName ) { m_groupName = groupName; }

	RED_INLINE void CreateCurves( const CName& paramName, const Int32 curveCount, CObject* curveParent )
	{
		ASSERT( curveParent );
		Int32 index = m_curveParams.Size();
		m_curveParams.Resize( index + 1 );
		m_curveParams[index].CreateCurves( paramName, curveCount, curveParent );
	}
	
	RED_INLINE void AddCurve( const CName& paramName, CCurve* curve )
	{
		ASSERT( curve );
		CurveParameter param = CurveParameter( paramName, 1 );
		param.SetCurve( 0, curve );
		m_curveParams.PushBack( param );
	}

	RED_INLINE void AddCurveParameter( const CurveParameter& curveParam )
	{
		m_curveParams.PushBack( curveParam );
	}

	RED_INLINE CurveParameter* GetCurveParameter( const CName& curveName )
	{
		for ( Uint32 i = 0; i < m_curveParams.Size(); ++i )
		{
			if ( m_curveParams[i].GetName() == curveName )
			{
				return &m_curveParams[i];
			}
		}
		return NULL;
	}
	RED_INLINE const CurveParameter* GetCurveParameter( const CName& curveName ) const
	{
		for ( Uint32 i = 0; i < m_curveParams.Size(); ++i )
		{
			if ( m_curveParams[i].GetName() == curveName )
			{
				return &m_curveParams[i];
			}
		}
		return NULL;
	}
	RED_INLINE CurveParameter& GetCurveParameter( const Int32 index ) {	return m_curveParams[index]; }
	RED_INLINE const CurveParameter& GetCurveParameter( const Int32 index ) const {	return m_curveParams[index]; }

	RED_INLINE Bool operator=( const CurveGroup& other ) // why bool?!
	{
		m_groupName = m_groupName;
		m_curveParams = other.m_curveParams;
		return true;
	}

	friend IFile& operator<<( IFile& file, CurveGroup& group )
	{
		file << group.m_groupName;
		file << group.m_curveParams;
		return file;
	}
};

namespace RedPropertyBuilder
{
	RED_INLINE CProperty* CreateRangedProperty
		(
		CClass* parentClass,
		size_t offset,
		const CName& name,
		const CName& typeName,
		const String& hint,
		Uint32 flags,
		CCurve* tag,
		Float minValue,
		Float maxValue,
		const String& customEditor,
		Bool customEditorArray
		)
	{
		return CreateRangedProperty( parentClass, offset, name, typeName, hint, flags, static_cast<SCurveBase*>( tag ), minValue, maxValue, customEditor, customEditorArray );
	}
}
