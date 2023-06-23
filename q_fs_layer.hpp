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


        static char* qfs_get_root_path(const char* path)
        {
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

		static int write_to_file( const char *path, const char *data_content, size_t size, off_t offset )
		{
			qube_log::_qlog->debug("QUBE_FS::Write_to_file---[{}]---[{}]---[{:d}]>", path, data_content, size);
            // Here we are writing only the hashes to the actual filesystem. Data blocks that are normally
            // in a filesystem are written into the DB via the qpsql class.
            qube_log::_qlog->flush();
            
            int fd;
            int write_result;
            size_t total_written = 0;

            // open the file
            fd = open(path, O_WRONLY);
            if (fd == -1) {
                return -errno;
            }

            //Seek to our writing offset point before doing anythign else
            lseek(fd, offset, SEEK_SET);

            // write the data to the file, deduplicating blocks as we go
            while (total_written < size) {

                // write the block to the file
                write_result = pwrite(fd, data_content + total_written, BLOCK_SIZE, offset + total_written);

                if (write_result == -1) {
                    close(fd);
                    return -errno;
                }
                total_written += write_result;

            }

            close(fd);
            qube_log::_qlog->flush();
            return total_written;
		}
};
