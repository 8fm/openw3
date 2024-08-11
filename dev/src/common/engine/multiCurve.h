#pragma once

#include "showFlags.h"
#include "../core/engineTransform.h"
#include "../core/curveData.h"

enum ECurveType
{
	ECurveType_Uninitialized,
	ECurveType_Float,
	ECurveType_Vector,
	ECurveType_EngineTransform,
	ECurveType_EulerAngles,
	ECurveType_MAX
};

BEGIN_ENUM_RTTI( ECurveType );
ENUM_OPTION( ECurveType_Uninitialized );
ENUM_OPTION( ECurveType_Float );
ENUM_OPTION( ECurveType_Vector );
ENUM_OPTION( ECurveType_EngineTransform );
ENUM_OPTION( ECurveType_EulerAngles );
END_ENUM_RTTI();

enum ECurveInterpolationMode : CEnum::TValueType
{
	ECurveInterpolationMode_Linear,		// Linear interpolation of values
	ECurveInterpolationMode_Automatic,	// Cardinal splines (by default Catmull-Rom spline)
	ECurveInterpolationMode_Manual		// Bezier or other manually-per-control-point-edited curve
};

BEGIN_ENUM_RTTI( ECurveInterpolationMode );
ENUM_OPTION( ECurveInterpolationMode_Linear );
ENUM_OPTION( ECurveInterpolationMode_Automatic );
ENUM_OPTION( ECurveInterpolationMode_Manual );
END_ENUM_RTTI();

enum ECurveManualMode : CEnum::TValueType
{
	ECurveManualMode_Bezier,
	ECurveManualMode_BezierSymmetricDirection,
	ECurveManualMode_BezierSymmetric,
};

BEGIN_ENUM_RTTI( ECurveManualMode );
ENUM_OPTION( ECurveManualMode_Bezier );
ENUM_OPTION( ECurveManualMode_BezierSymmetricDirection );
ENUM_OPTION( ECurveManualMode_BezierSymmetric );
END_ENUM_RTTI();

enum ECurveRelativeMode : CEnum::TValueType
{
	ECurveRelativeMode_None,				//!< Curve is stored in absolute coordinates
	ECurveRelativeMode_InitialTransform,	//!< Curve is stored relative to initial parent (CNode) transformation
	ECurveRelativeMode_CurrentTransform		//!< Curve is stored relative to current parent (CNode) transformation (is attached to moving parent)
};

BEGIN_ENUM_RTTI( ECurveRelativeMode );
ENUM_OPTION( ECurveRelativeMode_None );
ENUM_OPTION( ECurveRelativeMode_InitialTransform );
ENUM_OPTION( ECurveRelativeMode_CurrentTransform );
END_ENUM_RTTI();

struct SMultiCurve;
class CNode;
class EngineTransform;

class ICurveChangeListener
{
public:
	virtual ~ICurveChangeListener() {}
	virtual void OnCurveChanged( SMultiCurve* curve ) = 0;
};

//! Describes the speed of the curve motion on entry and exit to a particular control point
struct SCurveEaseParam
{
	DECLARE_RTTI_STRUCT( SCurveEaseParam );

	Float m_easeIn;		//!< Controls the speed of the entry to control point
	Float m_easeOut;	//!< Controls the speed of the exit from control point

	SCurveEaseParam( Float easeIn = 1.0f, Float easeOut = 1.0f )
		: m_easeIn( easeIn )
		, m_easeOut( easeOut )
	{}

	void Reset()
	{
		m_easeIn = m_easeOut = 1.0f;
	}
};

BEGIN_CLASS_RTTI( SCurveEaseParam );
	PROPERTY( m_easeIn );
	PROPERTY( m_easeOut );
END_CLASS_RTTI();

struct SMultiCurvePosition
{
	DECLARE_RTTI_STRUCT( SMultiCurvePosition );
	Int32					m_edgeIdx;
	Float					m_edgeAlpha;

	Bool operator<( const SMultiCurvePosition& p2 ) const { return m_edgeIdx < p2.m_edgeIdx ? true : m_edgeIdx > p2.m_edgeIdx ? false : m_edgeAlpha < p2.m_edgeAlpha; }
	Bool operator>( const SMultiCurvePosition& p2 ) const { return m_edgeIdx > p2.m_edgeIdx ? true : m_edgeIdx < p2.m_edgeIdx ? false : m_edgeAlpha > p2.m_edgeAlpha; }
};
BEGIN_CLASS_RTTI( SMultiCurvePosition );
END_CLASS_RTTI();

