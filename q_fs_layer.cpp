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


        q_FS::q_FS(q_log& q) {
            _Qflog = q.get_log_instance();
        }

        // All globally static methods below this point. They are interrupt-driven......
        char*  q_FS::qfs_get_root_path(const char* path) {
            TRACE("QUBE_FS::qfs_get_root_path---[path = {}]--->", path);

            std::string s(path);
            if (!s.empty() && s.at(0) == '/') { s.erase(0, 1); }
            if (s.empty()) {
                s = ".";
            }
            
            std::string rtemp(settings.root_dir->c_str());
            std::string result = rtemp + "/" + s;
            DEBUG("QUBE_FS::qfs_get_root_path: rtemp: {}, s: {}, fp: {}", rtemp, s, result);

            char *ftemp = new char[result.size() + 1];
            if ( *result.c_str() == '\0' ) {
                DEBUG("QUBE_FS::qfs_get_root_path: returning path is NULL = {}", result.c_str());   
            } else {
                DEBUG("QUBE_FS::qfs_get_root_path: resolving path = {}", result.c_str());
                ::realpath(result.c_str(), ftemp);
            }

            DEBUG("QUBE_FS::qfs_get_root_path: rootdir = {}, rel path = {}, full path = {}", settings.root_dir->c_str(), path, ftemp);
            TRACE("QUBE_FS::qfs_get_root_path:---[Leaving]--->");
            FLUSH;
            return ftemp;
        }

		int q_FS::qfs_write_to_file( int fd, const char *data_content, size_t size, off_t offset ) {
			TRACE("QUBE_FS::qfs_Write_to_file---[{:d}]---[{}]---[{:d}]>", fd, data_content, size);
            // Here we are writing only the hashes to the actual filesystem. Data blocks that are normally
            // in a filesystem are written into the DB via the qpsql class.
            int write_result;

            //Seek to our writing offset point before doing anythign else
            ::lseek(fd, offset, SEEK_SET);

            // write deduplicated data to the file, IE: we're writing hashes only here to the local FS.
            write_result = ::pwrite(fd, data_content, size, offset);
            if (write_result == -1) {
                DEBUG("QUBE_FS::qfs_Write_to_file: error writing to filehandle---[{:d}]--->", fd);
            } else {
                DEBUG("QUBE_FS::qfs_Write_to_file: wrote {} bytes to filehandle {:d} at offset {}, total now written = {} bytes.", size, fd, offset, write_result);
            }

            ::close(fd);
            TRACE("QUBE_FS::qfs_write_to_file:---[Leaving]--->");
            FLUSH;
            return write_result;
		}

        int  q_FS::qfs_compare_existing_hashes(std::string *new_hashes) {
			// ###########TBD##########
			// COMPARE ORIGINAL FILE HASHES TO THE HASES NOW. IF FILE CHANGED, DECREMENT UN-USED HASH COUNTS.
            // Could be an expensive search when used with locking for multiple edits. Need to examine.
			// !!!!!!!!
			// !!!!!!!!
            return 0;
        }        
