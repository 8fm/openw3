/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _MATH_H
#define _MATH_H

#include "classBuilder.h"
#include "coreTypeRegistry.h"

#include "mathFuncsFPU.inl"

RED_DISABLE_WARNING_MSC( 4201 ) // nonstandard extension used : nameless struct/union

/************************************************************************/
/* Forward declarations                                                 */
/************************************************************************/
struct Vector;
struct Vector2;
struct Vector3;
struct Matrix;
struct EulerAngles;
struct Color;
struct Box;
struct AACylinder;

/********************************/


RED_INLINE Bool IsFinite( Float r )
{
	// Check the 8 exponent bits.
	// Usually NAN == (exponent = all 1, mantissa = non-zero)
	//         INF == (exponent = all 1, mantissa = zero)
	// This simply checks the exponent

	static_assert( sizeof( Float ) == sizeof( Uint32 ), "Fundamental type invalid size" );

	union 
	{
		Float f;
		Uint32 i;
	} val;

	val.f = r;
	return ((val.i & 0x7f800000) != 0x7f800000);
}

/********************************/
/* Base four element vector		*/
/********************************/
RED_ALIGNED_STRUCT( Vector, 16 )
{
	DECLARE_RTTI_STRUCT( Vector );

	union
	{
		struct  
		{
			Float X, Y, Z, W;
		};
		Float A[4];
	};

	RED_INLINE Vector() {}
	RED_INLINE Vector( const Vector& v );
	RED_INLINE Vector( const Float* f);									
	RED_INLINE Vector( Float x, Float y, Float z, Float w=1.0f );

	const Float* AsFloat() const { return &X; }
	Float* AsFloat() { return &X; }

	// Setting
	RED_INLINE Vector& Set3( Float x, Float y, Float z ); 
	RED_INLINE Vector& Set4( Float x, Float y, Float z, Float w ); 
	RED_INLINE Vector& Set3( const Float* value );
	RED_INLINE Vector& Set4( const Float* value );
	RED_INLINE Vector& Set3( Float value );
	RED_INLINE Vector& Set4( Float value );
	RED_INLINE Vector& Set3( const Vector& a );
	RED_INLINE Vector& Set4( const Vector& a );
	RED_INLINE Vector& SetX( Float value );
	RED_INLINE Vector& SetY( Float value );
	RED_INLINE Vector& SetZ( Float value );
	RED_INLINE Vector& SetW( Float value );

	// Special sets
	RED_INLINE Vector& SetZeros();
	RED_INLINE Vector& SetOnes();

	// Nagate vector
	RED_INLINE Vector& Negate();
	RED_INLINE Vector Negated() const;

	// Make Abs vector
	RED_INLINE Vector Abs() const;

	// Calculate vector sum/length
	RED_INLINE Float Sum3() const;
	RED_INLINE Float Sum4() const;
	RED_INLINE Float SquareMag2() const;
	RED_INLINE Float SquareMag3() const;
	RED_INLINE Float SquareMag4() const;
	RED_INLINE Float Mag2() const;
	RED_INLINE Float Mag3() const;
	RED_INLINE Float Mag4() const;

	// Calculate distance to other point
	RED_INLINE Float DistanceTo( const Vector& other ) const;

	// Calculate squared distance to other point
	RED_INLINE Float DistanceSquaredTo( const Vector& other ) const;

	// Calculate 2D distance to other point
	RED_INLINE Float DistanceTo2D( const Vector& other ) const;

	// Calculate 2D squared distance to other point
	RED_INLINE Float DistanceSquaredTo2D( const Vector& other ) const;

	// Calculate distance to edge in 3D
	RED_INLINE Float DistanceToEdge( const Vector& a, const Vector &b ) const;

	// Calculate distance to edge in 2D (XY plane)
	RED_INLINE Float DistanceToEdge2D( const Vector& a, const Vector &b ) const;

	// Calculate nearest point on edge
	RED_INLINE Vector NearestPointOnEdge(const Vector& a, const Vector& b) const;

	// Normalization
	RED_INLINE Float Normalize2();
	RED_INLINE Float Normalize3();
	RED_INLINE Float Normalize4();
	RED_INLINE Vector Normalized2() const;
	RED_INLINE Vector Normalized3() const;
	RED_INLINE Vector Normalized4() const;

	RED_INLINE Bool IsNormalized3( Float eps = 1e-4f ) const;
	RED_INLINE Bool IsNormalized4( Float eps = 1e-4f ) const;

	// Min/Max
	RED_INLINE static Vector Max4( const Vector& a, const Vector& b );
	RED_INLINE static Vector Min4( const Vector& a, const Vector& b );

	RED_INLINE static Vector Clamp4( const Vector& a, Float min, Float max );

	// Upper/Lower bound
	RED_INLINE Float Upper3() const;
	RED_INLINE Float Upper4() const;
	RED_INLINE Float Lower3() const;
	RED_INLINE Float Lower4() const;

	// Binary vector-vector operations
	RED_INLINE static Vector Add4( const Vector& a, const Vector& b );
	RED_INLINE static Vector Sub4( const Vector& a, const Vector& b );
	RED_INLINE static Vector Mul4( const Vector& a, const Vector& b );
	RED_INLINE static Vector Div4( const Vector& a, const Vector& b );
	RED_INLINE static Vector Add3( const Vector& a, const Vector& b );
	RED_INLINE static Vector Sub3( const Vector& a, const Vector& b );
	RED_INLINE static Vector Mul3( const Vector& a, const Vector& b );
	RED_INLINE static Vector Div3( const Vector& a, const Vector& b );

	// Binary vector-scalar operations
	RED_INLINE static Vector Add4( const Vector& a, Float b );
	RED_INLINE static Vector Sub4( const Vector& a, Float b );
	RED_INLINE static Vector Mul4( const Vector& a, Float b );
	RED_INLINE static Vector Div4( const Vector& a, Float b );
	RED_INLINE static Vector Div3( const Vector& a, Float b );

	// Inplace vector operators
	RED_INLINE Vector& Add4( const Vector& a );
	RED_INLINE Vector& Add3( const Vector& a );
	RED_INLINE Vector& Sub4( const Vector& a );
	RED_INLINE Vector& Sub3( const Vector& a );
	RED_INLINE Vector& Mul4( const Vector& a );
	RED_INLINE Vector& Div4( const Vector& a );
	RED_INLINE Vector& Div3( const Vector& a );

	// Inplace scalar operators
	RED_INLINE Vector& Add4( Float a );
	RED_INLINE Vector& Sub4( Float a );
	RED_INLINE Vector& Mul4( Float a );
	RED_INLINE Vector& Mul3( Float a );
	RED_INLINE Vector& Div4( Float a );
	RED_INLINE Vector& Div3( Float a );

	// Dot product
	RED_INLINE Float Dot2( const Vector& a ) const;
	RED_INLINE Float Dot3( const Vector& a ) const;
	RED_INLINE Float Dot4( const Vector& a ) const;

	// Reset i-th element
	RED_INLINE Vector ZeroElement( Uint32 i ) const;

	// Equality testing
	RED_INLINE static Bool Equal2( const Vector& a, const Vector& b );
	RED_INLINE static Bool Equal3( const Vector& a, const Vector& b );
	RED_INLINE static Bool Equal4( const Vector& a, const Vector& b );

	// Near equality testing
	RED_INLINE static Bool Near2( const Vector& a, const Vector& b, Float eps=1e-3f );
	RED_INLINE static Bool Near3( const Vector& a, const Vector& b, Float eps=1e-3f );
	RED_INLINE static Bool Near4( const Vector& a, const Vector& b, Float eps=1e-3f );

	// Dot product
	RED_INLINE static Float Dot2( const Vector& a, const Vector& b );
	RED_INLINE static Float Dot3( const Vector& a, const Vector& b );
	RED_INLINE static Float Dot4( const Vector& a, const Vector& b );

	// Cross product
	RED_INLINE static Vector Cross( const Vector& a, const Vector& b, Float w=1.0f );
	RED_INLINE static Float  Cross2( const Vector& a, const Vector& b ); // == perpendicular Dot2

	// Permute vectors
	RED_INLINE static Vector Permute( const Vector& a, const Vector&b, Uint32 x, Uint32 y, Uint32 z, Uint32 w );

	RED_INLINE Vector operator-() const;

	RED_INLINE Vector operator+( const Vector& a ) const;
	RED_INLINE Vector operator-( const Vector& a ) const;
	RED_INLINE Vector operator*( const Vector& a ) const;
	RED_INLINE Vector operator/( const Vector& a ) const;

	RED_INLINE Vector operator+( Float a ) const;
	RED_INLINE Vector operator-( Float a ) const;
	RED_INLINE Vector operator*( Float a ) const;
	RED_INLINE Vector operator/( Float a ) const;

	RED_INLINE Vector& operator=( const Vector& a );
	RED_INLINE Vector& operator+=( const Vector& a );
	RED_INLINE Vector& operator-=( const Vector& a );
	RED_INLINE Vector& operator*=( const Vector& a );
	RED_INLINE Vector& operator/=( const Vector& a );

	RED_INLINE Vector& operator+=( Float a );
	RED_INLINE Vector& operator-=( Float a );
	RED_INLINE Vector& operator*=( Float a );
	RED_INLINE Vector& operator/=( Float a );

	RED_INLINE Bool operator==( const Vector& a ) const;
	RED_INLINE Bool operator!=( const Vector& a ) const;

	RED_INLINE const Vector2& AsVector2() const;
	RED_INLINE Vector2& AsVector2();
	RED_INLINE const Vector3& AsVector3() const;
	RED_INLINE Vector3& AsVector3();

	// Convert to euler angles that will transform forward vector (EY) into this one
	RED_INLINE EulerAngles ToEulerAngles() const;
	
	// Helper for some vertex templates
	RED_INLINE Vector& Position() { return *this; };
	RED_INLINE const Vector& Position() const { return *this; };

	// Interpolate
	RED_INLINE static Vector Interpolate( const Vector& a, const Vector& b, const Float weight );
	RED_INLINE void Interpolate( const Vector& a, const Float weight );

	// Project
	RED_INLINE static Vector Project( const Vector& vector, const Vector& onNormal );

	// Checks for bad values (denormals or infinities).
	RED_INLINE Bool IsOk() const;

	RED_INLINE Uint32 GetConvertedToUByte4Color() const
	{
		Uint8 red = (Uint8) ::Clamp< Float >( A[0] * 255.0f, 0.0f, 255.0f );
		Uint8 green = (Uint8) ::Clamp< Float >( A[1] * 255.0f, 0.0f, 255.0f );
		Uint8 blue = (Uint8) ::Clamp< Float >( A[2] * 255.0f, 0.0f, 255.0f );
		Uint8 alpha = (Uint8) ::Clamp< Float >( A[3] * 255.0f, 0.0f, 255.0f );
		return (alpha<<24)|(red<<16)|(green<<8)|blue;
	}

	RED_FORCE_INLINE Uint32 CalcHash() const
	{
		const Uint32* _A = ( const Uint32* ) &A;
		return _A[ 0 ] ^ _A[ 1 ] ^ _A[ 2 ] ^ _A[ 3 ];
	}

	// Some predefined vectors
	static const Vector ZEROS;
	static const Vector ZERO_3D_POINT;
	static const Vector ONES;
	static const Vector EX;
	static const Vector EY;
	static const Vector EZ;
	static const Vector EW;
};

