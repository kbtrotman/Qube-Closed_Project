/**
 * hivFS
 *
 * A Hive Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

//Low-Level functions directly to Module Layer.

#pragma once

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/parser.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>

#include "qube.hpp"
#include "q_log.hpp"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Trotman");
MODULE_DESCRIPTION("HivFS");


class hiv_FS {

	public:

        hiv_FS(q_log& q);

        // All globally static methods below this point. They are interrupt-driven......
        static char* qfs_get_root_path(const char* path);
		static int qfs_write_to_file( int fd, const char *data_content, size_t size, off_t offset );
        static std::string qfs_read_from_file( int fd, int buffer_num, size_t size, off_t offset );
        static int qfs_compare_existing_hashes(std::string *new_hashes);       
        static std::vector<uint8_t> get_a_block_from_buffer( std::vector<uint8_t> in_buffer, int block_num );
        static int handle_a_collision();
        
    private:
        static std::shared_ptr<spdlog::logger> _Qflog;
        static std::vector<uint8_t> cur_block;     
        struct file_system_type dummyfs_type = {
	        .owner = THIS_MODULE,
	        .name = "hivfs",
	        .mount = hivfs_mount,
	        .kill_sb = dummyfs_kill_superblock,
	        .fs_flags = FS_REQUIRES_DEV
        };

        const struct inode_operations dummy_inode_ops = {
	        .create = dm_create,
	        .mkdir = dm_mkdir,
	        .lookup = dm_lookup,
        };

        const struct file_operations dummy_file_ops = {
	        .read_iter = dummy_read,
	        .write_iter = dummy_write,
        };

        const struct super_operations dummy_sb_ops = {
	        .destroy_inode = dm_destroy_inode,
	        .put_super = dummyfs_put_super,
        };

        const struct file_operations dummy_dir_ops = {
	        .owner = THIS_MODULE,
	        .iterate_shared = dummy_readdir,
        };

        struct kmem_cache *dmy_inode_cache = NULL;

};

