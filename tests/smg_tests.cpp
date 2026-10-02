#include "../SmgPolicy.h"
#include "../NativeSmgMenuPolicy.h"
#include "../NativeSmgMessagePolicy.h"
#include <array>
#include <cassert>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>
using namespace SmgPolicy;
int main()
{
    // Exercise the actual native message builder and decode its glyph stream
    // back to text, including spaces, font identity, line height and terminator.
    alignas(4) std::array<unsigned char,2048> bank{};
    auto* header=reinterpret_cast<unsigned*>(bank.data());
    header[2]=40;header[3]=95;header[6]=800;header[7]=1;
    for(unsigned i=0;i<95;++i)
    {
        auto* symbol=reinterpret_cast<unsigned short*>(bank.data()+40+i*8);
        symbol[0]=3;symbol[1]=static_cast<unsigned short>(32+i);
        *reinterpret_cast<unsigned*>(symbol+2)=i;
    }
    *reinterpret_cast<unsigned*>(bank.data()+800)=3;
    *reinterpret_cast<float*>(bank.data()+808)=39;
    auto verifyMessages=[](void* data)
    {
        using namespace NativeSmgMessagePolicy;
        auto* bytes=static_cast<unsigned char*>(data);
        auto* fields=static_cast<unsigned*>(data);
        for(unsigned which=0;which<3;++which)
        {
            assert(Generate(data,which));
            const auto& m=messages[which];
            assert(m.text.count==(which==1?4u:1u));
            for(unsigned l=0;l<m.text.count;++l)
            {
                std::string decoded;
                const auto& line=m.lines[l];
                assert(line.height>0 && line.lengthWithEnd==line.length+1);
                for(unsigned c=0;c<line.length*2;c+=2)
                {
                    const auto token=m.content[l][c];
                    if(token==0x8001) {decoded+=' ';assert(m.content[l][c+1]==m.text.font);continue;}
                    bool found=false;
                    for(unsigned s=0;s<fields[3];++s)
                    {
                        auto* glyph=reinterpret_cast<unsigned short*>(bytes+fields[2]+s*8);
                        if(glyph[0]==m.text.font && *reinterpret_cast<unsigned*>(glyph+2)==token)
                        {decoded+=char(glyph[1]);found=true;break;}
                    }
                    assert(found);
                }
                assert(m.content[l][line.length*2]==0x8000);
                assert(decoded==(which==1?(l==3?LastLine:Description[l]):which==2?Name:Type));
            }
        }
    };
    verifyMessages(bank.data());
    std::ifstream font("local_assets/smg/ui/ui_core_us/messcore.mcd",std::ios::binary);
    if(font)
    {
        std::vector<unsigned char> nativeBank((std::istreambuf_iterator<char>(font)),{});
        verifyMessages(nativeBank.data());
    }
    header[3]=0;assert(!NativeSmgMessagePolicy::Generate(bank.data(),0));
    // Check every native inventory size, including a completely stocked menu.
    for(int count=1;count<=NativeSmgMenuPolicy::Capacity;++count)
    {
        std::array<int,13> entries{},amounts{},maxima{};
        entries.fill(0x12345678);amounts.fill(0x12345678);maxima.fill(0x12345678);
        for(int i=0;i<count-1;++i) {entries[i+1]=i;amounts[i+1]=i+1;maxima[i+1]=i+2;}
        entries[count]=-1;
        const int result=NativeSmgMenuPolicy::Insert(entries.data()+1,amounts.data()+1,maxima.data()+1,count);
        assert(result==(count<11 ? count+1 : count));
        assert(entries[count]==NativeSmgMenuPolicy::Entry);
        assert(amounts[count]==30 && maxima[count]==30);
        assert(NativeSmgMenuPolicy::Find(entries.data()+1,result)==count-1);
        assert(entries.front()==0x12345678 && entries.back()==0x12345678);
        assert(amounts.front()==0x12345678 && amounts.back()==0x12345678);
        assert(maxima.front()==0x12345678 && maxima.back()==0x12345678);
        for(int i=0;i<count-1;++i)
        {assert(entries[i+1]==i);assert(amounts[i+1]==i+1);assert(maxima[i+1]==i+2);}
        if(count<11) assert(entries[result]==-1);
        assert(NativeSmgMenuPolicy::Insert(entries.data()+1,amounts.data()+1,maxima.data()+1,result)==result);
        for(int cursor=0;cursor<result;++cursor)
            for(int row=0;row<8;++row)
            {const int index=NativeSmgMenuPolicy::RowIndex(cursor,result,row);assert(index>=0 && index<result);}
    }
    std::array<int,11> invalid{};
    invalid.fill(77);
    assert(NativeSmgMenuPolicy::Insert(invalid.data(),invalid.data(),invalid.data(),12)==12);
    assert(NativeSmgMenuPolicy::Insert(invalid.data(),invalid.data(),invalid.data(),0)==0);
    assert(NativeSmgMenuPolicy::Insert(invalid.data(),invalid.data(),invalid.data(),3)==3);
    for(int value:invalid) assert(value==77);
    assert(NativeSmgMenuPolicy::Slot!=10 && NativeSmgMenuPolicy::Alias!=0x154B4AAB);
    FireControl tap;tap.Tick(true,true,Burst);assert(!tap.Ready());
    tap.Tick(false,true,Burst);assert(tap.Ready());assert(tap.AnimationBank()==EnemyMotionBank);tap.Fired();
    for(unsigned i=0;i<Interval;i++) tap.Tick(false,true,Burst);
    assert(tap.Ready());tap.Fired();
    for(unsigned i=0;i<Interval;i++) tap.Tick(false,true,Burst);
    assert(tap.Ready());tap.Fired();assert(tap.rounds==27 && tap.queued==0);
    // Repeated normal taps and directional/aerial fire never select special clips.
    tap.Tick(true,true,Burst);tap.Tick(false,true,Burst);
    assert(!tap.comboPose && tap.AnimationBank()==EnemyMotionBank && !std::strcmp(tap.AnimationCode(),"2400"));
    tap.Cancel();tap.Tick(false,true,Burst);tap.Tick(true,true,Burst);tap.Tick(false,true,Burst);
    assert(!tap.comboPose);
    for(auto move:{Sweep,Aerial})
    {FireControl normal;normal.Tick(true,true,move);normal.Tick(false,true,move);assert(!normal.comboPose && normal.AnimationBank()==EnemyMotionBank);}
    FireControl charged;
    for(unsigned i=0;i<30;++i) {charged.Tick(true,true,Burst);assert(!charged.Ready());}
    charged.Tick(false,true,Burst);assert(charged.Ready() && charged.move==Charged && charged.AnimationBank()==MotionBank);
    assert(charged.queued==30);charged.Fired();assert(charged.rounds==29 && charged.queued==29);
    FireControl full;
    for(unsigned i=0;i<300;++i) {full.Tick(true,true,Burst);assert(!full.Ready());}
    full.Tick(false,true,Burst);assert(full.Ready() && full.move==FullCharged && full.charge.releasedFrames==90);
    assert(!std::strcmp(full.AnimationCode(),"2578") && full.queued==30);
    auto drain=[](FireControl& control)
    {
        unsigned fired=0;
        for(unsigned tick=0;tick<240 && control.queued;++tick)
        {
            if(control.Ready()) {control.Fired();++fired;if(!control.queued) break;}
            control.Tick(false,true,Burst);
        }
        return fired;
    };
    assert(drain(full)==30); // One release, exactly thirty successful bullets.
    assert(full.rounds==0 && full.reload==Reload);
    for(unsigned i=1;i<Reload;++i) {full.Tick(true,true,Burst);assert(!full.Ready());}
    full.Tick(true,true,Burst);assert(full.rounds==Magazine && !full.Ready());
    full.Tick(false,true,Burst);assert(!full.Ready());
    FireControl unfunded;unfunded.rounds=2;
    for(unsigned i=0;i<90;++i) unfunded.Tick(true,true,Burst);
    unfunded.Tick(false,true,Burst);assert(!unfunded.Ready());unfunded.RejectUnfunded();
    assert(unfunded.reload==Reload && unfunded.rounds==2 && unfunded.queued==30);
    for(unsigned i=0;i<Reload;++i) unfunded.Tick(false,true,Burst);
    assert(unfunded.rounds==30 && unfunded.Ready() && drain(unfunded)==30);
    FireControl launch;
    for(unsigned tick=0;tick<100;++tick)
    {launch.Tick(true,true,Launcher);if(launch.Ready()) launch.Fired();}
    assert(launch.rounds==Magazine && !launch.Ready()); // hold charges, never fires on its own
    launch.Tick(false,true,Launcher);assert(launch.Ready() && launch.move==FullCharged);launch.Fired();
    assert(launch.rounds==29 && launch.queued==29);
    launch.Tick(true,false,Launcher);launch.Tick(true,true,Launcher);assert(!launch.Ready());
    launch.Tick(false,true,Launcher);launch.Tick(true,true,Launcher);launch.Tick(false,true,Launcher);
    for(unsigned i=0;i<Interval;++i) launch.Tick(false,true,Launcher);
    assert(launch.Ready());
    FireControl failed;failed.Tick(true,true,Aerial);
    for(unsigned i=0;i<100;++i) failed.Tick(true,true,Aerial);
    assert(failed.rounds==Magazine); // resource/spawn failures consume nothing
    failed.Cancel();assert(!failed.Ready() && failed.queued==0);
    failed.Tick(true,true,Burst);assert(!failed.charge.frames);
    failed.Tick(false,true,Burst);failed.Tick(true,true,Burst);assert(failed.charge.frames==1);
    assert(OwnedHit(Attack,4,true));assert(OwnedHit(LaunchAttack,6,true));
    assert(!OwnedHit(Attack,4,false));assert(!OwnedHit(Attack,20,true));
    assert(!OwnedHit(LaunchAttack,12,true) && !OwnedHit(LaunchAttack,20,true));
    assert(RoundMove(Charged,0)==Launcher && RoundMove(FullCharged,29)==Burst);
    assert(Cost(Charged)==1 && Cost(FullCharged)==1 && Damage(Charged)==4);
    assert(drain(charged)==29 && charged.rounds==0 && charged.reload==Reload);
    FireControl cancelled;
    for(unsigned i=0;i<90;++i) cancelled.Tick(true,true,Burst);
    cancelled.Tick(false,true,Burst);cancelled.Fired();cancelled.Cancel();
    assert(!cancelled.queued && cancelled.rounds==29);
    std::array<unsigned char,0x320> descriptor{};
    Configure(descriptor.data(),Burst);
    unsigned prefab=0;std::memcpy(&prefab,descriptor.data()+4,4);
    assert(prefab==0x3B000 && prefab!=0x20046 && Muzzle==0x300);
    assert(Damage(Burst)<20 && Damage(Launcher)<20);
    assert(RoundMove(Burst,0)==Burst && RoundMove(Burst,1)==Burst && RoundMove(Burst,2)==Launcher);
    assert(RoundMove(Burst,5)==Launcher && RoundMove(Aerial,2)==Aerial && RoundMove(Sweep,2)==Sweep);
    GunChargePolicy::Hold native;
    for(unsigned i=0;i<30;++i) assert(!native.Tick(true,true));
    assert(native.Tick(false,true) && GunChargePolicy::Tier(native.releasedFrames)==1);
    native.Tick(true,true);native.Tick(true,false);assert(!native.Tick(false,true));
    native.Tick(true,true,false);assert(!native.Tick(false,true));
    assert(GunChargePolicy::LauncherSlot(5) && GunChargePolicy::LauncherSlot(6));
    assert(!GunChargePolicy::LauncherSlot(1) && !GunChargePolicy::LauncherSlot(11));
    assert(GunChargePolicy::LauncherObject(5)==0x31002 && GunChargePolicy::LauncherObject(6)==0x310E1);
    assert(GunChargePolicy::ChargedDamage(200,0)==200 && GunChargePolicy::ChargedDamage(200,1)==400 && GunChargePolicy::ChargedDamage(200,2)==600);
    assert(GunChargePolicy::ChargedDamage(0,2)==0);
    for(unsigned i=0;i<5;++i) assert(SmgPolicy::MotionRelease(SmgPolicy::Motion(Burst,i))>0);
    ReadyWindow idle;
    idle.Fired();assert(idle.remaining==180);
    for(unsigned i=0;i<60;++i) idle.Tick(true,true);
    assert(idle.remaining==120);
    idle.Tick(false,true);assert(idle.remaining==120);
    idle.Fired();assert(idle.remaining==180);
    idle.Tick(true,false);assert(!idle.remaining);
    idle.Fired();for(unsigned i=0;i<180;++i) idle.Tick(true,true);
    assert(!idle.remaining);
    assert(!std::strcmp(ReadyMotion,"0400") && RaiseTicks>0);
    std::cout << "SMG native menu, enemy ready/recoil policy, generic bullet prefab, exact 30-round dumps, cancellation and launcher policy passed.\n";
}