/**
 *	Group of float value curves used to represent other types of curves.
 *
 *	Note: position, rotation and scale components can optionally be relative to whatever parent the curve has
 *	Note: the only reason it is still using SCurveData is we want to be able to edit this curve (time in particular) in 2D editor that works only with SCurveData 
 */
struct SMultiCurve
{
	DECLARE_RTTI_STRUCT( SMultiCurve );

	SMultiCurve();
	SMultiCurve( const SMultiCurve& other );

	// Curve properties

	void SetCurveType( ECurveType curveType, const void* initialValue = NULL, Bool insertEndPoints = true, Uint32 numExtraDataStreams = 0 );
	ECurveType GetCurveType() const;
	void SetPositionInterpolationMode( ECurveInterpolationMode mode );
	ECurveInterpolationMode GetPositionInterpolationMode() const;
	void SetRotationInterpolationMode( ECurveInterpolationMode mode );
	ECurveInterpolationMode GetRotationInterpolationMode() const;
	void SetPositionManualMode( ECurveManualMode mode );
	ECurveManualMode GetPositionManualMode() const;
	Bool IsTimeEditingEnabled() const;
	Bool IsEditableIn3D() const;
	Bool IsEditableIn2D() const;
	void SetLooping( Bool loop );
	Bool IsLooping() const;
	Uint32 GetNumCurves() const;
	SCurveData* GetCurveData( Uint32 index );
	void EnableAutomaticTimeByDistanceRecalculation( Bool enable );
	Bool IsAutomaticTimeByDistanceRecalculationEnabled() { return m_enableAutomaticTimeByDistanceRecalculation; }
	void RecalculateTimeByDistance( const TDynArray< Uint32 >* indices = NULL, Uint32 numApproximationSegmentsBetweenControlPoints = 10 ); //!< Recalculates time based on distance; optionally done for selected control points only (if indices array is supplied)
	void EnableAutomaticTimeRecalculation( Bool enable );
	Bool IsAutomaticTimeRecalculationEnabled() { return m_enableAutomaticTimeRecalculation; }
	void RecalculateTime();
	void EnableAutomaticRotationFromDirectionRecalculation( Bool enable );
	Bool IsAutomaticRotationFromDirectionRecalculationEnabled() { return m_enableAutomaticRotationFromDirectionRecalculation; }
	void RecalculateRotationFromDirection();
	void EnableConsistentNumberOfControlPoints( Bool enable );
	void RecalculateBezierTangents();
	void SetAutomaticInterpolationSmoothness( Float smoothness );
	Float GetTotalTime() const;
	void SetTotalTime( Float totalTime ) { m_totalTime = totalTime; }
	void Translate( const Vector& translation );
	Bool HasRotation() const;
	Bool HasPosition() const;
	Bool HasScale() const;
	Bool HasEaseParams() const { return !m_easeParams.Empty(); }
	void EnableEaseParams( Bool enable );
	SCurveEaseParam& GetEaseParams( int index ) { return m_easeParams[index]; }
	TDynArray<SCurveData>& GetDataCurves() { return m_curves; }
	RED_FORCE_INLINE const SCurveData* GetPositionCurves() const { return ( m_type == ECurveType_EngineTransform || m_type == ECurveType_Vector ) ? &m_curves[0] : NULL; }
	RED_FORCE_INLINE SCurveData* GetPositionCurves() { return ( m_type == ECurveType_EngineTransform || m_type == ECurveType_Vector ) ? &m_curves[0] : NULL; }
	RED_FORCE_INLINE const SCurveData* GetRotationCurves() const { return ( m_type == ECurveType_EngineTransform ) ? &m_curves[3] : ( ( m_type == ECurveType_EulerAngles ) ? &m_curves[0] : NULL ); }
	RED_FORCE_INLINE SCurveData* GetRotationCurves() { return ( m_type == ECurveType_EngineTransform ) ? &m_curves[3] : ( ( m_type == ECurveType_EulerAngles ) ? &m_curves[0] : NULL ); }
	RED_FORCE_INLINE const SCurveData* GetScaleCurves() const { return ( m_type == ECurveType_EngineTransform ) ? &m_curves[6] : NULL; }
	RED_FORCE_INLINE SCurveData* GetScaleCurves() { return ( m_type == ECurveType_EngineTransform ) ? &m_curves[6] : NULL; }

	// Value at specified time

	Float GetFloat( Float time ) const; //!< Gets float value at given time
	void GetPosition( Float time, Vector& result ) const; //!< Gets position value at given time
	void GetRotation( Float time, EulerAngles& result ) const; //!< Gets rotation value at given time
	void GetScale( Float time, Vector& result ) const; //!< Gets scale value at given time
	void GetTransform( Float time, EngineTransform& result ) const; //!< Gets transform value at given time

