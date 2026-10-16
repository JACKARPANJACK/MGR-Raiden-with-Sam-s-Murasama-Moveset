#pragma once
#include "GunChargePolicy.h"
#include "PistolHandling.generated.h"
#include <algorithm>
#include <cstring>

namespace PistolPolicy {
enum Kind { M9, Socom, Count };
struct Profile {
    const char* name;
    unsigned object,alias,item,slot,magazine,interval;
    int damage;
    float recoil,kick;
};
inline constexpr Profile Profiles[]={
    {"M9 Tranq",0x32060,0x4D394851,49,13,15,18,0,.20f,.08f},
    {"SOCOM Mk23",0x32061,0x534F4333,50,14,12,15,12,.30f,.24f}
};
constexpr bool Object(unsigned id) {return id>=Profiles[0].object && id<=Profiles[1].object;}
constexpr bool Slot(int slot) {return slot>=13 && slot<=14;}
constexpr const Profile& Get(unsigned kind) {return Profiles[kind<Count?kind:0];}
constexpr unsigned ElectricTicks=30,FullTicks=90,ReloadTicks=90;
constexpr int ElectricDamage=18,FullDamage=30;
constexpr bool Round(int damage) {return damage==0 || damage==12 || damage==18 || damage==30;}
constexpr float Kick(int damage) {return damage==0?.08f:damage==12?.24f:damage==18?.32f:.42f;}
constexpr unsigned SleepTicks=30*60,DoseTicks=10*60;
constexpr unsigned SleepHits(int maxHp) {return maxHp<=200?1u:maxHp<=500?2u:3u;}
struct Sedation {
    unsigned hits=0,expiry=0;
    bool Hit(int maxHp) {expiry=DoseTicks;hits=(std::min)(3u,hits+1);return hits>=SleepHits(maxHp);}
    void Tick() {if(expiry && !--expiry)hits=0;}
    void Reset() {hits=expiry=0;}
};
inline void ConfigureEmp(void* descriptor) {
    auto* bytes=static_cast<unsigned char*>(descriptor);
    auto word=[bytes](unsigned offset,unsigned value){std::memcpy(bytes+offset,&value,4);};
    word(4,0x31051);word(0x110,0x51);word(0x114,0x11);
    word(0x10,0x58);word(0x14,0);word(0x18,1);word(0x1C,30);
}
// Semi-auto trigger and C/LB release share one pending round. Holding a
// trigger never repeats; firing during a C hold consumes that hold's release.
struct Control {
    GunChargePolicy::Hold hold;
    bool triggerDown=false,consumed=false,pending=false;
    unsigned tier=0;
    void Cancel() {hold.Cancel();triggerDown=true;consumed=pending=false;tier=0;}
    void Tick(bool aim,bool trigger,bool enabled,bool socom,bool canStart) {
        if(!enabled) {Cancel();return;}
        const bool pressed=trigger && !triggerDown;
        triggerDown=trigger;
        const bool released=hold.Tick(aim,true,canStart);
        if(pressed && aim && !hold.requireRelease && canStart && !pending) {
            tier=socom?GunChargePolicy::Tier(hold.frames):0;
            pending=true;consumed=true;
        }
        if(released && !consumed && canStart && !pending) {
            tier=socom?GunChargePolicy::Tier(hold.releasedFrames):0;
            pending=true;
        }
        if(!aim) consumed=false;
    }
    int Damage(unsigned kind) const {
        return kind==Socom && tier ? (tier==2?FullDamage:ElectricDamage) : Get(kind).damage;
    }
    void Fired() {pending=false;tier=0;}
};
}
