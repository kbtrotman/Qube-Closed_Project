/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

#include <fuse3/fuse.h>
#include <fuse3/fuse_opt.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "q_fs_layer.hpp"

class qube_fuse : public fuse_operations, public qube_hash, public qube_FS {

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
			qfs_operations_.write_buf       = &qube_fuse::qfs_write_buf;
			qfs_operations_.read_buf        = &qube_fuse::qfs_read_buf;
			qfs_operations_.flock           = &qube_fuse::qfs_flock;
			qfs_operations_.fallocate       = &qube_fuse::qfs_fallocate;
			qfs_operations_.init			= &qube_fuse::qfs_init;
		}

		~qube_fuse() {
			close(settings.src_fd);
		}

		void print_usage() {
			std::cout << "usage: ./" << settings.progname << " [FUSE and mount options] <devicesourcepath> <mountPoint>";
			abort();
		}

		int run(int argc, char* argv[]) {
			_qlog->debug("QUBE_FUSE::run---[{:d}]---[{}]---[{}]--->",argc, argv[argc-1], argv[argc-2]);
			settings.progname = "qubeFS";
			/* Check that a source directory and a mount point was given */
 			if ( (argc < 3) || (argv[argc-1][0] == '-') || (argv[argc-2][0] == '-') ) {
				print_usage();
				return 1;
			}

			settings.mount_point = argv[argc-1];
			argv[argc-1] = NULL;
			argc--;
			settings.root_dir = new std::string(argv[argc-1]);
    		argv[argc-1] = settings.mount_point;
			//argc--;
			_qlog->debug("QUBE_FUSE::run: progname: {}  Root_dir: {}  Mount_point: {}.", settings.progname, settings.root_dir->c_str(), settings.mount_point);
			_qlog->flush();
			settings.src_len = settings.root_dir->length();
			settings.mnt_len = strlen(settings.mount_point);

			struct stat st;

			if (::stat(settings.mount_point, &st) == -1) {
				_qlog->error("QUBE_FUSE::run: Error: The mount point [{}] doesn't exist.", settings.mount_point);
				perror("stat");
				return 1;
			}
			if (!(st.st_mode & S_IFDIR)) {
				_qlog->error("QUBE_FUSE::run: Error: The mount point [{}] is not a directory.", settings.mount_point);
				return 1;
			}
			if (::access(settings.mount_point, W_OK) == -1) {
				_qlog->error("QUBE_FUSE::run: Error: The mount point [{}] is not accessible.", settings.mount_point);
				perror("access");
				return 1;
			}
			_qlog->info("QUBE_FUSE::run: Setting command-line defaults in [run] method with source=[{}] and mount=[{}]", settings.root_dir->c_str(), settings.mount_point);
			_qlog->flush();

			qfs_data = (qfs_state*)malloc(sizeof(struct qfs_state));
			if (qfs_data == NULL) {
				perror("main calloc");
				abort();
			}

			fuse_args args = FUSE_ARGS_INIT(argc, argv);

			/* Open mount root for chrooting */
    		settings.src_fd = ::open(settings.root_dir->c_str(), O_RDONLY);
    		if (settings.src_fd == -1) {
        		_qlog->error("QUBE_FUSE::run: Could not open source directory: {}.", settings.root_dir->c_str());
        		return 1;
    		}

			if (fchdir(settings.src_fd) == 0) {
       			_qlog->debug("QUBE_FUSE::run: Current working directory changed to {}.", settings.root_dir->c_str());
    		} else {
        		_qlog->error("QUBE_FUSE::run: Failed to change working directory to {}. Are we on the right file path?", settings.root_dir->c_str());
    		}

			_qlog->debug("QUBE_FUSE::run---[Leaving]--->.");
			_qlog->flush();
  			return fuse_main(args.argc, args.argv, &qfs_operations_ , qfs_data);
		}

		/*

		Fuse Over-rides - To make it a real FS, these would be the layer to start at--the fuse inclusions--and go downward to the hardware layer. 
		This isn't nearly as hard as it sounds. It's easier to test on fuse and then migrate to physical disk sectors later. This can be done by
		migrating the code to being a device-driver that loads a filesystem module.

		*/
		static void *qfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg){
				_qlog->debug("QUBE_FUSE::qfs_init:---{}--->",  PRINT_CONN_STRING(conn) );

				cfg->use_ino = 0;
				// Disable caches so changes in base FS are visible immediately.
				// Especially the attribute cache must be disabled when different users
				// might see different file attributes, such as when mirroring users.
				cfg->entry_timeout = 0;
				cfg->attr_timeout = 0;
				cfg->negative_timeout = 0;
				//cfg->direct_io = settings.direct_io;

				_qlog->debug("QUBE_FUSE::qfs_init:---[Leaving]--->");
				_qlog->flush();
    			return QFS_DATA;
		}

		static int qfs_getattr( const char *path, struct stat *stbuf, struct fuse_file_info *fi ) {
			_qlog->debug("QUBE_FUSE::qfs_getattr---[{}]--->", path);
			int retstat = 0;

			last_full_path = qfs_get_root_path(path);
			_qlog->debug("QUBE_FUSE::qfs_getattr: Fullpath={} and path={}.", last_full_path, path);

			if (::lstat(last_full_path, stbuf) == -1) {
				_qlog->error("QUBE_FUSE::qfs_getattr: Failed to open path, Fullpath={} and path={}.", last_full_path, path);
				retstat = -errno;
			} else {
				_qlog->debug("QUBE_FUSE::qfs_getattr: Stat-ed, Fullpath={} and path={}.", last_full_path, path);
				retstat = 0;
			}

			_qlog->debug("QUBE_FUSE::qfs_getattr: {}", PRINT_STBUFF_STRING(stbuf) );
			_qlog->debug("QUBE_FUSE::qfs_getattr---[Leaving]---[retstat={:d}]>", retstat);
			_qlog->flush();
			return retstat;
		}

		static int qfs_readlink (const char *path, char *buf, size_t size) {
			_qlog->debug("QUBE_FUSE::qfs-readlink---[{}]---[{}]---[{:f}]--->", path, buf, size );
			int res;

			last_full_path = qfs_get_root_path(path);
			res = ::readlink(last_full_path, buf, size - 1);
			if (res == -1){
				buf[0] = '\0';
				_qlog->error("QUBE_FUSE::qfs_readlink: Failed to read link path, Fullpath={} and path={}.", last_full_path, path);
				res = -errno;
			} else {
				_qlog->error("QUBE_FUSE::qfs_readlink: Opened for read->link path, Fullpath={} and path={}.", last_full_path, path);
				res = 0;
			}

			_qlog->debug("QUBE_FUSE::qfs_readlink---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_mknod( const char *path, mode_t mode, dev_t rdev ) {
			_qlog->debug("QUBE_FUSE::qfs_mknod---[{}]---[{:f}]---[{:f}]--->", path, mode, rdev );
			int res;

			res = mknod_wrapper(AT_FDCWD, path, NULL, mode, rdev);
			if (res == -1)
				return -errno;

			_qlog->debug("QUBE_FUSE::qfs_mknod---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_mkdir( const char *path, mode_t mode )
		{
			_qlog->debug("QUBE_FUSE:qfs_mkdir---[{}]---[{:f}]--->", path, mode );
			int res;

			res = ::mkdir(path, mode);
			if (res == -1)
				return -errno;

			_qlog->debug("QUBE_FUSE::qfs_mkdir---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_unlink (const char *path) {
			_qlog->debug("QUBE_FUSE::qfs_unlink---[{}]--->", path );
			int res;

			last_full_path = qfs_get_root_path(path);
			res = ::unlink(last_full_path);
			if (res == -1){
				_qlog->error("QUBE_FUSE::qfs_unlink: Failed to un-link path, Fullpath={} and path={}.", last_full_path, path);
				res = -errno;
			} else {
				_qlog->error("QUBE_FUSE::qfs_unlink: Unlinked (deleted)->link path, Fullpath={} and path={}.", last_full_path, path);
				res = 0;
			}

			_qlog->debug("QUBE_FUSE::qfs_unlink---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_rmdir (const char *dirname) {
			return 0;
		}

		static int qfs_symlink(const char *from, const char *to) {
			_qlog->debug("QUBE_FUSE::qfs_symlink---[{}]---[{}]--->", *from, *to );
			int res;

			res = ::symlink(from, to);
			if (res == -1)
				return -errno;

			_qlog->debug("QUBE_FUSE::qfs_symlink---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_rename(const char *path, const char *c, unsigned int flags) {
			_qlog->debug("QUBE_FUSE::qfs_rename---[{}]---[{}]---[{:d}]--->", *path, *c, flags );
			return 0;
		}

		static int qfs_link(const char *path, const char *linkname) {
			_qlog->debug("QUBE_FUSE::qfs_link---[{}]---[{}]--->", path, linkname );

			return 0;
			_qlog->debug("QUBE_FUSE::qfs_link---[Leaving]--->");
		}
		
		static int qfs_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {return 0;} 
		static int qfs_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {return 0;}
		static int qfs_truncate(const char *path, off_t offset, struct fuse_file_info *fi) {return 0;}

		static int qfs_open(const char *path, struct fuse_file_info *fi) {
			_qlog->debug("QUBE_FUSE::qfs_open---[{}]--->", path );
			int local_fh;
			int res;

			last_full_path = qfs_get_root_path(path);
			_qlog->debug("QUBE_FUSE::qfs_open: Fullpath={} and path={}.", last_full_path, path);
			local_fh = ::open(last_full_path, fi->flags);
			if (local_fh == -1){
				_qlog->error("QUBE_FUSE::qfs_create: Failed to open path [{}] with flags [{:d}].", path, fi->flags);
				res = -errno;
			} else {
				_qlog->debug("QUBE_FUSE::qfs_create: opened path [{}] with result [{:d}].", path, local_fh);
				res = 0;
			}
			fi->fh = local_fh;
			_qlog->debug("QUBE_FUSE::qfs_open---[Leaving]--->");
			return res;
		}

		static int qfs_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
		{
			_qlog->debug("QUBE_FUSE:qfs_read---[{}]---[{}]---[{:f}]---[{:f}]--->", path, *buffer, size, offset );
			_qlog->flush();
			int fd;
			int res;
			int original_offset;
			int original_size;
			std::string tmpbuffer("");
			std::string block_hash = "";
			std::string actual_contents = "";
			const char *fpath = nullptr;

			fpath = qfs_get_root_path(path);
			_qlog->debug("Fullpath={} and path={}.", fpath, *path);
			_qlog->flush();

			if(fi == NULL)
				fd = ::open(fpath, O_RDONLY);
			else
				fd = fi->fh;
	
			if (fd == -1)
				return -errno;

			// Change our hashes in the file into blocks from the DB....
			original_offset = offset;
			original_size = size;
			offset = (offset * HASH_SIZE) / BLOCK_SIZE;
			size = int((size * HASH_SIZE) / BLOCK_SIZE);
			::lseek(fd, offset, SEEK_SET);
			res = pread(fd, &tmpbuffer, size, offset);
			if (res == -1)
				res = -errno;
			if(fi == NULL)
				close(fd);
			for (int i=0; i < (int((tmpbuffer.length() / HASH_SIZE))); i++) {
				block_hash = tmpbuffer.substr((i * HASH_SIZE), ((i + 1) * HASH_SIZE));
				actual_contents += qpsql_get_block_from_hash(block_hash);
			}
			size = original_size;
			offset = original_offset;
			_qlog->debug("read contents: {} ", actual_contents);
			_qlog->debug("{:d} Bytes of data read to buffer with qubeFS filepath {} and filehandle {:d}.", size, fpath, fd);
			_qlog->debug("QUBE_FUSE::qfs_read---[Leaving]--->");
			_qlog->flush();
			return actual_contents.length();
		}

		static int qfs_write( const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *info )
		{
			_qlog->debug("QUBE_FUSE::qfs_Write---[{}]---[{}]---[{:f}]---[{:f}]-->", *path, *buf, size, offset );
			_qlog->flush();
			double original_offset;
			std::string block_hash;
			std::string hash_buffer;
			std::string cur_block;
			std::string buffer(buf);
			const char *fpath = nullptr;

			fpath = qfs_get_root_path(path);
			_qlog->debug("Fullpath={} and path={}.", fpath, path);
			_qlog->flush();
       		if (buffer.empty()) {
            	_qlog->debug("Exiting qfs_write with no data to write to disk/DB: '{:d}'.", buffer.length());
            	return 0;
			} else {
            	_qlog->debug("Incoming binary buffer: fpath: {}---buffer: {}---size: {:f}---offset: {f}--->", fpath, buffer, size, offset);
				original_offset = offset;
			}

        	//# Generate the test hashes
        	//###########################
        	offset = (offset * HASH_SIZE) / BLOCK_SIZE;

        	for (int i=0; i < std::ceil(buffer.length() / BLOCK_SIZE); i++){
            	_qlog->debug("# of Times through hash loop: {:d} ",i);
				cur_block = buffer.substr(i * BLOCK_SIZE, (i + 1) * BLOCK_SIZE);
            	block_hash = qube_hash::get_sha512_hash(cur_block);
            	_qlog->debug("Hash that was generated: {}", block_hash);
            	hash_buffer += block_hash;
				_qlog->debug("Appended hash: {} to hash_buffer: {}.", block_hash, hash_buffer);
            	//# Check if the hash exists
            	//##########################
                if ( qpsql_get_block_from_hash(block_hash) == "" ) {
					//hash doesn't exist, save it...
					qpsql_insert_hash(block_hash, cur_block);
					_qlog->debug("Saved to DB: hash: {} Block: {}", block_hash, cur_block);
				}
			}

			//Write the results to the file we're modifying...
			_qlog->debug("End Hash Buffer: {}", hash_buffer);
			size = write_to_file( fpath, buffer.c_str(), hash_buffer.length(), offset );
			_qlog->debug("{} Bytes of data Written from buffer of length {} to qubeFS filename {} and filehandle {}.",
				size, buffer.length(), fpath);
			offset = original_offset;
			_qlog->debug("QUBE_FUSE::qfs_write---[Leaving]--->");
			_qlog->flush();
			return size;
		}

		static int qfs_statfs (const char *path, struct statvfs *stbuf) {

			_qlog->debug("QUBE_FUSE::qfs_statfs---[{}]--->", path );
			_qlog->flush();
			int res;

			last_full_path = qfs_get_root_path(path);
			_qlog->debug("QUBE_FUSE::qfs_statfs: Fullpath={} and path={}.", last_full_path, path);
			res = ::statvfs(last_full_path, stbuf);
			if (res == -1)
				return -errno;

			_qlog->debug("QUBE_FUSE::qfs_statfs---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_flush(const char *path, struct fuse_file_info *fi) {
			_qlog->debug("QUBE_FUSE::qfs_flush---[{}]--->", path);

			_qlog->debug("QUBE_FUSE::qfs_flush---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_release(const char *path, struct fuse_file_info *fi) {
			_qlog->debug("QUBE_FUSE::qfs_release---[{}]--->", path);
			(void) path;

			::close(fi->fh);
			_qlog->debug("QUBE_FUSE::qfs_release---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_fsync(const char *path, int sync, struct fuse_file_info *fi) {
			_qlog->trace("QUBE_FUSE::qfs_fsync---[{}]--->", path);
			::fsync(fi->fh);
			_qlog->trace("QUBE_FUSE::qfs_fsync---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_setxattr (const char *path, const char *name, const char *val, size_t size, int flags) {
			_qlog->trace("QUBE_FUSE::qfs_setxattr---[{}]---[{}]---[{:f}]---[{:f}]-->", *path, *name, *val, size );

			_qlog->trace("QUBE_FUSE::qfs_setxattr---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_getxattr (const char *path, const char *name, char *value, size_t size) {
			_qlog->trace("QUBE_FUSE::qfs_getxattr---[{}]---[{}]---[{:d}]>", path, name, size );
			int res;

			last_full_path = qfs_get_root_path(path);
			_qlog->debug("QUBE_FUSE::qfs_open: Fullpath={} and path={}.", last_full_path, path);
			res = ::lgetxattr(last_full_path, name, value, size);
			if (res == -1) {
				_qlog->error("QUBE_FUSE::qfs_getxattr: Error in getting xattr from full path [{}] name [{}] and size [{:d}]. Bailing.", last_full_path, name, size);
				res = -errno;
			} else {
				_qlog->debug("QUBE_FUSE::qfs_getxattr: Returned xattr from full path [{}] name [{}] and size [{:d}]. Bailing.", last_full_path, name, size);
				res = 0;
			}

			_qlog->trace("QUBE_FUSE::qfs_getxattr---[Leaving]--->");
			_qlog->flush();
			return res;
		}

		static int qfs_listxattr(const char *path, char *attr, size_t size) {
			_qlog->trace("QUBE_FUSE::qfs_listxattr---[{}]---[{}]---[{:d}]--->", *path, *attr, size );
			return 0;
			_qlog->trace("QUBE_FUSE::qfs_listxattr---[Leaving]--->");
			_qlog->flush();
		}

		static int qfs_removexattr(const char *path, const char *attr) {
			_qlog->trace("QUBE_FUSE::qfs_removexattr---[{}]---[{}]--->", *path, *attr );
			return 0;
			_qlog->trace("QUBE_FUSE::qfs_removexattr---[Leaving]--->");
			_qlog->flush();
		}
		
		static int qfs_opendir(const char *path, struct fuse_file_info *fi) {
			_qlog->trace("QUBE_FUSE::qfs_opendir---[{}]--->", path);
			DIR *local_fh;
			int res;

			last_full_path = qfs_get_root_path(path);
			_qlog->debug("QUBE_FUSE::qfs_opendir: Fullpath={} and path={}.", last_full_path, path);
			local_fh = ::opendir(last_full_path);
			if (local_fh == NULL){
				_qlog->error("QUBE_FUSE::qfs_opendir: Failed to open Full path {}, path [{}].", last_full_path, path);
				res = -errno;
			} else {
				_qlog->debug("QUBE_FUSE::qfs_opendir: opened path [{}] with result [{}].", last_full_path, path);
				res = 0;
			}
			fi->fh = (intptr_t)local_fh;

			_qlog->trace("QUBE_FUSE::qfs_opendir---[Leaving]--->");
			_qlog->flush();
			return res;
		}

		static int qfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, 
				enum fuse_readdir_flags flags) {

			_qlog->debug("QUBE_FUSE::qfs_readdir---[{}]--->", path );
			(void) offset;
    		(void) fi;
			DIR *dp;
			struct dirent *de;
			fuse_fill_dir_flags fdflags;
			fdflags = fuse_fill_dir_flags::FUSE_FILL_DIR_PLUS;

			last_full_path = qfs_get_root_path(path);
			_qlog->debug("QUBE_FUSE::qfs_readdir: Fullpath={} and path={}", last_full_path, path);

			dp = (DIR *) (uintptr_t)fi->fh;     //<------Needs to be changed to qfs_opendir.............
			if (dp == NULL) {
				_qlog->error("QUBE_FUSE::qfs_readdir: Failed to get directory info: {}.", last_full_path);
				return -errno;
			}
			int r = 0;

			while ((de = ::readdir(dp)) != NULL) {
				r++;
				_qlog->debug("QUBE_FUSE::qfs_readdir: Adding entry r:{} with name: {} from dir list into buffer.", r, de->d_name);
				struct stat st = {};
				st.st_ino = de->d_ino;
				st.st_mode = de->d_type << 12;
				last_full_path = qfs_get_root_path(de->d_name);
				if (::lstat(last_full_path, &st) == -1) {
					_qlog->error("QUBE_FUSE::qfs_readdir: Unable to stat path:{}", last_full_path);
					_qlog->flush();
                    break;
				}

				if (filler(buf, de->d_name, 0, 0, fdflags) !=0 ) {
					_qlog->error("QUBE_FUSE::qfs_readdir: Failed to add entry to directory: {}.", last_full_path);
			//		break;
				} else {
					_qlog->debug("QUBE_FUSE::qfs_readdir: Added dir entry {} ino: {:d} mode: {}.", de->d_name, st.st_ino, st.st_mode);
                }
			}
			
			//closedir(dp);
			_qlog->debug("QUBE_FUSE::qfs_readdir---[Leaving]--->");
			_qlog->flush();
			return 0;	
		}

		static int qfs_releasedir(const char *path, struct fuse_file_info *fi) {
			_qlog->debug("QUBE_FUSE::qfs_releasedir---[{}]--->", path);
			(void) path;
			if (fi->fh) {
				::closedir((DIR *) (uintptr_t)fi->fh);
				_qlog->debug("QUBE_FUSE::qfs_releasedir-->Closed Directory path [{}].", path);
			} else {
				_qlog->debug("QUBE_FUSE::qfs_releasedir-->path not open [{}].", path);
			}
			_qlog->debug("QUBE_FUSE::qfs_releasedir---[Leaving]--->");
			_qlog->flush();
			return 0;
		}
		
		static int qfs_fsyncdir(const char *path, int, struct fuse_file_info *fi)
		{
			_qlog->debug("QUBE_FUSE::qfs_fsyncdir---[{}]--->", path);
			//::fsyncdir(((DIR *) (uintptr_t)fi->fh);
			_qlog->debug("QUBE_FUSE::qfs_fsyncdir---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static void qfs_destroy(void *private_data) { return; }
		
		static int qfs_access(const char *path, int i) 
		{
			_qlog->debug("QUBE_FUSE::qfs_access---[{}]---[{:d}]--->", path, i);
			// Access poses a slight security risk, so user checks should be done when file is opened/modified.
			// Which means nothing to do here in this section.
			_qlog->debug("QUBE_FUSE::qfs_access---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) 
		{
			_qlog->debug("QUBE_FUSE::qfs_create---[{}]---[{:d}]--->", path, mode);
			int res;

			last_full_path = qfs_get_root_path(path);
			res = ::open(last_full_path, fi->flags, mode);
			if (res == -1){
				_qlog->debug("QUBE_FUSE::qfs_create: Failed to open fullpath [{}] path [{}] with mode [{:d}] and flags [{:d}].", last_full_path, path, mode, fi->flags);
				res = -errno;
			} else {
				_qlog->debug("QUBE_FUSE::qfs_create: opened full path [{}] path [{}] with result [{:d}].", last_full_path, path, res);
			}
			fi->fh = res;
	
			_qlog->debug("QUBE_FUSE::qfs_create---[Leaving]--->");
			_qlog->flush();
			return 0;
			
		}

		static int qfs_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *my_lock)
		{
			_qlog->debug("QUBE_FUSE::qfs_lock---[{}]---[{:d}]--->", path, cmd);

			_qlog->debug("QUBE_FUSE::qfs_lock---[Leaving]--->");
			_qlog->flush();
			return 0;
		}
		
		static int qfs_utimens(const char *path, const struct timespec *ts, struct fuse_file_info *fi)
		{
			_qlog->debug("QUBE_FUSE::qfs_utimens---[{}]--->", path);

			_qlog->debug("QUBE_FUSE::qfs_utimens---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_bmap(const char *path, size_t blocksize, uint64_t *idx)
		{
			_qlog->debug("QUBE_FUSE::qfs_bmap---[{}]---[{:d}]---[{:f}]--->", path, blocksize, *idx);

			_qlog->debug("QUBE_FUSE::qfs_bmap---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi, unsigned int flags, void *data)
		{
			_qlog->debug("QUBE_FUSE::qfs_ioctl---[{}]---[{:d}]---[{}]---[{:d}]---[{}]--->", path, cmd, arg, flags, data );

			_qlog->debug("QUBE_FUSE::qfs_ioctl---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_poll(const char *path, struct fuse_file_info *fi, struct fuse_pollhandle *ph, unsigned *reventsp)
		{
			_qlog->debug("QUBE_FUSE::qfs_poll---[{}]--->", path );

			_qlog->debug("QUBE_FUSE::qfs_poll---[Leaving]--->");
			_qlog->flush();
			return 0;
		}

		static int qfs_write_buf(const char *path, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *fi) {return 0;}
		static int qfs_read_buf(const char *path, struct fuse_bufvec **bufv, size_t size, off_t off, 
				struct fuse_file_info *fi) {return 0;}

		static int qfs_flock(const char *path, struct fuse_file_info *fi, int op)
		{
			_qlog->debug("QUBE_FUSE::qfs_flock---[{}]---[{:d}]--->", path, op );

			_qlog->debug("QUBE_FUSE::qfs_flock---[Leaving]--->");
			_qlog->flush();
			return 0;
		}
		static int qfs_fallocate(const char *path, int i, off_t off1, off_t off2, struct fuse_file_info *fi)
		{
			_qlog->debug("QUBE_FUSE::qfs_fallocate---[{}]---[{:d}]---[{:d}]---[{:d}]--->", path, i, off1, off2 );

			_qlog->debug("QUBE_FUSE::qfs_fallocate---[Leaving]--->");
			_qlog->flush();
			return 0;
		}



	// These obviously are not meant to be accessible outside the class ------------------>
	private:
		static int mknod_wrapper(int dirfd, const char *path, const char *link, int mode, dev_t rdev) {
			_qlog->debug("QUBE_FUSE::mknod_wrapper---[{:d}]---[{}]---[{}]---[{:d}]-->",dirfd, *path, link, mode);
			
			int res;

			if (S_ISREG(mode)) {
				res = openat(dirfd, path, O_CREAT | O_EXCL | O_WRONLY, mode);
				if (res >= 0)
					res = close(res);
			} else if (S_ISDIR(mode)) {
				res = mkdirat(dirfd, path, mode);
			} else if (S_ISLNK(mode) && link != NULL) {
				res = symlinkat(link, dirfd, path);
			} else if (S_ISFIFO(mode)) {
				res = mkfifoat(dirfd, path, mode);
			} else {
				res = mknodat(dirfd, path, mode, rdev);
			}

			return res;
		}


};
struct fuse_args qube_fuse::args = {};
struct qfs_state* qube_fuse::qfs_data = {};
struct fuse_operations qube_fuse::qfs_operations_ = {};
fuse_fill_dir_flags qube_fuse::fill_dir_plus = {};
std::string root_dir = "";
char* qube_fuse::last_full_path = NULL;