	// Extra data streams

	Uint32 GetNumBaseDataStreams() const; //!< Gets number of base data streams
	Uint32 GetNumExtraDataStreams() const; //!< Gets number of extra data streams
	void GetExtraDataValues( Float time, Float* result ) const; //!< Gets extra data float values at given time
	void GetControlPointExtraDataValues( Uint32 index, Float* result ) const; //!< Gets extra data float values at given control point
	void SetExtraDataValues( Uint32 index, const Float* values ); //!< Sets extra data float values at given control point
	RED_FORCE_INLINE const SCurveData* GetExtraDataCurves( Uint32& count ) const { const Uint32 numBase = GetNumBaseDataStreams(); count = m_curves.Size() - numBase; return count ? &m_curves[ numBase ] : NULL; }
	RED_FORCE_INLINE SCurveData* GetExtraDataCurves( Uint32& count ) { const Uint32 numBase = GetNumBaseDataStreams(); count = m_curves.Size() - numBase; return count ? &m_curves[ numBase ] : NULL; }

	// Control points

	Uint32 Size() const; //!< Gets number of control points
	Float GetControlPointTime( Uint32 index ) const; //!< Gets time at control point
	void GetControlPointPosition( Uint32 index, Vector& result ) const; //!< Gets the position at control point
	void SetControlPointPosition( Uint32 index, const Vector& value ); //!< Sets the position at control point
	void GetControlPointTangent( Uint32 index, Uint32 tangentIndex, Vector& result ); //!< Gets tangent vector (left:0 or right:1) at control point
	void SetControlPointTangent( Uint32 index, Uint32 tangentIndex, const Vector& value ); //!< Sets tangent vector (left:0 or right:1) at control point
	void GetControlPointTransform( Uint32 index, EngineTransform& result ) const; //!< Gets transform at control point
	void SetControlPointTransform( Uint32 index, const EngineTransform& value ); //!< Sets transform at control point
	void SetControlPointRotation( Uint32 index, const EulerAngles& value );  //!< Gets rotation at control point
	Uint32 AddControlPoint( Float time, const Vector& value ); //!< Adds control point with given position
	Uint32 AddControlPoint( Float time, const EngineTransform& value ); //!< Adds control point with given transform
	Uint32 AddControlPointAt( Uint32 index, const Vector* pos = NULL ); //! Adds control point at given index; its time gets actually calculated as an interpolated time between previous and next control points
	Uint32 AddControlPointAt( Uint32 index, const EngineTransform& transform ); //! Adds control point at given index; its time gets actually calculated as an interpolated time between previous and next control points
	Uint32 AddControlPoint( const Vector& pos ); //! Adds control point at given position; time and index are automatically deduced from the position (nearest edge is found and new control points is inserted into it)
	void RemoveControlPoint( Uint32 index ); //!< Removes control point at given index
	void SetControlPointTime( Uint32 i, Float time ); //!< Sets time at control point

	// Relative transformation related

	void SetTransformationRelativeMode( ECurveRelativeMode mode );
	void SetTranslationRelativeMode( ECurveRelativeMode mode );
	void SetRotationRelativeMode( ECurveRelativeMode mode );
	void SetScaleRelativeMode( ECurveRelativeMode mode );

	// Root transformation (root indicates relative to parent as per current relative mode)

	void GetRootTransform( EngineTransform& result ) const;
	void ToRootTransform( const EngineTransform& localTransform, EngineTransform& result ) const;
	void ToRootPosition( const Vector& localPosition, Vector& result ) const;

	void GetRootPosition( Float time, Vector& result ) const;
	void GetRootRotation( Float time, EulerAngles& result ) const;
	void GetRootTransform( Float time, EngineTransform& result ) const;
	void GetRootControlPointPosition( Uint32 index, Vector& result ) const;
	void GetRootControlPointTransform( Uint32 index, EngineTransform& result ) const;

	// Absolute to local space transformation

	void ToLocalMatrix( const EngineTransform& absoluteTransform, Matrix& result ) const;
	void ToLocalTransform( const EngineTransform& absoluteTransform, EngineTransform& result ) const;
	void ToLocalPosition( const Vector& absolutePosition, Vector& result ) const;

	// Miscellaneous

