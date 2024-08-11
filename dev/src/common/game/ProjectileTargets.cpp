#include "build.h"
#include "ProjectileTargets.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// 	RED_DEFINE_NAME( PROJECTILE_TARGET );
// 	RED_DEFINE_NAME( targetPosition );
// 	RED_DEFINE_NAME( targetNode );
// 	RED_DEFINE_NAME( targetEntity );
// 	RED_DEFINE_NAME( targetBoneIndex );


    CFixedTarget::CFixedTarget( const Vector& target )
        /*: IProjectileTarget( PTT_Fixed )
        ,*/: m_target( target )
    {
    }

    //////////////////////////////////////////////////////////////////////////

    Vector CFixedTarget::GetWorldPosition() const
    {
        return m_target;
    }

    //////////////////////////////////////////////////////////////////////////

//     void CFixedTarget::OnSaveGameplayState( IGameSaver* saver ) const
//     {
//         CGameSaverBlock block( saver, CNAME( PROJECTILE_TARGET ) );
//         saver->WriteValue( CNAME(targetPosition), m_target );
//     }
// 
//     //////////////////////////////////////////////////////////////////////////
// 
//     void CFixedTarget::OnLoadGameplayState( IGameLoader* loader )
//     {
//         CGameSaverBlock block( loader, CNAME( PROJECTILE_TARGET ) );
//         m_target = loader->ReadValue< Vector >( CNAME(targetPosition) );
//     }

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
    
    CStaticTarget::CStaticTarget( CNode* self, Float range )
        //: IProjectileTarget( PTT_Static )
    {
        Vector heading = EulerAngles::YawToVector( self->GetWorldYaw() );
        m_target = self->GetWorldPosition() + heading * range;
    }

    //////////////////////////////////////////////////////////////////////////

    Vector CStaticTarget::GetWorldPosition() const
    {
        return m_target;
    }

    //////////////////////////////////////////////////////////////////////////

//     void CStaticTarget::OnSaveGameplayState( IGameSaver* saver ) const
//     {
//         CGameSaverBlock block( saver, CNAME( PROJECTILE_TARGET ) );
//         saver->WriteValue( CNAME(targetPosition), m_target );
//     }
// 
//     //////////////////////////////////////////////////////////////////////////
// 
//     void CStaticTarget::OnLoadGameplayState( IGameLoader* loader )
//     {
//         CGameSaverBlock block( loader, CNAME( PROJECTILE_TARGET ) );
//         m_target = loader->ReadValue< Vector >( CNAME(targetPosition) );
//     }

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

    CNodeTarget::CNodeTarget( CNode* target )
        /*: IProjectileTarget( PTT_Node )
        ,*/: m_target( target )
    {
    }

    //////////////////////////////////////////////////////////////////////////

    Vector CNodeTarget::GetWorldPosition() const
    {
        return m_target.Get()->GetWorldPosition();
    }

    //////////////////////////////////////////////////////////////////////////

//     void CNodeTarget::OnSaveGameplayState( IGameSaver* saver ) const
//     {
//         CGameSaverBlock block( saver, CNAME( PROJECTILE_TARGET ) );
//         saver->WriteValue( CNAME(targetNode), m_target );
//     }
// 
//     //////////////////////////////////////////////////////////////////////////
// 
//     void CNodeTarget::OnLoadGameplayState( IGameLoader* loader )
//     {
//         CGameSaverBlock block( loader, CNAME( PROJECTILE_TARGET ) );
//         m_target = loader->ReadValue< CNode* >( CNAME(targetNode) );
//     }

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

    CBoneTarget::CBoneTarget( const THandle< CEntity >& entity, Int32 boneIdx )
        /*: IProjectileTarget( PTT_Bone )
        ,*/: m_entity( entity )
        , m_boneIdx( boneIdx )
    {}

    //////////////////////////////////////////////////////////////////////////

    Vector CBoneTarget::GetWorldPosition() const
    {
        const ISkeletonDataProvider* provider = NULL;
        CEntity *pEntity = m_entity.Get();
        if ( pEntity
            && pEntity->GetRootAnimatedComponent() 
            && pEntity->GetRootAnimatedComponent()->QuerySkeletonDataProvider() )
        {
            provider = pEntity->GetRootAnimatedComponent()->QuerySkeletonDataProvider();
        }

        if ( provider && m_boneIdx >= 0 )
        {
            Matrix worldMatrix = provider->GetBoneMatrixWorldSpace( m_boneIdx );
            return worldMatrix.GetTranslation();
        }
        else
        {
            return Vector::ZEROS;
        }
    }

    //////////////////////////////////////////////////////////////////////////

//     void CBoneTarget::OnSaveGameplayState( IGameSaver* saver ) const
//     {
//         CGameSaverBlock block( saver, CNAME( PROJECTILE_TARGET ) );
//         saver->WriteValue( CNAME(targetEntity), m_entity );
//         saver->WriteValue( CNAME(targetBoneIndex), m_boneIdx );
//     }
// 
//     //////////////////////////////////////////////////////////////////////////
// 
//     void CBoneTarget::OnLoadGameplayState( IGameLoader* loader )
//     {
//         CGameSaverBlock block( loader, CNAME( PROJECTILE_TARGET ) );
//         m_entity = loader->ReadValue< THandle<CEntity> >( CNAME(targetEntity) );
//         m_boneIdx = loader->ReadValue< Int32 >( CNAME(targetBoneIndex) );
//     }

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
