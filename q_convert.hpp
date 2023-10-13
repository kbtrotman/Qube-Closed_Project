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

class q_convert {

	public:
        static std::shared_ptr<spdlog::logger> _Qlog;

		q_convert (q_log& q);
        static std::string char2string (char *str);
        static const char *string2char (std::string *str);
        static const unsigned char *vect2uchar (std::vector<uint8_t> *vec);
        static const char *vect2char (const std::vector<uint8_t> *vec);
        template <typename T> static std::vector<uint8_t> char2vect(T* str, size_t length);
        static std::vector<uint8_t> string2vect (std::string *str);
        static std::string vect2string(const std::vector<uint8_t>& vec);
        static std::vector<uint8_t> substr_of_char(const char *str, int start, int end);
};
