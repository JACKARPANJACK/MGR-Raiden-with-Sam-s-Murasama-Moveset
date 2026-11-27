#pragma once
#include "SamArchiveLookup.h"
#include "SamBossSequence.h"
#include "SamConfig.h"
#include "AnimationArchive.h"
#include <map>
#include <cstdio>
#include <cmath>

// Buffers stay alive across player/scene cleanup: native sequences retain them.
class StealthCqcAnimations {
    struct Pair {std::vector<unsigned char> motion,sequence;char code[5]{};float contact=0,recovery=0;};
    std::map<int,Pair> clips;
    static bool Read(const std::string& path,std::vector<unsigned char>& bytes) {
        bytes=AnimationArchive::Read("pl1c00",path);
        return bytes.size()>=16 && bytes.size()<=16*1024*1024;
    }
public:
    static StealthCqcAnimations& Get(){static StealthCqcAnimations a;return a;}
    SamArchiveLookup::Clip Clip(int index) {
        auto it=clips.find(index);
        if(it==clips.end()) {
            const auto& names=AnimationArchiveNames::CqcBases;
            if(index<17 || index>32)return {};
            const std::string path=names[index-17];
            Pair pair;
            std::vector<unsigned char> timing;
            if(!Read(path+".mot",pair.motion) || !Read(path+"_0_seq.bxm",pair.sequence) ||
                !Read(path+"_timing.bin",timing) || timing.size()!=16 || std::memcmp(timing.data(),"CQT1",4) ||
                pair.motion.size()<44 || std::memcmp(pair.motion.data(),"mot\0",4) || SamBossSequence::Size(pair.sequence.data())!=pair.sequence.size())return {};
            unsigned frames=0;unsigned short motFrames=0;
            std::memcpy(&frames,timing.data()+4,4);std::memcpy(&motFrames,pair.motion.data()+10,2);
            std::memcpy(&pair.contact,timing.data()+8,4);std::memcpy(&pair.recovery,timing.data()+12,4);
            if(frames!=motFrames || !std::isfinite(pair.contact+pair.recovery) || pair.contact<=0 ||
                pair.contact>=pair.recovery || pair.recovery>=frames)return {};
            // Runtime indices 31/32 reverse the export order of the last two.
            std::memcpy(pair.code,pair.motion.data()+31,4);
            if(!SamArchiveLookup::ValidCode(pair.code))return {};
            it=clips.emplace(index,std::move(pair)).first;
        }
        return {it->second.motion.data(),it->second.sequence.data(),SamArchiveLookup::Source::Playable};
    }
    const char* Code(int index){return Clip(index).motion?clips[index].code:nullptr;}
    float Contact(int index){return Clip(index).motion?clips[index].contact:0;}
    float Recovery(int index){return Clip(index).motion?clips[index].recovery:0;}
};
