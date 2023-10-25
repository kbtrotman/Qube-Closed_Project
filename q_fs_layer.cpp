/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

//Low-Level functions directly to FS.

// My shorthand crap may one day be the death of me.....

#include "q_fs_layer.hpp"
#include "q_convert.hpp"

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

std::shared_ptr<spdlog::logger> q_FS::_Qflog;
extern Settings settings;

    q_FS::q_FS(q_log& q) {
        _Qflog = q.get_log_instance();
    }

    // All globally static methods below this point. They are interrupt-driven......
    char*  q_FS::qfs_get_root_path(const char* path) {
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

    int q_FS::qfs_write_to_file( int fd, const char *data_content, size_t size, off_t offset ) {
        TRACE("Q_FS::qfs_Write_to_file---[{:d}]---[{}]---[{:d}]>", fd, data_content, size, offset);
        // Here we are writing only the hashes to the actual filesystem.
        int write_result;

        //Seek to our writing offset point before doing anythign else
        //::lseek(fd, offset, SEEK_SET);

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

    std::vector<uint8_t>  q_FS::get_a_block_from_buffer( std::vector<uint8_t> in_buffer, int block_num ) {
        TRACE("Q_FS::get_a_block_from_buffer: getting block number: {:d} from a buffer of length: {:d}", block_num, in_buffer.size());
        static std::vector<uint8_t> cur_block;
        cur_block.clear();
        
        cur_block = q_convert::substr_of_vect(in_buffer, block_num * BLOCK_SIZE, ((block_num + 1) * BLOCK_SIZE) );
        TRACE("Q_FS::get_a_block_from_buffer: ---Leaving with block of size {:d}--------->", cur_block.size());
        FLUSH;
        return cur_block;
    }

    int q_FS::handle_a_collision( ) {
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

    int  q_FS::qfs_compare_existing_hashes(std::string *new_hashes) {
        // ###########TBD##########
        // COMPARE ORIGINAL FILE HASHES TO THE HASES NOW. IF FILE CHANGED, DECREMENT UN-USED HASH COUNTS.
        // Could be an expensive search when used with locking for multiple edits. Need to examine.
        // !!!!!!!!
        // !!!!!!!!
        return 0;
    }        