	void SetParent( CNode* parent );
	CNode* GetParent() const { return m_parent; }
	void UpdateInitialTransformFromParent( Bool force = false ) const;
	void Reset();
	void SetFromPoints( const TDynArray<Vector>& points, const TDynArray<Float>* times = NULL );
	void SetColor( const Color& color ) { m_color = color; }
	const Color& GetColor() const { return m_color; }
	void SetShowFlags( EShowFlags flags ) { m_showFlags = flags; }
	EShowFlags GetShowFlags() const { return m_showFlags; }
	Float CalculateLength( Uint32 numApproximationSegmentsBetweenControlPoints = 1 ) const;
	Float CalculateSegmentLength( Uint32 startIndex, Uint32 numApproximationSegmentsBetweenControlPoints = 1 ) const;

	Uint32 FindNearestControlPoint( const Vector& pos ) const;
	Uint32 FindNearestEdgeIndex( const Vector& pos ) const;
	void CalculateTangentFromCurveDirection( Float time, Vector& tangent ) const;
	void CalculateAbsoluteTangentFromCurveDirection( Float time, Vector& tangent ) const;
	void CalculateAnglesFromCurveDirection( Float time, EulerAngles& angles ) const;
	void CalculateAbsoluteTangentFromCurveDirection( const SMultiCurvePosition & curvePosition, Vector& tangent ) const;
	void CalculateAbsolute2DTangentFromCurveDirection( const SMultiCurvePosition & curvePosition, Vector2& tangent, Float minPrecision = 0.15f ) const;

	// Absolute values (used mostly for editing)

	void GetAbsoluteTransform( EngineTransform& result ) const;
	void GetAbsoluteMatrix( Matrix& result ) const;
	void ToAbsoluteMatrix( const EngineTransform& localTransform, Matrix& result ) const;
	void ToAbsoluteTransform( const EngineTransform& localTransform, EngineTransform& result ) const;
	void ToAbsolutePosition( const Vector& localPosition, Vector& result ) const;

	void GetAbsolutePosition( Float time, Vector& result ) const;
	void GetAbsoluteRotation( Float time, EulerAngles& result ) const;
	void GetAbsoluteTransform( Float time, EngineTransform& result ) const;
	void GetAbsoluteControlPointPosition( Uint32 index, Vector& result ) const;
	void GetAbsoluteControlPointTransform( Uint32 index, EngineTransform& result ) const;

	// Curve position related interface
	Float GetAlphaOnEdge( const Vector3& spot, Uint32 edgeIdx, Vector& outClosestSpot, Float epsilon = 0.001f ) const;
	void GetClosestPointOnCurve( const Vector& point, SMultiCurvePosition& outCurvePos, Vector& outClosestSpot, Float epsilon = 0.001f ) const;

	void GetPointOnCurveInDistance( SMultiCurvePosition& curvePosition, Float distance, Vector& outComputedSpot, Bool &isEndOfPath ) const;
	void GetCurvePoint( const SMultiCurvePosition& curvePosition, Vector& outCurvePoint ) const;
	void CorrectCurvePoint( const Vector& point, SMultiCurvePosition& inOutCurvePosition, Vector& outClosestSpot, Float epsilon = 0.01f ) const;

protected:

	// Miscellaneous

	void ValidateTangents();
	Float ToNormalizedTime( Float time ) const;
	Float ToNormalizedTimeAtControlPoint( Float time ) const;
	Float FromNormalizedTimeAtControlPoint( Float time ) const;
	RED_FORCE_INLINE Float FromNormalizedTime( Float normalizedTime ) const { return normalizedTime * m_totalTime; }
	void PostControlPointAdded( Uint32 newControlPointIndex );
	void PostControlPointRemoved( Uint32 removedControlPointIndex );
	void PostChange();
	static void DirectionToEulerAngles( const Vector& direction, EulerAngles& angles );
	void CalculateTangentFromCurveAtControlPoint( Uint32 index, Vector& direction ) const;
	void GetPositionInSegment( Uint32 startIndex, Float localTime, Vector& result ) const;
	void GetInterpolationControlPoints( Vector& p1, Vector& p2, Vector& p3, Vector& p4, Int32 edgeIdx ) const;
	RED_FORCE_INLINE Uint32 GetPrevIndex( const Uint32 index ) const { return index ? ( index - 1 ) : ( IsLooping() ? ( Size() - 1 ) : index ); }
	RED_FORCE_INLINE Uint32 GetNextIndex( const Uint32 index ) const { return ( index + 1 < Size() ) ? ( index + 1 ) : ( IsLooping() ? 0 : index ); }

