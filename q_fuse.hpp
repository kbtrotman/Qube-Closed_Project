/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 **/

#pragma once

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <fuse3/fuse_opt.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "qube.hpp"
#include "q_fs_layer.hpp"
#include "q_dedupe.hpp"
#include "q_log.hpp"

class q_fuse : public fuse_operations, public q_dedupe , public q_FS {

	public:
		static struct fuse_args args;
		static fuse_fill_dir_flags fill_dir_plus;
        static struct fuse_operations qfs_operations_;
		static struct qfs_state *qfs_data;
		static std::string root_dir;
		static char *last_full_path;

 		q_fuse(q_log& q);

		~q_fuse();
		void print_usage();
		int run(int argc, char* argv[]);

		/*

		Fuse Over-rides - To make it a real FS, these would be the layer to start at--the fuse inclusions--and go downward to the hardware layer. 
		This isn't nearly as hard as it sounds. It's easier to test on fuse and then migrate to physical disk sectors later. This can be done by
		migrating the code to being a device-driver that loads a filesystem module.

		All globally static methods below this point. They are interrupt-driven......

		*/

		static void *qfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg);
		static int qfs_getattr( const char *path, struct stat *stbuf, struct fuse_file_info *fi );
		static int qfs_readlink (const char *path, char *buf, size_t size);
		static int qfs_mknod( const char *path, mode_t mode, dev_t rdev );
		static int qfs_mkdir( const char *path, mode_t mode );
		static int qfs_unlink (const char *path);
		static int qfs_rmdir (const char *dirname);
		static int qfs_symlink(const char *from, const char *to);
		static int qfs_rename(const char *path, const char *c, unsigned int flags);
		static int qfs_link(const char *path, const char *linkname);
		static int qfs_chmod(const char *path, mode_t mode, struct fuse_file_info *fi);
		static int qfs_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi);
		static int qfs_truncate(const char *path, off_t offset, struct fuse_file_info *fi);
		static int qfs_open(const char *path, struct fuse_file_info *fi);
		static int qfs_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi );
		static int qfs_write( const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *info );
		static int qfs_statfs (const char *path, struct statvfs *stbuf);
		static int qfs_flush(const char *path, struct fuse_file_info *fi);
		static int qfs_release(const char *path, struct fuse_file_info *fi);
		static int qfs_fsync(const char *path, int sync, struct fuse_file_info *fi);
		static int qfs_setxattr (const char *path, const char *name, const char *val, size_t size, int flags);
		static int qfs_getxattr (const char *path, const char *name, char *value, size_t size);
		static int qfs_listxattr(const char *path, char *attr, size_t size);
		static int qfs_removexattr(const char *path, const char *attr);
		static int qfs_opendir(const char *path, struct fuse_file_info *fi);
		static int qfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, 
				enum fuse_readdir_flags flags);
		static int qfs_releasedir(const char *path, struct fuse_file_info *fi);
		static int qfs_fsyncdir(const char *path, int, struct fuse_file_info *fi);
		static void qfs_destroy(void *private_data);
		static int qfs_access(const char *path, int i);
		static int qfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
		static int qfs_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *my_lock);
		static int qfs_utime(const char *path, const struct timespec *ts, struct fuse_file_info *fi);
		static int qfs_utimens(const char *path, const struct timespec *ts, struct fuse_file_info *fi);
		static int qfs_bmap(const char *path, size_t blocksize, uint64_t *idx);
		static int qfs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi, unsigned int flags, void *data);
		static int qfs_poll(const char *path, struct fuse_file_info *fi, struct fuse_pollhandle *ph, unsigned *reventsp);
		static int qfs_flock(const char *path, struct fuse_file_info *fi, int op);
		static int qfs_fallocate(const char *path, int i, off_t off1, off_t off2, struct fuse_file_info *fi);


	// These obviously are not meant to be accessible outside the class ------------------>
	private:
		static int mknod_wrapper(int dirfd, const char *path, const char *link, int mode, dev_t rdev);



};