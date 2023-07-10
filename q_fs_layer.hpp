/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

//Low-Level functions directly to FS.

struct qfs_state{
    char *devicepath;
};

#define QFS_DATA ((struct qfs_state *) fuse_get_context()->private_data)

class qube_FS {

	public:

     	qube_FS() {

        }


        static char* qfs_get_root_path(const char* path) {
            qube_log::_qlog->debug("QUBE_FS::qfs_get_root_path: with path = {}", path);

            std::string s(path);
            if (!s.empty() && s.at(0) == '/') { s.erase(0, 1); }
            if (s.empty()) {
                s = ".";
            }
            
            std::string rtemp(settings.root_dir->c_str());
            std::string result = rtemp + "/" + s;
            qube_log::_qlog->debug("QUBE_FS::qfs_get_root_path: rtemp: {}, s: {}, fp: {}", rtemp, s, result);

            char *ftemp = new char[result.size() + 1];
            if ( *result.c_str() == '\0' ) {
                qube_log::_qlog->debug("QUBE_FS::qfs_get_root_path: returning path is NULL = {}", result.c_str());   
            } else {
                qube_log::_qlog->debug("QUBE_FS::qfs_get_root_path: resolving path = {}", result.c_str());
                ::realpath(result.c_str(), ftemp);
            }

            qube_log::_qlog->debug("QUBE_FS::qfs_get_root_path: rootdir = {}, rel path = {}, full path = {}", settings.root_dir->c_str(), path, ftemp);
            qube_log::_qlog->debug("QUBE_FS::qfs_get_root_path:---[Leaving]--->");
            qube_log::_qlog->flush();
            return ftemp;
        }

		static int qfs_write_to_file( int fd, const char *data_content, size_t size, off_t offset ) {
			qube_log::_qlog->debug("QUBE_FS::qfs_Write_to_file---[{:d}]---[{}]---[{:d}]>", fd, data_content, size);
            // Here we are writing only the hashes to the actual filesystem. Data blocks that are normally
            // in a filesystem are written into the DB via the qpsql class.
            
            int write_result;

            //Seek to our writing offset point before doing anythign else
            ::lseek(fd, offset, SEEK_SET);

            // write deduplicated data to the file, IE: we're writing hashes only here to the local FS.
            write_result = ::pwrite(fd, data_content, size, offset);
            if (write_result == -1) {
                qube_log::_qlog->debug("QUBE_FS::qfs_Write_to_file: error writing to filehandle---[{:d}]--->", fd);
            } else {
                qube_log::_qlog->debug("QUBE_FS::qfs_Write_to_file: wrote {} bytes to filehandle {:d} at offset {}, total now written = {} bytes.", size, fd, offset, write_result);
            }

            ::close(fd);
            qube_log::_qlog->debug("QUBE_FS::qfs_write_to_file:---[Leaving]--->");
            qube_log::_qlog->flush();
            return write_result;
		}

};
