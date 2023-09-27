/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

#pragma once

#include "qube.hpp"
#include "q_log.hpp"
	extern q_log* QLOG;


class q_convert {

	public:


		q_convert () {
        }

        static std::string char2string (char *str) {
            std::string tmp_field(str);
            return tmp_field;
        }

        static const char *string2char (std::string *str) {
            return str->c_str();
        }

        static const unsigned char *vect2char (std::vector<uint8_t> *vec) {
            const unsigned char *str( vec->data() );
            return str;
		}

        static std::vector<uint8_t> char2vect(const unsigned char* str) {
            std::vector<uint8_t> tmp_vect;
            size_t length = sizeof(str);

            for (size_t i = 0; i < length; ++i) {
                tmp_vect.push_back(static_cast<uint8_t>(str[i]));
            }

            return tmp_vect;
        }

        static std::vector<uint8_t> string2vect (std::string *str) {
            std::vector<uint8_t> tmp_vect;
            tmp_vect.assign(str->begin(), str->end());
            return tmp_vect;
		}

        static std::string vect2string(const std::vector<uint8_t>& vec) {
            // Assuming the vector contains binary data that may not be null-terminated
            return std::string(reinterpret_cast<const char*>(vec.data()), vec.size());
        }

        static std::vector<uint8_t> substr_of_char(const char *str, int start, int end) {
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

    private:

};
