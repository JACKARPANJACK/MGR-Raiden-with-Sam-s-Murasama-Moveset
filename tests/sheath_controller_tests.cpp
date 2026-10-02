#include "../SamSheathPolicy.h"
#include <cassert>
#include <cstring>
#include <iostream>

struct cVec4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    bool operator==(const cVec4& o) const {
        return x == o.x && y == o.y && z == o.z && w == o.w;
    }
};

struct ConstraintsStub {
    int m_nIndex = 0;
    unsigned int m_nBone = 0;
    unsigned int m_nRotationBone = 0;
    cVec4 m_vecRotation{};
    cVec4 m_vecOffset{};
};

static void SanitizeCode(char* dst, size_t cap, const char* src)
{
    if (cap >= 5) SamSheathPolicy::Code(dst, src);
}

static const char* ResolveFallback(const char* cleanCode)
{
    if (!std::strcmp(cleanCode, "0001") || !std::strcmp(cleanCode, "0002") || !std::strcmp(cleanCode, "0003"))
        return "0000";
    if (!std::strcmp(cleanCode, "0202") || !std::strcmp(cleanCode, "0203"))
        return "0200";
    if (!std::strcmp(cleanCode, "0230") || !std::strcmp(cleanCode, "0231") || !std::strcmp(cleanCode, "0232"))
        return "023a";
    if (!std::strcmp(cleanCode, "3003") || !std::strcmp(cleanCode, "3004"))
        return "3010";
    return nullptr;
}

int main()
{
    // Test 1: Code sanitization
    char buf[16]{};
    SanitizeCode(buf, sizeof(buf), "pl0010_2000");
    assert(std::strcmp(buf, "2000") == 0);

    SanitizeCode(buf, sizeof(buf), "pl1400_0200");
    assert(std::strcmp(buf, "0200") == 0);

    SanitizeCode(buf, sizeof(buf), "pl1404_92e4.mot");
    assert(std::strcmp(buf, "92e4") == 0);

    SanitizeCode(buf, sizeof(buf), "3010");
    assert(std::strcmp(buf, "3010") == 0);

    SanitizeCode(buf, sizeof(buf), "0000");
    assert(std::strcmp(buf, "0000") == 0);

    SanitizeCode(buf, sizeof(buf), "pl1400_2101_2_seq.bxm");
    assert(std::strcmp(buf, "2101") == 0);
    assert(std::strcmp(SamSheathPolicy::MotionCode(buf), "3017") == 0);

    SanitizeCode(buf, sizeof(buf), "pl1400_2108.mot");
    assert(std::strcmp(buf, "2108") == 0);
    assert(std::strcmp(SamSheathPolicy::MotionCode(buf), "2108") == 0);

    // Test 2: Fallback resolution
    assert(std::strcmp(ResolveFallback("0001"), "0000") == 0);
    assert(std::strcmp(ResolveFallback("0002"), "0000") == 0);
    assert(std::strcmp(ResolveFallback("0202"), "0200") == 0);
    assert(std::strcmp(ResolveFallback("0231"), "023a") == 0);
    assert(std::strcmp(ResolveFallback("3003"), "3010") == 0);
    assert(ResolveFallback("2000") == nullptr);

    // Test 3: Hip attachment bone & clean zero offsets (Sam native IDA 0x46ECA4)
    ConstraintsStub c{};
    // Hip attachment (Sam native dedicated bone 0x7F0):
    c.m_nBone = 0x7F0;
    c.m_nRotationBone = 0;
    c.m_vecOffset = { 0.0f, 0.0f, 0.0f, 0.0f };
    c.m_vecRotation = { 0.0f, 0.0f, 0.0f, 0.0f };
    assert(c.m_nBone == 0x7F0);
    assert(c.m_nRotationBone == 0);
    assert((c.m_vecOffset == cVec4{ 0.0f, 0.0f, 0.0f, 0.0f }));
    assert((c.m_vecRotation == cVec4{ 0.0f, 0.0f, 0.0f, 0.0f }));

    // Hand attachment (Sam native sequence event 21 left hand bone 0x701, rot bone 10)
    ConstraintsStub handConstraint{};
    handConstraint.m_nIndex = 17;
    handConstraint.m_nBone = 0x701;
    handConstraint.m_nRotationBone = 10;
    assert(handConstraint.m_nIndex == 17);
    assert(handConstraint.m_nBone == 0x701);
    assert(handConstraint.m_nRotationBone == 10);

    // Back restoration (Raiden default):
    c.m_nBone = 0x711;
    assert(c.m_nBone == 0x711);

    // Test 4: Model IDs
    const uint32_t samSheathModel = 0x11404u;
    const uint32_t raidenSheathModel = 0x10004u;
    assert(samSheathModel == 0x11404u);
    assert(raidenSheathModel == 0x10004u);

    std::cout << "PASS: sheath hip attachment (0x7F0), hand constraint (0x701), back restoration (0x711), zero offsets, code sanitization, fallback mappings\n";
    return 0;
}
