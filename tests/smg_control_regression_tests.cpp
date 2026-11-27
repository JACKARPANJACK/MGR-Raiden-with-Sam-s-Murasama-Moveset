#include "../SmgPolicy.h"
#include <cassert>
#include <iostream>

int main()
{
    constexpr unsigned light=0x100, jump=0x200, run=0x800;
    assert(SmgPolicy::GunLockActive(true,false)); // Toggle lock survives release.
    assert(SmgPolicy::GunLockActive(false,true));
    assert(!SmgPolicy::GunLockActive(false,false));
    assert(!SmgPolicy::ShotSpread(false,true)); // Lock beats moving/hip-fire spread.
    assert(!SmgPolicy::ShotSpread(true,true));
    assert(!SmgPolicy::ShotSpread(true,false));
    assert(SmgPolicy::ShotSpread(false,false));
    // A native re-poll overwrites early masks. Filter its fresh result, retaining
    // RMB/MMB and leaving ordinary mouse input intact outside gun aim.
    for(unsigned buttons=0;buttons<8;++buttons) {
        assert(SmgPolicy::FilterGunMouse(buttons,true)==(buttons&6u));
        assert(SmgPolicy::FilterGunMouse(buttons,false)==buttons);
    }
    const auto mouseMask=SmgPolicy::FireReservation(light,jump|run,true);
    assert((mouseMask&light)!=0 && (mouseMask&SmgPolicy::ControllerFire)!=0);
    assert(!(mouseMask&(jump|run)));
    assert(!(SmgPolicy::FireReservation(light,jump|run,false)&light));
    assert(!(SmgPolicy::FireReservation(jump,jump,true)&jump));
    // Feed mouse and RT through the same aim/automatic and semi-auto policies.
    SmgPolicy::AimControl mouse, pad;
    PistolPolicy::Control mousePistol, padPistol;
    for(unsigned frame=0;frame<90;++frame) {
        const bool aim=frame<75;
        const bool lmb=(frame>=5 && frame<35) || (frame>=45 && frame<60);
        const bool rt=SmgPolicy::ControllerFireHeld(0,lmb?1.0f:0.0f);
        assert(mouse.Tick(aim,lmb,true,false)==pad.Tick(aim,rt,true,false));
        assert(mouse.tapped==pad.tapped && mouse.aiming==pad.aiming);
        mousePistol.Tick(aim,lmb,true,true,true);
        padPistol.Tick(aim,rt,true,true,true);
        assert(mousePistol.pending==padPistol.pending && mousePistol.tier==padPistol.tier);
        if(mousePistol.pending) {mousePistol.Fired();padPistol.Fired();}
    }
    std::cout<<"PASS: LMB/RT firing parity and mouse melee reservation preserve movement.\n";
}
