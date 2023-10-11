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

    std::vector<uint8_t> q_convert::char2vect(const unsigned char *str) {
        std::vector<uint8_t> tmp_vect;
        size_t length = sizeof(str);

        for (size_t i = 0; i < length; ++i) {
            tmp_vect.push_back(static_cast<uint8_t>(str[i]));
        }

        return tmp_vect;
    }

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
        // Create a new vector as a substring of the original vector
        std::vector<uint8_t> substringVector;

        for (int i = 0; i < (end - start); ++i) {
            substringVector.push_back( str[i] );
        }
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