BEGIN_NODEFAULT_CLASS_RTTI( Vector );
	ALIGN_CLASS( 16 );
	PROPERTY_EDIT_NAME( X, TXT("X"), TXT("Vector X value") );
	PROPERTY_EDIT_NAME( Y, TXT("Y"), TXT("Vector Y value") );
	PROPERTY_EDIT_NAME( Z, TXT("Z"), TXT("Vector Z value") );
	PROPERTY_EDIT_NAME( W, TXT("W"), TXT("Vector W value") );
END_CLASS_RTTI();

// Conversion for vector
template<> RED_INLINE String ToString( const Vector& value )
{
	return String::Printf( TXT("%g %g %g %g"), value.X, value.Y, value.Z, value.W );
}

// Convert string to string
template<>
RED_INLINE Bool FromString( const String& text, Vector& value )
{
	TDynArray< String > vals = text.Split( TXT(" ") );
	RED_ASSERT( vals.Size() == 4 );
	if ( vals.Size() != 4 )
	{
		return false;
	}
	FromString( vals[0], value.X );
	FromString( vals[1], value.Y );
	FromString( vals[2], value.Z );
	FromString( vals[3], value.W );

	return true;
};

/********************************/
/* 4x4 Matrix, Row Major		*/
/********************************/
struct Matrix
{
	DECLARE_RTTI_STRUCT( Matrix );

	Vector	V[4];

	RED_INLINE Matrix() {}
	RED_INLINE Matrix( const Float* f );
	RED_INLINE Matrix( const Matrix& a );
	RED_INLINE Matrix( const Vector& x, const Vector& y, const Vector& z, const Vector& w );

	const Float* AsFloat() const { return &V[0].X; }
	Float* AsFloat() { return &V[0].X; }

	RED_INLINE Bool operator==( const Matrix& a ) const;
	RED_INLINE Bool operator!=( const Matrix& a ) const { return !( *this == a ); }
	RED_INLINE Matrix& operator=( const Matrix& rhs );

	// Setting
	RED_INLINE Matrix& Set( const Matrix& a );
	RED_INLINE Matrix& SetRows( const Float* f );
	RED_INLINE Matrix& SetRows( const Vector& x, const Vector& y, const Vector& z, const Vector& w );
	RED_INLINE Matrix& SetCols( const Float* f );
	RED_INLINE Matrix& SetCols( const Vector& x, const Vector& y, const Vector& z, const Vector& w );
	RED_INLINE Matrix& SetZeros();
	RED_INLINE Matrix& SetIdentity();

	// Set the 3x3 part of the matrix
	RED_INLINE Matrix& Set33( const Matrix& a );
	RED_INLINE Matrix& Set33( const Vector& x, const Vector& y, const Vector& z );
	RED_INLINE Matrix& SetRotX33( Float ccwRadians );
	RED_INLINE Matrix& SetRotY33( Float ccwRadians );
	RED_INLINE Matrix& SetRotZ33( Float ccwRadians );
	RED_INLINE Matrix& SetScale33( const Vector& scale );
	RED_INLINE Matrix& SetScale44( const Vector& scale );
	RED_INLINE Matrix& SetPreScale33( const Vector& scale );
	RED_INLINE Matrix& SetPreScale44( const Vector& scale );
	RED_INLINE Matrix& SetScale33( Float uniformScale );
	RED_INLINE Vector  GetScale33() const;
	RED_INLINE Vector  GetPreScale33() const;

	// Set translation part, only XYZ is used
	RED_INLINE Matrix& SetTranslation( const Vector& a );
	RED_INLINE Matrix& SetTranslation( Float x, Float y, Float z );
	RED_INLINE Vector GetTranslation() const;
	RED_INLINE const Vector& GetTranslationRef() const;
	
	// Convert rotation to euler angles
	RED_INLINE EulerAngles ToEulerAngles() const;
	RED_INLINE Float GetYaw() const;
	// Convert rotation to euler angles, works properly also for scaled matrices
	RED_INLINE EulerAngles ToEulerAnglesFull() const;
	RED_INLINE void ToAngleVectors( Vector* forward, Vector* right, Vector* up ) const;
	RED_INLINE Vector ToQuat() const;
	
	// Build more complex type of matrices
	RED_INLINE Matrix& BuildPerspectiveLH( Float fovy, Float aspect, Float zn, Float zf );
	RED_INLINE Matrix& BuildOrthoLH( Float w, Float h, Float zn, Float zf );	
	
	// Build matrix with EY from given direction vector
	RED_INLINE Matrix& BuildFromDirectionVector( const Vector& dirVec );
	RED_INLINE Matrix& BuildFromQuaternion( const Vector& quaternion );

	// Determinant
	RED_INLINE Float Det() const;
	RED_INLINE Float CoFactor( Int32 i, Int32 j ) const;

	// Invert/Transpose
	RED_INLINE Matrix& Invert();
	RED_INLINE Matrix& FullInvert();
	RED_INLINE Matrix& Transpose();
	RED_INLINE Matrix Inverted() const;
	RED_INLINE Matrix FullInverted() const;
	RED_INLINE Matrix Transposed() const;

	// Column/Row access
	RED_INLINE Vector GetColumn( Int32 index ) const;
	RED_INLINE Matrix& SetColumn( Int32 index, const Vector& a );
	RED_INLINE const Vector& GetRow( Int32 index ) const;
	RED_INLINE Matrix& SetRow( Int32 index, const Vector& a );

    RED_INLINE Vector GetAxisX() const;
    RED_INLINE Vector GetAxisY() const;
    RED_INLINE Vector GetAxisZ() const;

	RED_INLINE void GetColumnMajor( Float* data ) const;
	RED_INLINE void GetColumnMajor3x4( Float* data ) const;

	// Multiply matrix
	RED_INLINE static Matrix Mul( const Matrix& a, const Matrix& b );
	RED_INLINE Matrix operator*( const Matrix& other ) const;

	// Transformations
	RED_INLINE Vector TransformVector( const Vector& a ) const;			// Assumed W = 0.0f
	RED_INLINE Vector TransformVectorWithW( const Vector& a ) const;		// W used directly
	RED_INLINE Vector TransformVectorAsPoint( const Vector& a ) const;	// Assumed W = 1.0f
	RED_INLINE Vector TransformPoint( const Vector& a ) const;			// Assumed W = 1.0f
	RED_INLINE Box TransformBox( const Box& box ) const;

	// Decompose matrix to the orthonormal part and the scale
	RED_INLINE void ExtractScale( Matrix &trMatrix, Vector& scale ) const;

	// Checks for bad values (denormals or infinities).
	RED_INLINE Bool IsOk() const;

	RED_INLINE static Bool Equal( const Matrix& a, const Matrix& b );
	RED_INLINE static Bool Near( const Matrix& a, const Matrix& b, Float eps=1e-3f );

	// Some predefined matrices
	static const Matrix ZEROS;
	static const Matrix IDENTITY;

};

