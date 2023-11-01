/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

//Low-Level functions directly to FS.

#include "q_fs_layer.hpp"
#include "q_convert.hpp"

// My shorthand crap may one day be the death of me.....
#undef TRACE
#undef DEBUG
#undef INFO 
#undef WARN 
#undef ERROR 
#undef CRITICAL 
#undef FLUSH  

#define TRACE _Qflog->trace
#define DEBUG _Qflog->debug
#define INFO _Qflog->info
#define WARN _Qflog->warn
#define ERROR _Qflog->error
#define CRITICAL _Qflog->critical
#define FLUSH  _Qflog->flush()

std::shared_ptr<spdlog::logger> hiv_FS::_Qflog;
extern Settings settings;

    hiv_FS::hiv_FS(q_log& q) {
        _Qflog = q.get_log_instance();
    }

    static void __exit dummy_exit(void)
    {
	    int ret;

	    ret = unregister_filesystem(&dummyfs_type);
	    kmem_cache_destroy(dmy_inode_cache);

	    if (!ret)
		    printk(KERN_INFO "Unregister dummy FS success\n");
	    else
		    printk(KERN_ERR "Failed to unregister dummy FS\n");

    }
    module_exit(dummy_exit);

    static int __init dummy_init(void)
    {
	    int ret = 0;

	    dmy_inode_cache = kmem_cache_create("dmy_inode_cache", sizeof(struct dm_inode), 0,
					(SLAB_RECLAIM_ACCOUNT| SLAB_MEM_SPREAD), NULL);

	    if (!dmy_inode_cache)
		    return -ENOMEM;

	    ret = register_filesystem(&dummyfs_type);

	    if (ret == 0)
		    printk(KERN_INFO "Sucessfully registered dummyfs\n");
	    else
		    printk(KERN_ERR "Failed to register dummyfs. Error code: %d\n", ret);

	    return ret;
    }

    module_init(dummy_init);

    // All globally static methods below this point. They are interrupt-driven......
    char*  hiv_FS::qfs_get_root_path(const char* path) {
        TRACE("Q_FS::qfs_get_root_path---[path = {}]---[root: {}]--->", path, settings.root_dir->c_str());

        std::string s(path);
        if (!s.empty() && s.at(0) == '/') { s.erase(0, 1); }
        if (s.empty()) {
            s = ".";
        }
        
        std::string rtemp(settings.root_dir->c_str());
        std::string result = rtemp + "/" + s;
        DEBUG("Q_FS::qfs_get_root_path: rtemp: {}, s: {}, fp: {}", rtemp, s, result);

        char *ftemp = new char[result.size() + 1];
        if ( *result.c_str() == '\0' ) {
            DEBUG("Q_FS::qfs_get_root_path: returning path is NULL = {}", result.c_str());   
        } else {
            DEBUG("Q_FS::qfs_get_root_path: resolving path = {}", result.c_str());
            ::realpath(result.c_str(), ftemp);
        }

        DEBUG("Q_FS::qfs_get_root_path: rootdir = {}, rel path = {}, full path = {}", settings.root_dir->c_str(), path, ftemp);
        TRACE("Q_FS::qfs_get_root_path:---[Leaving]--->");
        FLUSH;
        return ftemp;
    }

    int hiv_FS::qfs_write_to_file( int fd, const char *data_content, size_t size, off_t offset ) {
        TRACE("Q_FS::qfs_Write_to_file---[{:d}]---[{}]---[{:d}]>", fd, data_content, size, offset);
        // Here we are writing only the hashes to the actual filesystem.
        int write_result;

        write_result = ::pwrite(fd, data_content, size, offset);
        if (write_result == -1) {
            DEBUG("Q_FS::qfs_Write_to_file: error writing to filehandle---[{:d}]--->", fd);
        } else {
            DEBUG("Q_FS::qfs_Write_to_file: wrote {} bytes to filehandle {:d} at offset {}, total now written = {} bytes.", size, fd, offset, write_result);
        }

        TRACE("Q_FS::qfs_write_to_file:---[Leaving]--->");
        FLUSH;
        return write_result;
    }

    std::string hiv_FS::qfs_read_from_file(int fd, int buffer_num, size_t size, off_t offset) {
        TRACE("Q_FS::qfs_read_from_file:---[Entering]---with fd = {}, buffer_num = {}, size = {}>", fd, buffer_num, size);
        char in_buffer[HASH_SIZE + 1] = {0}; // allocate memory for in_buffer
        std::string str = "";
        ::lseek(fd, (offset + (buffer_num * HASH_SIZE)), SEEK_SET);

        ssize_t res = ::read(fd, in_buffer, HASH_SIZE);
        if (res == -1) {
            ERROR("Q_FUSE:qfs_read: error reading from provided file handle {:d}.", fd);
        } else {
            str = q_convert::char2string(in_buffer);
        }

        TRACE("Q_FS::qfs_read_from_file:---[Leaving]---with hash = {}>", str);
        FLUSH;

        return str;
    }

    std::vector<uint8_t>  hiv_FS::get_a_block_from_buffer( std::vector<uint8_t> in_buffer, int block_num ) {
        TRACE("Q_FS::get_a_block_from_buffer: getting block number: {:d} from a buffer of length: {:d}", block_num, in_buffer.size());
        static std::vector<uint8_t> cur_block;
        cur_block.clear();
        
        cur_block = q_convert::substr_of_vect(in_buffer, block_num * BLOCK_SIZE, ((block_num + 1) * BLOCK_SIZE) );
        TRACE("Q_FS::get_a_block_from_buffer: ---Leaving with block of size {:d}--------->", cur_block.size());
        FLUSH;
        return cur_block;
    }

    int hiv_FS::handle_a_collision( ) {
        ERROR("Q_FUSE::qfs_write: Hash Colission! Hard Error. Saving data and working around the problem.");

        //TODO: We don't allow collisions in this FS, so we deal with the error in a way that makes sense.
        //***********************************************************************************************
        // Most de-dupe platforms realize that a collision is less likely than filesystem corruption, but
        // we want to at the least, log the event and note it. Also, we want to save the file in a way its
        // recoverable. There are several ways we can do this. But its not important at this moment.
        //***********************************************************************************************
        //    1. Save a copy of the full file in an admin dir.
        //    2. List all data about hashes and blocks in a text file in the same dir.
        //    3. Log the time, date, and relavant info.
        return 0;
    }

    int  hiv_FS::qfs_compare_existing_hashes(std::string *new_hashes) {
        // ###########TBD##########
        // COMPARE ORIGINAL FILE HASHES TO THE HASES NOW. IF FILE CHANGED, DECREMENT UN-USED HASH COUNTS.
        // Could be an expensive search when used with locking for multiple edits. Need to examine.
        // !!!!!!!!
        // !!!!!!!!
        return 0;
    }        
