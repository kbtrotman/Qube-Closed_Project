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
//			qfs_operations_.write_buf       = &qube_fuse::qfs_write_buf;
//			qfs_operations_.read_buf        = &qube_fuse::qfs_read_buf;
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
			TRACE("QUBE_FUSE::run---[{:d}]---[{}]---[{}]--->",argc, argv[argc-1], argv[argc-2]);
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
			DEBUG("QUBE_FUSE::run: progname: {}  Root_dir: {}  Mount_point: {}.", settings.progname, settings.root_dir->c_str(), settings.mount_point);
			settings.src_len = settings.root_dir->length();
			settings.mnt_len = strlen(settings.mount_point);

			struct stat st;

			if (::stat(settings.mount_point, &st) == -1) {
				ERROR("QUBE_FUSE::run: Error: The mount point [{}] doesn't exist.", settings.mount_point);
				perror("stat");
				return 1;
			}
			if (!(st.st_mode & S_IFDIR)) {
				ERROR("QUBE_FUSE::run: Error: The mount point [{}] is not a directory.", settings.mount_point);
				return 1;
			}
			if (::access(settings.mount_point, W_OK) == -1) {
				ERROR("QUBE_FUSE::run: Error: The mount point [{}] is not accessible.", settings.mount_point);
				perror("access");
				return 1;
			}
			INFO("QUBE_FUSE::run: Setting command-line defaults in [run] method with source=[{}] and mount=[{}]", settings.root_dir->c_str(), settings.mount_point);

			qfs_data = (qfs_state*)malloc(sizeof(struct qfs_state));
			if (qfs_data == NULL) {
				perror("main calloc");
				abort();
			}

			fuse_args args = FUSE_ARGS_INIT(argc, argv);

			/* Open mount root for chrooting */
    		settings.src_fd = ::open(settings.root_dir->c_str(), O_RDONLY);
    		if (settings.src_fd == -1) {
        		ERROR("QUBE_FUSE::run: Could not open source directory: {}.", settings.root_dir->c_str());
        		return 1;
    		}

			if (fchdir(settings.src_fd) == 0) {
       			DEBUG("QUBE_FUSE::run: Current working directory changed to {}.", settings.root_dir->c_str());
    		} else {
        		ERROR("QUBE_FUSE::run: Failed to change working directory to {}. Are we on the right file path?", settings.root_dir->c_str());
    		}

			DEBUG("QUBE_FUSE::run---[Leaving]--->.");
			FLUSH;
  			return fuse_main(args.argc, args.argv, &qfs_operations_ , qfs_data);
		}

		/*

		Fuse Over-rides - To make it a real FS, these would be the layer to start at--the fuse inclusions--and go downward to the hardware layer. 
		This isn't nearly as hard as it sounds. It's easier to test on fuse and then migrate to physical disk sectors later. This can be done by
		migrating the code to being a device-driver that loads a filesystem module.

		All globally static methods below this point. They are interrupt-driven......

		*/

		static void *qfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg){
				TRACE("QUBE_FUSE::qfs_init:---{}--->",  PRINT_CONN_STRING(conn) );

				cfg->use_ino = 0;
				// Disable caches so changes in base FS are visible immediately.
				// Especially the attribute cache must be disabled when different users
				// might see different file attributes, such as when mirroring users.
				cfg->entry_timeout = 0;
				cfg->attr_timeout = 0;
				cfg->negative_timeout = 0;
				//cfg->direct_io = settings.direct_io;

				TRACE("QUBE_FUSE::qfs_init:---[Leaving]--->");
				FLUSH;
    			return QFS_DATA;
		}

		static int qfs_getattr( const char *path, struct stat *stbuf, struct fuse_file_info *fi ) {
			TRACE("QUBE_FUSE::qfs_getattr---[{}]--->", path);
			int retstat = 0;

			last_full_path = qfs_get_root_path(path);
			DEBUG("QUBE_FUSE::qfs_getattr: Fullpath={} and path={}.", last_full_path, path);

			if (::lstat(last_full_path, stbuf) == -1) {
				WARN("QUBE_FUSE::qfs_getattr: Failed to open path, Fullpath={} and path={}. Is it a new file?", last_full_path, path);
				retstat = -errno;
			} else {
				DEBUG("QUBE_FUSE::qfs_getattr: Stat-ed, Fullpath={} and path={}.", last_full_path, path);
				retstat = 0;
			}

			DEBUG("QUBE_FUSE::qfs_getattr: {}", PRINT_STBUFF_STRING(stbuf) );
			TRACE("QUBE_FUSE::qfs_getattr---[Leaving]---[retstat={:d}]>", retstat);
			FLUSH;
			return retstat;
		}

		static int qfs_readlink (const char *path, char *buf, size_t size) {
			TRACE("QUBE_FUSE::qfs-readlink---[{}]---[{}]---[{:f}]--->", path, buf, size );
			int res;

			last_full_path = qfs_get_root_path(path);
			res = ::readlink(last_full_path, buf, size - 1);
			if (res == -1){
				buf[0] = '\0';
				ERROR("QUBE_FUSE::qfs_readlink: Failed to read link path, Fullpath={} and path={}.", last_full_path, path);
				res = -errno;
			} else {
				ERROR("QUBE_FUSE::qfs_readlink: Opened for read->link path, Fullpath={} and path={}.", last_full_path, path);
				res = 0;
			}

			TRACE("QUBE_FUSE::qfs_readlink---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_mknod( const char *path, mode_t mode, dev_t rdev ) {
			TRACE("QUBE_FUSE::qfs_mknod---[{}]---[{:d}]---[{:d}]--->", path, mode, rdev );
			int res;

			res = mknod_wrapper(AT_FDCWD, path, NULL, mode, rdev);
			if (res == -1)
				return -errno;

			TRACE("QUBE_FUSE::qfs_mknod---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_mkdir( const char *path, mode_t mode )
		{
			TRACE("QUBE_FUSE:qfs_mkdir---[{}]---[{:d}]--->", path, mode );
			int res;

			last_full_path = qfs_get_root_path(path);
			res = ::mkdir(last_full_path, mode);
			if (res == -1) {
				ERROR("QUBE_FUSE::qfs_mkdir: Failed to make directory, Fullpath={} and path={}.", last_full_path, path);
				res = -errno;
			} else {
				DEBUG("QUBE_FUSE::qfs_mkdir: Made directory with path, Fullpath={} and path={}.", last_full_path, path);
				res = 0;
			}  

			TRACE("QUBE_FUSE::qfs_mkdir---[Leaving]--->");
			FLUSH;
			return res;
		}

		static int qfs_unlink (const char *path) {
			TRACE("QUBE_FUSE::qfs_unlink---[{}]--->", path );
			int res;

			last_full_path = qfs_get_root_path(path);
			res = ::unlink(last_full_path);
			if (res == -1){
				ERROR("QUBE_FUSE::qfs_unlink: Failed to un-link path, Fullpath={} and path={}.", last_full_path, path);
				res = -errno;
			} else {
				DEBUG("QUBE_FUSE::qfs_unlink: Unlinked (deleted)->link path, Fullpath={} and path={}.", last_full_path, path);
				res = 0;
			}  

			TRACE("QUBE_FUSE::qfs_unlink---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_rmdir (const char *dirname) {
			TRACE("QUBE_FUSE::qfs_rmdir---[{}]--->", dirname );
			int res;

			last_full_path = qfs_get_root_path(dirname);
			res = ::rmdir(last_full_path);
			if (res == -1) {
				DEBUG("QUBE_FUSE::qfs_rmdir: Failed to remove directory [{}]--->", dirname);
				res = -errno;
			} else {
				DEBUG("QUBE_FUSE::qfs_rmdir: Removed directory [{}]-->", dirname);
				res = 0;
			}
			TRACE("QUBE_FUSE::qfs_rmdir---[Leaving]--->");
			return 0;
		}

		static int qfs_symlink(const char *from, const char *to) {
			TRACE("QUBE_FUSE::qfs_symlink---[{}]---[{}]--->", *from, *to );
			int res;

			res = ::symlink(from, to);
			if (res == -1)
				return -errno;

			TRACE("QUBE_FUSE::qfs_symlink---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_rename(const char *path, const char *c, unsigned int flags) {
			TRACE("QUBE_FUSE::qfs_rename---[{}]---[{}]---[{:d}]--->", *path, *c, flags );
			TRACE("QUBE_FUSE::qfs_rename---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_link(const char *path, const char *linkname) {
			TRACE("QUBE_FUSE::qfs_link---[{}]---[{}]--->", path, linkname );
			TRACE("QUBE_FUSE::qfs_link---[Leaving]--->");
			FLUSH;
			return 0;

		}
		
		static int qfs_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_chmod---[{}]---[{:d}]--->", path, mode );
			TRACE("QUBE_FUSE::qfs_chmod---[Leaving]--->");
			FLUSH;
			return 0;
		} 
		
		static int qfs_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_chown---[{}]---[{:d}]---[{:d}]--->", path, uid, gid );
			TRACE("QUBE_FUSE::qfs_chown---[Leaving]--->");
			FLUSH;
			return 0;
		}
		
		static int qfs_truncate(const char *path, off_t offset, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_truncate---[{}]---[{:d}]--->", path, offset );
			TRACE("QUBE_FUSE::qfs_truncate---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_open(const char *path, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_open---[{}]--->", path );
			FLUSH;
			int local_fh;
			int res;

			last_full_path = qfs_get_root_path(path);
			DEBUG("QUBE_FUSE::qfs_open: Fullpath={} and path={}.", last_full_path, path);

			local_fh = ::open(last_full_path, fi->flags);
			if (local_fh == -1){
				ERROR("QUBE_FUSE::qfs_create: Failed to open path [{}] with flags [{:d}].", path, fi->flags);
				res = -errno;
			} else {
				DEBUG("QUBE_FUSE::qfs_create: opened path [{}] with result [{:d}].", path, local_fh);
				res = 0;
			}
			fi->fh = local_fh;
			TRACE("QUBE_FUSE::qfs_open---[Leaving]--->");
			FLUSH;
			return res;
		}

		static int qfs_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi ) {
			TRACE("QUBE_FUSE:qfs_read---[{}]--->", path );
			int fd;
			int res;
			int num_in_a_partial = size % 4096;
			int num_of_hashes = size / 4096;
			int original_offset;
			int original_size;
			char tmpbuffer[HASH_SIZE];
			std::string block_hash;
			std::string actual_contents;

			if (num_in_a_partial > 0) { num_of_hashes++; }
			fd = fi->fh;

			// Change our hashes in the file into blocks from the DB....
			original_offset = offset;
			original_size = size;
			offset = (original_offset / 4096) + (original_offset % 4096);
			::lseek(fd, offset, SEEK_SET);
			DEBUG("QUBE_FUSE:qfs_read: seeked to offset {:d} in file handle {:d} to read size {:d}.", offset, fd, size);

			for (int i=0; i < num_of_hashes; i++) {

				res = pread(fd, tmpbuffer, HASH_SIZE, (offset + (i * HASH_SIZE)) );
				if (res == -1) {
					ERROR("QUBE_FUSE:qfs_read: error reading from provided file handle {:d}.", fd);
					res = -errno;
					return 0;
				}

				DEBUG("QUBE_FUSE:qfs_read: read {} bytes into temp buffer from filehandle {:d}, # of hashes {:d}.", strlen(tmpbuffer), fd, (strlen(tmpbuffer) / HASH_SIZE));
			
				std::string read_buffer(tmpbuffer);
				block_hash = read_buffer.substr((i * HASH_SIZE), ((i + 1) * HASH_SIZE));
				actual_contents += qpsql_get_block_from_hash(block_hash);
			}

			size = original_size;
			offset = original_offset;
			DEBUG("QUBE_FUSE:qfs_read: {:d} Bytes of data read to buffer with filehandle {:d}, data is: {}.", actual_contents.length(), fd, actual_contents);

			memcpy( buffer, actual_contents.c_str(), actual_contents.length() );
			TRACE("QUBE_FUSE::qfs_read---[Leaving]--->");
			FLUSH;
			return actual_contents.length();
		}

		static int qfs_write( const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *info ) {
			TRACE("QUBE_FUSE::qfs_write---[{}]---[{}]-->", path, buf);
			double original_offset;
			std::string block_hash;
			std::string hash_buffer;
			std::string cur_block;
			std::string buffer(buf);
			int res;

       		if (buffer.empty()) {
            	DEBUG("QUBE_FUSE::qfs_write: Exiting qfs_write with no data to write to disk/DB: '{:d}'.", buffer.length());
            	res = 0;
			} else {
            	DEBUG("QUBE_FUSE::qfs_Write: Incoming full path: {}---buffer: {}--- and buffer length: {}--->", last_full_path, buffer, buffer.length());
				original_offset = offset;

        		//# Generate the test hashes
        		//###########################
        		offset = (offset * HASH_SIZE) / BLOCK_SIZE;
				int fracBuffers = buffer.length() % BLOCK_SIZE; //Most algs use ceiling, but Ceiling ignores very small fractions.
				int numBuffers = buffer.length() / BLOCK_SIZE;	 								          
																   
				if (fracBuffers > 0) {numBuffers++;}             // If we have extra data, then we need to add a buffer frame to store it.
				DEBUG("QUBE_FUSE::qfs_write: There are {:d} buffers in the data stream, including {:d} bytes in a fractional buffer.", numBuffers, fracBuffers);

        		for (int i=0; i < numBuffers; i++) {
            		DEBUG("QUBE_FUSE::qfs_write: # of Times through hash loop: {:d} ",i);
					cur_block = buffer.substr(i * BLOCK_SIZE, (i + 1) * BLOCK_SIZE);
            		block_hash = qube_hash::get_sha512_hash(cur_block);
            		DEBUG("QUBE_FUSE::qfs_write: Hash that was generated: {}", block_hash);
            		hash_buffer += block_hash;
					DEBUG("QUBE_FUSE::qfs_write: Appended hash: {} to hash_buffer: {}.", block_hash, hash_buffer);
            		//# Check if the hash exists
            		//##########################
                	if ( qpsql_get_block_from_hash(block_hash) == NO_RECORD_S ) {
						//hash doesn't exist, save it...
						int ins_rows = 0;
						ins_rows = qpsql_insert_hash(block_hash, cur_block);
						DEBUG("QUBE_FUSE::qfs_write: Saved to DB: hash: {} Block: {} in {:d} rows.", block_hash, cur_block, ins_rows);
						if ( ins_rows < 1 ) {ERROR("QUBE_FUSE::qfs_write: data was not saved to the DB, rows = {:d}}!", ins_rows);}
					} else {
						// Handle Collisions:
						// Hash does exist, check if the data block in the DB is the same...
						// if (! (cur_block == ) ) {
							// Then we have a hard collision. Let's deal with it.

						// }


						// If no collisions, then increment the use count and statistics.
						int null_recs = qpsql_incr_hash_count(block_hash);
						if (null_recs != 0) {
							ERROR("Record count not = 0 in incrementing a hash's use count.");
						}
					}

				}
				// Find which hashes were modified and remove/alter as necessary.
				qfs_compare_existing_hashes( &hash_buffer );

				//Write the results to the file we're modifying...					
				DEBUG("QUBE_FUSE::qfs_write: End Hash Buffer: {}", hash_buffer);
				res = qfs_write_to_file( info->fh, hash_buffer.c_str(), hash_buffer.length(), offset );
				DEBUG("QUBE_FUSE::qfs_write: {} Bytes of data Written from buffer of length {} to qubeFS filename {} and filehandle {}.",
					res, buffer.length(), path, info->fh);
			}
			offset = original_offset;
			TRACE("QUBE_FUSE::qfs_write---[Leaving]--->");
			FLUSH;
			//return res;   // Fuse expects us to write the same data we recieved into this sub. :P
			return size;   
		}

		static int qfs_statfs (const char *path, struct statvfs *stbuf) {

			TRACE("QUBE_FUSE::qfs_statfs---[{}]--->", path );
			int res;

			last_full_path = qfs_get_root_path(path);
			DEBUG("QUBE_FUSE::qfs_statfs: Fullpath={} and path={}.", last_full_path, path);
			res = ::statvfs(last_full_path, stbuf);
			if (res == -1)
				return -errno;

			TRACE("QUBE_FUSE::qfs_statfs---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_flush(const char *path, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_flush---[{}]--->", path);
			// This is a no-op call. We do nothing. Nothing at all. Zip. Zero. Nada. 1 less than 1.
			TRACE("QUBE_FUSE::qfs_flush---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_release(const char *path, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_release---[{}]--->", path);
			(void) path;

			::close(fi->fh);
			TRACE("QUBE_FUSE::qfs_release---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_fsync(const char *path, int sync, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_fsync---[{}]--->", path);
			::fsync(fi->fh);
			TRACE("QUBE_FUSE::qfs_fsync---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_setxattr (const char *path, const char *name, const char *val, size_t size, int flags) {
			TRACE("QUBE_FUSE::qfs_setxattr---[{}]---[{}]---[{:f}]---[{:f}]-->", *path, *name, *val, size );

			TRACE("QUBE_FUSE::qfs_setxattr---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_getxattr (const char *path, const char *name, char *value, size_t size) {
			TRACE("QUBE_FUSE::qfs_getxattr---[{}]---[{}]---[{:d}]>", path, name, size );
			int res = 0;

#ifdef HAVE_SETXATTR
			last_full_path = qfs_get_root_path(path);
			DEBUG("QUBE_FUSE::qfs_getxattr: Fullpath={} and path={}.", last_full_path, path);
			res = ::getxattr(last_full_path, name, value, size);
			if (res == -1) {
				res = -errno;
				ERROR("QUBE_FUSE::qfs_getxattr: Error in getting xattr from full path [{}] name [{}] result [{:d}] and size [{:d}] with errno [{:d}]. Bailing.", last_full_path, name, res, size, errno);
			} else {
				DEBUG("QUBE_FUSE::qfs_getxattr: Returned xattr from full path [{}] name [{}] and size [{:d}]. Bailing.", last_full_path, name, size);
				res = 0;
			}
#endif

			TRACE("QUBE_FUSE::qfs_getxattr---[Leaving]--->");
			FLUSH;
			return res;
		}

		static int qfs_listxattr(const char *path, char *attr, size_t size) {
			TRACE("QUBE_FUSE::qfs_listxattr---[{}]---[{}]---[{:d}]--->", *path, *attr, size );
			return 0;
			TRACE("QUBE_FUSE::qfs_listxattr---[Leaving]--->");
			FLUSH;
		}

		static int qfs_removexattr(const char *path, const char *attr) {
			TRACE("QUBE_FUSE::qfs_removexattr---[{}]---[{}]--->", *path, *attr );
			return 0;
			TRACE("QUBE_FUSE::qfs_removexattr---[Leaving]--->");
			FLUSH;
		}
		
		static int qfs_opendir(const char *path, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_opendir---[{}]--->", path);
			DIR *local_fh;
			int res;

			last_full_path = qfs_get_root_path(path);
			DEBUG("QUBE_FUSE::qfs_opendir: Fullpath={} and path={}.", last_full_path, path);
			local_fh = ::opendir(last_full_path);
			if (local_fh == NULL){
				ERROR("QUBE_FUSE::qfs_opendir: Failed to open Full path {}, path [{}].", last_full_path, path);
				res = -errno;
			} else {
				DEBUG("QUBE_FUSE::qfs_opendir: opened path [{}] with result [{}].", last_full_path, path);
				res = 0;
			}
			fi->fh = (intptr_t)local_fh;

			TRACE("QUBE_FUSE::qfs_opendir---[Leaving]--->");
			FLUSH;
			return res;
		}

		static int qfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, 
				enum fuse_readdir_flags flags) {

			TRACE("QUBE_FUSE::qfs_readdir---[{}]--->", path );
			(void) offset;
    		(void) fi;
			DIR *dp;
			struct dirent *de;
			fuse_fill_dir_flags fdflags;
			fdflags = fuse_fill_dir_flags::FUSE_FILL_DIR_PLUS;

			last_full_path = qfs_get_root_path(path);
			DEBUG("QUBE_FUSE::qfs_readdir: Fullpath={} and path={}", last_full_path, path);

			dp = (DIR *) (uintptr_t)fi->fh; 
			if (dp == NULL) {
				ERROR("QUBE_FUSE::qfs_readdir: Failed to get directory info: {}.", last_full_path);
				return -errno;
			}
			int r = 0;

			while ((de = ::readdir(dp)) != NULL) {
				r++;
				DEBUG("QUBE_FUSE::qfs_readdir: Adding entry r:{} with name: {} from dir list into buffer.", r, de->d_name);
				struct stat st = {};
				st.st_ino = de->d_ino;
				st.st_mode = de->d_type << 12;
				last_full_path = qfs_get_root_path(de->d_name);
				if (::lstat(last_full_path, &st) == -1) {
					ERROR("QUBE_FUSE::qfs_readdir: Unable to stat path:{}", last_full_path);
					FLUSH;
                    break;
				}

				if (filler(buf, de->d_name, 0, 0, fdflags) !=0 ) {
					ERROR("QUBE_FUSE::qfs_readdir: Failed to add entry to directory: {}.", last_full_path);
				} else {
					DEBUG("QUBE_FUSE::qfs_readdir: Added dir entry {} ino: {:d} mode: {}.", de->d_name, st.st_ino, st.st_mode);
                }
			}
			
			TRACE("QUBE_FUSE::qfs_readdir---[Leaving]--->");
			FLUSH;
			return 0;	
		}

		static int qfs_releasedir(const char *path, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_releasedir---[{}]--->", path);
			(void) path;
			if (fi->fh) {
				::closedir((DIR *) (uintptr_t)fi->fh);
				DEBUG("QUBE_FUSE::qfs_releasedir-->Closed Directory path [{}] and filehandle [{}].", path, fi->fh);
			} else {
				DEBUG("QUBE_FUSE::qfs_releasedir-->path not open [{}].", path);
			}
			TRACE("QUBE_FUSE::qfs_releasedir---[Leaving]--->");
			FLUSH;
			return 0;
		}
		
		static int qfs_fsyncdir(const char *path, int, struct fuse_file_info *fi)
		{
			TRACE("QUBE_FUSE::qfs_fsyncdir---[{}]--->", path);
			//::fsyncdir(((DIR *) (uintptr_t)fi->fh);
			TRACE("QUBE_FUSE::qfs_fsyncdir---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static void qfs_destroy(void *private_data) { return; }
		
		static int qfs_access(const char *path, int i) 
		{
			TRACE("QUBE_FUSE::qfs_access---[{}]---[{:d}]--->", path, i);
			// Access poses a slight security risk, so user checks should be done when file is opened/modified.
			// Which means nothing to do here in this section.
			TRACE("QUBE_FUSE::qfs_access---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) 
		{
			TRACE("QUBE_FUSE::qfs_create---[{}]---[{:d}]--->", path, mode);
			int res;

			last_full_path = qfs_get_root_path(path);
			res = ::open(last_full_path, fi->flags, mode);
			if (res == -1){
				DEBUG("QUBE_FUSE::qfs_create: Failed to open fullpath [{}] path [{}] with mode [{:d}] and flags [{:d}].", last_full_path, path, mode, fi->flags);
				res = -errno;
			} else {
				DEBUG("QUBE_FUSE::qfs_create: opened full path [{}] path [{}] with result [{:d}].", last_full_path, path, res);
			}
			fi->fh = res;
	
			TRACE("QUBE_FUSE::qfs_create---[Leaving]--->");
			FLUSH;
			return 0;
			
		}

		static int qfs_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *my_lock)
		{
			TRACE("QUBE_FUSE::qfs_lock---[{}]---[{:d}]--->", path, cmd);
			//No-op at present. This needs to be filled in once the read/write is working perfectly.
			//We want to be able to share files like NFS (as a very crude example).
			TRACE("QUBE_FUSE::qfs_lock---[Leaving]--->");
			FLUSH;
			return 0;
		}
		
		
		static int qfs_utime(const char *path, const struct timespec *ts, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_utimens---[{}]--->", path);
			//utime & utimens are no-ops until we can have written files. Then we add to these to modify the file data/time.
			TRACE("QUBE_FUSE::qfs_utimens---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_utimens(const char *path, const struct timespec *ts, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_utimens---[{}]--->", path);

			TRACE("QUBE_FUSE::qfs_utimens---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_bmap(const char *path, size_t blocksize, uint64_t *idx)
		{
			TRACE("QUBE_FUSE::qfs_bmap---[{}]---[{:d}]---[{:f}]--->", path, blocksize, *idx);

			TRACE("QUBE_FUSE::qfs_bmap---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi, unsigned int flags, void *data)
		{
			TRACE("QUBE_FUSE::qfs_ioctl---[{}]---[{:d}]---[{}]---[{:d}]---[{}]--->", path, cmd, arg, flags, data );

			TRACE("QUBE_FUSE::qfs_ioctl---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_poll(const char *path, struct fuse_file_info *fi, struct fuse_pollhandle *ph, unsigned *reventsp) {
			TRACE("QUBE_FUSE::qfs_poll---[{}]--->", path );

			TRACE("QUBE_FUSE::qfs_poll---[Leaving]--->");
			FLUSH;
			return 0;
		}

		static int qfs_flock(const char *path, struct fuse_file_info *fi, int op) {
			TRACE("QUBE_FUSE::qfs_flock---[{}]---[{:d}]--->", path, op );

			TRACE("QUBE_FUSE::qfs_flock---[Leaving]--->");
			FLUSH;
			return 0;
		}
		
		static int qfs_fallocate(const char *path, int i, off_t off1, off_t off2, struct fuse_file_info *fi) {
			TRACE("QUBE_FUSE::qfs_fallocate---[{}]---[{:d}]---[{:d}]---[{:d}]--->", path, i, off1, off2 );

			TRACE("QUBE_FUSE::qfs_fallocate---[Leaving]--->");
			FLUSH;
			return 0;
		}



	// These obviously are not meant to be accessible outside the class ------------------>
	private:
		static int mknod_wrapper(int dirfd, const char *path, const char *link, int mode, dev_t rdev) {
			TRACE("QUBE_FUSE::mknod_wrapper---[{:d}]---[{}]---[{}]---[{:d}]-->",dirfd, *path, link, mode);
			
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

			TRACE("QUBE_FUSE::mknod_wrapper---[Leaving]--->");
			return res;
		}


};
struct fuse_args qube_fuse::args = {};
struct qfs_state* qube_fuse::qfs_data = {};
struct fuse_operations qube_fuse::qfs_operations_ = {};
fuse_fill_dir_flags qube_fuse::fill_dir_plus = {};
std::string root_dir = "";
char* qube_fuse::last_full_path = NULL;