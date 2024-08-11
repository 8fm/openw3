#pragma once

/// From a tag will return rider or mount
/// depending if rider is mounted or not
class CGetRiderOrMountHelper
{
public:
	// User interface

	/// Call this before using GetActor, this call is more expensive
	void Initialise( CName riderTag );

	/// Returns rider or mount depending if rider is mounted or not
	CEntity *const GetEntity()const;
public:
	CGetRiderOrMountHelper()
		: m_riderEntity()
		, m_riderExternalAIStorage()			{}

private:
	THandle<CEntity>								m_riderEntity;
	THandle< CAIStorageRiderData >					m_riderExternalAIStorage;
};