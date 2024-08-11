/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

RED_INLINE void QuaternionToMatrix( const Vector& quat, Matrix& matrix )
{
	RED_UNUSED( matrix );
	RED_UNUSED( quat );
	HALT( "Not implemented!");
}

RED_INLINE void MatrixToQuaternion( const Matrix& matrix, Vector& quat )
{
	RED_UNUSED( matrix );
	RED_UNUSED( quat );
	HALT( "Not implemented!" );
}
