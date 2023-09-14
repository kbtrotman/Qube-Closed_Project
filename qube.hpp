/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

#define FUSE_USE_VERSION 31

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Legacy C incs
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

// C++ incs
#include <cstring>
#include <string>
#include <iostream>
#include <cstdio>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>

// Simple Macros
// & Constants
//#############################
#define BLOCK_SIZE 4096
#define HASH_SIZE 128   // The hexidecimal hash size is twice as long as the 64-char key (512-bit key).

#define NO_HASH_S "NO_ENC_HASHES_RETURNED"

#define TRACE _qlog->trace
#define DEBUG _qlog->debug
#define INFO _qlog->info
#define WARN _qlog->warn
#define ERROR _qlog->error
#define CRITICAL _qlog->critical
#define FLUSH _qlog->flush()

#define PRINT_CONN_STRING(s) \
    ([](const auto& s){ \
        std::ostringstream oss; \
        oss << "proto_major: " << s->proto_major << ", proto_minor: " << s->proto_minor; \
        return oss.str(); \
    })(s)

#define PRINT_STBUFF_STRING(s) \
    ([](const auto& s){ \
        std::ostringstream oss; \
        oss << "st_mode: " << s->st_mode << ", st_uid: " << s->st_uid << ", st_size " << s->st_size << ", st_atime: " << s->st_atime; \
        return oss.str(); \
    })(s)


// Database Constants
//#############################
#define DATABASE "fuse"
#define USER "postgres"
#define PASSWORD "Postgres!909"
#define HOST "10.100.122.20"
#define PORT "5432"

#define NO_RECORDS 0x099

#define DBSTRING "user=" USER " dbname=" DATABASE " password=" PASSWORD " hostaddr=" HOST " port=" PORT

struct UserMap {
    uid_t *user_from;
    uid_t *user_to;
    gid_t *group_from;
    gid_t *group_to;
    int user_capacity;
    int group_capacity;
    int user_size;
    int group_size;
};


// SETTINGS & Config stuff
//############################

static struct Settings {
    const char *progname;
    std::string *root_dir;
    char *mount_point;  //Mount point is really the same as root_dir?? Is this redundant, or can they be different?
    int src_len;
    int mnt_len; /* caches strlen(mntdest) */
    int src_fd;
    int mnt_fd;
    int file_fd;

    int hide_hard_links;
    int resolve_symlinks;
    int realistic_permissions;
    int ctime_from_mtime;
    int enable_lock_forwarding;
    int enable_ioctl;
    bool direct_io;

    uid_t uid_offset;
    gid_t gid_offset;

} settings;
  