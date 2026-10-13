#pragma once
#include <cstring>
namespace SamVoicePolicy
{
    // Raiden event names verified in data002 Debug/sound/pl/vo_pl0010.txt.
    inline const char* Attack(const char* event)
    {
        if(!event)return nullptr;
        struct Pair {const char* from;const char* to;};
        static constexpr Pair pairs[]={
            {"pl1400_vo_atk_light","pl0010_vo_atk_light"},
            {"pl1400_vo_atk_medium","pl0010_vo_atk_medium"},
            {"pl1400_vo_atk_heavy","pl0010_vo_atk_heavy"},
            {"pl1400_vo_atk_charge","pl0010_vo_atk_charge"},
            {"pl1400_vo_atk_charge_l","pl0010_vo_atk_charge"},
            {"pl1400_vo_atk_zangeki","pl0010_vo_atk_zangeki_line"},
            {"em0020_vo_atk_light","pl0010_vo_atk_light"},
            {"em0020_vo_atk_medium","pl0010_vo_atk_medium"},
            {"em0020_vo_atk_heavy","pl0010_vo_atk_heavy"},
            {"em0020_vo_atk_charge","pl0010_vo_atk_charge"},
            {"em0020_vo_atk_charge_l","pl0010_vo_atk_charge"},
            {"em0020_vo_atk_end_1","pl0010_vo_cmb_finish_line"},
            {"em0020_vo_atk_end_2","pl0010_vo_cmb_finish_line"},
            {"em0020_vo_atk_end_3","pl0010_vo_cmb_finish_line"},
            {"em0020_vo_finish","pl0010_vo_cmb_finish_line"},
            {"em0020_vo_qte_win","pl0010_vo_cmb_finish_line"},
            {"em0020_vo_grd_light","pl0010_vo_grd_light"},
            {"em0020_vo_grd_medium","pl0010_vo_grd_medium"},
            {"pl1400_vo_grd_light","pl0010_vo_grd_light"},
            {"pl1400_vo_grd_medium","pl0010_vo_grd_medium"},
            {"Comn9000_1b1010","pl0010_vo_cmb_finish_line"},
            {"Comn9000_1n1010","pl0010_vo_cmb_finish_line"},
            {"Comn9000_1o1010","pl0010_vo_cmb_finish_line"},
        };
        for(const auto& pair:pairs)if(!std::strcmp(event,pair.from))return pair.to;
        return event;
    }
    inline const char* Select(const char* event,bool activePlayer,bool combat)
    {return activePlayer && combat ? Attack(event) : event;}
}