BEGIN_NODEFAULT_CLASS_RTTI( Matrix );
	ALIGN_CLASS( 16 );
	PROPERTY_EDIT_NAME( V[0], TXT("X"), TXT("Matrix X row") );
	PROPERTY_EDIT_NAME( V[1], TXT("Y"), TXT("Matrix Y row") );
	PROPERTY_EDIT_NAME( V[2], TXT("Z"), TXT("Matrix Z row") );
	PROPERTY_EDIT_NAME( V[3], TXT("W"), TXT("Matrix W row") );
END_CLASS_RTTI();


/*********************************************/
/* 4x4 Matrix, Row Major, Double precision   */
/*********************************************/
struct MatrixDouble
{
	Double	V[4][4];

	void Import( const Matrix &src );
	void Export( Matrix &dest ) const;
	Double Det() const;
	Double CoFactor( Int32 i, Int32 j ) const;
	MatrixDouble FullInverted() const;
	MatrixDouble operator*( const MatrixDouble& other ) const;
	static MatrixDouble Mul( const MatrixDouble& a, const MatrixDouble& b );
};

/********************************************************/
/* Euler angles, rotations are CCW, order: Y X Z		*/
/********************************************************/
struct EulerAngles
{
	Float Roll;			//!< Rotation around the Y axis
	Float Pitch;		//!< Rotation around the X axis
	Float Yaw;			//!< Rotation around the Z axis

	RED_INLINE EulerAngles() {};
	RED_INLINE EulerAngles( const EulerAngles &ea );
	RED_INLINE EulerAngles( Float roll, Float pitch, Float yaw );

	RED_INLINE EulerAngles operator-() const;

	RED_INLINE EulerAngles operator+( const EulerAngles& a ) const;
	RED_INLINE EulerAngles operator-( const EulerAngles& a ) const;
	RED_INLINE EulerAngles operator*( const EulerAngles& a ) const;
	RED_INLINE EulerAngles operator/( const EulerAngles& a ) const;

	RED_INLINE EulerAngles operator+( Float a ) const;
	RED_INLINE EulerAngles operator-( Float a ) const;
	RED_INLINE EulerAngles operator*( Float a ) const;
	RED_INLINE EulerAngles operator/( Float a ) const;

	RED_INLINE EulerAngles& operator+=( const EulerAngles& a );
	RED_INLINE EulerAngles& operator-=( const EulerAngles& a );
	RED_INLINE EulerAngles& operator*=( const EulerAngles& a );
	RED_INLINE EulerAngles& operator/=( const EulerAngles& a );

	RED_INLINE EulerAngles& operator+=( Float a );
	RED_INLINE EulerAngles& operator-=( Float a );
	RED_INLINE EulerAngles& operator*=( Float a );
	RED_INLINE EulerAngles& operator/=( Float a );

	RED_INLINE Bool operator==( const EulerAngles& a ) const;
	RED_INLINE Bool operator!=( const EulerAngles& a ) const;

	RED_INLINE Bool AlmostEquals( const EulerAngles& a, Float epsilon = 0.01f ) const;

	// Convert this euler rotation to matrix and matrix to euler angles
	RED_INLINE Matrix ToMatrix() const;

	// Convert this euler rotation to matrix and matrix to euler angles. Alternative version, eliminates the use of the operator=
	RED_INLINE void ToMatrix( Matrix& out_matrix ) const;
	
	// Calculate angle vectors
	RED_INLINE void ToAngleVectors( Vector* forward, Vector* right, Vector* up ) const;

    // Calculate quaternion
    RED_INLINE Vector ToQuat() const;
	
	// Transform point
	RED_INLINE Vector TransformPoint( const Vector& a ) const;

	// Transform vector
	RED_INLINE Vector TransformVector( const Vector& a ) const;

	// Normalize angle [0 360]
	RED_INLINE static Float NormalizeAngle( Float angle );
	RED_INLINE EulerAngles& Normalize();

	// Normalize angle [-180 180]
	RED_INLINE static Float NormalizeAngle180( Float angle );

	// Converts angle to an angle nearest to referenceAngle by adding/subtracting 360
	RED_INLINE static Float ToNearestAngle( Float angle, Float referenceAngle );

	RED_INLINE static Float  YawFromXY( Float x, Float y );
	RED_INLINE static Double  YawFromXY( Double x, Double y );
	RED_INLINE static Vector YawToVector( Float yaw );
	RED_INLINE static Vector2 YawToVector2( Float yaw );

	RED_INLINE static Float AngleDistance( Float a, Float b );
	RED_INLINE static EulerAngles AngleDistance( const EulerAngles& a, const EulerAngles& b );

	RED_INLINE static Float Interpolate( Float a, Float b, Float weight );
	RED_INLINE static EulerAngles Interpolate( const EulerAngles& a, const EulerAngles& b, Float weight );
	RED_INLINE void Interpolate( const EulerAngles& a, Float weight );

	// Predefined values
	static const EulerAngles ZEROS;

	DECLARE_RTTI_STRUCT( EulerAngles );
};

BEGIN_NODEFAULT_CLASS_RTTI( EulerAngles );
	PROPERTY_EDIT_NAME( Pitch, TXT("Pitch"), TXT("Rotation around the X axis") );
	PROPERTY_EDIT_NAME( Yaw, TXT("Yaw"), TXT("Rotation around the Z axis") );
	PROPERTY_EDIT_NAME( Roll, TXT("Roll"), TXT("Banking, rotation around the Y axis") );
END_CLASS_RTTI();

// Conversion for tag list type type
template<> RED_INLINE String ToString( const EulerAngles& value )
{
	return String::Printf( TXT("%g %g %g"), value.Pitch, value.Yaw, value.Roll );
}

// Convert string to tag list
template<>
RED_INLINE Bool FromString( const String& text, EulerAngles& value )
{
	TDynArray< String > vals = text.Split( TXT(" ") );
	RED_ASSERT( vals.Size() == 3 );
	if ( vals.Size() != 3 )
	{
		return false;
	}
	FromString( vals[0], value.Pitch );
	FromString( vals[1], value.Yaw );
	FromString( vals[2], value.Roll );

	return true;
}


/********************************/
/* HDR color					*/
/********************************/
#define COLOR_UINT32(R,G,B) (((Uint32)R<<0)|((Uint32)G<<8)|((Uint32)B<<16))
struct Color
{
	union
	{
		struct  
		{
			Uint8	R, G, B, A;
		};

		struct  
		{
			Uint8	RGBA[4];
		};
	};


	RED_INLINE Color() {};
	RED_INLINE Color( Uint8 r, Uint8 g, Uint8 b, Uint8 a=255 );
	RED_INLINE Color( const Vector& x );
	explicit RED_INLINE Color( Uint32 x );
	RED_INLINE Bool operator==(const Color& other) const
	{
		return ( R == other.R ) && ( G == other.G ) && ( B == other.B ) && ( A == other.A );
	}

	// Convert to Vector
	RED_INLINE Vector ToVector() const;

	// Convert to Vector with gamma->linear conversion
	RED_INLINE Vector ToVectorLinear() const;

	// Convert to Uint32
	RED_FORCE_INLINE Uint32 ToUint32() const
	{
		return ((Uint32)R<<0)|((Uint32)G<<8)|((Uint32)B<<16)|((Uint32)A<<24);
	}

	// Mul, b[0 1]
	RED_INLINE static Color Mul3( const Color& a, Float b );
	RED_INLINE static Color Mul4( const Color& a, Float b );
	RED_INLINE void Mul3( Float b );
	RED_INLINE void Mul4( Float b );
	RED_INLINE static Color Lerp( Float coef , const Color&a , const Color&b );

	// Some predefined colors
	static const Color BLACK;
	static const Color WHITE;
	static const Color RED;
	static const Color GREEN;
	static const Color BLUE;
	static const Color YELLOW;
	static const Color CYAN;
	static const Color MAGENTA;
	static const Color LIGHT_RED;
	static const Color LIGHT_GREEN;
	static const Color LIGHT_BLUE;
	static const Color LIGHT_YELLOW;
	static const Color LIGHT_CYAN;
	static const Color LIGHT_MAGENTA;
	static const Color DARK_RED;
	static const Color DARK_GREEN;
	static const Color DARK_BLUE;
	static const Color DARK_YELLOW;
	static const Color DARK_CYAN;
	static const Color DARK_MAGENTA;
	static const Color BROWN;
	static const Color GRAY;
	static const Color NORMAL;
	static const Color CLEAR;	// name TRANSPARENT doesn't compile yourself

	DECLARE_RTTI_STRUCT( Color );
};

BEGIN_NODEFAULT_CLASS_RTTI( Color );
	PROPERTY_EDIT_NAME( R, TXT("Red"), TXT("Red component [0-255]") );
	PROPERTY_EDIT_NAME( G, TXT("Green"), TXT("Green component [0-255]") );
	PROPERTY_EDIT_NAME( B, TXT("Blue"), TXT("Blue component [0-255]") );
	PROPERTY_EDIT_NAME( A, TXT("Alpha"), TXT("Alpha component [0-255]") );
END_CLASS_RTTI();



/************************************************************************/
/* Rectangle (float properties) ( with left <= right and top <= bottom )                   */
/************************************************************************/
struct RectF
{
	Float	m_left;
	Float	m_top;
	Float	m_right;
	Float	m_bottom;

