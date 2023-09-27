/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 **/
    
#include "q_dedupe.hpp"

unsigned char q_dedupe::hash[SHA512_DIGEST_LENGTH] = {};
std::stringstream q_dedupe::ss=std::stringstream{};

    std::string q_dedupe::get_sha512_hash(const std::vector<uint8_t> v_str){
        TRACE("qube_hash::get_sha512_hash---[{}]--->", (char*)v_str.data());
        
        // We have static sections, so let's blank everything to be safe here.
        std::memset(hash, 0, SHA512_DIGEST_LENGTH);
        ss.str("");
        ss.clear();

        SHA512_CTX sha512;
        SHA512_Init(&sha512);
        SHA512_Update(&sha512, v_str.data(), v_str.size());
        SHA512_Final(hash, &sha512);

        for(int i = 0; i < (SHA512_DIGEST_LENGTH); i++){
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( hash[i] );
        }
        TRACE("qube_hash::get_sha512_hash---[Leaving]---with hash string---[{}]--->.", ss.str());

        return ss.str();
    }