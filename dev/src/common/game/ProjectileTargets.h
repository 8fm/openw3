#pragma once

//////////////////////////////////////////////////////////////////////////

// RED_DECLARE_NAME( PROJECTILE_TARGET );
// RED_DECLARE_NAME( targetPosition );
// RED_DECLARE_NAME( targetNode );
// RED_DECLARE_NAME( targetEntity );
// RED_DECLARE_NAME( targetBoneIndex );

//////////////////////////////////////////////////////////////////////////

class CNode;
class CProjectileTrajectory;
class CProjectile;

//////////////////////////////////////////////////////////////////////////

// enum EProjectileTargetType
// {
//     PTT_Null,
//     PTT_Fixed,
//     PTT_Static,
//     PTT_Node,
//     PTT_Bone
// };

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

    class IProjectileTarget
    {
    public:
//         IProjectileTarget( EProjectileTargetType type )
//             : m_type( type )
//         {}

        virtual ~IProjectileTarget() {}

        virtual Vector  GetWorldPosition() const = 0;
//         virtual void    OnSaveGameplayState( IGameSaver* saver ) const = 0;
//         virtual void    OnLoadGameplayState( IGameLoader* loader ) = 0;

//         EProjectileTargetType GetType() const { return m_type; }
// 
//     private:
//         EProjectileTargetType m_type;
    };

    //////////////////////////////////////////////////////////////////////////

    class CFixedTarget : public IProjectileTarget
    {
    private:
        Vector m_target;

    public:
        CFixedTarget( const Vector& target );
        virtual Vector  GetWorldPosition() const;
//         virtual void    OnSaveGameplayState( IGameSaver* saver ) const;
//         virtual void    OnLoadGameplayState( IGameLoader* loader );
    };

    //////////////////////////////////////////////////////////////////////////

    class CStaticTarget : public IProjectileTarget
    {
    private:
        Vector m_target;

    public:
        CStaticTarget( CNode* self, Float range );
        virtual Vector  GetWorldPosition() const;
//         virtual void    OnSaveGameplayState( IGameSaver* saver ) const;
//         virtual void    OnLoadGameplayState( IGameLoader* loader );
    };

    //////////////////////////////////////////////////////////////////////////

    class CNodeTarget : public IProjectileTarget
    {
    private:
        THandle<CNode> m_target;

    public:
        CNodeTarget( CNode*	target );
        virtual Vector  GetWorldPosition() const;
//         virtual void    OnSaveGameplayState( IGameSaver* saver ) const;
//         virtual void    OnLoadGameplayState( IGameLoader* loader );
    };

    //////////////////////////////////////////////////////////////////////////

    class CBoneTarget : public IProjectileTarget
    {
    private:
        THandle< CEntity >  m_entity;
        Int32				m_boneIdx;

    public:
        CBoneTarget( const THandle< CEntity >& entity, Int32 boneIdx );
        virtual Vector  GetWorldPosition() const;
//         virtual void    OnSaveGameplayState( IGameSaver* saver ) const;
//         virtual void    OnLoadGameplayState( IGameLoader* loader );
    };

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