	RED_INLINE RectF() {}

	// Construct from box XY plane, X values growing left to right and Y values growing up to down
	RED_INLINE RectF( const Box& box );

	// Construct
	RED_INLINE RectF( Float left, Float right, Float top, Float bottom);

	// Check if rectangle intersects with another
	RED_INLINE Bool Intersects( const RectF& other ) const;

	// Get width
	RED_INLINE Float Width() const { return m_right - m_left; }

	// Get height
	RED_INLINE Float Height() const { return m_top - m_bottom; }

	// Get intersection of two rectangles
	RED_INLINE static Bool GetIntersection( const RectF& r1, const RectF& r2, RectF& out );

	DECLARE_RTTI_STRUCT( RectF );

	// predefined empty Rect
	static const RectF EMPTY;
};

/************************************************************************/
/* Rectangle ( with left < right and top < bottom )					    */
/************************************************************************/
struct Rect
{
	enum EResetState { RESET_STATE };

	Int32	m_left;
	Int32	m_top;
	Int32	m_right;
	Int32	m_bottom;

	RED_INLINE Rect() {}

	// Construct from box XY plane, X values growing left to right and Y values growing up to down
	RED_INLINE Rect( const Box& box );

	// Construct an empty rectangle
	RED_INLINE Rect( EResetState );

	// Construct
	RED_INLINE Rect( Int32 left, Int32 right, Int32 top, Int32 bottom);

	// All zeroes
	RED_INLINE void Clear();

	// Check if rectangle is empty
	RED_INLINE Bool IsEmpty() const;

	// Translate all rectangle by x,y
	RED_INLINE void Translate( Int32 x, Int32 y );

	// Get translated rectangle
	RED_INLINE Rect GetTranslated( Int32 x, Int32 y ) const;

	// Trim to another rectangle
	RED_INLINE void Trim( const Rect& trimmerRect );

	// Get trimmed to another rectangle
	RED_INLINE Rect GetTrimmed( const Rect& trimmerRect ) const;

	// Grow rect by sx, sy in every direction
	RED_INLINE void Grow( Int32 sx, Int32 sy );

	// Get grown rect by sx, sy in every direction
	RED_INLINE Rect GetGrown( Int32 sx, Int32 sy ) const;

	// Add rect to this rect, to get combined bounds
	RED_INLINE void Add( const Rect& addRect );

	// Check if rectangle intersects with another
	RED_INLINE Bool Intersects( const Rect& other ) const;

	// Check if rectangle completely contains another
	RED_INLINE Bool Contains( const Rect& other ) const;

	// Get width
	RED_INLINE Int32 Width() const { return m_right - m_left; }

	// Get height
	RED_INLINE Int32 Height() const { return m_bottom - m_top; }

	// Get intersection of two rectangles
	RED_INLINE static Bool GetIntersection( const Rect& r1, const Rect& r2, Rect& out );

	DECLARE_RTTI_STRUCT( Rect );

	// predefined empty Rect
	static const Rect EMPTY;
};

BEGIN_NODEFAULT_CLASS_RTTI( Rect );
	PROPERTY_EDIT_NAME( m_left, TXT("m_left"), TXT("Left") );
	PROPERTY_EDIT_NAME( m_top, TXT("m_top"), TXT("Top") );
	PROPERTY_EDIT_NAME( m_right, TXT("m_right"), TXT("Right") );
	PROPERTY_EDIT_NAME( m_bottom, TXT("m_bottom"), TXT("Bottom") );
END_CLASS_RTTI();



/********************************/
/* Plane						*/
/********************************/
struct Plane
{
	Vector NormalDistance;

public:
	enum ESide
	{
		PS_None	 = 0,
		PS_Front = 1,
		PS_Back  = 2,
		PS_Both	 = PS_Front | PS_Back
	};

public:
	RED_INLINE Plane() {};
	RED_INLINE Plane( const Vector& normal, const Vector& point );
	RED_INLINE Plane( const Vector& normal, const Float& distance );
	RED_INLINE Plane( const Vector& p1, const Vector& p2, const Vector& p3 );

	RED_INLINE void SetPlane( const Vector& normalAndDistance ) { NormalDistance = normalAndDistance; }
	RED_INLINE void SetPlane( const Vector& normal, const Vector& point );
	RED_INLINE void SetPlane( const Vector& p1, const Vector& p2, const Vector& p3 );

	RED_INLINE Float DistanceTo( const Vector& point ) const;
	RED_INLINE static Float DistanceTo( const Vector& plane, const Vector& point );
	RED_INLINE ESide GetSide( const Vector& point ) const;
	RED_INLINE ESide GetSide( const Box& box ) const;
	RED_INLINE ESide GetSide( const Vector& boxCenter, const Vector& boxExtents ) const;
	RED_INLINE const Vector& GetVectorRepresentation() const { return NormalDistance; }

	RED_INLINE Vector Project( const Vector& point ) const;
	RED_INLINE Bool IntersectRay( const Vector& origin, const Vector& direction, Vector& intersectionPoint, Float &intersectionDistance ) const;

	RED_INLINE Plane operator-() const;

	DECLARE_RTTI_STRUCT( Plane );
};

BEGIN_NODEFAULT_CLASS_RTTI( Plane );
	PROPERTY_EDIT_NAME( NormalDistance, TXT("NormalDistance"), TXT("Normal and distance") );
END_CLASS_RTTI();


template <> struct TCopyableType<Vector>										{ enum { Value = true }; };
template <> struct TCopyableType<Matrix>										{ enum { Value = true }; };
template <> struct TCopyableType<EulerAngles>									{ enum { Value = true }; };
template <> struct TCopyableType<Color>											{ enum { Value = true }; };
template <> struct TCopyableType<Box>											{ enum { Value = true }; };
template <> struct TCopyableType<Plane>											{ enum { Value = true }; };
template <> struct TCopyableType<Rect>											{ enum { Value = true }; };

// Some generic helpers
RED_INLINE Float DistanceBetweenAngles( Float a, Float b );		// Returns values in range [-180.0, 180.0]
RED_INLINE Float DistanceBetweenAnglesAbs( Float a, Float b );	// Returns values in range [0.0, 180.0]
RED_INLINE Float PointToLineDistanceSquared2( const Vector &lineRoot, const Vector &lineDirection, const Vector &point, Float &projectionAlpha );
RED_INLINE Float PointToLineDistanceSquared3( const Vector &lineRoot, const Vector &lineDirection, const Vector &point, Float &projectionAlpha );
void Compute2DConvexHull( TDynArray<Vector>& points );
Bool IsPointInsideConvexShape( const Vector & point, const TDynArray< Vector > & shape );

struct Vector2;
// Vector3 is for memory critical structures. It lacks many features so don't use it everywhere. Note: must not be aligned !
struct Vector3
{
	DECLARE_RTTI_STRUCT( Vector3 );

	union
	{
		struct  
		{
			Float X, Y, Z;
		};
		Float A[3];
	};

	RED_INLINE Vector3()
	{
	}

	RED_INLINE Vector3( const Vector& v )
	{
		X = v.X;
		Y = v.Y;
		Z = v.Z;
	}

	RED_INLINE Vector3( const Vector3& v )
	{
		X = v.X;
		Y = v.Y;
		Z = v.Z;
	}

	explicit RED_INLINE Vector3( const Vector2& v );
	
	RED_INLINE Vector3( const Float* f)
	{
		X = f[0];
		Y = f[1];
		Z = f[2];
	}

	RED_INLINE Vector3( Float x, Float y, Float z )
	{
		X = x;
		Y = y;
		Z = z;
	}

	RED_INLINE void operator = ( const Vector& v )
	{
		X = v.X;
		Y = v.Y;
		Z = v.Z;
	}

	RED_INLINE void operator = ( Float v )
	{
		X = v;
		Y = v;
		Z = v;
	}

	RED_INLINE Vector3 operator * ( Float scale ) const
	{
		return Vector3( X * scale, Y * scale, Z * scale );
	}
	RED_INLINE Vector3 operator / ( Float scale ) const
	{
		return Vector3( X / scale, Y / scale, Z / scale );
	}


	RED_INLINE Vector3 operator + ( const Vector3& vec ) const
	{
		return Vector3( X + vec.X, Y + vec.Y, Z + vec.Z );
	}

	RED_INLINE Vector3 operator - ( const Vector3& vec ) const
	{
		return Vector3( X - vec.X, Y - vec.Y, Z - vec.Z );
	}


	RED_INLINE Vector3 operator * ( const Vector3& vec ) const
	{
		return Vector3( X * vec.X, Y * vec.Y, Z * vec.Z );
	}

	RED_INLINE void operator += ( const Vector3& vec )
	{
		X += vec.X;
		Y += vec.Y;
		Z += vec.Z;
	}

	RED_INLINE void operator -= ( const Vector3& vec )
	{
		X -= vec.X;
		Y -= vec.Y;
		Z -= vec.Z;
	}

	RED_INLINE void operator *= ( const Vector3& vec )
	{
		X *= vec.X;
		Y *= vec.Y;
		Z *= vec.Z;
	}

	RED_INLINE void operator *= ( Float scale )
	{
		X *= scale;
		Y *= scale;
		Z *= scale;
	}

