/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */


#pragma once

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
#include <cstdint>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>

// Simple Macros
// & Constants
//#############################
#define BLOCK_SIZE 4096
#define HASH_SIZE 128   // The key we generate is 512 bits long. We can reduce that to 128 chars hex or 64 char string. Hex is best. Longer but better.
                        // So, here for each 4096 block that's identical, we will reduce to 128 hex chars.

#define NO_HASH_S "NO_ENC_HASHES_RETURNED"

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

struct Settings {
    const char *progname;
    std::string *root_dir;  //Source dir of the actual data
    char *mount_point;     // Where it's mounted to
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

};
  