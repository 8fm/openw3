/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//---------------------------------------------------------------------------

// This file contains some basic integer based math for world position related calculations.
// Assumptions are that the world size even if huge is limited to some extent so we can
// represent the coordinates in the world space with some pre-defined precission (like 10 bits per meter).
// and still be able to represent a world that is 22bits (~4096 km) in size.
// Extra bits are there for allowing easier computations without overflow.

//---------------------------------------------------------------------------

/// Unit covnersion helper between engine and trigger system
/// Trigger system internally stores all locations as integers to preserve precission
class IntegerUnit
{
private:
	static const Float TRIGGER_TO_WORLD;
	static const Float WORLD_TO_TRIGGER;

	Int32 m_value;

public:
	RED_INLINE IntegerUnit()
		: m_value(0)
	{}

	RED_INLINE explicit IntegerUnit(const Int32 val)
		: m_value(val)
	{
	}

	RED_INLINE explicit IntegerUnit(const Float val)
		: m_value( ToTriggerUnits(val) )
	{
	}

	RED_INLINE Bool operator==(const IntegerUnit& other) const
	{
		return m_value == other.m_value;
	}

	RED_INLINE Bool operator!=(const IntegerUnit& other) const
	{
		return m_value != other.m_value;
	}

	RED_INLINE Bool operator<=(const IntegerUnit& other) const
	{
		return m_value <= other.m_value;
	}

	RED_INLINE Bool operator>=(const IntegerUnit& other) const
	{
		return m_value >= other.m_value;
	}

	RED_INLINE Bool operator<(const IntegerUnit& other) const
	{
		return m_value < other.m_value;
	}

	RED_INLINE Bool operator>(const IntegerUnit& other) const
	{
		return m_value > other.m_value;
	}

	RED_INLINE IntegerUnit operator-(const IntegerUnit& other) const
	{
		return IntegerUnit( m_value - other.m_value );
	}

	RED_INLINE IntegerUnit operator+(const IntegerUnit& other) const
	{
		return IntegerUnit( m_value + other.m_value );
	}

	RED_INLINE IntegerUnit operator/(const Int32 value) const
	{
		ASSERT(value != 0);
		return IntegerUnit( m_value / value );
	}

	RED_INLINE IntegerUnit operator*(const Int32 value) const
	{
		ASSERT(value != 0);
		return IntegerUnit( m_value * value );
	}

	RED_INLINE IntegerUnit& operator=(const IntegerUnit& other)
	{
		m_value = other.m_value;
		return *this;
	}

	// convert to floating point world units, returned value can be inprecise
	// newer use floating point arithmethic on trigger units, 
	// especially do not subtract large coordinates from each other when using floats
	RED_INLINE const Float ToFloat() const
	{
		return FromTriggerUnits(m_value);
	}

	// convert to integer value
	RED_INLINE const Int32 ToInt32() const
	{
		return m_value;
	}

	// serialization
	RED_INLINE friend IFile& operator<<(IFile& f, IntegerUnit& u)
	{
		return f << u.m_value;
	}

private:
	static Int32 ToTriggerUnits(const Float x);
	static Float FromTriggerUnits(const Int32 x);
};

//---------------------------------------------------------------------------

struct IntegerVector4
{
public:
	IntegerUnit X,Y,Z;

private:
	// padding (so we can use this as a SSE vector)
	Uint32 __padding;

public:
	RED_INLINE IntegerVector4()
	{}

	RED_INLINE explicit IntegerVector4(const Int32 vx, const Int32 vy, const Int32 vz)
		: X(vx)
		, Y(vy)
		, Z(vz)
	{}

	RED_INLINE explicit IntegerVector4(const IntegerUnit& vx, const IntegerUnit& vy, const IntegerUnit& vz)
		: X(vx)
		, Y(vy)
		, Z(vz)
	{}

	RED_INLINE IntegerVector4(const IntegerVector4& other)
		: X(other.X)
		, Y(other.Y)
		, Z(other.Z)
	{}

	RED_INLINE explicit IntegerVector4(const Vector3& pos)
		: X(pos.X)
		, Y(pos.Y)
		, Z(pos.Z)
	{
	}

	RED_INLINE explicit IntegerVector4(const Vector& pos)
		: X(pos.X)
		, Y(pos.Y)
		, Z(pos.Z)
	{
	}

	RED_INLINE IntegerVector4 operator-(const IntegerVector4& other) const
	{
		return IntegerVector4(X - other.X, Y - other.Y, Z - other.Z);
	}