	RED_INLINE void operator /= ( Float scale )
	{
		X /= scale;
		Y /= scale;
		Z /= scale;
	}

	RED_INLINE Bool IsZero() const
	{
		return X == 0.f && Y == 0.f && Z == 0.f;
	}
	RED_INLINE Bool IsAlmostZero( Float epsilon = NumericLimits< Float >::Epsilon() ) const
	{
		return fabs( X ) < epsilon && fabs( Y ) < epsilon && fabs( Z ) < epsilon;
	}

	RED_INLINE Float Mag() const
	{
		return MSqrt( X * X + Y * Y + Z * Z );
	}

	RED_INLINE Float SquareMag() const
	{
		return X * X + Y * Y + Z * Z;
	}

	RED_INLINE Float Normalize()
	{
		float len = MSqrt( X * X + Y * Y + Z * Z );
		if( len != 0 )
		{
			float oolen = 1.0f / len;
			X *= oolen;
			Y *= oolen;
			Z *= oolen;
		}
		return len;
	}

	RED_INLINE Vector3 Normalized() const
	{
		float len = MSqrt( X * X + Y * Y + Z * Z );
		if( len == 0 )
			return *this;
		float oolen = 1.0f / len;
		return Vector3( X * oolen, Y * oolen, Z * oolen );
	}

	RED_INLINE operator Vector () const
	{
		return Vector( X, Y, Z );
	}

	RED_INLINE Float Dot( const Vector3& v ) const;
	RED_INLINE Vector3 Cross( const Vector3& v) const;

	RED_INLINE void Set( Float x, Float y, Float z )
	{
		X = x; Y = y; Z = z;
	}

	RED_INLINE Vector2& AsVector2();
	RED_INLINE const Vector2& AsVector2() const;

	RED_INLINE Float Pitch() const
	{
		return ( !X && !Y ) ? ( (Z > 0) ? 90.0f : -90.0f ) : RAD2DEG( MATan2( -Z, MSqrt( X * X + Y * Y ) ) );  
	}

	RED_INLINE Float Yaw() const
	{
		return EulerAngles::YawFromXY( X, Y );
	}

	static const Vector3 ZEROS;
};

BEGIN_NODEFAULT_CLASS_RTTI( Vector3 );
PROPERTY_EDIT_NAME( X, TXT("X"), TXT("Vector X value") );
PROPERTY_EDIT_NAME( Y, TXT("Y"), TXT("Vector Y value") );
PROPERTY_EDIT_NAME( Z, TXT("Z"), TXT("Vector Z value") );
END_CLASS_RTTI();

struct Vector2
{
	DECLARE_RTTI_STRUCT( Vector2 );

	union
	{
		struct  
		{
			Float X, Y;
		};
		Float A[2];
	};

	RED_INLINE Vector2()
	{
	}

	RED_INLINE Vector2( const Vector& v )
	{
		X = v.X;
		Y = v.Y;
	}

	RED_INLINE Vector2( const Vector2& v )
	{
		X = v.X;
		Y = v.Y;
	}

	RED_INLINE Vector2( const Float* f)
	{
		X = f[0];
		Y = f[1];
	}

	RED_INLINE Vector2( Float v )
	{
		X = v;
		Y = v;
	}

	RED_INLINE Vector2( Float x, Float y )
	{
		X = x;
		Y = y;
	}

	RED_INLINE void operator = ( const Vector& v )
	{
		X = v.X;
		Y = v.Y;
	}

	RED_INLINE void operator = ( Float v )
	{
		X = v;
		Y = v;
	}

	RED_INLINE Vector2 operator * ( Float scale ) const
	{
		return Vector2( X * scale, Y * scale );
	}

	RED_INLINE Vector2 operator / ( Float scale ) const
	{
		return Vector2( X / scale, Y / scale );
	}

	RED_INLINE Vector2 operator + ( const Vector2& vec ) const
	{
		return Vector2( X + vec.X, Y + vec.Y );
	}

	RED_INLINE Vector2 operator * ( const Vector2& vec ) const
	{
		return Vector2( X * vec.X, Y * vec.Y );
	}

	RED_INLINE Vector2 operator - ( ) const
	{
		return Vector2( -X, -Y );
	}

	RED_INLINE Vector2 operator - ( const Vector2& vec ) const
	{
		return Vector2( X - vec.X, Y - vec.Y );
	}

	RED_INLINE void operator += ( const Vector2& vec )
	{
		X += vec.X;
		Y += vec.Y;
	}

	RED_INLINE void operator -= ( const Vector2& vec )
	{
		X -= vec.X;
		Y -= vec.Y;
	}

	RED_INLINE void operator *= ( const Vector2& vec )
	{
		X *= vec.X;
		Y *= vec.Y;
	}

	RED_INLINE void operator *= ( Float scale )
	{
		X *= scale;
		Y *= scale;
	}

	RED_INLINE void operator /= ( Float scale )
	{
		X /= scale;
		Y /= scale;
	}

	RED_INLINE Bool operator==( const Vector2& v ) const
	{
		return X == v.X && Y == v.Y;
	}

	RED_INLINE Bool operator!=( const Vector2& v ) const
	{
		return X != v.X || Y != v.Y;
	}

	// this simplifies template functions that can assume to have some interface over vector class
	RED_INLINE const Vector2& AsVector2() const { return *this; }
	RED_INLINE Vector2& AsVector2() { return *this; }

	RED_INLINE Bool IsZero() const
	{
		return X == 0.f && Y == 0.f;
	}
	RED_INLINE Bool IsAlmostZero( Float epsilon = NumericLimits< Float >::Epsilon() ) const
	{
		return fabs( X ) < epsilon && fabs( Y ) < epsilon;
	}

	RED_INLINE Float Mag() const
	{
		return MSqrt( X * X + Y * Y );
	}

	RED_INLINE Float SquareMag() const
	{
		return X * X + Y * Y;
	}

	RED_INLINE Float Normalize()
	{
		float len = MSqrt( X * X + Y * Y );
		if( len != 0 )
		{
			float oolen = 1.0f / len;
			X *= oolen;
			Y *= oolen;
		}
		return len;
	}

	RED_INLINE Vector2 Normalized() const
	{
		float len = MSqrt( X * X + Y * Y );
		if( len == 0 )
			return *this;
		float oolen = 1.0f / len;
		return Vector2( X * oolen, Y * oolen );
	}

	RED_INLINE Float Dot( const Vector2& v ) const;

	RED_INLINE void Set( Float x, Float y )
	{
		X = x; Y = y;
	}

	RED_INLINE Float CrossZ(const Vector2& vec) const
	{
		return X*vec.Y-Y*vec.X;
	}

	RED_INLINE operator Vector () const
	{
		return Vector( X, Y, 0.0f );
	}

	RED_INLINE Float Yaw() const
	{
		return EulerAngles::YawFromXY( X, Y );
	}

	RED_INLINE static Vector2 Min( const Vector2& a, const Vector2& b )
	{
		return Vector2( ::Min( a.X, b.X ), ::Min( a.Y, b.Y ) );
	}

	RED_INLINE static Vector2 Max( const Vector2& a, const Vector2& b )
	{
		return Vector2( ::Max( a.X, b.X ), ::Max( a.Y, b.Y ) );
	}

};

BEGIN_NODEFAULT_CLASS_RTTI( Vector2 );
	PROPERTY_EDIT_NAME( X, TXT("X"), TXT("Vector X value") );
	PROPERTY_EDIT_NAME( Y, TXT("Y"), TXT("Vector Y value") );
END_CLASS_RTTI();

struct Box2
{
	Vector2 Min, Max;
	static Box2 ZERO;
	static Box2 IDENTITY;

	Box2() {}
	Box2(const Box2& box)
	{
		Min = box.Min;
		Max = box.Max;
	}
	Box2( const Vector2& min, const Vector2& max ) 
	{ 
		Min = min; 
		Max = max; 
	}
	Box2( Float minX, Float minY, Float maxX, Float maxY) 
	{
		Min.Set(minX, minY);
		Max.Set(maxX, maxY);
	}
	Box2( const Vector2& center, const Float radius )
	{
		Min.X = center.X - radius;
		Min.Y = center.Y - radius;
		Max.X = center.X + radius;
		Max.Y = center.Y + radius;
	}

	RED_INLINE Box2& operator=( const Box2& box )
	{
		Min = box.Min;
		Max = box.Max;

		return *this;
	}

	RED_INLINE Bool Contains( const Vector2& point ) const
	{
		return point.X >= Min.X && point.Y >= Min.Y && point.X <= Max.X && point.Y <= Max.Y;
	}

	RED_INLINE Bool Contains( float x, float y ) const
	{
		return x >= Min.X && y >= Min.Y && x <= Max.X && y <= Max.Y;
	}

	RED_INLINE Bool Contains( const Box2& box ) const
	{
		if ( box.Min.X >= Min.X && box.Min.Y >= Min.Y && box.Max.X <= Max.X && box.Max.Y <= Max.Y )
		{
			return true;
		}
		return false;
	}

