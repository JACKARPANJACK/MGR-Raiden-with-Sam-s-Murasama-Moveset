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
		// Costume rows contain body, hair, visor, sheath and face IDs, not a
		// shared blade/core/sheath tuple. Mutating them made scene reloads ask
		// for nonexistent pl0001/pl0004 archives and dereference a null sword.
		// Keep native scene construction intact; live sheath placement is owned
		// by SheathController and Sam's combat graph remains per-player.
	}

private:
	ChangePlayerHandle() = default;
	ChangePlayerHandle(const ChangePlayerHandle&) = delete;
	ChangePlayerHandle& operator=(const ChangePlayerHandle&) = delete;
};

inline ChangePlayerHandle& g_ChangePlayerHandle = ChangePlayerHandle::GetInstance();
