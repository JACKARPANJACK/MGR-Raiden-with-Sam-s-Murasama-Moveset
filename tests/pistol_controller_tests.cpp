#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#include "../PistolComboPolicy.h"
#include "../StealthPoseFile.h"

using DWORD=unsigned long;
constexpr int FALSE=0;
static DWORD mockNow=1;
static DWORD GetTickCount(){return mockNow;}
static void* GetForegroundWindow(){return reinterpret_cast<void*>(1);}
static void GetWindowThreadProcessId(void*,DWORD* id){*id=1;}
static DWORD GetCurrentProcessId(){return 1;}
namespace gui {static bool IsMenuVisible(){return false;}}
namespace EncounterPolicy {static bool Scripted(bool a,bool b,bool c,bool d){return a||b||c||d;}}
namespace PistolPolicy {static bool Slot(unsigned slot){return slot==42;}}
struct Manager {unsigned slot=42;unsigned getSubWeaponEquipped(){return slot;}};
static Manager manager;
static Manager* g_pPlayerManager=&manager;
struct {bool STA_EVENT=false,STA_QTE=false,STA_CODEC=false,STA_SOFT_EVENT=false,STA_PAUSE=false;} g_StaFlags;
namespace Trigger {static struct {bool STP_OBJ=false;} StpFlags;}
namespace GunRuntimeLog {static void Write(const char*,...) {}}
namespace SamArchiveLookup {
enum class Source {None,Playable};
struct Clip {void* motion=nullptr;void* sequence=nullptr;Source source=Source::None;};
}
namespace SamBossSequence {
static size_t Size(const void* data) {
    const auto* p=static_cast<const unsigned char*>(data);
    const auto u16=[](const unsigned char* b){return unsigned(b[0])*256+b[1];};
    return 16+u16(p+8)*8+u16(p+10)*4+(u16(p+12)*65536+u16(p+14));
}
}
namespace PistolAssets {
struct Bank {std::map<std::string,std::vector<unsigned char>> members;};
static Bank animations;
static bool LoadAnimations(){return !animations.members.empty();}
static std::vector<unsigned char> Member(const Bank& bank,const char* name) {
    auto i=bank.members.find(name);return i==bank.members.end()?std::vector<unsigned char>{}:i->second;
}
}
struct Behavior {unsigned m_Rno0=0;};
struct Pl0000:Behavior {
    unsigned flags=0x40000,*m_pSeqFlags=&flags;
    unsigned m_ButtonJump=0,m_ButtonHeavyAttack=0,m_ButtonLightAttack=0,m_ButtonNinjarun=0;
    struct {unsigned m_Trig=0;} m_CurrentInput;
    float remaining=36,rate=1;
    bool advance=true;
    unsigned idles=0,maintenance=0;
    std::vector<std::string> starts;
    bool isAlive(){return true;} int isInAir(){return FALSE;} bool isBladeModeActive(){return false;}
    void setRno(unsigned r,int,int,int){m_Rno0=r;}
    void requestAnimationByName(const char*,int,float,int,unsigned,int,int){++idles;}
    float getRemainingAnimationFrames(int){return remaining;}
    int setDirectAnimation(void* motion,void*,int,float,float,unsigned,float,float speed) {
        const auto* p=static_cast<unsigned char*>(motion);unsigned short frames;
        std::memcpy(&frames,p+10,2);remaining=float(frames-1);rate=speed;
        starts.emplace_back(reinterpret_cast<const char*>(p+31),4);return 0;
    }
};
namespace KunaiSubweapon {
struct Smg {
    unsigned captures=0,ends=0,manual=0,stages=0;
    bool BeginPistolAttack(Pl0000*,const void*){++captures;return true;}
    void EndPistolAttack(){++ends;} void BeginPistolStage(unsigned=4){++stages;}
    bool IsEquipped(){return true;}
    void ResumePistolManual(Pl0000*,bool queue){assert(queue);++manual;}
};
static Smg smg;
static Smg& GetSmg(){return smg;}
}
static void __fastcall Maintain(Pl0000* p,void*,float,float) {
    ++p->maintenance;mockNow+=17;
    if(p->advance)p->remaining=(std::max)(0.0f,p->remaining-p->rate);
}
namespace shared {static std::uintptr_t base=reinterpret_cast<std::uintptr_t>(&Maintain)-0x794790u;}
#define PISTOL_COMBO_CONTROLLER_TEST
#include "../PistolComboController.h"

