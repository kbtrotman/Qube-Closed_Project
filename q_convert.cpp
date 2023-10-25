/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */


#include "q_convert.hpp"

std::shared_ptr<spdlog::logger> q_convert::_Qlog = nullptr;
extern Settings settings;

    q_convert::q_convert(q_log& q) {
        q_convert::_Qlog = q.get_log_instance();
    }

    std::string q_convert::char2string (char *str) {
        std::string tmp_field(str);
        return tmp_field;
    }

    const char *q_convert::string2char (std::string *str) {
        return str->c_str();
    }

    const unsigned char *q_convert::vect2uchar (std::vector<uint8_t> *vec) {
        const unsigned char *str( vec->data() );
        return str;
    }

    const char *q_convert::vect2char (const std::vector<uint8_t> *vec) {
        if (vec == nullptr) {
            return nullptr; // Handle null pointer case if necessary
        }
        char* cstring = new char[vec->size() + 1];

        // Copy the data from the vector to the char array
        for (size_t i = 0; i < vec->size(); ++i) {
            cstring[i] = static_cast<char>((*vec)[i]);
        }
        cstring[vec->size()] = '\0';
        DEBUG("q_convert::vect2char: Converted a vector to: {}", cstring);
        // Return a C-style string
        return cstring;
    }

    template <typename T> std::vector<uint8_t> q_convert::char2vect(T* str, size_t length) {
        std::vector<uint8_t> tmp_vect;
    
        for (size_t i = 0; i < length; ++i) {
            tmp_vect.push_back(static_cast<uint8_t>(str[i]));
        }

        return tmp_vect;
    }
    // Explicit instantiation for the types we expect to use here.
    template std::vector<uint8_t> q_convert::char2vect(char* str, size_t length);
    template std::vector<uint8_t> q_convert::char2vect(const char* str, size_t length);
    template std::vector<uint8_t> q_convert::char2vect(unsigned char* str, size_t length);

    std::vector<uint8_t> q_convert::string2vect (std::string *str) {
        std::vector<uint8_t> tmp_vect;
        tmp_vect.assign(str->begin(), str->end());
        return tmp_vect;
    }

    std::string q_convert::vect2string(const std::vector<uint8_t>& vec) {
        // Assuming the vector contains binary data that may not be null-terminated
        return std::string(reinterpret_cast<const char*>(vec.data()), vec.size());
    }

    std::vector<uint8_t> q_convert::substr_of_char(const char *str, int start, int end) {
        TRACE("q_convert::substr_of_char: String in to convert: {:x}  from start={:d} to end={:d}.", str, start, end);
        // Create a new vector as a substring of the original string.
        std::vector<uint8_t> substringVector;

        for (int i = start; i < (end - start); ++i) {
            substringVector.push_back( str[i] );
        }
        TRACE("q_convert::substr_of_char: ---Leaving with vector = {:x}----->", (char *)substringVector.data());
        return substringVector;
    }

    std::vector<uint8_t> q_convert::substr_of_vect(std::vector<uint8_t> vect, int start, int end) {
        TRACE("q_convert::substr_of_char: String in to convert: from start={:d} to end={:d}.", start, end);
        // Create a new vector as a substring of the original string.
        std::vector<uint8_t> substringVector;

        for (int i = start; i < (end - start); ++i) {
            substringVector.push_back( vect[i] );
        }
        TRACE("q_convert::substr_of_char: ---Leaving with vector----->");
        FLUSH;
        return substringVector;
    }

/*
    static int vect_is_errorcode(std::vector<uint8_t> vec) {
        int isErrorCode = 0;

            if (vec.size() == sizeof(NO_RECORDS)) {
            for (size_t i = 0; i < vec.size(); i++) {
                if (vec[i] != ((NO_RECORDS >> (i * 8)) & 0xFF)) {
                    isErrorCode = 1;
                    break;
                }
            }
        } else {
            isErrorCode = 0;
        }
        return isErrorCode;
    }
*/