#pragma once
#include <vector>

namespace SwordPosePolicy
{
    constexpr bool ShouldPark(int customWeapon,bool lowPose,bool gunPose)
    {return customWeapon==0 && (lowPose || gunPose);}

    // Render-only placement: no constraint, sword-state or native draw calls.
    // The native mount remains authoritative for the following game update.
    template<class Handle,class Part,class Matrix>
    class VisualPark
    {
        Handle sword{};
        Part* root=nullptr;
        struct Saved {Part* part;Matrix matrix;unsigned short flags;};
        std::vector<Saved> saved;
    public:
        template<class Entity>
        bool Apply(Entity* entity,Part* part,const Matrix& target)
        {
            if(root || !entity || !part)return false;
            sword=entity;root=part;
            saved.push_back({part,part->m_LocalMatrix,part->m_PartsFlag.m_Flag});
            part->m_LocalMatrix=target;part->m_PartsFlag.m_Flag|=4;
            return true;
        }
        template<class Entity,class Rebase>
        bool ApplyAll(Entity* entity,Part* parts,unsigned count,const Matrix& oldRoot,const Matrix& target,Rebase rebase)
        {
            return ApplyParts(entity,count,[parts](unsigned i){return parts?parts+i:nullptr;},oldRoot,target,rebase);
        }
        template<class Entity,class ResolvePart,class Rebase>
        bool ApplyParts(Entity* entity,unsigned count,ResolvePart partAt,const Matrix& oldRoot,const Matrix& target,Rebase rebase)
        {
            if(root || !entity || !count || count>128)return false;
            for(unsigned i=0;i<count;++i)if(!partAt(i))return false;
            const Matrix originalRoot=oldRoot,placement=target;
            sword=entity;root=partAt(0);saved.reserve(count);
            for(unsigned i=0;i<count;++i) {
                auto* part=partAt(i);
                saved.push_back({part,part->m_LocalMatrix,part->m_PartsFlag.m_Flag});
            }
            for(auto& s:saved) {
                s.part->m_LocalMatrix=rebase(s.matrix,originalRoot,placement);
                s.part->m_PartsFlag.m_Flag|=4;
            }
            return true;
        }
        template<class Resolve>
        void Restore(Resolve resolve)
        {
            auto* entity=sword.getEntity();
            if(root && entity && resolve(entity)==root)
                for(auto& s:saved)
                {s.part->m_LocalMatrix=s.matrix;s.part->m_PartsFlag.m_Flag=s.flags;}
            root=nullptr;sword={};saved.clear();
        }
    };

}
