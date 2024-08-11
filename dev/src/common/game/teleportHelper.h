/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CTeleportHelper
{
private:
	const STeleportInfo* m_info;

public:
	CTeleportHelper( const STeleportInfo* info );
	Bool GetPlayerStartingPoint( Vector& position, EulerAngles& rotation );
	Bool TeleportAllActors();

private:
	CNode* GetTeleportLocation() const;
	void TeleportToDestination( CNode* teleportDest, const Vector& offset, CActor* actor ) const;
	void TeleportAroundDestination( CNode* teleportDest, const TDynArray< CActor* >& actors ) const;
	void ExtractActorsForTeleport( TDynArray< CActor* >& outActors ) const;

	// Game specific
	Bool GetCustomPlayerStartingPoint( Vector& position, EulerAngles& rotation );
};