/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */
 
// For Logging!
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
// Dedupe & crypto algorythms
#include <openssl/sha.h>
#include <openssl/evp.h>

// Globals
#include "qube.hpp"

// Functions & classes

//Classes -> Lowest Level to highest.
class qube_log {

	public:

		static std::shared_ptr<spdlog::logger> _qlog;

		qube_log() {
			/* Init Logger */
			if (!logger_init) {
				auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>("/var/log/qube.log", 23, 59);
        		_qlog = std::make_shared<spdlog::logger>("qube_log", file_sink);

				_qlog->set_level(spdlog::level::debug);
				_qlog->debug("qube_log::qube_log: *** Qube logging started. ***");
				_qlog->flush();
				logger_init = true;
			}			
		}

		~qube_log () {
			_qlog->flush();
			exit(exStatus);
		}
	
	private:
		static bool logger_init;
		static int exStatus;
};
std::shared_ptr<spdlog::logger> qube_log::_qlog; // initialize static member
bool qube_log::logger_init = false;
int qube_log::exStatus = 0;

#include "q_psql.hpp"

class qube_hash : public qube_psql {

	public:

		qube_hash () {
			_qlog->debug("qube_hash::qube_hash: Hash Init--->");
			_qlog->flush();
		}

		static std::string get_sha512_hash(const std::string str){
			_qlog->debug("qube_hash::get_sha512_hash---[{}]--->", str);
  			SHA512_CTX ;
			EVP_MD_CTX *sha512;
			std::string tmp_hash;
			unsigned int hash_size = (unsigned int)HASH_SIZE;

			if((sha512 = EVP_MD_CTX_new()) == NULL) {std::memcpy(hash, NO_HASH_S, std::strlen(NO_HASH_S));}
			if( EVP_DigestInit_ex(sha512, EVP_sha512(), NULL) != 1 ) {std::memcpy(hash, NO_HASH_S, std::strlen(NO_HASH_S));}
			if( EVP_DigestUpdate(sha512, str.c_str(), str.size()) != 1 ) {std::memcpy(hash, NO_HASH_S, std::strlen(NO_HASH_S));}
			if((*hash = ( char * )OPENSSL_malloc(EVP_MD_size(EVP_sha512()))) == NULL) {std::memcpy(hash, NO_HASH_S, std::strlen(NO_HASH_S));}
			if( EVP_DigestFinal_ex(sha512, (unsigned char*) *hash, &hash_size) != 1) {
				std::memcpy(hash, NO_HASH_S, std::strlen(NO_HASH_S));
			} else {
				tmp_hash.assign(*hash);	
			}

			EVP_MD_CTX_free(sha512);
			_qlog->debug("qube_hash::get_sha512_hash---[Leaving]---with hash string---[{}]--->.", hash);
			_qlog->flush();
  			return tmp_hash;
		}

	private:
		static char *hash[SHA512_DIGEST_LENGTH];
};
char *qube_hash::hash[SHA512_DIGEST_LENGTH] = {};
#include "q_fuse.hpp"


// Main entry
int main( int argc, char *argv[] )
{

	qube_fuse qf;

	qube_log::_qlog->info("===============================\n");
	qube_log::_qlog->info("=== PG Server version = {:d} ===", qf.qpsql_getServerVers());
	qube_log::_qlog->info("===============================\n");	

    qube_log::_qlog->info("=================================");
    qube_log::_qlog->info("=====  Filesystem Starting ======");
    qube_log::_qlog->info("=================================");

	// Startup the filesystem
  	return qf.run(argc, argv);

}