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
    constexpr unsigned LauncherObject(int slot) {return slot==6?0x310E1:0x31002;}
    constexpr unsigned LauncherAlias(int slot) {return slot==6?0x7089ED6C:0x3800CB76;}
    constexpr unsigned Cooldown(unsigned tier) {return tier==2?60:45;}
    constexpr int ChargedDamage(int nativeDamage,unsigned tier)
    {return nativeDamage<=0?0:nativeDamage>100000?100000:nativeDamage*int(tier+1);}
}
