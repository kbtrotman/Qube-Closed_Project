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
#include <sys/xattr.h>

// C++ incs
#include <cstring>
#include <string>
#include <iostream>
#include <cstdio>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>

// Constants
//#############################
#define BLOCK_SIZE 4096
#define HASH_SIZE (2 * SHA512_DIGEST_LENGTH)

#define NO_HASH_S "NO_ENC_HASHES_RETURNED"

// Database configuration
//#############################
#define DATABASE "fuse"
#define USER "postgres"
#define PASSWORD "Postgres!909"
#define HOST "10.100.122.20"
#define PORT "5432"

#define NO_RECORD_S "NO_DB_RECORDS_RETURNED"

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


/* SETTINGS */
static struct Settings {
    const char *progname;
    std::string *root_dir;
//    std::string *source_dir;
    char *mount_point;  //Mount point is really the same as root_dir?? Is this redundant, or can te be different?
    int src_len;
    int mnt_len; /* caches strlen(mntdest) */
    int src_fd;
    int mnt_fd;
    int file_fd;

    struct permchain *permchain; /* permission bit rules. see permchain.h */
    uid_t new_uid; /* user-specified uid */
    gid_t new_gid; /* user-specified gid */
    uid_t create_for_uid;
    gid_t create_for_gid;
    char *original_working_dir;
    mode_t original_umask;

    UserMap *usermap; /* From the --map option. */
    UserMap *usermap_reverse;

    enum CreatePolicy {
        CREATE_AS_USER,
        CREATE_AS_MOUNTER
    } create_policy;

    struct permchain *create_permchain; /* the --create-with-perms option */

    enum ChownPolicy {
        CHOWN_NORMAL,
        CHOWN_IGNORE,
        CHOWN_DENY
    } chown_policy;

    enum ChgrpPolicy {
        CHGRP_NORMAL,
        CHGRP_IGNORE,
        CHGRP_DENY
    } chgrp_policy;

    enum ChmodPolicy {
        CHMOD_NORMAL,
        CHMOD_IGNORE,
        CHMOD_DENY
    } chmod_policy;

    int chmod_allow_x;

    struct permchain *chmod_permchain; /* the --chmod-filter option */

    enum XAttrPolicy {
        XATTR_UNIMPLEMENTED,
        XATTR_READ_ONLY,
        XATTR_READ_WRITE
    } xattr_policy;

    int delete_deny;
    int rename_deny;

    int mirrored_users_only;
    uid_t *mirrored_users;
    int num_mirrored_users;
    gid_t *mirrored_members;
    int num_mirrored_members;

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

    