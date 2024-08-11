/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef _H_CORE_TYPE_REGISTRY
#define _H_CORE_TYPE_REGISTRY

#include "names.h"
#include "typeName.h"

// this file contains list of all types in 'core' project

#if !defined( REGISTER_RTTI_TYPE )
	#define REGISTER_RTTI_TYPE( _className ) RED_DECLARE_RTTI_NAME( _className ) template<> struct TTypeName< _className >{ static const CName& GetTypeName() { return CNAME( _className ); } };
	#define REGISTER_NOT_REGISTERED
#endif

#define REGISTER_RTTI_CLASS( _className ) class _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_STRUCT( _className ) struct _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_ENUM( _className ) enum _className; REGISTER_RTTI_TYPE( _className );

REGISTER_RTTI_TYPE( CObject );

REGISTER_RTTI_CLASS( IReferencable );
REGISTER_RTTI_CLASS( ISerializable );
REGISTER_RTTI_CLASS( IScriptable );
REGISTER_RTTI_CLASS( CScriptableState );

REGISTER_RTTI_STRUCT( SSimplexTreeStruct );
REGISTER_RTTI_STRUCT( Vector );
REGISTER_RTTI_STRUCT( Vector3 );
REGISTER_RTTI_STRUCT( Vector2 );
REGISTER_RTTI_STRUCT( Tetrahedron );
REGISTER_RTTI_STRUCT( CutCone );
REGISTER_RTTI_STRUCT( Matrix );
REGISTER_RTTI_STRUCT( EulerAngles );
REGISTER_RTTI_STRUCT( Color );
REGISTER_RTTI_STRUCT( Box );
REGISTER_RTTI_STRUCT( Rect );
REGISTER_RTTI_STRUCT( Sphere );
REGISTER_RTTI_STRUCT( AACylinder );
REGISTER_RTTI_STRUCT( Segment );
REGISTER_RTTI_STRUCT( OrientedBox );
REGISTER_RTTI_STRUCT( FixedCapsule );
REGISTER_RTTI_STRUCT( ConvexHull );
REGISTER_RTTI_STRUCT( Cylinder );
REGISTER_RTTI_STRUCT( Quad );
REGISTER_RTTI_STRUCT( Plane );

REGISTER_RTTI_STRUCT( SSimpleCurvePoint );
REGISTER_RTTI_STRUCT( SSimpleCurve );
REGISTER_RTTI_STRUCT( SCurveData );
REGISTER_RTTI_STRUCT( SCurveDataEntry );
REGISTER_RTTI_STRUCT( SCurve3DData );
REGISTER_RTTI_STRUCT( Bezier2D );
REGISTER_RTTI_CLASS( Bezier2dHandle );

REGISTER_RTTI_CLASS( CResource );
REGISTER_RTTI_CLASS( CPackage );
REGISTER_RTTI_CLASS( IFactory );
REGISTER_RTTI_CLASS( IImporter );
REGISTER_RTTI_CLASS( IExporter );
REGISTER_RTTI_STRUCT( EngineTime );
REGISTER_RTTI_CLASS( LongBitField );

REGISTER_RTTI_CLASS( IThumbnailImageLoader );
REGISTER_RTTI_CLASS( IThumbnailGenerator );
REGISTER_RTTI_CLASS( IThumbnail );
REGISTER_RTTI_CLASS( CThumbnail );

REGISTER_RTTI_CLASS( ICommandlet );

#ifndef NO_RESOURCE_COOKING
REGISTER_RTTI_CLASS( ICooker );
#endif

#ifndef NO_EDITOR
REGISTER_RTTI_CLASS( IAnalyzer );
#endif

REGISTER_RTTI_CLASS( C2dArray );
REGISTER_RTTI_CLASS( CIndexed2dArray );

REGISTER_RTTI_CLASS( CResourceDefEntry );

#undef REGISTER_RTTI_CLASS
#undef REGISTER_RTTI_STRUCT
#undef REGISTER_RTTI_ENUM

#if defined( REGISTER_NOT_REGISTERED )
	#undef REGISTER_RTTI_TYPE
	#undef REGISTER_NOT_REGISTERED
#endif

#endif