	//Returns true if boxes intersect or contain each other
	RED_INLINE Bool Intersects( const Box2& box ) const
	{
		if( ( Max.X < box.Min.X ) || ( box.Max.X < Min.X ) 
			|| ( Max.Y < box.Min.Y ) || ( box.Max.Y < Min.Y ) )
		{
			return false;
		}
		return true;
	}


	//Returns true if boxes intersect but NOT contain each other
	RED_INLINE Bool Overlaps( const Box2& box ) const
	{
		if( ( Max.X < box.Min.X ) || ( box.Max.X < Min.X ) 
			|| ( Max.Y < box.Min.Y ) || ( box.Max.Y < Min.Y ) )
		{
			return false;
		}
		else if( ( Max.X > box.Max.X && Min.X < box.Min.X && Max.Y > box.Max.Y && Min.Y < box.Min.Y )
			|| (  box.Min.X < Min.X && box.Max.X > Max.X && box.Max.Y > Max.Y && box.Min.Y < Min.Y ) )
		{
			return false;
		}

		return true;
	}

	RED_INLINE Bool operator==( const Box2& other ) const
	{
		return other.Min == Min && other.Max == Max;
	}

	Box2& AddBox( const Box2& box )
	{
		Min = Vector2::Min( Min, box.Min);
		Max = Vector2::Max( Max, box.Max );
		return *this;
	}

	RED_INLINE Box2 operator+( const Vector& other ) const
	{
		Box2 result( Min, Max );
		result.Min.X += other.X;
		result.Max.X += other.X;
		result.Min.Y += other.Y;
		result.Max.Y += other.Y;
		return result;
	}

	RED_INLINE Box2 operator*( const Vector& other ) const
	{
		Box2 result( Min, Max );
		result.Min.X *= other.X;
		result.Max.X *= other.X;
		result.Min.Y *= other.Y;
		result.Max.Y *= other.Y;
		return result;
	}

	Vector2 CalcSize() const
	{
		return ( Max - Min );
	}

	Vector2 CalcCenter() const
	{
		return ( Max + Min ) * 0.5f;
	}

};

struct Segment;

struct Quad
{
public:
	Vector m_points[4];

	RED_INLINE Quad() {};
	Quad( const Quad& other );
	Quad( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4 );

	// Return translated by a vector
	Quad operator+( const Vector& dir ) const;
	// Return translated by a -vector
	Quad operator-( const Vector& dir ) const;
	// Translate by a vector
	Quad& operator+=( const Vector& dir );
	// Translate by a -vector
	Quad& operator-=( const Vector& dir );

	Vector GetPosition() const { return m_points[0]; };

	Bool IntersectSegment( const Segment& segment, Vector& enterPoint );
	Bool IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const;
	Bool IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const;

	DECLARE_RTTI_STRUCT( Quad );
};

BEGIN_NODEFAULT_CLASS_RTTI( Quad );
PROPERTY_EDIT_NAME( m_points[0], TXT("p1"), TXT("p1") );
PROPERTY_EDIT_NAME( m_points[1], TXT("p2"), TXT("p2") );
PROPERTY_EDIT_NAME( m_points[2], TXT("p3"), TXT("p3") );
PROPERTY_EDIT_NAME( m_points[3], TXT("p4"), TXT("p4") );
END_CLASS_RTTI();

/************************************************************************/
/* Segment                                                              */
/* Segment first point is stored in m_origin, second point is           */
/* m_origin + m_direction ( it's more convenient for most of            */
/* computations                                                         */
/************************************************************************/
struct Segment
{
public:
	Vector m_origin;
	Vector m_direction;

	RED_INLINE Segment() {};
	Segment( const Segment& other ) { m_origin = other.m_origin; m_direction = other.m_direction; }
	Segment( const Vector& origin, const Vector& direction ) : m_origin( origin ), m_direction( direction ) {}

	// Return translated by a vector
	Segment operator+( const Vector& dir ) const;
	// Return translated by a -vector
	Segment operator-( const Vector& dir ) const;
	// Translate by a vector
	Segment& operator+=( const Vector& dir );
	// Translate by a -vector
	Segment& operator-=( const Vector& dir );

	Vector GetPosition() const { return m_origin; };

	DECLARE_RTTI_STRUCT( Segment );
};

BEGIN_NODEFAULT_CLASS_RTTI( Segment );
PROPERTY_EDIT_NAME( m_origin, TXT("origin"), TXT("origin") );
PROPERTY_EDIT_NAME( m_direction, TXT("direction"), TXT("direction") );
END_CLASS_RTTI();

/************************************************************************/
/* Tetrahedron                                                          */
/************************************************************************/
struct Tetrahedron
{
private:
	void CalculatePlanes( Vector* planes ) const;
public:
	Vector m_points[4];

	RED_INLINE Tetrahedron() {};
	Tetrahedron( const Tetrahedron& tetra ) { m_points[0] = tetra.m_points[0]; m_points[1] = tetra.m_points[1]; m_points[2] = tetra.m_points[2]; m_points[3] = tetra.m_points[3]; }
	Tetrahedron( const Vector& pos1, const Vector& pos2, const Vector& pos3, const Vector& pos4 ) { m_points[0] = pos1; m_points[1] = pos2; m_points[2] = pos3; m_points[3] = pos4; }

	// Return translated by a vector
	Tetrahedron operator+( const Vector& dir ) const;
	// Return translated by a -vector
	Tetrahedron operator-( const Vector& dir ) const;
	// Translate by a vector
	Tetrahedron& operator+=( const Vector& dir );
	// Translate by a -vector
	Tetrahedron& operator-=( const Vector& dir );

	Vector GetPosition() const { return m_points[0]; };

	Bool Contains( const Vector& point ) const;
	Bool IntersectSegment( const Segment& segment, Vector& enterPoint );
	Bool IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const;
	Bool IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const;

	DECLARE_RTTI_STRUCT( Tetrahedron );
};

BEGIN_NODEFAULT_CLASS_RTTI( Tetrahedron );
PROPERTY_EDIT_NAME( m_points[0], TXT("point1"), TXT("point 1") );
PROPERTY_EDIT_NAME( m_points[1], TXT("point2"), TXT("point 2") );
PROPERTY_EDIT_NAME( m_points[2], TXT("point3"), TXT("point 3") );
PROPERTY_EDIT_NAME( m_points[3], TXT("point4"), TXT("point 4") );
END_CLASS_RTTI();

/************************************************************************/
/* CutCone                                                              */
/************************************************************************/
struct CutCone
{
	Vector m_positionAndRadius1; //<! position = (x,y,z), radius1 = (w)
	Vector m_normalAndRadius2; //<! orientation = (x,y,z), radius2 = (w)
	Float m_height;

	RED_INLINE CutCone() {};
	CutCone( const CutCone& cone ) : m_positionAndRadius1( cone.m_positionAndRadius1 ), m_normalAndRadius2( cone.m_normalAndRadius2 ), m_height( cone.m_height ) {}
	CutCone( const Vector& pos1, const Vector& pos2, Float radius1, Float radius2 );
	CutCone( const Vector& pos, const Vector& normal, Float radius1, Float radius2, Float height ); //<! normal length has to be 1

	// Return translated by a vector
	CutCone operator+( const Vector& dir ) const;
	// Return translated by a -vector
	CutCone operator-( const Vector& dir ) const;
	// Translate by a vector
	CutCone& operator+=( const Vector& dir );
	// Translate by a -vector
	CutCone& operator-=( const Vector& dir );

	RED_INLINE Vector GetMassCenter() const { return m_positionAndRadius1 + m_normalAndRadius2 * ( GetRadius1() + 2 * GetRadius2() ) * m_height / ( 3 * ( GetRadius1() + GetRadius2() ) ); }
	RED_INLINE Float GetMass() const { Float mr = ( GetRadius1() + GetRadius2() ) * 0.5f; return M_PI * mr * mr * m_height; }

	Bool Contains( const Vector& point ) const;
	Bool IntersectSegment( const Segment& segment, Vector& enterPoint );
	Bool IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const;
	Bool IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const;
	RED_INLINE Float GetHeight() const { return m_height; }
	RED_INLINE Float GetRadius1() const { return m_positionAndRadius1.W; }
	RED_INLINE Float GetRadius2() const { return m_normalAndRadius2.W; }
	RED_INLINE Vector GetPosition() const { return Vector( m_positionAndRadius1.X, m_positionAndRadius1.Y, m_positionAndRadius1.Z ); }
	RED_INLINE Vector GetPosition2() const { return GetPosition() + GetNormal() * GetHeight(); }
	EulerAngles GetOrientation() const;
	RED_INLINE Vector GetNormal() const { return Vector( m_normalAndRadius2.X, m_normalAndRadius2.Y, m_normalAndRadius2.Z ); }

	DECLARE_RTTI_STRUCT( CutCone );
};

BEGIN_NODEFAULT_CLASS_RTTI( CutCone );
PROPERTY_EDIT_NAME( m_positionAndRadius1, TXT("positionAndRadius1"), TXT("position and radius") );
PROPERTY_EDIT_NAME( m_normalAndRadius2, TXT("normalAndRadius2"), TXT("normal and radius 2") );
PROPERTY_EDIT_NAME( m_height, TXT("height"), TXT("height") );
END_CLASS_RTTI();

