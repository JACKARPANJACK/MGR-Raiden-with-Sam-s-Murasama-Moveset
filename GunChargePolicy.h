#pragma once
namespace GunChargePolicy
{
    constexpr unsigned First=30,Full=90;
    constexpr unsigned Tier(unsigned frames) {return frames>=Full?2:frames>=First?1:0;}
    struct Hold
    {
        unsigned frames=0,releasedFrames=0;
        bool down=false,requireRelease=false,accepting=false;
        void Cancel() {frames=0;releasedFrames=0;down=accepting=false;requireRelease=true;}
        bool Tick(bool held,bool enabled,bool canStart=true)
        {
            if(!enabled) {Cancel();return false;}
            if(!held)
            {
                const bool release=down && accepting && !requireRelease;
                releasedFrames=release?frames:0;
                frames=0;down=accepting=false;requireRelease=false;
                return release;
            }
            if(requireRelease) return false;
            if(!down) {down=true;accepting=canStart;frames=0;}
            if(accepting && frames<Full) ++frames;
            return false;
        }
    };
    constexpr bool LauncherSlot(int slot) {return slot==5 || slot==6;}
    constexpr bool GrenadeSlot(int slot) {return slot>=1 && slot<=4;}
    constexpr bool AerialGrenade(int slot,bool airborne) {return GrenadeSlot(slot) && airborne;}
    constexpr bool AerialSlot(int slot,bool airborne)
    {return airborne && (LauncherSlot(slot) || GrenadeSlot(slot));}
    constexpr const char* AirMotion(int slot) {return LauncherSlot(slot)?"2570":"2566";}
    constexpr unsigned AirReleaseTicks(int slot) {return LauncherSlot(slot)?17:12;}
    constexpr unsigned GrenadeObject(int slot)
    {return slot==2?0x31031:slot==3?0x31041:slot==4?0x31051:0x31011;}
    constexpr unsigned GrenadeAlias(int slot)
    {return slot==2?0x20CEB102:slot==3?0x126FFCBB:slot==4?0x70DCE4D0:0x4B20F7AE;}
    constexpr unsigned LauncherObject(int slot) {return slot==6?0x310E1:0x31002;}
    constexpr unsigned LauncherAlias(int slot) {return slot==6?0x7089ED6C:0x3800CB76;}
    constexpr unsigned Cooldown(unsigned tier) {return tier==2?60:45;}
    constexpr int ChargedDamage(int nativeDamage,unsigned tier)
    {return nativeDamage<=0?0:nativeDamage>100000?100000:nativeDamage*int(tier+1);}
}
