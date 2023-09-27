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

#include "q_fs_layer.hpp"
#include "q_dedupe.hpp"
#include "q_log.hpp"

class qube_fuse : public fuse_operations, public q_dedupe , public qube_FS {

	public:
		static struct fuse_args args;
		static fuse_fill_dir_flags fill_dir_plus;
        static struct fuse_operations qfs_operations_;
		static struct qfs_state *qfs_data;
		static std::string root_dir;
		static char *last_full_path;

 		qube_fuse() {
			// set the methods of the fuse_operations struct to the methods of the QubeFileSystem class
        	qfs_operations_.getattr         = &qube_fuse::qfs_getattr;
			qfs_operations_.readlink        = &qube_fuse::qfs_readlink;
			qfs_operations_.mknod	        = &qube_fuse::qfs_mknod;
			qfs_operations_.mkdir	        = &qube_fuse::qfs_mkdir;
			qfs_operations_.unlink          = &qube_fuse::qfs_unlink;
			qfs_operations_.rmdir	        = &qube_fuse::qfs_rmdir;
			qfs_operations_.symlink         = &qube_fuse::qfs_symlink;
			qfs_operations_.rename          = &qube_fuse::qfs_rename; 
			qfs_operations_.link            = &qube_fuse::qfs_link; 
			qfs_operations_.chmod           = &qube_fuse::qfs_chmod; 
			qfs_operations_.chown           = &qube_fuse::qfs_chown; 
			qfs_operations_.truncate        = &qube_fuse::qfs_truncate; 
			qfs_operations_.open            = &qube_fuse::qfs_open; 
			qfs_operations_.read	        = &qube_fuse::qfs_read;
			qfs_operations_.write	        = &qube_fuse::qfs_write;
			qfs_operations_.statfs          = &qube_fuse::qfs_statfs;
			qfs_operations_.flush           = &qube_fuse::qfs_flush;
			qfs_operations_.release         = &qube_fuse::qfs_release;
			qfs_operations_.fsync           = &qube_fuse::qfs_fsync;
			qfs_operations_.getxattr        = &qube_fuse::qfs_getxattr;
			qfs_operations_.setxattr        = &qube_fuse::qfs_setxattr;
			qfs_operations_.listxattr       = &qube_fuse::qfs_listxattr;
			qfs_operations_.removexattr     = &qube_fuse::qfs_removexattr;
			qfs_operations_.opendir         = &qube_fuse::qfs_opendir;
			qfs_operations_.readdir         = &qube_fuse::qfs_readdir;
			qfs_operations_.releasedir      = &qube_fuse::qfs_releasedir;
			qfs_operations_.fsyncdir        = &qube_fuse::qfs_fsyncdir;
			qfs_operations_.destroy         = &qube_fuse::qfs_destroy;
			qfs_operations_.access          = &qube_fuse::qfs_access;
			qfs_operations_.create          = &qube_fuse::qfs_create;
			qfs_operations_.lock            = &qube_fuse::qfs_lock;
			qfs_operations_.utimens         = &qube_fuse::qfs_utimens;
			qfs_operations_.bmap            = &qube_fuse::qfs_bmap;
			qfs_operations_.ioctl           = &qube_fuse::qfs_ioctl;
			qfs_operations_.poll            = &qube_fuse::qfs_poll;
//			qfs_operations_.write_buf       = &qube_fuse::qfs_write_buf;
//			qfs_operations_.read_buf        = &qube_fuse::qfs_read_buf;
			qfs_operations_.flock           = &qube_fuse::qfs_flock;
			qfs_operations_.fallocate       = &qube_fuse::qfs_fallocate;
			qfs_operations_.init			= &qube_fuse::qfs_init;
		}

		~qube_fuse() {
			close(settings.src_fd);
		}

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
		static q_log *QLOG;
		static int mknod_wrapper(int dirfd, const char *path, const char *link, int mode, dev_t rdev);



};
/**
struct fuse_args qube_fuse::args = {};
struct qfs_state* qube_fuse::qfs_data = {};
struct fuse_operations qube_fuse::qfs_operations_ = {};
fuse_fill_dir_flags qube_fuse::fill_dir_plus = {};
std::string root_dir = "";
char* qube_fuse::last_full_path = NULL;
**/