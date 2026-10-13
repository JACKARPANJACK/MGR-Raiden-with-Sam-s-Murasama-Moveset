#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <array>

namespace WallTraversalPolicy {
    constexpr uint32_t Action = 0x10007Bu;
    struct Vec { float x=0,y=0,z=0; };
    inline Vec operator+(Vec a,Vec b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
    inline Vec operator-(Vec a,Vec b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
    inline Vec operator*(Vec a,float s) {return {a.x*s,a.y*s,a.z*s};}
    inline float Dot(Vec a,Vec b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
    inline float Length(Vec a) {return std::sqrt(Dot(a,a));}
    inline bool Finite(Vec a) {return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
    inline Vec Unit(Vec a) {float l=Length(a);return Finite(a)&&l>.0001f?a*(1/l):Vec{};}
    struct Contact {Vec point,normal;float distance=0;bool valid=false;uintptr_t surface=0;unsigned queryLayer=6;float minimumWallHeight=6.0f;};
    inline Contact RayContact(Vec start,Vec end,Vec normal,float fraction,unsigned layer,uintptr_t surface=0) {
        Contact hit{};
        const float length=Length(normal);
        if(!Finite(start)||!Finite(end)||!Finite(normal)||!std::isfinite(fraction)||fraction<0||fraction>1||length<.8f||length>1.2f)return hit;
        hit.point=start+(end-start)*fraction;hit.normal=Unit(normal);
        if(Dot(start-hit.point,hit.normal)<0)hit.normal=hit.normal*-1;
        hit.distance=Length(hit.point-start);hit.valid=true;hit.surface=surface;hit.queryLayer=layer;
        return hit;
    }
    // checkRay masks its query to five bits. Layer 31 is explicitly disabled
    // by GroupFilterImplement; every other layer is a possible query row.
    inline std::array<unsigned,31> QueryLayers(unsigned preferred) {
        std::array<unsigned,31> result{};
        if(preferred>=31)preferred=6;
        result[0]=preferred;unsigned count=1;
        if(preferred!=6)result[count++]=6;
        for(unsigned layer=0;layer<31;++layer)
            if(layer!=preferred&&layer!=6)result[count++]=layer;
        return result;
    }
    enum class SweepContacts {Clear,Inspect,Invalid};
    inline SweepContacts ClassifySweepContacts(int count,bool hasData) {
        // Native query success does not imply that the collector contains a hit.
        if(count==0)return SweepContacts::Clear;
        return count>0&&count<=128&&hasData?SweepContacts::Inspect:SweepContacts::Invalid;
    }
    // Native background objects include ordinary buildings/props across BM/BH/BA/ET categories.
    inline bool Structure(uint32_t id) {
        switch(id) {
        case 0xD0301:case 0xD030E:case 0xD030F:case 0xF00D8: // Armored cars and scripted car.
        case 0xF00A0:case 0xF00A1:case 0xF00A2:case 0xF00A3:case 0xF00D1:case 0xF00D3: // Containers and parts.
            return false;
        default:break;
        }
        const uint32_t cat=id&0xFFFF0000u;
        if(cat==0xD0000u||cat==0xE0000u||cat==0xF0000u)return true;
        return id==0u;
    }
    inline bool NameContains(const char* name,size_t length,const char* token) {
        if(!name)return false;
        size_t n=0;while(token[n])++n;
        for(size_t i=0;i+n<=length&&name[i];++i) {
            size_t j=0;
            for(;j<n;++j) {char c=name[i+j];if(c>='A'&&c<='Z')c=char(c-'A'+'a');if(c!=token[j])break;}
            if(j==n)return true;
        }
        return false;
    }
    inline bool ObjectName(const char* name,size_t length) {
        for(const char* token:{"car","vehicle","truck","bus","container","crate","box","barrel","drum","tree","pole","sign","debris"})
            if(NameContains(name,length,token))return true;
        return false;
    }
    inline bool Vertical(Vec n);
    inline bool LargeFacade(const std::array<Contact,4>& edges,Vec expected,float minimumHeight=6.0f) {
        Vec n=Unit({expected.x,0,expected.z});
        if(Length(n)<.5f)return false;
        for(const auto& edge:edges) {
            if(!edge.valid||!Vertical(edge.normal)||Dot(Unit(edge.normal),n)<.92f)return false;
            if(std::fabs(Dot(edge.point-edges[0].point,n))>.25f)return false;
        }
        Vec side{n.z,0,-n.x};
        return std::fabs(Dot(edges[1].point-edges[0].point,side))>=2.3f&&
            std::fabs(edges[3].point.y-edges[2].point.y)>=minimumHeight-.02f;
    }
    inline bool Vertical(Vec n) {
        float length=Length(n);
        return Finite(n)&&length>.8f&&length<1.2f&&std::fabs(n.y/length)<.35f;
    }
    inline bool SamePlane(const Contact& a,const Contact& b) {
        return a.valid&&b.valid&&Vertical(a.normal)&&Vertical(b.normal)&&
            Dot(Unit(a.normal),Unit(b.normal))>.97f&&std::fabs(Dot(b.point-a.point,Unit(a.normal)))<.12f;
    }
    inline bool Support(const std::array<Contact,4>& samples,Vec expected,Contact& out) {
        out={};
        const bool hasExpected=Length(expected)>.1f;
        const Vec expUnit=hasExpected?Unit(expected):Vec{};
        bool found=false;
        for(const auto& sample:samples) {
            if(!sample.valid||!Vertical(sample.normal))continue;
            if(hasExpected&&Dot(Unit(sample.normal),expUnit)<=.75f)continue;
            if(!found||sample.distance<out.distance) {
                out=sample;found=true;
            }
        }
        return found;
    }
    template<size_t N>
    inline bool AttachedSupport(const std::array<Contact,N>& samples,Vec feet,Vec expected,float clearance,Contact& out) {
        out={};
        for(size_t i=0;i<samples.size();++i) {
            const auto& a=samples[i];
            if(!a.valid||!Vertical(a.normal)||Dot(Unit(a.normal),Unit(expected))<.88f)continue;
            const Vec n=Unit({a.normal.x,0,a.normal.z});
            const float gap=Dot(feet-a.point,n);
            if(!std::isfinite(gap)||std::fabs(gap-clearance)>.14f)continue;
            for(size_t j=i+1;j<samples.size();++j) {
                const auto& b=samples[j];
                if(!b.valid||!Vertical(b.normal)||Dot(Unit(a.normal),Unit(b.normal))<.88f)continue;
                const float otherGap=Dot(feet-b.point,n);
                if(std::fabs(otherGap-clearance)>.14f||std::fabs(otherGap-gap)>.16f)continue;
                if(!out.valid||a.distance<out.distance)out=a;
            }
        }
        return out.valid;
    }
    // Entry wall check accepts buildings with recessed windows, sills, baseboards, or trims,
    // while rejecting ankle-only curbs where upper body rays all miss.
    template<size_t N>
    inline bool EntryWallSupport(const std::array<Contact,N>& samples,Contact& out) {
        out={};
        // Require at least one upper-body hit (waist/chest/head: indices 0, 1, 2).
        int primaryIdx=-1;
        for(int i:{1,0,2}) {
            if(samples[i].valid&&Vertical(samples[i].normal)) {primaryIdx=i;break;}
        }
        if(primaryIdx<0)for(size_t i=4;i<N;++i)
            if(samples[i].valid&&Vertical(samples[i].normal)){primaryIdx=int(i);break;}
        if(primaryIdx<0)return false;
        const Vec refNormal=Unit({samples[primaryIdx].normal.x,0,samples[primaryIdx].normal.z});
        if(Length(refNormal)<.5f)return false;
        int matching=0;
        bool found=false;
        for(size_t i=0;i<N;++i) {
            if(!samples[i].valid||!Vertical(samples[i].normal))continue;
            Vec flatN=Unit({samples[i].normal.x,0,samples[i].normal.z});
            if(Dot(flatN,refNormal)<.80f)continue;
            ++matching;
            if(!found||samples[i].distance<out.distance) {
                out=samples[i];out.normal=flatN;found=true;
            }
        }
        return found&&(matching>=2||samples[1].valid||samples[2].valid);
    }
    inline bool SupportingSweepContact(Vec point,Vec contactNormal,Vec planePoint,Vec planeNormal,Vec from,Vec to,float radius) {
        planeNormal=Unit(planeNormal);
        return Finite(point)&&Vertical(contactNormal)&&
            std::fabs(Dot(Unit(contactNormal),planeNormal))>.80f&&
            std::fabs(Dot(point-planePoint,planeNormal))<.38f&&
            (Dot(to-from,planeNormal)>=-.05f||Dot(to-planePoint,planeNormal)>=radius+.08f)&&
            Dot(to-planePoint,planeNormal)>=radius+.02f;
    }
    inline Vec Tangent(Vec normal,Vec forward) {
        normal=Unit({normal.x,0,normal.z});
        Vec t{normal.z,0,-normal.x};return Dot(t,forward)<0?t*-1:t;
    }
    inline Vec WallRight(Vec normal) {
        // Right while facing into the wall, independent of camera orbit.
        return Unit({-normal.z,0,normal.x});
    }
    inline bool Ascend(Vec normal,Vec forward) {
        normal=Unit({normal.x,0,normal.z});forward=Unit({forward.x,0,forward.z});
        return Dot(normal,forward)<-.65f;
    }
    inline bool HasEntryIntent(uint32_t action,Vec normal,Vec forward,bool explicitRun,bool cling,bool closeParallel=false) {
        if(explicitRun||cling)return true;
        if(action==0u||action==0xDu||action==0xEu||action==0x100000u||action==0x100004u)return false;
        normal=Unit({normal.x,0,normal.z});forward=Unit({forward.x,0,forward.z});
        return Length(normal)>.5f&&Length(forward)>.5f&&
            (Dot(normal,forward)<=-.18f||(closeParallel&&Dot(normal,forward)<=.10f));
    }
    inline Vec Travel(Vec normal,Vec forward) {return Ascend(normal,forward)?Vec{0,1,0}:Tangent(normal,forward);}
    // Preserve which shoulder faces the wall, including a 90-degree corner where
    // projecting the old direction onto the new plane would produce zero.
    inline Vec TransportTravel(Vec oldNormal,Vec newNormal,Vec travel) {
        Vec flat=Unit({travel.x,0,travel.z});
        if(Length(flat)<.1f)return travel;
        Vec oldSide=Unit({oldNormal.z,0,-oldNormal.x});
        Vec newSide=Unit({newNormal.z,0,-newNormal.x});
        if(Dot(oldSide,flat)<0)newSide=newSide*-1;
        const float horizontal=Length({travel.x,0,travel.z});
        return Unit(newSide*horizontal+Vec{0,travel.y,0});
    }
    inline bool EntryDistance(float distance,float radius,float clearance) {
        return std::isfinite(distance)&&distance>=radius-.22f&&distance<=clearance+.68f;
    }
    inline bool Top(const Contact& floor,Vec feet) {
        return floor.valid&&Finite(floor.normal)&&Unit(floor.normal).y>.75f&&
            std::fabs(floor.point.y-feet.y)<.25f;
    }
    inline float Correction(const Contact& wall,Vec body,float clearance) {
        return (std::clamp)(clearance-Dot(body-wall.point,Unit(wall.normal)),-.08f,.08f);
    }
    inline bool EntryAction(uint32_t action) {
        // Native parkour/wall-pop/obstacle actions must finish in the game.
        if(action==0x47u||action==0x51u)return true;
        switch(action) {
        case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 8:case 9:case 0xA:case 0xB:case 0xC:case 0xD:case 0xE:case 0xF:
        case 0x100000:case 0x100001:case 0x100002:case 0x100003:case 0x100004:
        case 0x100005:case 0x100006:case 0x100008:case 0x100009:case 0x10000A:
        case 0x10000B:case 0x10000C:case 0x10000D:
        case 0x100014:case 0x100015:case 0x100016:case 0x100017:
        case 0x100055:case 0x100056:
            return true;
        default:return false;
        }
    }
    enum class Mode {Off,Run,Slide,Cling,Launch};
    enum class Clip {None,RunUp,RunSideLeft,RunSideRight,SlideDown,SlideSideLeft,SlideSideRight,Cling,Launch,SlideUp,SlideUpLeft,SlideUpRight};
    inline bool WallOnLeft(Vec normal,Vec facing) {
        facing=Unit({facing.x,0,facing.z});normal=Unit({normal.x,0,normal.z});
        return Dot(Vec{facing.z,0,-facing.x},normal)<0;
    }
    inline Clip SelectClip(Mode mode,bool ascending,Vec normal,Vec tangent) {
        if(mode==Mode::Off)return Clip::None;
        if(mode==Mode::Cling)return Clip::Cling;
        if(mode==Mode::Launch)return Clip::Launch;
        Vec flat{tangent.x,0,tangent.z};
        if(mode==Mode::Run) {
            if(ascending)return Clip::RunUp;
            return WallOnLeft(normal,flat)?Clip::RunSideLeft:Clip::RunSideRight;
        }
        if(tangent.y>.15f) {
            if(Length(flat)<.20f)return Clip::SlideUp;
            return WallOnLeft(normal,flat)?Clip::SlideUpLeft:Clip::SlideUpRight;
        }
        if(Length(flat)<.20f)return Clip::SlideDown;
        return WallOnLeft(normal,flat)?Clip::SlideSideLeft:Clip::SlideSideRight;
    }
    inline Vec SlideTravel(bool ascending,Vec tangent) {
        Vec flat=Unit({tangent.x,0,tangent.z});
        if(ascending||Length(flat)<.20f)return {0,-1,0};
        return Unit(flat*.72f+Vec{0,-.69f,0});
    }
    struct WallSteer {
        Mode mode=Mode::Run;
        Vec tangent{0,1,0};
        bool ascending=true;
    };
    inline WallSteer SteerOnWall(
        Mode currentMode,
        Vec currentTangent,
        bool currentAscending,
        Vec normal,
        Vec camForward,
        float stickX,
        float stickY,
        bool clingInput,
        unsigned ticks)
    {
        const bool hasX=std::fabs(stickX)>=.25f;
        const bool hasY=std::fabs(stickY)>=.25f;
        Vec camF=Unit({camForward.x,0,camForward.z});
        if(Length(camF)<.1f)camF=Unit({-normal.x,0,-normal.z});
        Vec wallRight=WallRight(normal);
        if(clingInput)return {Mode::Cling,{0,0,0},false};
        if(!hasX&&!hasY) {
            if(currentMode==Mode::Slide)return {Mode::Slide,currentTangent,currentAscending};
            if(currentMode==Mode::Cling||ticks>=15u)return {Mode::Cling,{0,0,0},false};
            return {currentMode,currentTangent,currentAscending};
        }
        if(stickY<=-.25f) {
            if(hasX) {
                Vec side=wallRight*(stickX>0.0f?1.0f:-1.0f);
                return {Mode::Slide,SlideTravel(false,side),false};
            }
            return {Mode::Slide,{0,-1,0},false};
        }
        if(currentMode==Mode::Slide&&stickY>=.25f) {
            if(hasX)return {Mode::Slide,Unit(wallRight*(stickX>0?.72f:-.72f)+Vec{0,.69f,0}),false};
            return {Mode::Slide,{0,1,0},true};
        }
        if(hasX) {
            Vec side=wallRight*(stickX>0.0f?1.0f:-1.0f);
            if(currentMode==Mode::Slide&&stickY<.25f)return {Mode::Slide,SlideTravel(false,side),false};
            if(stickY>=.25f)return {Mode::Run,Unit(side*.88f+Vec{0,.47f,0}),false};
            return {Mode::Run,side,false};
        }
        if(!currentAscending&&currentMode==Mode::Run&&ticks<12u&&std::fabs(Dot(camF,Unit(normal)))<.65f)
            return {Mode::Run,currentTangent,false};
        return {Mode::Run,{0,1,0},true};
    }
    inline Vec WallLaunchVelocity(Vec normal,Vec camForward,Vec currentTangent,float stickX,float stickY) {
        normal=Unit({normal.x,0,normal.z});
        Vec camF=Unit({camForward.x,0,camForward.z});
        if(Length(camF)<.1f)camF=normal;
        Vec camRight=WallRight(normal);
        Vec wish=camF*stickY+camRight*stickX;
        Vec horiz{};
        if(Length(wish)>=.25f) {
            wish=Unit(wish);
            float into=Dot(wish,normal);
            if(into<.35f)wish=Unit(wish+normal*(.45f-into));
            horiz=wish*.38f;
        } else {
            Vec flatTan=Unit({currentTangent.x,0,currentTangent.z});
            Vec away=Dot(camF,normal)>.15f?Unit(normal*.65f+camF*.35f):normal;
            horiz=Unit(away*.80f+flatTan*.35f)*.36f;
        }
        float up=stickY>.25f?.42f:(stickY<-.25f?.24f:.36f);
        return {horiz.x,up,horiz.z};
    }
    inline Vec ObstacleHopStep(Vec normal,Vec travel,unsigned tick) {
        if(tick>=12)return {};
        Vec flat=Unit({travel.x,0,travel.z});
        const float away=tick<3?.16f:(tick>=9?-.16f:0.0f);
        const float up=tick<3?.04f:.32f-.018f*float(tick-3);
        return Unit({normal.x,0,normal.z})*away+flat*(tick<3?0.0f:.16f)+Vec{0,up,0};
    }
    inline Vec HomingLaunchStep(Vec from,Vec target,Vec wallNormal,unsigned tick,float speed=.65f) {
        Vec toTarget=target-from;
        float dist=Length(toTarget);
        if(dist<.001f)return {};
        Vec dir=toTarget*(1.0f/dist);
        if(tick<5u) {
            float kickWeight=float(5u-tick)*.12f;
            Vec kick=Unit({wallNormal.x,.35f,wallNormal.z});
            dir=Unit(dir+kick*kickWeight);
        }
        return dir*(std::min)(speed,dist);
    }
    struct State {
        Mode mode=Mode::Off;
        unsigned ticks=0,cooldown=0;
        bool requireRelease=false;
        bool Active() const {return mode!=Mode::Off;}
        bool Begin(Mode next) {if(Active()||cooldown||requireRelease||next==Mode::Off)return false;mode=next;ticks=0;return true;}
        void End(bool release=true,unsigned delay=24) {mode=Mode::Off;ticks=0;cooldown=delay;requireRelease=release;}
        void TickIdle(bool run,bool cling) {if(cooldown)--cooldown;if(!run&&!cling)requireRelease=false;}
        bool Advance(unsigned runLimit,unsigned clingLimit,unsigned slideLimit=90,unsigned launchLimit=24) {
            if(!Active())return false;
            ++ticks;
            unsigned limit=mode==Mode::Run?runLimit:(mode==Mode::Slide?slideLimit:(mode==Mode::Launch?launchLimit:clingLimit));
            return limit!=0&&ticks>=limit;
        }
        void Switch(Mode next) {if(Active()&&next!=Mode::Off){mode=next;ticks=0;}}
    };
}
