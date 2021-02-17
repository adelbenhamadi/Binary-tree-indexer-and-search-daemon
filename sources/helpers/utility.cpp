
#include "./utility.h"
#include <random>

namespace mynodes {

    namespace utility {

        std::string random_string(int seed)
        {

            std::string possible_characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

            unsigned int seed0 = (unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(STD_NOW.time_since_epoch()).count();
            std::mt19937 engine((unsigned int)seed * 5 + seed0);
            std::uniform_int_distribution<> dist(0, (int)possible_characters.size() - 1);
            int irand = dist(engine) % 16 + 3;
            std::string ret = "";
            for (int i = 0; i < irand; i++) {
                int irand = dist(engine); //get index between 0 and possible_characters.size()-1
                ret += possible_characters[irand];
            }
            return ret + " ";
        }
        std::string random_string_2()
        {
            std::string possible_characters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
            /* initialize random seed: */
            std::random_device rd;
            std::mt19937 generator(rd());

            std::shuffle(possible_characters.begin(), possible_characters.end(), generator);

            std::uniform_int_distribution<> dist(0, 16);
            int irand = dist(rd) % 16 + 3;
            return possible_characters.substr(0, irand) + " ";    // assumes 32 < number of characters in possible_characters
        }

        void print_time(const char* msg, timePoint_t t_s)
        {

            int64_t duration = std::chrono::duration_cast<std::chrono::microseconds>(((timePoint_t)STD_NOW) - t_s).count();
            printInfo("\n%s %8.2f milliseconds", msg, (float)duration / 1000);
            //t_s = std::chrono::high_resolution_clock::now();
        };

        inline void normalize_doc(string_t& doc)
        {
            std::transform(doc.begin(), doc.end(), doc.begin(), ::tolower);
            std::replace_if(doc.begin(), doc.end(), [](char c) {
                std::string s = ",/;:%\"'|`@[]\\";
                return (s.find(c) != std::string::npos);
                }, ' ');
            // doc.erase(remove_if(doc.begin(), doc.end(), [](char c) { return !isspace(c) && !isalnum(c) ; } ), doc.end());
        }
        void scan_stdin(const char* fmt, const int co, ...)
        {
            va_list ap;
            va_start(ap, co);
            if (std::vfscanf(stdin, fmt, ap) != co)
                throw std::runtime_error("parsing error");
            va_end(ap);
        }

        sVector_t scan_exp(const string_t& exp)
        {
            string_t  expc = exp, temp;
            sVector_t tokens;
            tokens.clear();
            while (expc.find(" ", 0) != STD_NPOS)
            { //does the string a comma in it?
                size_t  pos = expc.find(" ", 0); //store the position of the delimiter
                temp = expc.substr(0, pos);      //get the token
                expc.erase(0, pos + 1);         //erase it from the source
                tokens.push_back(temp);        //and put it into the array
            }
            tokens.push_back(expc);           //the last token is all alone
            return tokens;
        }
        bool writeVector(std::ofstream& input, char* pVec, size_t sz, size_t co, bool bWriteCount, bool bClose)
        {
            if (input)
            {
                if (bWriteCount) {
                    input.write((char*)&co, sizeof(size_t));
                }
                input.write(pVec, co * sz);
                if (bClose) {
                    input.close();
                }
            }
            else
            {
                perror("Error opening file for writing");
                return false;
            }

            return true;
        }

        void writevecstring(std::ostream& os, const std::vector<std::string>& vec)
        {
            typename std::vector<std::string>::size_type size = vec.size();
            os.write((char*)&size, sizeof(size));

            for (typename std::vector<std::string>::size_type i = 0; i < size; ++i)
            {
                typename std::vector<std::string>::size_type element_size = vec[i].size();
                os.write((char*)&element_size, sizeof(element_size));
                os.write(&vec[i][0], element_size);
            }
        }

        void readvecstring(std::istream& is, std::vector<std::string>& vec)
        {
            typename std::vector<std::string>::size_type size = 0;
            is.read((char*)&size, sizeof(size));
            vec.resize(size);

            for (typename std::vector<std::string>::size_type i = 0; i < size; ++i)
            {
                typename std::vector<std::string>::size_type element_size = 0;
                is.read((char*)&element_size, sizeof(element_size));
                vec[i].resize(element_size);
                is.read(&vec[i][0], element_size);
            }
        }


        void writeStringToBinFile(const int fdes, const string_t& st)
        {
            unsigned int sz = (unsigned int)st.size();
            _write(fdes, &sz, sizeof(size_t));
            _write(fdes, st.c_str(), sz * sizeof(char));
        }

        void writeStringToBinFile(const string_t& st, std::FILE* fh)
        {
            unsigned int sz = (unsigned int)st.size();
            std::cout << "\t" << sz;
            int iLen = (int)std::fwrite(&sz, sizeof(size_t), 1, fh);
            if (iLen != 1) {
                std::cout << "\nerror iLen: " << iLen;
            }
            iLen = (int)std::fwrite(st.c_str(), sz * sizeof(char), 1, fh);
            if (iLen != 1) {
                std::cout << "\nerror iLen: " << iLen;
            }
        }
        void writeStringToBinFile(const string_t& st, std::ostream& os)
        {
            unsigned int sz = (unsigned int)st.size();
            os.write((char*)&sz, sizeof(size_t));
            os.write(&st[0], sz);

        }
        bool readStringFromBinFile(std::istream& is, char* dest)
        {
            unsigned int sz;
            is.read((char*)&sz, sizeof(size_t));
            is.read(dest, sz);
            return true;
        }
        string_t readStringFromBinFile(std::istream& is)
        {
            unsigned int sz;
            is.read((char*)&sz, sizeof(size_t));
            string_t st = "";
            st.resize(sz);
            is.read(&st[0], sz);
            return st;
        }
        string_t readStringFromBinFile(std::FILE* fh)
        {
            
            unsigned int sz;
            int iLen = (int)std::fread(&sz, sizeof(size_t), 1, fh);
            if (iLen != 1) {
                std::cout << "\nsize error: iLen: " << iLen;
            }
            char cstr[100];
            cstr[sz] = '\0'; //very important!!!!
            iLen = (int)std::fread(&cstr, sz * sizeof(char), 1, fh);
            if (iLen != 1) {
                std::cout << "\nread error: iLen: " << iLen;
            }
            //convert
            string_t st = (char*)cstr;
            return st;
        }
        string_t readStringFromBinFile(const int fdes)
        {
            unsigned int sz;
            _read(fdes, &sz, sizeof(size_t));
            char cstr[100];
            cstr[sz] = '\0'; //very important
            _read(fdes, &cstr, sz);

            string_t st = (char*)cstr;
            //myprintf("\nread string: %s coverted from %s",st.c_str(),cstr);
            return st;
        }

        u64_t getFilesize(const char* filename)
        {
            struct stat st;
            stat(filename, &st);
            return st.st_size;
        }
        
        void explode(sVector_t& dOut, string_t const& s, char delim)
        {
            std::istringstream iss(s);
            dOut.clear();
            for (std::string token; std::getline(iss, token, delim);) {
                dOut.push_back(std::move(token));
            }

        }

        string_t implode(sVector_t& elements, char delim)
        {
            string_t full;
            for (sVector_t::iterator it = elements.begin();
                it != elements.end(); ++it) {
                full += (*it);
                if (it != elements.end() - 1) {
                    full += delim;

                }

            }
            return full;
        }


    }
}