int main() {
    for(const auto& c:PistolAttackCatalog::Clips)for(const char* suffix:{".mot",".gwm","_0_seq.bxm","_timing.bin"}) {
        const std::string name=std::string("wp2062_")+c.code+suffix;
        std::ifstream file("Release/PistolControllerFixtures/"+name,std::ios::binary);assert(file);
        PistolAssets::animations.members[name]={(std::istreambuf_iterator<char>(file)),{}};
    }
    using namespace PistolComboPolicy;
    auto& controller=PistolComboController::Get();Pl0000 player;
    // A matching action number is not evidence that this controller owns it.
    player.m_Rno0=Action;
    assert(!controller.Active(&player));
    assert(!controller.HandleAction(&player));
    assert(player.m_Rno0==Action && player.idles==0 && player.maintenance==0);
    player.m_Rno0=0;
    const auto press=[&](float horizontal,float vertical) {
        input.Tick(false,0,0,true,controller.Active(&player));
        input.Tick(true,horizontal,vertical,true,controller.Active(&player));
        assert(controller.HandleAction(&player));assert(controller.Active(&player));
    };
    press(0,1); // Load all sixteen real pairs and play the forward route.
    unsigned emitted=0;bool queuedSecond=false,queuedThird=false;
    for(unsigned tick=0;controller.Active(&player) && tick<1000;++tick) {
        if((player.starts.size()==1 && player.remaining<=7 && !queuedSecond) ||
           (player.starts.size()==2 && player.remaining<=15 && !queuedThird)) {
            input.Tick(false,0,0,true,true);input.Tick(true,0,1,true,true);
            if(player.starts.size()==1)queuedSecond=true;else queuedThird=true;
        } else input.Tick(true,0,1,true,true);
        controller.HandleAction(&player);
        emitted+=shots;shots=0;
        if(controller.Active(&player)) {
            float frame=-1;const auto* pose=static_cast<const char*>(controller.VisualPose(&player,frame));
            assert(pose && !std::memcmp(pose,"FBW1",4) && frame>=0);
        }
    }
    assert(emitted==9);
    assert(!controller.Active(&player) && player.m_Rno0==0);
    assert((player.starts==std::vector<std::string>{"2f50","2f53","2f56"}));
    assert(KunaiSubweapon::smg.captures==1 && KunaiSubweapon::smg.stages==3 && player.maintenance>150);
    const auto beforeSingle=player.starts.size();mockNow+=1000;
    press(0,1);assert(player.starts.back()=="2f50"); // idle delay resets the route to its opener
    for(unsigned tick=0;controller.Active(&player) && tick<100;++tick) {controller.HandleAction(&player);shots=0;}
    assert(!controller.Active(&player) && player.starts.size()==beforeSingle+1); // one input never starts a scripted three-move routine
    press(0,-1);
    input.Tick(true,0,-1,true,true,true);
    assert(!controller.HandleAction(&player));
    assert(!controller.Active(&player) && input.Aim() && KunaiSubweapon::smg.manual==1 && shots==0);
    press(-1,0);
    player.m_Rno0=0x100001;const unsigned idleBefore=player.idles;
    assert(!controller.HandleAction(&player));
    assert(player.m_Rno0==0x100001 && player.idles==idleBefore && !controller.Active(&player));
    player.m_Rno0=0;press(1,0);player.advance=false;
    for(unsigned tick=0;controller.Active(&player) && tick<16;++tick)controller.HandleAction(&player);
    assert(!controller.Active(&player) && player.m_Rno0==0 && shots==0);
    player.advance=true;press(0,1);
    player.m_ButtonJump=player.m_CurrentInput.m_Trig=0x10;
    assert(!controller.HandleAction(&player) && !controller.Active(&player) && shots==0);
    player.m_ButtonJump=player.m_CurrentInput.m_Trig=0;
    // Ownership is also released when the selected subweapon changes.
    press(0,1);manager.slot=0;
    assert(!controller.HandleAction(&player) && player.m_Rno0==0);
    controller.Forget(&player);
    std::puts("PASS: actual controller/real 16-clip bank; foreign action preserved; three stages advance and finish; manual fire handoff; native interruption; stalled clock release; weapon-switch cleanup");
}
