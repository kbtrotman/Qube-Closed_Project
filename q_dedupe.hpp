/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 **/

#pragma once

// Dedupe & crypto algorythms
#include <openssl/sha.h>

#include "q_psql.hpp"
#include "q_log.hpp"

class q_dedupe : public q_psql {

	public:

		q_dedupe (q_log& q) : q_psql(q){
			TRACE("q_hash::q_hash: Hash Init--->");
			FLUSH;
		}
		static std::string get_sha512_hash(const std::vector<uint8_t> v_str);

	private:
		static unsigned char hash[SHA512_DIGEST_LENGTH];
  		static std::stringstream ss;
};
//unsigned char q_dedupe::hash[SHA512_DIGEST_LENGTH] = {};
//std::stringstream q_dedupe::ss=std::stringstream{};