/************************************************************************/
/* Axis Aligned Cylinder                                                */
/************************************************************************/
struct AACylinder
{
	DECLARE_RTTI_STRUCT( AACylinder );

	Vector m_positionAndRadius; //<! position = (x,y,z), radius = (w)
	Float m_height;

	RED_INLINE AACylinder() {};
	AACylinder( const AACylinder& cyl );
	AACylinder( const Vector& pos, Float radius, Float height );

	RED_INLINE Vector GetMassCenter() const { return m_positionAndRadius + Vector ( 0, 0, m_height * 0.5f ); }
	RED_INLINE Float GetMass() const { return M_PI * m_positionAndRadius.W * m_positionAndRadius.W * m_height; }

	// Return translated by a vector
	AACylinder operator+( const Vector& dir ) const;
	// Return translated by a -vector
	AACylinder operator-( const Vector& dir ) const;
	// Translate by a vector
	AACylinder& operator+=( const Vector& dir );
	// Translate by a -vector
	AACylinder& operator-=( const Vector& dir );

	EulerAngles GetOrientation() const { return EulerAngles::ZEROS; }

	Bool Contains( const Vector& point ) const;
	Bool IntersectSegment( const Segment& segment, Vector& enterPoint );
	Bool IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const;
	Bool IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const;
	RED_INLINE Float GetHeight() const { return m_height; }
	RED_INLINE Float GetRadius() const { return m_positionAndRadius.W; }
	RED_INLINE Vector GetPosition() const { return Vector( m_positionAndRadius.X, m_positionAndRadius.Y, m_positionAndRadius.Z ); }

};

BEGIN_NODEFAULT_CLASS_RTTI( AACylinder );
PROPERTY_EDIT_NAME( m_positionAndRadius, TXT("positionAndRadius"), TXT("position and radius") );
PROPERTY_EDIT_NAME( m_height, TXT("height"), TXT("height") );
END_CLASS_RTTI();

/************************************************************************/
/* Cylinder                                                             */
/************************************************************************/
struct Cylinder
{
	Vector m_positionAndRadius; //<! position = (x,y,z), radius = (w)
	Vector m_normalAndHeight; //<! orientation = (x,y,z), height = (w)

	RED_INLINE Cylinder() {};
	Cylinder( const Cylinder& cyl );
	Cylinder( const Vector& pos1, const Vector& pos2, Float radius);
	Cylinder( const Vector& pos, const Vector& normal, Float radius, Float height ); //<! normal length has to be 1

	RED_INLINE Vector GetMassCenter() const { return m_positionAndRadius + m_normalAndHeight * m_normalAndHeight.W * 0.5f; }
	RED_INLINE Float GetMass() const { return M_PI * m_positionAndRadius.W * m_positionAndRadius.W * m_normalAndHeight.W; }

	EulerAngles GetOrientation() const;

	// Return translated by a vector
	Cylinder operator+( const Vector& dir ) const;
	// Return translated by a -vector
	Cylinder operator-( const Vector& dir ) const;
	// Translate by a vector
	Cylinder& operator+=( const Vector& dir );
	// Translate by a -vector
	Cylinder& operator-=( const Vector& dir );

	Bool Contains( const Vector& point ) const;
	Bool IntersectSegment( const Segment& segment, Vector& enterPoint );
	Bool IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const;
	Bool IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const;
	RED_INLINE Float GetHeight() const { return m_normalAndHeight.W; }
	RED_INLINE Float GetRadius() const { return m_positionAndRadius.W; }
	RED_INLINE Vector GetPosition() const { return Vector( m_positionAndRadius.X, m_positionAndRadius.Y, m_positionAndRadius.Z ); }
	RED_INLINE Vector GetPosition2() const { return GetPosition() + GetNormal() * GetHeight(); }
	RED_INLINE Vector GetNormal() const { return Vector( m_normalAndHeight.X, m_normalAndHeight.Y, m_normalAndHeight.Z ); }

	DECLARE_RTTI_STRUCT( Cylinder );
};

BEGIN_NODEFAULT_CLASS_RTTI( Cylinder );
PROPERTY_EDIT_NAME( m_positionAndRadius, TXT("positionAndRadius"), TXT("position and radius") );
PROPERTY_EDIT_NAME( m_normalAndHeight, TXT("normalAndHeight"), TXT("normal and height") );
END_CLASS_RTTI();


/************************************************************************/
/* Convex Hull                                                          */
/************************************************************************/
struct ConvexHull
{
	TDynArray< Vector >		m_planes;

	RED_INLINE ConvexHull() {};
	RED_INLINE ConvexHull( const ConvexHull& cyl ) : m_planes( cyl.m_planes ) {}
	RED_INLINE ConvexHull( TDynArray< Vector > planes ) : m_planes( planes ) {}

	static void MovePlane( Vector& plane, const Vector& translation );

	// Check if bounding box contains point
	RED_INLINE Bool Contains( const Vector& point ) const;
	Bool IntersectSegment( const Segment& segment, Vector& enterPoint );
	Bool IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const;
	Bool IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const;

	// Return translated by a vector
	RED_INLINE ConvexHull operator+( const Vector& dir ) const;
	// Return translated by a -vector
	RED_INLINE ConvexHull operator-( const Vector& dir ) const;
	// Translate by a vector
	RED_INLINE ConvexHull& operator+=( const Vector& dir );
	// Translate by a -vector
	RED_INLINE ConvexHull& operator-=( const Vector& dir );

	DECLARE_RTTI_STRUCT( ConvexHull );
};

BEGIN_NODEFAULT_CLASS_RTTI( ConvexHull );
PROPERTY_EDIT_NAME( m_planes, TXT("planes"), TXT("planes") );
END_CLASS_RTTI();
/************************************************************************/
/* Oriented Bounding Box                                                */
/************************************************************************/
struct OrientedBox
{
	Vector m_position; //<! position, m_position.W i length of third edge
	Vector m_edge1; //<! edge1 - normalized, length in w component
	Vector m_edge2; //<! edge2 - normalized, length in w component
	//<! edge3 is m_edge1 x m_edge2 * m_position.W

	RED_INLINE OrientedBox() {};
	OrientedBox( const OrientedBox& cyl );
	OrientedBox( const Vector& pos, const Vector& forward, const Vector& right, const Vector& up );
	RED_INLINE Vector GetEdge3() const { return Vector::Cross( m_edge1, m_edge2 ) * m_position.W; }
	RED_INLINE Vector GetEdge2() const { return m_edge2 * m_edge2.W; }
	RED_INLINE Vector GetEdge1() const { return m_edge1 * m_edge1.W; }

	EulerAngles GetOrientation() const;

	RED_INLINE Vector GetMassCenter() const { return m_position + GetEdge1() * 0.5f + GetEdge2() * 0.5f + GetEdge3() * 0.5f; }
	RED_INLINE Float GetMass() const { return m_position.W * m_edge1.W * m_edge2.W; }

	// Check if bounding box contains point
	Bool Contains( const Vector& point ) const;
	Bool IntersectSegment( const Segment& segment, Vector& enterPoint );
	Bool IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const;
	Bool IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const;

	// Return translated by a vector
	OrientedBox operator+( const Vector& dir ) const;
	// Return translated by a -vector
	OrientedBox operator-( const Vector& dir ) const;
	// Translate by a vector
	OrientedBox& operator+=( const Vector& dir );
	// Translate by a -vector
	OrientedBox& operator-=( const Vector& dir );

	DECLARE_RTTI_STRUCT( OrientedBox );
};

BEGIN_NODEFAULT_CLASS_RTTI( OrientedBox );
PROPERTY_EDIT_NAME( m_position, TXT("position"), TXT("position and edge 3 length") );
PROPERTY_EDIT_NAME( m_edge1, TXT("edge 1"), TXT("normalised edge 1 and it's length") );
PROPERTY_EDIT_NAME( m_edge2, TXT("edge 2"), TXT("normalised edge 2 and it's length") );
END_CLASS_RTTI();

/********************************/
/* Bounding box					*/
/********************************/
struct Box
{
	Vector	Min;
	Vector	Max;

	enum EResetState { RESET_STATE };
	enum EMaximize { MAXIMIZE };

	RED_INLINE Box() {};
	RED_INLINE Box( const Box& rhs );
	RED_INLINE Box( const Vector& min, const Vector& max );
	RED_INLINE Box( const Vector& center, float radius );
	RED_INLINE Box( EResetState );
	RED_INLINE Box( EMaximize );

	RED_INLINE Bool operator==( const Box& box ) const;
	RED_INLINE Bool operator!=( const Box& box ) const;

	EulerAngles GetOrientation() const { return EulerAngles::ZEROS; }

	RED_INLINE Vector GetMassCenter() const { return ( Min + Max ) * 0.5f; }
	RED_INLINE Float GetMass() const { return ( Max.X - Min.X ) * ( Max.Y - Min.Y ) * ( Max.Z - Min.Z ); }

	// Return box translated by a vector
	RED_INLINE Box operator+( const Vector& dir ) const;
	// Return box translated by a -vector
	RED_INLINE Box operator-( const Vector& dir ) const;
	// Return box scaled box by a vector
	RED_INLINE Box operator*( const Vector& scale ) const;
	RED_INLINE Box& operator=( const Box& rhs );
	// Translate by a vector
	RED_INLINE Box& operator+=( const Vector& dir );
	// Translate by a -vector
	RED_INLINE Box& operator-=( const Vector& dir );
	// Scale box by a vector
	RED_INLINE Box& operator*=( const Vector& scale );

