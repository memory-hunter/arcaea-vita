/*
 * fios.c -- use FIOS2 for optimized I/O
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "fios.h"
#include "utils/logger.h"
#include "utils/utils.h"

#include <psp2/kernel/clib.h>

#define MAX_PATH_LENGTH 256
#define RAMCACHEBLOCKSIZE (128 * 1024)
#define PSARCCACHEBLOCKSIZE (192 * 1024)
#define RAMCACHEBLOCKNUM 64

static int64_t g_OpStorage[SCE_FIOS_OP_STORAGE_SIZE(64, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_ChunkStorage[SCE_FIOS_CHUNK_STORAGE_SIZE(1024) / sizeof(int64_t) + 1];
static int64_t g_FHStorage[SCE_FIOS_FH_STORAGE_SIZE(1024, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
static int64_t g_DHStorage[SCE_FIOS_DH_STORAGE_SIZE(32, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];

static SceFiosRamCacheContext g_RamCacheContext = SCE_FIOS_RAM_CACHE_CONTEXT_INITIALIZER;
static SceFiosPsarcDearchiverContext g_PsarcContext;
static char *g_RamCacheWorkBuffer;
static int32_t g_PsarcHandle;
static SceFiosBuffer g_MountBuffer;

int fios_init(const char *path)
{
    int res;
    SceFiosParams params = SCE_FIOS_PARAMS_INITIALIZER;
    params.opStorage.pPtr = g_OpStorage;
    params.opStorage.length = sizeof(g_OpStorage);
    params.chunkStorage.pPtr = g_ChunkStorage;
    params.chunkStorage.length = sizeof(g_ChunkStorage);
    params.fhStorage.pPtr = g_FHStorage;
    params.fhStorage.length = sizeof(g_FHStorage);
    params.dhStorage.pPtr = g_DHStorage;
    params.dhStorage.length = sizeof(g_DHStorage);
    params.pathMax = MAX_PATH_LENGTH;
    params.threadAffinity[SCE_FIOS_IO_THREAD] = 0x20000;
    params.threadAffinity[SCE_FIOS_CALLBACK_THREAD] = 0;
    params.threadAffinity[SCE_FIOS_DECOMPRESSOR_THREAD] = 0;
    params.threadPriority[SCE_FIOS_IO_THREAD] = 64;
    params.threadPriority[SCE_FIOS_CALLBACK_THREAD] = 191;
    params.threadPriority[SCE_FIOS_DECOMPRESSOR_THREAD] = 191;

    res = sceFiosInitialize(&params);
    l_info("sceFiosInitialize: 0x%x", res);
    if (res < 0)
        return res;

    uint8_t psarc_exists = file_exists("ux0:data/arcaea/data.psarc");
    l_info("data.psarc exists: %d", psarc_exists);

    if (psarc_exists)
    {
        sceClibMemset(&g_PsarcContext, 0, sizeof(SceFiosPsarcDearchiverContext));
        g_PsarcContext.size = sizeof(SceFiosPsarcDearchiverContext);
        g_PsarcContext.pWorkBuffer = memalign(64, PSARCCACHEBLOCKSIZE);
        g_PsarcContext.workBufferSize = PSARCCACHEBLOCKSIZE;

        // PSARC filter FIRST at index 0
        res = sceFiosIOFilterAdd(0, sceFiosIOFilterPsarcDearchiver, &g_PsarcContext);
        l_info("sceFiosIOFilterAdd(psarc): 0x%x", res);
        if (res < 0)
            return res;

        res = sceFiosArchiveGetMountBufferSizeSync(NULL, "ux0:data/arcaea/data.psarc", NULL);
        l_info("GetMountBufferSize: 0x%x (%d bytes)", res, res);
        if (res < 0)
            return res;

        g_MountBuffer.length = res;
        g_MountBuffer.pPtr = malloc(res);

        res = sceFiosArchiveMountSync(NULL, &g_PsarcHandle, "ux0:data/arcaea/data.psarc", "/assets", g_MountBuffer, NULL);
        l_info("sceFiosArchiveMountSync: 0x%x (handle=%d)", res, g_PsarcHandle);
        if (res < 0)
            return res;
    }

    g_RamCacheWorkBuffer = memalign(8, RAMCACHEBLOCKNUM * RAMCACHEBLOCKSIZE);
    if (!g_RamCacheWorkBuffer)
        return -1;

    g_RamCacheContext.pPath = path;
    g_RamCacheContext.pWorkBuffer = g_RamCacheWorkBuffer;
    g_RamCacheContext.workBufferSize = RAMCACHEBLOCKNUM * RAMCACHEBLOCKSIZE;
    g_RamCacheContext.blockSize = RAMCACHEBLOCKSIZE;

    res = sceFiosIOFilterAdd(psarc_exists ? 1 : 0, sceFiosIOFilterCache, &g_RamCacheContext);
    l_info("sceFiosIOFilterAdd(cache at %d): 0x%x", psarc_exists ? 1 : 0, res);
    if (res < 0)
        return res;

    return 0;
}

void fios_terminate(void)
{
    sceFiosTerminate();
    free(g_RamCacheWorkBuffer);
}
