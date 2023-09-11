/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

class q_convert : public qube_log {

	public:


		q_convert () { }

        static std::vector<uint8_t> string2vect (std::string *str) {
            std::vector<uint8_t> *tmp_vect;
            return tmp_vect.assign(str.begin(), str.end());
		}

    private:

};
