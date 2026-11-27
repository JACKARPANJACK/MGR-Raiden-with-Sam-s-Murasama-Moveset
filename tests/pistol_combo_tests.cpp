#include "../PistolComboPolicy.h"
#include "../SmgPolicy.h"
#include "../PistolPolicy.h"
#include <cassert>
#include <limits>
#include <cstdio>
int main() {
    using namespace PistolComboPolicy;
    assert(Resolve(0,1)==Forward && Resolve(0,-1)==Back);
    assert(Resolve(-1,0)==Left && Resolve(1,0)==Right);
    assert(Resolve(.2f,.2f)==None && Resolve(1,1)==None);
    assert(Resolve(std::numeric_limits<float>::quiet_NaN(),1)==None);
    assert(Length(Forward)==1 && Length(Back)==1);
    std::array<bool,PistolAttackCatalog::Count> reached{};
    for(Direction d:{Forward,Back,Left,Right})for(unsigned step=0;step<24;++step) {
        const auto clip=Clip(d,step);assert(clip<PistolAttackCatalog::Count);reached[clip]=true;
    }
    for(bool playable:reached)assert(playable); // every Humanoid attack variant is reachable
    assert(Clip(Forward,0)==0 && Clip(Forward,1)==3 && Clip(Forward,2)==6);
    assert(Clip(Back,0)==1 && Clip(Back,1)==2);
    Input detached;
    detached.Tick(false,0,0,true,false,false,false);
    for(auto axis:std::array<std::array<float,2>,5>{{{0,1},{0,-1},{-1,0},{1,0},{1,1}}}) {
        detached.Tick(true,axis[0],axis[1],true,false,false,false);
        assert(detached.Aim() && detached.Take()==None && !detached.gunFu && !detached.combo);
        detached.Tick(true,axis[0],axis[1],true,false,true,false);
        assert(detached.fire && detached.Aim() && detached.Take()==None);
        detached.Tick(false,0,0,true,false,false,false);
    }
    Input controls;
    controls.Tick(false,0,0,true,false);
    controls.Tick(true,0,1,true,false);
    assert(controls.Take()==Forward && !controls.Aim()); // direct W + C, no aim frame
    assert(controls.Take()==None); // native dispatch cannot consume the command twice
    controls.Tick(true,0,0,true,false);
    assert(controls.combo && !controls.Aim() && controls.Take()==None); // centering is not aim
    controls.Cancel();
    controls.Tick(true,0,0,true,false);
    assert(!controls.Aim() && controls.Take()==None); // held C after combo completion
    controls.Tick(false,0,0,true,false);
    assert(controls.gunFu && !controls.Aim()); // release cannot leave a gun-ready pose
    controls.Tick(true,0,0,true,false);
    assert(controls.Aim() && !controls.gunFu); // neutral C retains normal aim
    controls.Tick(true,1,0,true,false);
    assert(controls.Take()==Right && !controls.Aim()); // direction added during aim hands off
    controls.Tick(false,0,0,true,false);
    controls.Tick(true,1,1,true,false);
    assert(controls.Take()==Forward && !controls.Aim()); // W+D is gun-fu too
    controls.Tick(false,0,0,true,false);
    controls.Tick(true,-1,-1,true,false);
    assert(controls.Take()==Back && !controls.Aim());
    controls.Tick(false,0,0,true,true);
    controls.Tick(true,0,0,true,true);
    assert(!controls.Aim() && controls.Take()==None); // C re-press during an active route
    controls.Tick(true,0,0,false,false);
    controls.Tick(true,0,1,true,false);
    assert(!controls.Aim() && controls.Take()==None); // interruption requires release
    controls.Tick(false,0,0,true,false);
    controls.Tick(true,-1,0,true,false);
    assert(controls.Take()==Left && !controls.Aim());
    controls.Tick(true,0,1,true,true);
    assert(controls.Take()==Forward); // new direction during an active attack buffers once
    controls.Tick(true,0,1,true,true);assert(controls.Take()==None);
    controls.Tick(false,0,0,true,true);controls.Tick(true,0,1,true,true);assert(controls.Take()==Forward);
    controls.Tick(true,0,1,true,true,true);
    assert(controls.fire && controls.Aim() && controls.Take()==None); // regular trigger beats gun-fu
    controls.Cancel();controls.ManualHandoff();assert(controls.Aim());
    controls.Tick(true,-1,0,true,false,false);assert(controls.Aim() && controls.Take()==None);
    controls.Tick(true,-1,0,false,false,true);controls.Tick(true,-1,0,true,false,true);assert(!controls.Aim());
    for(unsigned kind=0;kind<PistolPolicy::Count;++kind)for(float horizontal:{-1.0f,0.0f,1.0f}) {
        Input manual;PistolPolicy::Control pistol;
        manual.Tick(false,0,0,true,false);pistol.Tick(false,false,true,kind==PistolPolicy::Socom,true);
        manual.Tick(true,horizontal,1,true,false,true);pistol.Tick(manual.Aim(),true,true,kind==PistolPolicy::Socom,true);
        assert(manual.Take()==None && manual.Aim() && pistol.pending);
        assert(pistol.Damage(kind)==PistolPolicy::Get(kind).damage);pistol.Fired();
        for(unsigned tick=0;tick<10;++tick) {
            manual.Tick(true,-horizontal,-1,true,false,true);pistol.Tick(manual.Aim(),true,true,kind==PistolPolicy::Socom,true);
            assert(!pistol.pending && manual.Take()==None); // holding LMB/RT remains semi-auto, even while moving
        }
        manual.Tick(true,horizontal,1,true,false,false);pistol.Tick(manual.Aim(),false,true,kind==PistolPolicy::Socom,true);
        assert(manual.Aim() && !pistol.pending);
        manual.Tick(true,horizontal,1,true,false,true);pistol.Tick(manual.Aim(),true,true,kind==PistolPolicy::Socom,true);
        assert(pistol.pending);pistol.Fired();
        manual.Tick(false,0,0,true,false);pistol.Tick(false,false,true,kind==PistolPolicy::Socom,true);
        assert(!pistol.pending); // releasing C/LB cannot add a phantom round
    }
    Input chargeInput;PistolPolicy::Control charged;
    chargeInput.Tick(false,0,0,true,false);
    for(unsigned tick=0;tick<90;++tick){chargeInput.Tick(true,0,0,true,false);charged.Tick(chargeInput.Aim(),false,true,true,true);}
    chargeInput.Tick(true,1,0,true,false,true);charged.Tick(chargeInput.Aim(),true,true,true,true);
    assert(charged.pending && charged.Damage(PistolPolicy::Socom)==PistolPolicy::FullDamage);
    Chain chain;Chain::Step buffered;
    assert(chain.Push(Left,0) && chain.Push(Left,5));
    assert(chain.count==1 && chain.Pop(buffered,10) && buffered.clip==4); // repeated taps refresh, never skip the next move
    assert(chain.Push(Left,10) && chain.Push(Right,12));
    assert(chain.Pop(buffered,20) && buffered.direction==Right && buffered.clip==6); // latest direction wins
    assert(chain.Push(Back,30) && !chain.Pop(buffered,55)); // stale inputs cannot trigger a delayed attack
    assert(chain.Push(Forward,60) && chain.Pop(buffered,84)); // exact buffer boundary accepted
    assert(!chain.Push(None) && !chain.Pop(buffered));
    chain.Reset();chain.Push(Back);chain.Clear();
    assert(chain.Push(Back) && chain.Pop(buffered) && buffered.clip==2);
    constexpr unsigned cues[]={2,18,34};
    Playback run;
    unsigned emitted=0;
    for(unsigned f=0;f<=36;++f) {
        const auto result=run.Tick(float(f),cues,3,36,37);
        emitted+=run.due;
        assert(result==(f==2||f==18||f==34?Playback::Fire:f==36?Playback::Finished:Playback::Playing));
    }
    assert(emitted==3);
    run.Reset();
    assert(run.Tick(0,cues,3,36,37)==Playback::Playing);
    for(unsigned f=0;f<7;++f)assert(run.Tick(0,cues,3,36,37)==Playback::Playing);
    assert(run.Tick(0,cues,3,36,37)==Playback::Failed);
    run.Reset();
    for(unsigned f=0;f<300;++f)assert(run.Tick(0,cues,3,36,37,true)==Playback::Playing);
    assert(run.ticks==0 && run.stalled==0 && run.due==0);
    assert(run.Tick(std::numeric_limits<float>::quiet_NaN(),cues,3,36,37)==Playback::Failed);
    run.Reset();
    assert(run.Tick(19,cues,3,36,37)==Playback::Fire && run.due==2);
    assert(run.Tick(20,cues,3,36,37)==Playback::Playing && run.due==0); // no repeated cues
    run.Reset(); // cancellation discards the previous route's outstanding shots
    assert(run.Tick(0,cues,3,36,37)==Playback::Playing && run.due==0);
    assert(run.Tick(60,cues,3,36,37)==Playback::Finished && run.due==0); // no shots after recovery
    run.Reset();
    assert(run.Tick(32,cues,3,36,37)==Playback::Fire && run.due==2);
    assert(run.Tick(36,cues,3,36,37)==Playback::Fire && run.due==1); // low FPS cannot swallow the final volley
    assert(run.Tick(36,cues,3,36,37)==Playback::Finished && run.due==0);
    Point end;
    assert(BarrelTarget({10,20,30},{0,0,1},end));
    assert(end.x==10 && end.y==20 && end.z==130);
    assert(BarrelTarget({10,20,30},{2,0,0},end)); // translated and turned player
    assert(end.x==110 && end.y==20 && end.z==30);
    assert(BarrelTarget({0,10,0},{0,-1,0},end) && end.y==-90); // downward attack
    assert(!BarrelTarget({0,0,0},{0,0,0},end));
    for(unsigned kind=0;kind<PistolPolicy::Count;++kind)for(Direction route:{Forward,Back,Left,Right}) {
        SmgPolicy::FireControl magazine;
        const auto& profile=PistolPolicy::Get(kind);
        magazine.rounds=magazine.capacity=profile.magazine;magazine.interval=profile.interval;
        unsigned rounds=0;
        for(unsigned step=0;step<Length(route);++step) {
            Playback stage;
            for(unsigned f=0;f<=36;++f) {
                magazine.Tick(false,true,SmgPolicy::Burst);
                if(stage.Tick(float(f),cues,3,36,37)==Playback::Fire) {
                    magazine.move=SmgPolicy::Burst;magazine.queued=1;magazine.delay=0;
                    assert(magazine.Ready()); // animation-clock cues own volley cadence
                    magazine.Fired();++rounds;magazine.queued=0;
                }
            }
        }
        assert(rounds==Length(route)*3 && magazine.rounds==profile.magazine-rounds);
    }
    std::puts("PASS: manual fire priority, moving semi-auto and charge; bounded follow-up buffer; volley/cancel/pose safety");
}
