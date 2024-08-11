
#pragma once

enum ERPWidgetSnapMode
{
	SNAP_ToNothing = 0,
	SNAP_ToTerrainVisual,
	SNAP_ToTerrainPhysical,
	SNAP_ToStaticCollision,
};

enum ERPWidgetSnapOrgin
{
	SNAP_ByPivot = 0,
	SNAP_ByBoundingVolume,
};

/// Widget mode
enum ERPWidgetMode
{
	RPWM_None,
	RPWM_Move,
	RPWM_Rotate,
	RPWM_Scale,
};

/// Widget space
enum ERPWidgetSpace
{
	RPWS_Local,
	RPWS_Global,
	RPWS_Foreign
};

class CWorld;
class CSelectionManager;

class CNodeTransformManager
{
public:
	CNodeTransformManager( CWorld* world, CSelectionManager* selMan );

	~CNodeTransformManager();

	//! @name Transforming given nodes
	//@{ 

	//!
	void TransformChangeStart( const TDynArray< CNode* >& nodes );

	//! Move nodes
	Bool Move( const TDynArray< CNode* >& nodes, 
			   const Vector& delta );

	//! Rotate nodes
	Bool Rotate( const TDynArray< CNode* >& nodes,
				 const Matrix& rotationMatrix, 
				 Bool individually,
				 CNode* pivotNode,
				 Vector& pivotOffset );

	//! Rotate nodes using euler angles. May help with precision problems.
	Bool Rotate( const TDynArray< CNode* >& nodes,
				 const EulerAngles& eulerAngles, 
				 Bool individually,
				 CNode* pivotNode,
				 Vector& pivotOffset );

	//! Scale nodes
	Bool Scale( const TDynArray< CNode* >& nodes,
				const Vector& delta, 
				const Matrix& space,
				Bool individually,
				CNode* pivotNode,
				Vector& pivotOffset,
				ERPWidgetSpace widgetSpace );

	//!
	void TransformChangeStop( const TDynArray< CNode* >& nodes );

	//@}

	//! @name Transforming selection
	//@{

	//! Start moving selected roots
	void TransformSelectionStart();

	//! Move selected roots
	Bool MoveSelection( const Vector& delta );

	//! Rotate selected roots
	Bool RotateSelection( const Matrix& rotationMatrix, 
						  Bool individually );

	//! Rotate using euler angles. May help with precision problems.
	Bool RotateSelection( const EulerAngles& eulerAngles, 
						  Bool individually );

	//! Scale selected roots
	Bool ScaleSelection( const Vector& delta, 
						 const Matrix& space, 
						 ERPWidgetSpace widgetSpace, 
						 Bool individually );

	//! End moving selected roots
	void TransformSelectionStop();

	//! Get selection pivot local space
	Matrix CalculatePivotSpace( ERPWidgetSpace widgetSpace );

	//@}


	//! @name Transform modifications
	//@{

	//!
	void SetForeignRotation( const EulerAngles& angles ) { m_foreignRotation = angles; }

	//!
	void SetSnapMode( ERPWidgetSnapMode snapMode ) { m_snapMode = snapMode; }

	//!
	void SetSnapOrgin( ERPWidgetSnapOrgin snapOrgin ) { m_snapOrgin = snapOrgin; }

	//!
	ERPWidgetSnapMode GetSnapMode() const { return m_snapMode; }

	//!
	ERPWidgetSnapOrgin GetSnapOrgin() const { return m_snapOrgin; }

	//@}

private:
	void DoMoveNode( CNode* node, const Vector& newPosition );
	Bool MarkModified( const TDynArray< CNode* >& nodes );

	CWorld*            m_world;
	CSelectionManager* m_selManager;
	ERPWidgetSnapMode  m_snapMode;
	ERPWidgetSnapOrgin m_snapOrgin;
	EulerAngles        m_foreignRotation; //!< Rotation for foreign space mode
};

