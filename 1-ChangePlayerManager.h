#pragma once
#include "1-GlobleVarious.h"
#include <mutex>

class ChangePlayerHandle
{
private:
	static constexpr uint32_t MODEL_COUNT = 14;
	static constexpr uint32_t MODEL_ID_COUNT = 3;
	const uintptr_t BASE_ADDR = shared::base;
	mutable std::mutex m_mutex;

	struct MemoryFix
	{
		uintptr_t offset;
		uint32_t value;
		bool isPointer;
	};

	struct ModelAddress
	{
		uintptr_t offsets[MODEL_ID_COUNT];
	};

	void ApplyFixes(const MemoryFix* fixes, size_t count, uintptr_t sliderTarget, const uintptr_t* sliderOffsets, size_t sliderCount) const
	{
		for (size_t i = 0; i < count; i++)
		{
			const auto& fix = fixes[i];
			uintptr_t address = BASE_ADDR + fix.offset;
			uintptr_t value = fix.isPointer ? (BASE_ADDR + fix.value) : fix.value;
			injector::WriteMemory(address, static_cast<uint32_t>(value), true);
		}

		for (size_t i = 0; i < sliderCount; i++)
			injector::WriteMemory(BASE_ADDR + sliderOffsets[i], static_cast<uint32_t>(sliderTarget), true);
	}

public:
	[[nodiscard]] static ChangePlayerHandle& GetInstance() noexcept
	{
		static ChangePlayerHandle instance;
		return instance;
	}

	void ChangePlayerOnce(bool PlayerIsSam) const noexcept
	{
		std::lock_guard lock(m_mutex);
		const auto route = [&](uintptr_t offset, uintptr_t target) {
			injector::WriteMemory<uint32_t>(BASE_ADDR + offset, static_cast<uint32_t>(target), true);
		};
		if (PlayerIsSam)
		{
			// Reference Sam-in-Raiden route table: controller input, state
			// factory, update tick, animation dispatch and attack effects.
			route(0x129EE48, BASE_ADDR + 0x492EB0);
			route(0x129EE50, BASE_ADDR + 0x493BD0);
			route(0x12492C0, BASE_ADDR + 0x468E60);
			route(0x129EAD0, BASE_ADDR + 0x45C700);
			route(0x129EAE4, BASE_ADDR + 0x805000);
			route(0x129EBB4, BASE_ADDR + 0x46BC60);
		route(0x129EBD4, BASE_ADDR + 0x7F5690);
			return;
		}
		route(0x129EE48, BASE_ADDR + 0x8104B0);
		route(0x129EE50, BASE_ADDR + 0x8104B0);
		route(0x12492C0, BASE_ADDR + 0x791CC0);
		route(0x129EAD0, BASE_ADDR + 0x8064C0);
		route(0x129EAE4, BASE_ADDR + 0x69D3D0);
		route(0x129EBB4, BASE_ADDR + 0x7E6E90);
		route(0x129EBD4, BASE_ADDR + 0x46DF00);

		static const MemoryFix raidenFixes[] =
		{
			{0x8202A5, 70656, false},
			{0x7F7197, 181, false},
			{0x7C798D, 333, false},
			{0x7C7A74, 333, false},
			{0x7C7A24, 334, false},
			{0x7C7CD0, 335, false},
			{0x7D641F, 235, false},
			{0x7B6EF7, 239, false},
			{0x7B6EF0, 323, false},
			{0x7B7284, 267, false},
			{0x7B9B9E, 250, false}, {0x7B9BC8, 242, false}, {0x7B9BEC, 247, false},
			{0x7B9C34, 245, false}, {0x7B9CA9, 248, false}, {0x7B9CF4, 246, false},
			{0x7B9D17, 292, false}, {0x7B9D41, 284, false}, {0x7B9D65, 289, false},
			{0x7B9D89, 286, false}, {0x7B9DAD, 287, false}, {0x7B9DD4, 291, false},
			{0x7B9DFB, 283, false}, {0x7B9E22, 290, false}, {0x7B9E49, 285, false},
			{0x7B9E6D, 288, false},
			{0x1CA3EC, 240, false},
			{0x6E46E7, 0x700, false},
			{0x014A1D70, 0x00010010, false},
			{0x014A4420, 0x00011400, false},
			{0x0129EBD4, 0x46DF00, true},
			{0x0129EB18, 0x6C3900, true},
			{0x0129EAE4, 0x69D3D0, true}
		};

		static const uintptr_t sliderOffsets[] =
		{
			0xB7DA3, 0x95B878, 0x96DCF3, 0x192407,
			0xC747B, 0x95B51A, 0x9687FD, 0x1F5B0
		};

		static const size_t sliderCount = sizeof(sliderOffsets) / sizeof(sliderOffsets[0]);

		ApplyFixes(raidenFixes, sizeof(raidenFixes) / sizeof(raidenFixes[0]),
			BASE_ADDR + 0x17E9DB8, sliderOffsets, sliderCount);
	}