	ECurveType				m_type;												// Stored curve type
	EShowFlags				m_showFlags;										// Debug show flags
	Color					m_color;											// Debug draw color
	ECurveInterpolationMode m_positionInterpolationMode;						// Position interpolation mode
	ECurveManualMode		m_positionManualMode;								// Manual position interpolation mode (only valid when m_positionInterpolationMode is set to ECurveInterpolationMode_Manual)
	Float					m_automaticPositionInterpolationSmoothness;			// Automatic position interpolation smoothness factor within 0..1 (only used when m_positionInterpolationMode is set to ECurveInterpolationMode_Automatic)
	ECurveInterpolationMode m_rotationInterpolationMode;						// Rotation interpolation mode
	Float					m_automaticRotationInterpolationSmoothness;			// Automatic rotation interpolation smoothness factor within 0..1 (only used when m_positionInterpolationMode is set to ECurveInterpolationMode_Automatic)
	Float					m_totalTime;										// Total curve time
	Bool					m_enableConsistentNumberOfControlPoints;			// Makes sure all curves (X, Y, Z and so on) have equal number of control points
	Bool					m_enableAutomaticTimeByDistanceRecalculation;		// Enables automatic time recalculation based on distance between control points (only vectors and transforms)
	Bool					m_enableAutomaticTimeRecalculation;					// Enables automatic time recalculation based on control point index
	Bool					m_enableAutomaticRotationFromDirectionRecalculation;// Performs automatic rotation recalculation based on curve shape (only transforms)
	TDynArray<SCurveData>	m_curves;											// Internal curves (one for each float component)
	TDynArray<Vector, MC_CurveData>		m_leftTangents;							// Left tangents for each control point (only valid when m_positionInterpolationMode is set to ECurveInterpolationMode_Manual)
	TDynArray<Vector, MC_CurveData>		m_rightTangents;						// Right tangents for each control point (only valid when m_positionInterpolationMode is set to ECurveInterpolationMode_Manual)
	TDynArray<SCurveEaseParam>	m_easeParams;									// Ease in and out parameters for each control points (empty if unused)
	ECurveRelativeMode		m_translationRelativeMode;							// Translation transformation relative mode
	ECurveRelativeMode		m_rotationRelativeMode;								// Rotation transformation relative mode
	ECurveRelativeMode		m_scaleRelativeMode;								// Scale transformation relative mode
	CNode*					m_parent;											// Transformation parent
	mutable Bool			m_hasInitialParentTransform;						// Indicates whether we have valid initial parent transform
	mutable EngineTransform	m_initialParentTransform;							// Initial parent transformation
#ifndef NO_EDITOR
	static TDynArray< ICurveChangeListener* > m_changeListeners;
public:
	static void RegisterChangeListener( ICurveChangeListener* listener );
	static void UnregisterChangeListener( ICurveChangeListener* listener );
	static void PostCurveChanged( SMultiCurve* curve );
#endif
};

BEGIN_CLASS_RTTI( SMultiCurve );
	PROPERTY( m_type );
	PROPERTY_EDIT( m_color, TXT("Editor line color") );
	PROPERTY( m_showFlags );
	PROPERTY( m_positionInterpolationMode );
	PROPERTY( m_positionManualMode );
	PROPERTY( m_rotationInterpolationMode );
	PROPERTY_EDIT( m_totalTime, TXT("Total curve time") );
	PROPERTY_EDIT( m_automaticPositionInterpolationSmoothness, TXT("Automatic position interpolation smoothness scale within 0..1 range; only used when position interpolation mode is set to automatic") );
	PROPERTY_EDIT( m_automaticRotationInterpolationSmoothness, TXT("Automatic rotation interpolation smoothness scale within 0..1 range; when set to 1 aligns 100% with curve direction; only used when rotation interpolation mode is set to automatic") );
	PROPERTY( m_enableConsistentNumberOfControlPoints );
	PROPERTY( m_enableAutomaticTimeByDistanceRecalculation );
	PROPERTY( m_enableAutomaticTimeRecalculation );
	PROPERTY( m_enableAutomaticRotationFromDirectionRecalculation );
	PROPERTY( m_curves );
	PROPERTY( m_leftTangents );
	PROPERTY( m_rightTangents );
	PROPERTY( m_easeParams );
	PROPERTY_EDIT( m_translationRelativeMode, TXT("Translation relative mode (relative to parent component/entity/node)") );
	PROPERTY_EDIT( m_rotationRelativeMode, TXT("Rotation relative mode (relative to parent component/entity/node)") );
	PROPERTY_EDIT( m_scaleRelativeMode, TXT("Scale relative mode (relative to parent component/entity/node)") );
	PROPERTY( m_initialParentTransform );
	PROPERTY( m_hasInitialParentTransform );
END_CLASS_RTTI();
