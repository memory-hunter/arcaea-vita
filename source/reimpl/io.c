/*
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2022      Rinnegatamante
 * Copyright (C) 2022-2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "reimpl/io.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdarg.h>
#include <limits.h>
#include <psp2/kernel/threadmgr.h>

#ifdef USE_SCELIBC_IO
#include <libc_bridge/libc_bridge.h>
#endif

#include "utils/logger.h"
#include "utils/utils.h"
#include "reimpl/_existing_files.h"
#include "fios/fios.h"

// Includes the following inline utilities:
// int oflags_musl_to_newlib(int flags);
// dirent64_bionic * dirent_newlib_to_bionic(struct dirent* dirent_newlib);
// void stat_newlib_to_bionic(struct stat * src, stat64_bionic * dst);
#include "reimpl/bits/_struct_converters.c"

static const char *fix_android_path(const char *path, char *buf, size_t buf_size)
{
    static const char *prefix = "/data/data/";
    size_t prefix_len = strlen(prefix);

    if (strncmp(path, prefix, prefix_len) == 0)
    {
        snprintf(buf, buf_size, "%s%s", "ux0:data/", path + prefix_len);
        return buf;
    }
    return path;
}

int fios_asset_path(const char *path, char *normalized, size_t normalized_size,
                    char *fios_path, size_t fios_path_size)
{
    static const char *prefixes[] = {
        DATA_PATH"/assets/",
        DATA_PATH,
    };
    const char *rel = path;
    const char *candidate = path;
    if (!path || !normalized || !fios_path)
        return 0;
    while (*rel == '/')
        rel++;
    candidate = rel;
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++)
    {
        size_t n = strlen(prefixes[i]);
        if (strncmp(candidate, prefixes[i], n) == 0)
        {
            rel = candidate + n;
            break;
        }
    }
    while (strncmp(rel, "assets/", 7) == 0)
        rel += 7;
    if (snprintf(normalized, normalized_size, "%s", rel) >= (int)normalized_size)
        return 0;
    int listed = 0;
    for (int i = 0; i < existing_files_len; i++)
    {
        if (strcmp(existing_files[i], normalized) == 0)
        {
            listed = 1;
            break;
        }
    }
    if (!listed)
        return 0;
    if (snprintf(fios_path, fios_path_size, "/assets/%s", normalized) >= (int)fios_path_size)
        return 0;
    return 1;
}

int32_t fios_asset_open(const char *path)
{
    char normalized[512], archive_path[640];
    int32_t handle = -1;
    if (!fios_asset_path(path, normalized, sizeof(normalized), archive_path, sizeof(archive_path)))
        return -1;
    if (sceFiosFHOpenSync(NULL, &handle, archive_path, NULL) < 0)
        return -1;
    return handle;
}

int64_t fios_asset_size(int32_t handle)
{
    int64_t size = sceFiosFHSeek(handle, 0, SEEK_END);
    if (size < 0 || sceFiosFHSeek(handle, 0, SEEK_SET) < 0) return -1;
    return size;
}

int64_t fios_asset_read(int32_t handle, void *buffer, int64_t length) { return sceFiosFHReadSync(NULL, handle, buffer, length); }
int64_t fios_asset_seek(int32_t handle, int64_t offset, int whence) { return sceFiosFHSeek(handle, offset, whence); }
int fios_asset_close(int32_t handle) { return sceFiosFHCloseSync(NULL, handle); }

typedef struct fios_stdio_file {
    struct fios_stdio_file *next;
    int32_t handle;
    int eof;
} fios_stdio_file;

#define FIOS_FD_BASE 0x40000000
#define FIOS_FD_COUNT 128
static fios_stdio_file *g_fios_stdio_files;
static int32_t g_fios_fds[FIOS_FD_COUNT];
static unsigned char g_fios_fd_used[FIOS_FD_COUNT];

static fios_stdio_file *find_fios_stdio(FILE *stream)
{
    for (fios_stdio_file *f = g_fios_stdio_files; f; f = f->next)
        if ((FILE *)f == stream) return f;
    return NULL;
}

static int fios_fd_slot(int fd)
{
    int slot = fd - FIOS_FD_BASE;
    return slot >= 0 && slot < FIOS_FD_COUNT && g_fios_fd_used[slot] ? slot : -1;
}

static int fios_readonly_mode(const char *mode)
{
    return mode && mode[0] == 'r' && !strchr(mode, '+') && !strchr(mode, 'w') && !strchr(mode, 'a');
}

FILE *fopen_soloader(const char *filename, const char *mode)
{
    if (strcmp(filename, "/proc/cpuinfo") == 0)
    {
        return fopen_soloader("app0:/cpuinfo", mode);
    }
    else if (strcmp(filename, "/proc/meminfo") == 0)
    {
        return fopen_soloader("app0:/meminfo", mode);
    }

    char remapped[512];
    filename = fix_android_path(filename, remapped, sizeof(remapped));

    if (fios_readonly_mode(mode)) {
        int32_t handle = fios_asset_open(filename);
        if (handle >= 0) {
            fios_stdio_file *f = (fios_stdio_file *)calloc(1, sizeof(*f));
            if (!f) { fios_asset_close(handle); return NULL; }
            f->handle = handle;
            f->next = g_fios_stdio_files;
            g_fios_stdio_files = f;
            l_debug("fopen<psarc>(%s, %s): %p", filename, mode, (FILE *)f);
            return (FILE *)f;
        }
    }

#ifdef USE_SCELIBC_IO
    FILE *ret = sceLibcBridge_fopen(filename, mode);
#else
    FILE *ret = fopen(filename, mode);
#endif

    if (ret)
        l_debug("fopen(%s, %s): %p", filename, mode, ret);
    else
        l_warn("fopen(%s, %s): %p", filename, mode, ret);

    return ret;
}

size_t fread_soloader(void *ptr, size_t size, size_t count, FILE *stream)
{
    fios_stdio_file *f = find_fios_stdio(stream);
    if (!f) {
#ifdef USE_SCELIBC_IO
        return sceLibcBridge_fread(ptr, size, count, stream);
#else
        return fread(ptr, size, count, stream);
#endif
    }
    if (!size || count > (size_t)INT64_MAX / size) return 0;
    int64_t bytes = fios_asset_read(f->handle, ptr, (int64_t)(size * count));
    if (bytes < 0) return 0;
    f->eof = bytes == 0 || (size_t)bytes < size * count;
    return (size_t)bytes / size;
}

int fseek_soloader(FILE *stream, long offset, int whence)
{
    fios_stdio_file *f = find_fios_stdio(stream);
    if (!f) {
#ifdef USE_SCELIBC_IO
        return sceLibcBridge_fseek(stream, offset, whence);
#else
        return fseek(stream, offset, whence);
#endif
    }
    int64_t result = fios_asset_seek(f->handle, offset, whence);
    if (result < 0) return -1;
    f->eof = 0;
    return 0;
}

long ftell_soloader(FILE *stream)
{
    fios_stdio_file *f = find_fios_stdio(stream);
    if (!f) {
#ifdef USE_SCELIBC_IO
        return sceLibcBridge_ftell(stream);
#else
        return ftell(stream);
#endif
    }
    int64_t result = fios_asset_seek(f->handle, 0, SEEK_CUR);
    return result < 0 || result > LONG_MAX ? -1L : (long)result;
}

int feof_soloader(FILE *stream)
{
    fios_stdio_file *f = find_fios_stdio(stream);
    if (!f) {
#ifdef USE_SCELIBC_IO
        return sceLibcBridge_feof(stream);
#else
        return feof(stream);
#endif
    }
    return f->eof;
}

int ferror_soloader(FILE *stream)
{
    if (find_fios_stdio(stream)) return 0;
#ifdef USE_SCELIBC_IO
    return sceLibcBridge_ferror(stream);
#else
    return ferror(stream);
#endif
}

int fgetc_soloader(FILE *stream)
{
    unsigned char ch;
    fios_stdio_file *f = find_fios_stdio(stream);
    if (!f) {
#ifdef USE_SCELIBC_IO
        return sceLibcBridge_fgetc(stream);
#else
        return fgetc(stream);
#endif
    }
    int64_t n = fios_asset_read(f->handle, &ch, 1);
    if (n == 1) return ch;
    f->eof = n == 0;
    return EOF;
}

int getc_soloader(FILE *stream) { return fgetc_soloader(stream); }

char *fgets_soloader(char *buffer, int size, FILE *stream)
{
    fios_stdio_file *f = find_fios_stdio(stream);
    if (!f) {
#ifdef USE_SCELIBC_IO
        return sceLibcBridge_fgets(buffer, size, stream);
#else
        return fgets(buffer, size, stream);
#endif
    }
    if (!buffer || size <= 0) return NULL;
    int i = 0;
    while (i < size - 1) {
        int ch = fgetc_soloader(stream);
        if (ch == EOF) break;
        buffer[i++] = (char)ch;
        if (ch == '\n') break;
    }
    if (!i) return NULL;
    buffer[i] = '\0';
    return buffer;
}

ssize_t read_soloader(int fd, void *buffer, size_t count)
{
    int slot = fios_fd_slot(fd);
    if (slot >= 0) return (ssize_t)fios_asset_read(g_fios_fds[slot], buffer, (int64_t)count);
    return read(fd, buffer, count);
}

int open_soloader(const char *path, int oflag, ...)
{
    if (strcmp(path, "/proc/cpuinfo") == 0)
    {
        return open_soloader("app0:/cpuinfo", oflag);
    }
    else if (strcmp(path, "/proc/meminfo") == 0)
    {
        return open_soloader("app0:/meminfo", oflag);
    }
    else if (strcmp(path, "/dev/urandom") == 0)
    {
        return open_soloader("app0:/urandom", oflag);
    }

    mode_t mode = 0666;
    if (((oflag & BIONIC_O_CREAT) == BIONIC_O_CREAT) ||
        ((oflag & BIONIC_O_TMPFILE) == BIONIC_O_TMPFILE))
    {
        va_list args;
        va_start(args, oflag);
        mode = (mode_t)(va_arg(args, int));
        va_end(args);
    }

    char remapped[512];
    path = fix_android_path(path, remapped, sizeof(remapped));
    if ((oflag & (BIONIC_O_WRONLY | BIONIC_O_RDWR | BIONIC_O_CREAT | BIONIC_O_TRUNC | BIONIC_O_APPEND | BIONIC_O_TMPFILE)) == 0) {
        int32_t handle = fios_asset_open(path);
        if (handle >= 0) {
            for (int i = 0; i < FIOS_FD_COUNT; i++) {
                if (!g_fios_fd_used[i]) {
                    g_fios_fd_used[i] = 1;
                    g_fios_fds[i] = handle;
                    return FIOS_FD_BASE + i;
                }
            }
            fios_asset_close(handle);
            return -1;
        }
    }

    oflag = oflags_bionic_to_newlib(oflag);
    int ret = open(path, oflag, mode);
    if (ret >= 0)
        l_debug("open(%s, %x): %i", path, oflag, ret);
    else
        l_warn("open(%s, %x): %i", path, oflag, ret);
    return ret;
}

int fstat_soloader(int fd, stat64_bionic *buf)
{
    struct stat st;
    int res = fstat(fd, &st);

    if (res == 0)
        stat_newlib_to_bionic(&st, buf);

    l_debug("fstat(%i): %i", fd, res);
    return res;
}

int stat_soloader(const char *path, stat64_bionic *buf)
{
    if (strcmp(path, "/system/lib/libOpenSLES.so") == 0)
    {
        l_debug("stat(%s): returning 0 in case this is a check for OpenSLES support", path);
        return 0;
    }

    struct stat st;
    int res = stat(path, &st);

    if (res == 0)
        stat_newlib_to_bionic(&st, buf);

    l_debug("stat(%s): %i", path, res);
    return res;
}

int fclose_soloader(FILE *f)
{
    fios_stdio_file **link = &g_fios_stdio_files;
    while (*link) {
        if ((FILE *)*link == f) {
            fios_stdio_file *entry = *link;
            *link = entry->next;
            int ret = fios_asset_close(entry->handle);
            free(entry);
            return ret;
        }
        link = &(*link)->next;
    }
#ifdef USE_SCELIBC_IO
    int ret = sceLibcBridge_fclose(f);
#else
    int ret = fclose(f);
#endif

    l_debug("fclose(%p): %i", f, ret);
    return ret;
}

int close_soloader(int fd)
{
    int slot = fios_fd_slot(fd);
    if (slot >= 0) {
        int ret = fios_asset_close(g_fios_fds[slot]);
        g_fios_fd_used[slot] = 0;
        return ret;
    }
    int ret = close(fd);
    l_debug("close(%i): %i", fd, ret);
    return ret;
}

DIR *opendir_soloader(char *_pathname)
{
    DIR *ret = opendir(_pathname);
    l_debug("opendir(\"%s\"): %p", _pathname, ret);
    return ret;
}

struct dirent64_bionic *readdir_soloader(DIR *dir)
{
    static struct dirent64_bionic dirent_tmp;

    struct dirent *ret = readdir(dir);
    l_debug("readdir(%p): %p", dir, ret);

    if (ret)
    {
        dirent64_bionic *entry_tmp = dirent_newlib_to_bionic(ret);
        memcpy(&dirent_tmp, entry_tmp, sizeof(dirent64_bionic));
        free(entry_tmp);
        return &dirent_tmp;
    }

    return NULL;
}

int readdir_r_soloader(DIR *dirp, dirent64_bionic *entry,
                       dirent64_bionic **result)
{
    struct dirent dirent_tmp;
    struct dirent *pdirent_tmp;

    int ret = readdir_r(dirp, &dirent_tmp, &pdirent_tmp);

    if (ret == 0)
    {
        dirent64_bionic *entry_tmp = dirent_newlib_to_bionic(&dirent_tmp);
        memcpy(entry, entry_tmp, sizeof(dirent64_bionic));
        *result = (pdirent_tmp != NULL) ? entry : NULL;
        free(entry_tmp);
    }

    l_debug("readdir_r(%p, %p, %p): %i", dirp, entry, result, ret);
    return ret;
}

int closedir_soloader(DIR *dir)
{
    int ret = closedir(dir);
    l_debug("closedir(%p): %i", dir, ret);
    return ret;
}

int fcntl_soloader(int fd, int cmd, ...)
{
    l_warn("fcntl(%i, %i, ...): not implemented", fd, cmd);
    return 0;
}

int ioctl_soloader(int fd, int request, ...)
{
    l_warn("ioctl(%i, %i, ...): not implemented", fd, request);
    return 0;
}

int fsync_soloader(int fd)
{
    int ret = fsync(fd);
    l_debug("fsync(%i): %i", fd, ret);
    return ret;
}

_off64_t lseek64_soloader(int fd, _off64_t offset, int whence)
{
    int slot = fios_fd_slot(fd);
    if (slot >= 0) return (_off64_t)fios_asset_seek(g_fios_fds[slot], offset, whence);
    _off64_t ret = lseek(fd, offset, whence);
    l_debug("lseek64(%i, %lld, %i): %lld", fd, (long long)offset, whence, (long long)ret);
    return ret;
}

off_t lseek_soloader(int fd, off_t offset, int whence)
{
    return (off_t)lseek64_soloader(fd, (_off64_t)offset, whence);
}
