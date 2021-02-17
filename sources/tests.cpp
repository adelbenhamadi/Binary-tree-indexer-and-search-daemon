#include "config.h"
#include "helpers/utility.h"
#include "core/mynodes.h"


using namespace mynodes;
using namespace mynodes::globals;

bool setup_index(MyIndexSettings& tSettings)
{
	// misc settings
	tSettings.m_iMinPrefixLen = 0;
	tSettings.m_iMinInfixLen = 0;
	tSettings.m_iMaxSubstringLen = 0;

	tSettings.m_bHtmlStrip = true;
	

	//stemming
	tSettings.m_bStemming = true;
	tSettings.m_iMemLimit = sizeof(LeafItem) * 3;
	return true;
}
MySource* get_source_mysql(const char* sSourceName)
{

	MySourceParams_SQL tParams;

	tParams.m_sQuery = "select 111528 as id, \"tunisia algeria turkey city tunisia cruse ship tunisia manhatten newyork city\" as comment;";
	tParams.m_sQueryPre = "";
	tParams.m_sQueryPost = "";
	tParams.m_sQueryRange = "";
	//tParams.m_sQueryPostIndex	= "sql_query_post_index";
	//tParams.m_sGroupColumn		=  "sql_group_column";
	//tParams.m_sDateColumn		= "sql_date_column";
	tParams.m_sHost = "localhost";
	tParams.m_sUser = "root";
	tParams.m_sPass = "root";
	tParams.m_sDB = "mynodes_db";
	tParams.m_sUsock = "";
	tParams.m_iPort = 3306;
	tParams.m_iRangeStep = 0;

	char* sError = const_cast <char*>("");
	MySource_MySQL* pSrcMySQL = new MySource_MySQL(sSourceName);
	bool res = pSrcMySQL->setup(tParams, sError) /*&& pSrcMySQL->load(sError)*/;

	if (!res) {
		safeDelete(pSrcMySQL);
		sprintf(sError, "source %s cannot be setup or load!", sSourceName);

	}


	return pSrcMySQL;
}
void test_load_source() {
	std::cout << "test_load_source";
	MySource* pSource = get_source_mysql("source_test_mysql");
	assert(pSource && "get_source_mysql failed");
	std::cout << " -> OK" << std::endl;
}
MyIndex* create_index_0() {
	std::cout << "create_index_0";
	MyIndexSettings tSettings;
	assert(setup_index(tSettings) && "setup_index failed");
	string_t sIndexPath = "./data/test_data";
	string_t sIndexName = "index_0";
	MyIndex* pIndex = new MyIndex(sIndexName, sIndexPath);
	assert(pIndex && "Index creation failed");
	//pIndex->assignProgressCallback(PrintProgress);
	pIndex->m_tSettings = tSettings;
	pIndex->m_tSettings.m_bVerbose = true;
	std::cout << " -> OK" << std::endl;
	return pIndex;
}
void test_setup_source(MyIndex* pIndex) {
	std::cout << "test_setup_source";
	MySource* pSource = get_source_mysql("source_test_mysql");
	pIndex->vSources.push_back(pSource);
	assert(pIndex->vSources[0] && "vSources is empty!");
	std::cout << " -> OK" << std::endl;
}
void test_save_leaves_raw(MyIndex* pIndex) {
	std::cout << "test_save_leaves_raw";
	std::vector<LeafItem> vLeafItems;
	vLeafItems.push_back({ 99,100021,1,1 });
	vLeafItems.push_back({ 104,100021,1,1 });
	vLeafItems.push_back({ 101,100001,1,1 });
	vLeafItems.push_back({ 101,100001,1,10 });
	vLeafItems.push_back({ 101,100001,1,23 });
	vLeafItems.push_back({ 102,100011,1,1 });
	vLeafItems.push_back({ 102,100011,1,1 });
	vLeafItems.push_back({ 103,100111,1,1 });
	vLeafItems.push_back({ 103,101111,1,1 });
	vLeafItems.push_back({ 105,111111,1,1 });
	//sort leafItems by tokenId
	std::sort(vLeafItems.begin(), vLeafItems.end(), [](LeafItem& lh, LeafItem& rh) { return lh.tokenId > rh.tokenId;  });

	FileWriter os(pIndex->getIndexFilename("lvs"));
	os.write((char*)&(vLeafItems)[0], vLeafItems.size() * sizeof(LeafItem));
	std::cout << " -> OK" << std::endl;
}
void test_load_leaves_raw(MyIndex* pIndex) {

	std::cout << "test_load_leaves_raw" ;
	pIndex->m_tLeaves.reset();
	pIndex->load_leaves();
	assert(pIndex->m_tLeaves.m_iLeavesCount == 7 && "m_tLeaves.m_iLeavesCount wrong");
	{
		auto& i = pIndex->m_tLeaves.m_vDict.at(100001);
		assert(i.m_vLeafItems.size() == 1);
		assert(i.m_vLeafItems.front().hits == 3);
	}
	{
		auto& i = pIndex->m_tLeaves.m_vDict.at(100011);
		assert(i.m_vLeafItems.size() == 1);
		assert(i.m_vLeafItems.front().hits == 2);
	}
	{
		auto& i = pIndex->m_tLeaves.m_vDict.at(100021);
		assert(i.m_vLeafItems.size() == 2);
		
	}
	std::cout << " -> OK" << std::endl;
}
void test_build_index(MyIndex* pIndex) {
	std::cout << "test_build_index";
	assert(pIndex->build() && "build index failed");

	std::cout << " -> OK" << std::endl;
}
void test_save_index(MyIndex* pIndex) {
	std::cout << "test_save_index";
	assert(pIndex->save_data());

	std::cout << " -> OK" << std::endl;
}
void test_load_index(MyIndex* pIndex) {
	std::cout << "test_load_index";
	
	assert(pIndex->load_data(true));

	std::cout << " -> OK" << std::endl;
}
void test_index_stats(MyIndex* pIndex) {
	std::cout << "test_index_stats";
	assert(pIndex->m_tLeaves.m_iLeavesCount == 8);
	assert(pIndex->m_tLeaves.m_vDict.size() == 8);
	auto& hasher = pIndex->m_hasher;
	bool bStem = pIndex->m_tSettings.m_bStemming;
	tid_t city = hasher.hash("city",bStem),
		newyork = hasher.hash("newyork",bStem),
		tunisia = hasher.hash("tunisia",bStem),
		algeria = hasher.hash("algeria", bStem),
		france = hasher.hash("france",bStem);
	{
		assert(pIndex->m_tLeaves.find(city) == true);
		auto& leaf = pIndex->m_tLeaves.m_vDict[city];
		assert(leaf.m_vLeafItems.size() == 1);
		//assert(leaf.m_iCount == 1);
		assert(leaf.m_vLeafItems[0].hits == 2);
	}
	{
		assert(pIndex->m_tLeaves.find(tunisia) == true);
		auto& leaf = pIndex->m_tLeaves.m_vDict[tunisia];
		assert(leaf.m_vLeafItems.size() == 1);
		//assert(leaf.m_iCount == 1);
		assert(leaf.m_vLeafItems[0].hits == 3);
	}
	assert(pIndex->m_tLeaves.find(newyork) == true);
	assert(pIndex->m_tLeaves.find(algeria) == true);
	assert(pIndex->m_tLeaves.find(france) == false);

	std::cout << " -> OK" << std::endl;
}
void test_indexing(MyIndex* pIndex) {

	std::cout << "test_indexing" << std::endl;
	test_setup_source(pIndex);
	test_build_index(pIndex);
	test_save_index(pIndex);
	test_load_index(pIndex);
	test_index_stats(pIndex);
	std::cout << "OK" << std::endl;
}
inline int isValidChar(const char* c)
{
	return (*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'z') || (*c == '-') || (*c == '_');
}
inline int isValidChar2(const char* c)
{
	return (*c >= '0' && *c <= '9') + (*c >= 'a' && *c <= 'z') + (*c == '-') + (*c == '_');
}
void bench_tokenizer() {
	
	constexpr int _max_word_len = 64;
	size_t _max = 100000;
	char sToken[ _max_word_len ];
	string_t word;
	const char* sNext;
	int pw = 0;
	size_t sz0 = 0,iToken = 0, tokenCo = 0 , co = 0, bytes = 0;
	Hasher_t hasher;
	std::vector<string_t> docs;
	docs.reserve(_max);
	std::cout << "Fill " << _max << " docs" << std::endl;
	string_t someLongText = "<html><head><title>my-site-title</title><meta charset=\"utf - 8\"><meta name=\"viewport\" content=\"width = device - width, initial - scale = 1.0,maximum - scale = 1.0, user - scalable = no\"></head>";
	someLongText += "<style>/* Copyright stuff */ button{ border: 0;	border - radius: 4px;} </style>";
	someLongText += "<body><P>ParaGraph< / p><br>noRMal - text<span>span - text< / span> < / div>";
	someLongText += "<section><div><P>ParaGraph</p><br>noRMal-text<span>span-text</span></div></section>";
	someLongText += "<article><DIV class=\"myclass bold otherclass\">some-div-Text-here-again-contenT</Div></Article>";
	someLongText += "testing1 in<b><font color='red'>line</font> ele<em>men</em>ts";
	someLongText += "testing2 &#1040;&#1041;&#1042; utf-encoding";
	someLongText += "<UL><li>item1</lI><LI>ITem2</li></ul>";
	someLongText += "<oL><li>item11</lI><LI>ITem12</li></ul>";
	someLongText += "some comm<!-- should-not-be-seen -->ents";
	someLongText += "<pre>some-pre-here</pre>";
	someLongText += "<code>should-not-be-tokenized</code>";
	someLongText += "<script type=\"javascript\">function(){ alert(\"something\");}</script>";
	someLongText += "<template type=\"type1\" property1=\"0\" property2=\"6464\">should-not-tokenize-this</template>";
	someLongText += "<img alt=\"myimage\" title=\"imagetitle\" src=\"http://somesite/img.jpg\"/>";
	someLongText += "<a title=\"imagetitle\" href=\"http://somesite/somelink.html\">some-link_here</a></body></html>";
	for (int i = 0; i < _max; ++i) {
		docs.push_back( someLongText );
	}
	std::cout << "Tokenizing.." << std::endl;
	timePoint_t ts = STD_NOW;
	for (auto& doc : docs) {
		co++;
		for (auto it = doc.begin(); it != doc.end(); ++it) {
			*it |= 0x20; //tolower
			//*it &= ~0x20;//toupper

		}
		//this is slower!!
		//std::transform(doc.begin(), doc.end(), doc.begin(), ::tolower);
		const char* p = doc.c_str();
		bytes += doc.size();
		while (*p)
		{
			//strip html & tokenize in 1 pass
			while (*p && !isValidChar(p)) {
				if (*p == '&') {
					while (*p) {
						p++;
						if (*p == ';') break;
					}
				}
				else if (*p == '<') {
					
					if (*(p+2) && (p[1] == 'p' || p[1] == 'i' || p[1] == 'b') && p[2] == '>') p += 3;
					else if (*(p + 3) && ((p[1] == 'u' && p[2] == 'l') || (p[1] == 'o' && p[2] == 'l') || (p[1] == 'l' && p[2] == 'i')) && p[3] == '>') p += 4;
					else if (
						*(p + 4)
						&& ((p[1] == 'p' && p[2] == 'r' && p[3] == 'e'))
						&& p[4] == '>') p += 5;
					else if (
						
						(*(p + 6) && (p[1] == 's' && p[2] == 'c' && p[3] == 'r' && p[4] == 'i' && p[5] == 'p' && p[6] == 't') )
						||
						(*(p + 5) && (p[1] == 's' && p[2] == 't' && p[3] == 'y' && p[4] == 'l' && p[5] == 'e') )
						||
						(*(p + 4) && (p[1] == 'c' && p[2] == 'o' && p[3] == 'd' && p[4] == 'e') )
						||
						(*(p + 8) && (p[1] == 't' && p[2] == 'e' && p[3] == 'm' && p[4] == 'p' && p[5] == 'l' && p[6] == 'a' && p[7] == 't' && p[8] == 'e') )

						) {
						while (*p) {
							p++;
							if (*p == '<' && p[1] == '/') break;
						}
					}
					else {
						while (*p) {
							p++;
							if (*p == '>') break;
						}
					}
					
				}
				else  p++;

			}
			//tokenize
			sNext = p;
			while (isValidChar(p))
				p++;
			if (sNext == p)
			{
				continue;
			}
			sz0 = p - sNext;
			if (sz0 < g_iMinKeywordSize) continue;
			if (sz0 >= _max_word_len)	sz0 = _max_word_len-1;
			strncpy(sToken, sNext, sz0);
			sToken[sz0] = '\0';//important!
			tokenCo++;
			//stemming & hashing
			iToken = hasher.hash(sToken, false);
			if(co<2) printf("\nsToken = %s\ttokenId = %llu", sToken,iToken);

		}//end while
	
	}
	
	float duration = std::chrono::duration_cast<std::chrono::microseconds>(((timePoint_t)STD_NOW) - ts).count() ;
	printf("\nDone in %.3f ms\t%.3f Mb\t%lluk tokens",  duration / 1000 , (float)bytes / 1000000, tokenCo / 1000);
	printf("\n%.1fK docs/s\t%.1fM token/s\t%.2f Mb/s\n", (float)co * 1000 / duration , (float)tokenCo  / duration, (float)bytes / duration);
}
int main(int* argc, char* argv) {
	std::cout << "Running Tests.." << std::endl;

	//prepare
	MyIndex* pIndex = create_index_0();

	//tests
	test_indexing(pIndex);

	//test_save_leaves_raw(pIndex);
	//test_load_leaves_raw(pIndex);

	bench_tokenizer();

	std::cout << "--------------------" << std::endl;
	std::cout << "ALL TESTS SUCCEEDED!" << std::endl;
	return 0;
}