	RED_INLINE Bool IntersectSegment( const Segment& segment, Vector& enterPoint );
	RED_INLINE Bool IntersectSegment( const Segment& segment, Vector& enterPoint, Vector& exitPoint );
	RED_INLINE Bool IntersectRay( const Vector& origin, const Vector& direction, Float & enterDistFromOrigin, Float* exitDistFromOrigin = nullptr ) const;
	RED_INLINE Bool IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const;

	// Check if bounding box intersects sphere (Arvo's algorithm)
	RED_INLINE Bool IntersectSphere( const Sphere& sphere ) const;
	
	// Clear to empty box
	RED_INLINE Box& Clear();

	// Check if bounding box contains point
	RED_INLINE Bool Contains( const Vector& point ) const;

	// Check if bounding box contains point
	RED_INLINE Bool Contains( const Vector3& point, Float zExt ) const;

	// Check if bounding box contains other box
	RED_INLINE Bool Contains( const Box& box ) const;

	// Check if bounding box contains other box in 2D (checks only X and Y)
	RED_INLINE Bool Contains2D( const Box& box ) const;

	// Check if bounding box contains point
	RED_INLINE Bool Contains2D( const Vector3& point ) const;

	// Check if bounding box contains point, excluding edges
	RED_INLINE Bool ContainsExcludeEdges( const Vector& point ) const;

	// Check if bounding box touches other box
	RED_INLINE Bool Touches( const Box& box ) const;

	// Check if bounding box touches other box defined with min and max points
	RED_INLINE Bool Touches( const Vector3& bMin, const Vector3& bMax ) const;

	// Check if bounding box touches other box; only takes X and Y in consideration
	RED_INLINE Bool Touches2D( const Box& box ) const;

	// Add point to bounding box
	RED_INLINE Box& AddPoint( const Vector& point );

	// Add point to bounding box
	RED_INLINE Box& AddPoint( const Vector3& point );

	// Add other box to bounding box
	RED_INLINE Box& AddBox( const Box& box );

	// Check if bounding box is empty
	RED_INLINE Bool IsEmpty() const;

	// Calculate box corners
	RED_INLINE void CalcCorners( Vector* corners ) const;

	// Calculate box center
	RED_INLINE Vector CalcCenter() const;

	// Calculate box extents
	RED_INLINE Vector CalcExtents() const;

	// Calculate box size
	RED_INLINE Vector CalcSize() const;

	// Extrude by a direction
	RED_INLINE Box& Extrude( const Vector& dir );

	// Extrude by a value
	RED_INLINE Box& Extrude( const Float value );

	// Crop to box
	RED_INLINE void Crop( const Box& box );

	// Normalize to unit box space
	RED_INLINE Box& Normalize( const Box& unitBox );

	// Distance from point
	RED_INLINE Float Distance( const Vector& pos ) const;

	// Squared distance from point
	RED_INLINE Float SquaredDistance( const Vector& pos ) const;

	// Squared distance from point; only takes X and Y in consideration
	RED_INLINE Float SquaredDistance2D( const Vector& pos ) const;

	// Squared distance from other box
	RED_INLINE Float SquaredDistance( const Box& box ) const;

	// Unit Box expanding from 0,0,0 to 1,1,1
	static const Box UNIT;
	// Empty Box
	static const Box EMPTY;

	DECLARE_RTTI_STRUCT( Box );
};

BEGIN_NODEFAULT_CLASS_RTTI( Box );
PROPERTY_EDIT_NAME( Min, TXT("Min"), TXT("Min bounds") );
PROPERTY_EDIT_NAME( Max, TXT("Max"), TXT("Max bounds") );
END_CLASS_RTTI();


/************************************************************************/
/* Sphere                                                               */
/************************************************************************/
struct Sphere
{
	// Holds (Px, Py, Pz, Radius)
	Vector	CenterRadius2;

	RED_INLINE Sphere();
	RED_INLINE Sphere( const Vector& center, Float radius );

	RED_INLINE Vector GetCenterOfVolume() const { return CenterRadius2; }
	RED_INLINE Float GetVolume() const { return 4 * M_PI * CenterRadius2.W * CenterRadius2.W * CenterRadius2.W / 3; }

	// Return translated by a vector
	RED_INLINE Sphere operator+( const Vector& dir ) const;
	// Return translated by a -vector
	RED_INLINE Sphere operator-( const Vector& dir ) const;
	// Translate by a vector
	RED_INLINE Sphere& operator+=( const Vector& dir );
	// Translate by a -vector
	RED_INLINE Sphere& operator-=( const Vector& dir );

	EulerAngles GetOrientation() const { return EulerAngles::ZEROS; }
	// Gets sphere's center position
	RED_INLINE const Vector& GetCenter() const;

	// Gets square of sphere's radius
	RED_INLINE Float GetSquareRadius() const;

	// Gets sphere's radius
	RED_INLINE Float GetRadius() const;

	// Gets shortest distance to point (==0 if point lies on sphere's boundary, <0 if point is inside sphere)
	RED_INLINE Float GetDistance( const Vector& point ) const;

	// Gets shortest distance from this sphere's boundary to another sphere's boundary (<0 if spheres intersect)
	RED_INLINE Float GetDistance( const Sphere& sphere ) const;

	// Check if sphere contains point
	RED_INLINE Bool Contains( const Vector& point ) const;

	// Check if sphere wholly contains other sphere
	RED_INLINE Bool Contains( const Sphere& box ) const;

	// Check if sphere touches other sphere
	RED_INLINE Bool Touches( const Sphere& box ) const;

	// Check if ray intersect sphere: 0 not found, 1 enters and exits, 2 only exits (enterPoint same as origin)
	RED_INLINE Int32 IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint, Vector& exitPoint ) const;

	// Check if ray intersects sphere: same as above but returns distance from origin along the ray
	RED_INLINE Int32 IntersectRay( const Vector& origin, const Vector& direction, Float& enterPoint, Float& exitPoint ) const;

	// Check edge-sphere intersection, returns number of intersection points
	RED_INLINE Int32 IntersectEdge( const Vector& a,  const Vector& b, Vector& intersectionPoint0, Vector& intersectionPoint1 ) const;

	DECLARE_RTTI_STRUCT( Sphere );
};

BEGIN_NODEFAULT_CLASS_RTTI( Sphere );
PROPERTY_EDIT_NAME( CenterRadius2, TXT("CenterRadius2"), TXT("Center and radius") );
END_CLASS_RTTI();

template <> struct TCopyableType<Sphere> { enum { Value = true }; };

/********************************/
/* FixedCapsule					*/
/*	__							*/
/* /  \							*/
/* |  |	<--- Point B			*/
/* |  |							*/
/* |  |	<--- Point A			*/
/* \__/							*/
/*	 <--- Position				*/
/********************************/

struct FixedCapsule
{
	DECLARE_RTTI_STRUCT( FixedCapsule );
	Vector PointRadius;
	Float Height;

	RED_INLINE FixedCapsule() {};
	RED_INLINE FixedCapsule( const Vector& point, Float radius, Float height );


	RED_INLINE Vector GetMassCenter() const { return PointRadius + Vector( 0, 0, Height * 0.5f ); }
	RED_INLINE Float GetMass() const { return 4 * M_PI * PointRadius.W * PointRadius.W * PointRadius.W / 3 + M_PI * PointRadius.W * PointRadius.W * ( Height - 2 * Height ); }

	RED_INLINE void Set( const Vector& point, Float radius, Float height );

	// Gets capsule's position
	RED_INLINE const Vector& GetPosition() const;

	// Calc capsule's start position
	RED_INLINE Vector CalcPointA() const;

	EulerAngles GetOrientation() const { return EulerAngles::ZEROS; }

	// Calc capsule's end position
	RED_INLINE Vector CalcPointB() const;

	// Gets capsule's radius
	RED_INLINE Float GetRadius() const;

	// Gets capsule's height
	RED_INLINE Float GetHeight() const;

	// Check if capsule contains point
	RED_INLINE Bool Contains( const Vector& point ) const;

	// Check if sphere contains sphere
	RED_INLINE Bool Contains( const Sphere& sphere ) const;

	// Return box translated by a vector
	RED_INLINE FixedCapsule operator+( const Vector& dir ) const;

	// Return box translated by a -vector
	RED_INLINE FixedCapsule operator-( const Vector& dir ) const;

	// Translate by a vector
	RED_INLINE FixedCapsule& operator+=( const Vector& dir );

	// Translate by a -vector
	RED_INLINE FixedCapsule& operator-=( const Vector& dir );
};
BEGIN_NODEFAULT_CLASS_RTTI( FixedCapsule );
PROPERTY_EDIT_NAME( PointRadius, TXT("PointRadius"), TXT("Center and radius^2") );
PROPERTY_EDIT_NAME( Height, TXT("Height"), TXT("Height") );
END_CLASS_RTTI();


/************************************************************************/
/* Inline implementation                                                */
/************************************************************************/
#include "math.inl"

#endif // _MATH_H