	RED_INLINE IntegerVector4 operator+(const IntegerVector4& other) const
	{
		return IntegerVector4(X + other.X, Y + other.Y, Z + other.Z);
	}

	RED_INLINE Bool operator!=(const IntegerVector4& other) const
	{
		return (X != other.X) || (Y != other.Y) || (Z != other.Z);
	}

	RED_INLINE Bool operator==(const IntegerVector4& other) const
	{
		return (X == other.X) && (Y == other.Y) && (Z == other.Z);
	}

	RED_INLINE IntegerVector4 operator/(const Int32 v) const
	{
		return IntegerVector4(X / v, Y / v, Z / v);
	}	

	RED_INLINE Vector3 ToVector() const
	{
		return Vector3( X.ToFloat(), Y.ToFloat(), Z.ToFloat() );
	}

	RED_INLINE friend IFile& operator<<(IFile& f, IntegerVector4& p)
	{
		return f << p.X << p.Y << p.Z;
	}

	RED_INLINE void SetMin(const IntegerVector4& v)
	{
		X = Min(X, v.X);
		Y = Min(Y, v.Y);
		Z = Min(Z, v.Z);
	}

	RED_INLINE void SetMax(const IntegerVector4& v)
	{
		X = Max(X, v.X);
		Y = Max(Y, v.Y);
		Z = Max(Z, v.Z);
	}

	RED_INLINE Float SqrLength( void ) const
	{
		const Float vX = X.ToFloat();
		const Float vY = Y.ToFloat();
		const Float vZ = Z.ToFloat();
		return (vX * vX + vY * vY + vZ * vZ);
	}	
};

//---------------------------------------------------------------------------

/// Trigger space bounds
struct IntegerBox
{
public:
	IntegerVector4 Min;
	IntegerVector4 Max;

public:
	RED_INLINE IntegerBox()
		: Min(INT_MAX, INT_MAX, INT_MAX)
		, Max(INT_MIN, INT_MIN, INT_MIN)
	{
	}

	RED_INLINE IntegerBox(const IntegerVector4& min, const IntegerVector4& max)
		: Min(min)
		, Max(max)
	{
	}

	RED_INLINE IntegerBox(const IntegerBox& box)
		: Min(box.Min)
		, Max(box.Max)
	{
	}

	RED_INLINE IntegerBox& operator=(const IntegerBox& other)
	{
		Min = other.Min;
		Max = other.Max;
		return* this;
	}

	RED_INLINE IntegerBox& AddPoint(const IntegerVector4& v)
	{
		Min.SetMin(v);
		Max.SetMax(v);
		return *this;
	}

	RED_INLINE IntegerBox& AddBox(const IntegerBox& box)
	{
		Min.SetMin(box.Min);
		Max.SetMax(box.Max);
		return *this;
	}

	RED_INLINE Bool IsEmpty() const
	{
		return (Min.X >= Max.X) || (Min.Y >= Max.Y) || (Min.Z >= Max.Z);
	}

	RED_INLINE Bool Contains(const IntegerVector4& point) const
	{
		if (point.X < Min.X) return false;
		if (point.Y < Min.Y) return false;
		if (point.Z < Min.Z) return false;
		if (point.X > Max.X) return false;
		if (point.Y > Max.Y) return false;
		if (point.Z > Max.Z) return false;
		return true;
	}

	RED_INLINE Bool Contains(const IntegerBox& box) const
	{
		if (box.Min.X < Min.X) return false;
		if (box.Min.Y < Min.Y) return false;
		if (box.Min.Z < Min.Z) return false;
		if (box.Max.X > Max.X) return false;
		if (box.Max.Y > Max.Y) return false;
		if (box.Max.Z > Max.Z) return false;
		return true;
	}

	RED_INLINE Bool Touches(const IntegerBox& box) const
	{
		if (box.Max.X < Min.X) return false;
		if (box.Max.Y < Min.Y) return false;
		if (box.Max.Z < Min.Z) return false;
		if (box.Min.X > Max.X) return false;
		if (box.Min.Y > Max.Y) return false;
		if (box.Min.Z > Max.Z) return false;
		return true;
	}

	RED_INLINE IntegerVector4 CalcCenter() const
	{
		IntegerVector4 v;
		v.X = (Min.X + Max.X) / 2;
		v.Y = (Min.Y + Max.Y) / 2;
		v.Z = (Min.Z + Max.Z) / 2;
		return v;
	}
};

//---------------------------------------------------------------------------
