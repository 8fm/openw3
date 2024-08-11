#include "build.h"
#include "r4SwarmUtils.h"
#include "swarmCellMap.h"
#include "../../common/core/directory.h"

CR4SwarmScriptRegistration CR4SwarmScriptRegistration::s_r4SwarmScriptRegistration;

CR4SwarmScriptRegistration::CR4SwarmScriptRegistration()
	: m_unused( false )
{

}

void CR4SwarmScriptRegistration::RegisterScriptFunctions()const
{
	extern void funcFlyingGroupIdCompare( IScriptable* context, CScriptStackFrame& stack, void* result );
	NATIVE_GLOBAL_FUNCTION( "FlyingGroupId_Compare", funcFlyingGroupIdCompare );

	extern void funcFlyingGroupIdIsValid( IScriptable* context, CScriptStackFrame& stack, void* result );
	NATIVE_GLOBAL_FUNCTION( "FlyingGroupId_IsValid", funcFlyingGroupIdIsValid );
}

Bool R4SwarmUtils::FindCellToClearPath( const Vector3 & position, const Vector & displacementVector, const CSwarmCellMap *cellMap, Vector3 & freeCellPosition )
{
	// Damned we are in an obstacle
	// brute force course correct on the prefered Axis first
	const Vector3 absVelocity = Vector3( Abs( displacementVector.X ), Abs( displacementVector.Y ), Abs( displacementVector.Z ) );
	const Bool XGreaterThanY = absVelocity.X > absVelocity.Y;
	const Bool XGreaterThanZ = absVelocity.X > absVelocity.Z;
	const Bool YGreaterThanX = absVelocity.Y > absVelocity.X;
	const Bool YGreaterThanZ = absVelocity.Y > absVelocity.Z;
	const Bool ZGreaterThanX = absVelocity.Z > absVelocity.X;
	const Bool ZGreaterThanY = absVelocity.Z > absVelocity.Y;

	const Vector3 absoluteFirstAxis( XGreaterThanY && XGreaterThanZ ? 1.0f : 0.0f, YGreaterThanX && YGreaterThanZ ?  1.0f : 0.0f, ZGreaterThanX && ZGreaterThanY ?  1.0f : 0.0f );
	const Vector3 firstAxis = absoluteFirstAxis * displacementVector; // filters out unwanted axis and gives us sign

	const Vector3 absoluteSecondAxis( firstAxis.X == 0.0f && ( XGreaterThanY || firstAxis.Y != 0.0f ) && ( XGreaterThanZ || firstAxis.Z != 0.0f )  ? 1.0f : 0.0f,
									firstAxis.Y == 0.0f && ( YGreaterThanX || firstAxis.X != 0.0f ) && ( YGreaterThanZ || firstAxis.Z != 0.0f )  ? 1.0f : 0.0f, 
									firstAxis.Z == 0.0f && ( ZGreaterThanX || firstAxis.X != 0.0f ) && ( ZGreaterThanY || firstAxis.Y != 0.0f )  ? 1.0f : 0.0f );
	const Vector3 secondAxis = absoluteSecondAxis * displacementVector; // filters out unwanted axis and gives us sign

	const Vector3 absoluteThirdAxis( firstAxis.X == 0.0f && secondAxis.X == 0.0f  ? 1.0f : 0.0f,
									firstAxis.Y == 0.0f && secondAxis.Y == 0.0f  ? 1.0f : 0.0f,
									firstAxis.Z == 0.0f && secondAxis.Z == 0.0f  ? 1.0f : 0.0f );
	const Vector3 thirdAxis = absoluteThirdAxis * displacementVector; // filters out unwanted axis and gives us sign

	if ( cellMap->GetFreeCellAtDistance_AxisAligned( position, freeCellPosition, firstAxis, displacementVector.W ) )
	{
		return true;
	}
	if ( cellMap->GetFreeCellAtDistance_AxisAligned( position, freeCellPosition, secondAxis, displacementVector.W ) )
	{
		return true;
	}
	if ( cellMap->GetFreeCellAtDistance_AxisAligned( position, freeCellPosition, thirdAxis, displacementVector.W ) )
	{
		return true;
	}
	if ( cellMap->GetFreeCellAtDistance_AxisAligned( position, freeCellPosition, firstAxis * -1.0f, displacementVector.W ) )
	{
		return true;
	}
	if ( cellMap->GetFreeCellAtDistance_AxisAligned( position, freeCellPosition, secondAxis * -1.0f, displacementVector.W ) )
	{
		return true;
	}
	if ( cellMap->GetFreeCellAtDistance_AxisAligned( position, freeCellPosition, thirdAxis * -1.0f, displacementVector.W ) )
	{
		return true;
	}
	return false;
						
}

CDirectory*const R4SwarmUtils::GetDataDirectory(  )
{
	static const String dirName( TXT("cellmaps") );
	//static const String DIR_LOCAL( TXT("navi_local") );
	
	CWorld* world			= GGame->GetActiveWorld();
	CDiskFile* worldFile	= world->GetFile();
	if ( !worldFile )
	{
		return NULL;
	}

	return worldFile->GetDirectory()->CreateNewDirectory( dirName );
}