	void ChangeModelID()
	{
		std::lock_guard lock(m_mutex);
		bool isSam = g_GameStateManager.IsMainSamPlayer;

		// Update constraint indices: sheath (0x710 for Sam hip, 0x711 for Raiden back)
		injector::WriteMemory<DWORD>(BASE_ADDR + 0x007BD7CE + 1, isSam ? 0x710 : 0x711, true);
		injector::WriteMemory<DWORD>(BASE_ADDR + 0x007BD968 + 1, 0x700, true);

		// Default Raiden sword models: 10001 (blade), 10005 (core?), 10004 (sheath)
		// Sam sword models: 11401 (blade), 11405 (core?), 11404 (sheath)
		uint32_t activeModelIds[3] = {
			isSam ? 0x11401u : 0x10001u,
			isSam ? 0x11405u : 0x10005u,
			isSam ? 0x11404u : 0x10004u
		};

		static const ModelAddress modelOffsets[MODEL_COUNT] =
		{
			{{0x14A982C, 0x14A9838, 0x14A9834}},
			{{0x14A9840, 0x14A984C, 0x14A9848}},
			{{0x14A9854, 0x14A9860, 0x14A985C}},
			{{0x14A9868, 0x14A9874, 0x14A9870}},
			{{0x14A987C, 0x14A9888, 0x14A9884}},
			{{0x14A9890, 0x14A989C, 0x14A9898}},
			{{0x14A98A8, 0x14A98B0, 0x14A98AC}},
			{{0x14A98B8, 0x14A98C4, 0x14A98C0}},
			{{0x14A98CC, 0x14A98D8, 0x14A98D4}},
			{{0x14A98E0, 0x14A98EC, 0x14A98E8}},
			{{0x14A98F4, 0x14A9900, 0x14A98FC}},
			{{0x14A9908, 0x14A9914, 0x14A9910}},
			{{0x14A991C, 0x14A9920, 0x14A9924}},
			{{0x14A9930, 0x14A993C, 0x14A9938}}
		};

		for (uint32_t i = 0; i < MODEL_COUNT; i++)
			for (uint32_t j = 0; j < MODEL_ID_COUNT; j++)
				injector::WriteMemory(BASE_ADDR + modelOffsets[i].offsets[j], activeModelIds[j], true);

		// Do not change the global ID to Sam's (0x11012) so the HUD stays blue!
		injector::WriteMemory(BASE_ADDR + 0x14A99D4, 0x00010012, true);
	}

private:
	ChangePlayerHandle() = default;
	ChangePlayerHandle(const ChangePlayerHandle&) = delete;
	ChangePlayerHandle& operator=(const ChangePlayerHandle&) = delete;
};

inline ChangePlayerHandle& g_ChangePlayerHandle = ChangePlayerHandle::GetInstance();
