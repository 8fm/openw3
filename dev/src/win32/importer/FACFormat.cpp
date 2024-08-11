
#include "build.h"
#include "FACFormat.h"
#include "../../common/engine/mimicFac.h"

namespace FacFormatImporter
{
	Bool InternalImportPoses( const fac::fpose* inputPoses, Int32 inputPosesNum, TDynArray< SMimicTrackPose >& poses, Float defaultValue, const String& namePattern )
	{
		static const Float THRES = 0.01f;

		for ( Int32 i=0; i<inputPosesNum; ++i )
		{
			const fac::fpose& inputPose = inputPoses[ i ];

			CName poseName = CName( ANSI_TO_UNICODE( inputPose.name.data ) );

			if ( !namePattern.Empty() && !poseName.AsString().BeginsWith( namePattern ) )
			{
				continue;
			}

			const Uint32 poseIndex = static_cast< Uint32 >( poses.Grow( 1 ) );

			SMimicTrackPose& outputPose = poses[ poseIndex ];
			outputPose.m_name = poseName;

			Bool allSet = true;
			for ( Int32 t=0; t<inputPose.numweights; ++t )
			{
				if ( MAbs( inputPose.weights[ t ] - defaultValue ) > THRES )
				{
					allSet = false;
					break;
				}
			}

			if ( allSet )
			{
				outputPose.m_mapping.Clear();
				outputPose.m_tracks.Reserve( inputPose.numweights );

				for ( Int32 t=0; t<inputPose.numweights; ++t )
				{
					outputPose.m_tracks.PushBack( inputPose.weights[ t ] );
				}
			}
			else
			{
				for ( Int32 t=0; t<inputPose.numweights; ++t )
				{
					const Float w = inputPose.weights[ t ];

					if ( MAbs( w - defaultValue ) > THRES )
					{
						outputPose.m_mapping.PushBack( t );
						outputPose.m_tracks.PushBack( w );
					}
				}
			}
		}

		return true;
	}

	Bool ImportTrackPoses( const fac::cface& loadedData, TDynArray< SMimicTrackPose >& poses )
	{
		return InternalImportPoses( loadedData.poses, loadedData.numposes, poses, 0.f, TXT("pose__") );
	}

	Bool ImportFilterPoses( const fac::cface& loadedData, TDynArray< SMimicTrackPose >& filters )
	{
		return InternalImportPoses( loadedData.filters, loadedData.numfilters, filters, 1.f );
	}
}
