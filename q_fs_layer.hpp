/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

//Low-Level functions directly to FS.

#pragma once

#include "qube.hpp"
#include "q_log.hpp"

struct qfs_state{
    char *devicepath;
};

#define QFS_DATA ((struct qfs_state *) fuse_get_context()->private_data)

class q_FS {

	public:

        q_FS(q_log& q);

        // All globally static methods below this point. They are interrupt-driven......
        static char* qfs_get_root_path(const char* path);
		static int qfs_write_to_file( int fd, const char *data_content, size_t size, off_t offset );
        static std::string qfs_read_from_file( int fd, int buffer_num, size_t size, off_t offset );
        static int qfs_compare_existing_hashes(std::string *new_hashes);       
        static std::vector<uint8_t> get_a_block_from_buffer( std::vector<uint8_t> in_buffer, int block_num );
        static int handle_a_collision( );
        


    private:
        static std::shared_ptr<spdlog::logger> _Qflog;
        static std::vector<uint8_t> cur_block;     
};

