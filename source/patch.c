/*
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/**
 * @file  patch.c
 * @brief Patching some of the .so internal functions or bridging them to native
 *        for better compatibility.
 */

#include <kubridge.h>
#include <so_util/so_util.h>
#include <stdio.h>
#include <string.h>
#include <vitasdk.h>

#ifdef __cplusplus
extern "C"
{
#endif
	extern so_module so_mod;
	extern so_module fmod_mod;
	extern so_module provider_mod;
#ifdef __cplusplus
};
#endif

#define SCE_KERNEL_MEMBLOCK_TYPE_USER_RX (0x0C20D050)

#include "utils/logger.h"
#include "utils/dialog.h"
#include "reimpl/sys.h"
#include <stdbool.h>

void __kuser_memory_barrier(void)
{
	__sync_synchronize();
}

void kuser_patch(void)
{
	SceKernelAllocMemBlockKernelOpt opt;
	memset(&opt, 0, sizeof(SceKernelAllocMemBlockKernelOpt));
	opt.size = sizeof(SceKernelAllocMemBlockKernelOpt);
	opt.attr = 0x1;
	opt.field_C = (SceUInt32)0x9F000000;
	if (kuKernelAllocMemBlock("atomic", SCE_KERNEL_MEMBLOCK_TYPE_USER_RX, 0x1000, &opt) < 0)
		fatal_error("Error could not allocate atomic block.");
	kuKernelMemProtect((void *)0x9F000000, (SceSize)0x1000, KU_KERNEL_PROT_EXEC | KU_KERNEL_PROT_READ | KU_KERNEL_PROT_WRITE);

	hook_addr(0x9F000FA0, (uintptr_t)__kuser_memory_barrier);
	hook_addr(0x9F000FC0, (uintptr_t)__atomic_cmpxchg);

	uint32_t patched_addr;
	for (uint32_t addr = so_mod.text_base; addr < so_mod.text_base + so_mod.text_size; addr += 4)
	{
		uint32_t *a = (uint32_t *)addr;
		if (*a == 0xFFFF0FC0)
		{
			l_debug("Patching 0x%x -> __kuser_cmpxchg", a);
			patched_addr = 0x9F000FC0;
			kuKernelCpuUnrestrictedMemcpy((void *)(addr), &patched_addr, sizeof(uint32_t));
		}
		else if (*a == 0xFFFF0FA0)
		{
			l_debug("Patching 0x%x -> __kuser_memory_barrier", a);
			patched_addr = 0x9F000FA0;
			kuKernelCpuUnrestrictedMemcpy((void *)(addr), &patched_addr, sizeof(uint32_t));
		}
	}

	for (uint32_t addr = fmod_mod.text_base; addr < fmod_mod.text_base + fmod_mod.text_size; addr += 4)
	{
		uint32_t *a = (uint32_t *)addr;
		if (*a == 0xFFFF0FC0)
		{
			l_debug("Patching 0x%x -> __kuser_cmpxchg", a);
			patched_addr = 0x9F000FC0;
			kuKernelCpuUnrestrictedMemcpy((void *)(addr), &patched_addr, sizeof(uint32_t));
		}
		else if (*a == 0xFFFF0FA0)
		{
			l_debug("Patching 0x%x -> __kuser_memory_barrier", a);
			patched_addr = 0x9F000FA0;
			kuKernelCpuUnrestrictedMemcpy((void *)(addr), &patched_addr, sizeof(uint32_t));
		}
	}

	for (uint32_t addr = provider_mod.text_base; addr < provider_mod.text_base + provider_mod.text_size; addr += 4)
	{
		uint32_t *a = (uint32_t *)addr;
		if (*a == 0xFFFF0FC0)
		{
			l_debug("Patching 0x%x -> __kuser_cmpxchg", a);
			patched_addr = 0x9F000FC0;
			kuKernelCpuUnrestrictedMemcpy((void *)(addr), &patched_addr, sizeof(uint32_t));
		}
		else if (*a == 0xFFFF0FA0)
		{
			l_debug("Patching 0x%x -> __kuser_memory_barrier", a);
			patched_addr = 0x9F000FA0;
			kuKernelCpuUnrestrictedMemcpy((void *)(addr), &patched_addr, sizeof(uint32_t));
		}
	}
}

so_hook sqlite3_open_v2_hook;

static int sqlite3_open_v2_patched(const char *filename, void **ppDb, int flags, const char *zVfs)
{
	int ret = SO_CONTINUE(int, sqlite3_open_v2_hook, filename, ppDb, flags, "unix-none");

	int (*sqlite3_exec_func)(void *, const char *, void *, void *, char **) = (int (*)(void *, const char *, void *, void *, char **))so_symbol(&so_mod, "sqlite3_exec");
	sqlite3_exec_func(*ppDb, "PRAGMA journal_mode=MEMORY;", NULL, NULL, NULL);
	sqlite3_exec_func(*ppDb, "PRAGMA locking_mode=EXCLUSIVE;", NULL, NULL, NULL);

	return ret;
}

// so_hook cxa_throw_hook;

// static int cxa_throw_patched(void *thrown_exception, void *type_info, void (*dest)(void *))
// {
// 	l_debug("__cxa_throw(obj=%p, type_info=%p, dest=%p) called, caller: %p", thrown_exception, type_info, dest, __builtin_return_address(0));

// 	return SO_CONTINUE(int, cxa_throw_hook, thrown_exception, type_info, dest);
// }

// so_hook __throw_system_error_hook;

// static int __throw_system_error_patched(int error_code, const char *message)
// {
// 	l_debug("__throw_system_error(%d, \"%s\") caller=%p", error_code, message ? message : "(null)", __builtin_return_address(0));

// 	SO_CONTINUE(int, __throw_system_error_hook, error_code, message);
// }

static int ret0(void) { return 0; }
static void *retthis(void *this_) { return this_; }

void so_patch(void)
{
	kuser_patch();

	// sqlite3 to use unix-none
	sqlite3_open_v2_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "sqlite3_open_v2"), (uintptr_t)&sqlite3_open_v2_patched);

	// disable google play games achievements
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxy10nativeInitERKNS_4JsonE"), (uintptr_t)&ret0);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxy16loadAchievementsEb"), (uintptr_t)&ret0);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxy16showAchievementsEv"), (uintptr_t)&ret0);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxy6revealERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE"), (uintptr_t)&ret0);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxy6unlockERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE"), (uintptr_t)&ret0);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxy8setStepsERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEi"), (uintptr_t)&ret0);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxy9incrementERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEi"), (uintptr_t)&ret0);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxyC1Ev"), (uintptr_t)&retthis);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxyC2Ev"), (uintptr_t)&retthis);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxyD0Ev"), (uintptr_t)&ret0);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxyD1Ev"), (uintptr_t)&ret0);
	hook_addr((uintptr_t)so_symbol(&so_mod, "_ZN6sdkbox20GPGAchievementsProxyD2Ev"), (uintptr_t)&ret0);

	// patch string for userdefault.xml
	// char *data_data_str = (char *)(so_mod.text_base + 0x93F7CC);
	// strcpy(data_data_str, "ux0:data/");

	// cxa_throw_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "__cxa_throw"), (uintptr_t)&cxa_throw_patched);
	// __throw_system_error_hook = hook_addr((uintptr_t)so_symbol(&so_mod, "_ZNSt6__ndk120__throw_system_errorEiPKc"), (uintptr_t)&__throw_system_error_patched);
}