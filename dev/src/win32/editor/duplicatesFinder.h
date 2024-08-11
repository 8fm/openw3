#pragma once
class CDuplicatesFinder
{
	struct DuplicateInfo
	{
		String m_depotPath;
		String m_layer;
		Vector m_worldPos;
		EulerAngles m_worldRotation;
		CEntityTemplate* m_entityTemplate;

		DuplicateInfo( const String& path, const Vector& pos, const EulerAngles& rotation, const String& layer, CEntityTemplate* entityTemplate )
			: m_depotPath( path ), m_worldPos( pos ), m_worldRotation( rotation ), m_layer( layer )
		{
			m_entityTemplate = entityTemplate;
		}

		RED_INLINE Bool operator==( const DuplicateInfo& a ) const
		{
			return m_depotPath == a.m_depotPath && m_worldPos == a.m_worldPos && m_layer == a.m_layer && m_worldRotation == a.m_worldRotation;
		}

		RED_INLINE Uint32 CalcHash() const
		{
			String hashString = String::Printf( TXT( "%ls%ls%ls%ls" ), m_depotPath.AsChar(), m_layer.AsChar(), ToString( m_worldPos ).AsChar(), ToString( m_worldRotation ).AsChar() );
			Uint32 hash = GetHash( hashString );

			return hash;
		}
	};

	typedef THashMap< String, Uint32 >	DuplicatesContainer;

public:
	static void FindDuplicates( TDynArray< String >& duplicatesOutInfo, const TDynArray< CLayerInfo* >& layers, Bool modifiedOnly = true );

private:
	static void FindDuplicatesInLayer( const CLayerInfo* li, THashMap< DuplicateInfo, DuplicatesContainer >& duplicatesChecker );
	static void HandleKey( const String& name, const String& path, const String& layerPath, const Vector& pos, const EulerAngles& angles, THashMap< DuplicateInfo, DuplicatesContainer >& duplicatesChecker, CEntityTemplate* entityTemplate );
	static void PrepareDuplicatesInfo( TDynArray< String >& duplicatesOutInfo, const THashMap< DuplicateInfo, DuplicatesContainer >& duplicatesChecker );

	static void HandleTemplateInfo( TDynArray< String >& duplicatesOutInfo, C2dArray* outCSV, const DuplicateInfo& info, const DuplicatesContainer& container );
	static void HandleLayerInfo( TDynArray< String >& duplicatesOutInfo, C2dArray* outCSV, const DuplicateInfo& info, const DuplicatesContainer& container );
};

