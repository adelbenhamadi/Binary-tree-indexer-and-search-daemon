
#include "utils.h"
namespace mynodes {

std::string random_string(int seed){

    std::string possible_characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    auto seed2 =  std::chrono::duration_cast<std::chrono::milliseconds>(STD_NOW.time_since_epoch()).count();
    std::mt19937 engine(seed*5+seed2);
    std::uniform_int_distribution<> dist(0, possible_characters.size()-1);
    int irand = dist(engine) % 16 + 3;
    std::string ret = "";
    for(int i = 0; i < irand; i++){
        int irand = dist(engine); //get index between 0 and possible_characters.size()-1
        ret += possible_characters[irand];
    }
    return ret+" ";
}
std::string random_string2()
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

void print_time(const char* mess,timePoint_t t_s){

    float duration = std::chrono::duration_cast<std::chrono::microseconds>( ( (timePoint_t) STD_NOW ) - t_s ).count();
    myprintf("\n%s %8.3f milliseconds",mess,duration/1000);
    //t_s = std::chrono::high_resolution_clock::now();
  };
void strToLower(char *s)
{
  char  *p;
  for (p = s; *p != '\0'; p++)
    *p = (char) tolower(*p);
}
void normalize_word(string_t &word){
    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    std::replace_if(word.begin(), word.end(), [](char c) {
                    std::string s = ",/;:%\"'|`@[]\\";
                    return (s.find(c) != std::string::npos );
                    } , ' ');
    //std::replace(word.begin(), word.end(), ',', ' ');
    word.erase(remove_if(word.begin(), word.end(), [](char c) { return !isspace(c) && !isalnum(c) ; } ), word.end());
}
void scan_stdin(const char* fmt,const int co, ...)
{
    va_list ap;
    va_start(ap, co);
    if(std::vfscanf(stdin,fmt, ap) != co)
        throw std::runtime_error("parsing error");
    va_end(ap);
}


MYSQL* mysql_connection_setup(struct serverConnectionConfig mysql_details)
{

    MYSQL *connection = mysql_init(NULL);
    if (!mysql_real_connect(connection,mysql_details.server, mysql_details.user, mysql_details.password, mysql_details.database, 0, NULL, 0)) {
      printf("Conection error : %s\n", mysql_error(connection));
      exit(1);
    }
    return connection;
}

MYSQL_RES* mysql_perform_query(MYSQL *connection, char *sql_query)
{
   // send the query to the database
   if (mysql_query(connection, sql_query))
   {
      printf("MySQL query error : %s\n", mysql_error(connection));
      exit(1);
   }

   return mysql_use_result(connection);
}
sVector_t scan_exp(const string_t& exp){
    string_t  expc= exp, temp;
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
//std::chrono::high_resolution_clock::time_point t_s;
bool fixBufferSize(std::FILE* fh, const __blksize_t newSize) {
	struct stat stats;

	if (fstat(fileno(fh), &stats) == -1) { // POSIX only
		perror("fstat");
		return false;
	}
	//__blksize_t newSize = 8092*4;
	//__blksize_t newSize =stats.st_blksize;
	if (newSize == BUFSIZ)
		return true;
	std::cout << "\nBUFSIZ is " << BUFSIZ << ", changing it size to " << newSize
			<< '\n';
	/*std::cout << "\nBUFSIZ is " << BUFSIZ << ", but optimal block size is "
	 << stats.st_blksize << '\n';
	 */
	if (std::setvbuf(fh, NULL, _IOFBF, newSize) != 0) {
		perror("setvbuf failed"); // POSIX version sets errno
		return false;
	}
	return true;
}
void writeStringToBinFile(const int fdes, const string_t& st) {
	size_t sz = st.size();
	write(fdes, &sz, sizeof(size_t));
	write(fdes, st.c_str(), sz);
}
void writeStringToBinFile(const string_t& st, std::FILE* fh) {
	size_t sz = st.size();
	std::fwrite(&sz, sizeof(size_t), 1, fh);
	std::fwrite(st.c_str(), sz, 1, fh);
}
string_t readStringFromBinFile(std::FILE* fh) {
	size_t sz;
	std::fread(&sz, sizeof(size_t), 1, fh);
	char cstr[sz];
	cstr[sz] = '\0'; //very important
	std::fread(&cstr, sz, 1, fh);

	string_t st = (char*) cstr;
	return st;
}
string_t readStringFromBinFile(const int fdes) {
	size_t sz;
	read(fdes, &sz, sizeof(size_t));
	char cstr[sz];
	cstr[sz] = '\0'; //very important
	read(fdes, &cstr, sz);

	string_t st = (char*) cstr;
	//myprintf("\nread string: %s coverted from %s",st.c_str(),cstr);
	return st;
}

u64_t getFilesize(const char* filename) {
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}
bool hasSuffix(const string_t& in, const string_t & suf) {
	size_t pos = in.find(suf);      // position of suf in in
	if (pos == STD_NPOS) {
		return false;
	}
	string_t f = in.substr(pos);     // get from pos to the end
	return f == suf;
}
sVector_t explode(string_t const & s, char delim) {
	sVector_t result;
	std::istringstream iss(s);

	for (std::string token; std::getline(iss, token, delim);) {
		result.push_back(std::move(token));
	}

	return result;
}
string_t implode(sVector_t & elements, char delim) {
	string_t full;
	for (sVector_t::iterator it = elements.begin();
			it != elements.end(); ++it) {
		full += (*it);
		if (it != elements.end() - 1)
			full += delim;

	}
	return full;
}
;
}
