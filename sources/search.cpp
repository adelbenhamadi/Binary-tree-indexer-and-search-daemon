
#include "config.h"
#include "helpers/utility.h"
#include "core/mynodes.h"
#include "helpers/config-parser.h"

using namespace mynodes;
using namespace mynodes::globals;

MySource * PrepareSourceMySQL ( const CSphConfigSection & hSource, const char * sSourceName,char* sError)
{
	assert ( hSource["type"]=="mysql" );

	#define LOC_GET(_key) \
				hSource.Exists(_key) ? hSource[_key].cstr() : NULL;

	MySourceParams_SQL tParams;

	tParams.m_sQuery			= LOC_GET ( "sql_query" );
	tParams.m_sQueryPre			= LOC_GET ( "sql_query_pre" );
	tParams.m_sQueryPost		= LOC_GET ( "sql_query_post" );
	tParams.m_sQueryRange		= LOC_GET ( "sql_query_range" );
	//tParams.m_sQueryPostIndex	= LOC_GET ( "sql_query_post_index" );
	//tParams.m_sGroupColumn		= LOC_GET ( "sql_group_column" );
	//tParams.m_sDateColumn		= LOC_GET ( "sql_date_column" );
	tParams.m_sHost				= LOC_GET ( "sql_host" );
	tParams.m_sUser				= LOC_GET ( "sql_user" );
	tParams.m_sPass				= LOC_GET ( "sql_pass" );
	tParams.m_sDB				= LOC_GET ( "sql_db" );
	tParams.m_sUsock			= LOC_GET ( "sql_sock" );

	#undef LOC_GET
	#define LOC_GET(_arg,_key) \
		if ( hSource.Exists(_key) && hSource[_key].intval() ) \
			_arg = hSource[_key].intval();

	LOC_GET ( tParams.m_iPort,		"sql_port" );
	LOC_GET ( tParams.m_iRangeStep,	"sql_range_step" );

	#undef LOC_GET

	MySource_MySQL * pSrcMySQL = new MySource_MySQL( sSourceName );
	bool res = pSrcMySQL->setup ( tParams , sError)/* && pSrcMySQL->load(sError)*/;

	if( !res ){
		safeDelete ( pSrcMySQL );
		sprintf(sError,"source %s cannot be setup or load!",sSourceName);

	}


	return pSrcMySQL;
}
bool SetupIndex(const CSphConfigSection &hIndex, MyIndexSettings &tSettings, char *sError)
{
	// misc settings
	tSettings.m_iMinPrefixLen = std::max(hIndex.GetInt("min_prefix_len"), 0);
	tSettings.m_iMinInfixLen = std::max(hIndex.GetInt("min_infix_len"), 0);
	tSettings.m_iMaxSubstringLen = std::max(hIndex.GetInt("max_substring_len"), 0);

	bool bWordDict = (strcmp(hIndex.GetStr("dict", "keywords"), "keywords") == 0);

	if (bWordDict && tSettings.m_iMaxSubstringLen > 0)
	{
		sError = const_cast <char*>("max_substring_len can not be used with dict=keywords");
		return false;
	}

	// html stripping
	if (hIndex("html_strip"))
	{
		tSettings.m_bHtmlStrip = hIndex.GetInt("html_strip") != 0;
		
	}
	//stemming
	tSettings.m_bStemming = false;
	if (hIndex("stemmer")) {
		if (hIndex["stemmer"] == "en")  tSettings.m_bStemming = true;
	}
	
	return true;
}
bool compareHits(LeafItem& lh,LeafItem& rh){ return lh.hits > rh.hits;}
void doSearch(MyIndex *pIndex,bool b_showResultDocs)
{

	//searching
	printInfo("\n\nbegin searching\n");
	char word[64];
	size_t climit = 0;
	timePoint_t t_s1,t_s2;
    Expression_t hExpression;
	while (1)
	{
		printInfo("\nPlease enter a word to search for (press ! to quit): ");
		utility::scan_stdin("%s %d", 2, word, &climit);
		if (strcasecmp(word, "!") == 0)
			return;

		printInfo("\n-----------------\n");
		if (climit == 0)
		{
			climit = MAX_RESULT_LIMIT;
		}
		
		t_s1 = STD_NOW;
        hExpression.value = (string_t)word;
		pIndex->doSearch(hExpression);

        if(pIndex->m_tResultItems.size()>0){
		t_s2 = STD_NOW;
        std::sort(pIndex->m_tResultItems.begin(), pIndex->m_tResultItems.end(),compareHits );
        auto last = std::unique(pIndex->m_tResultItems.begin(), pIndex->m_tResultItems.end());
        pIndex->m_tResultItems.erase(last, pIndex->m_tResultItems.end());
		utility::print_time("sorting results...in ", t_s2);
		size_t i = 1;
        u64_t lastDid = -1;
		printInfo("\n-----------------\n");
		for (LeafItemsVector_t::iterator it = pIndex->m_tResultItems.begin();
			 i <= climit && lastDid!=it->did  && it != pIndex->m_tResultItems.end(); ++it)
		{

			lastDid = it->did;

			if(b_showResultDocs){
              std::string doc = pIndex->get_document(pIndex->vSources[0],*it);
			//only first 100 char
			//doc = doc.substr(0, 300);
			  printInfo("\n%d.[%llu %d hits] \n%s", i++,it->did,it->hits , doc.c_str());
			}else{
				printInfo("\n%d.%llu [%d]", i++,it->did,it->hits );
			}
		}
        }
		printInfo("\n-----------------\n");
		printInfo("\n %llu docs found for %s\n", (u64_t)(pIndex->m_iResultCount), ((string_t)word).c_str());

		utility::print_time("searching done in", t_s1);
	}
	return;
}
MyIndex* PrepareIndex(const CSphConfigSection &hIndex, const string_t sIndexName,
			 const MyConfigType &hSources, bool bVerbose)
{
	//TODO check index type

	// progress bar
	if (bVerbose)
	{
		fprintf(stdout, "loading index '%s'...\n", sIndexName.c_str());
		fflush(stdout);
	}

	// check config
	if (!hIndex("path"))
	{
		fprintf(stdout, "ERROR: index '%s': key 'path' not found.\n", sIndexName.c_str());
		return NULL;
	}

	char* sError = const_cast <char*>("");
	MyIndexSettings tSettings;
	if (!SetupIndex(hIndex, tSettings, sError))
		diePrintf("index '%s': %s", sIndexName.c_str(), sError);
    string_t sIndexPath;
	sIndexPath = hIndex["path"].strval();
	//if (g_bRotate)	sIndexPath += ".tmp";

	//create indexer
	MyIndex* pIndex = new MyIndex(sIndexName, sIndexPath);
	assert(pIndex);

	//pIndex->assignProgressCallback(PrintProgress);

	tSettings.m_bVerbose = bVerbose;
	pIndex->m_tSettings = tSettings;
	// parse and prepare all sources

	bool bGotAttrs = false;
	bool bPrepareSourceFailed = false;

	for (CSphVariant *pSourceName = hIndex("source"); pSourceName; pSourceName = pSourceName->m_pNext)
	{
		auto sourceName = pSourceName->cstr();
		if (!hSources(sourceName))
		{
			fprintf(stdout, "ERROR: index '%s': source '%s' not found.\n", sIndexName.c_str(), sourceName);
			continue;
		}

		const CSphConfigSection &hSource = hSources[sourceName];
		MySource *pSource = PrepareSourceMySQL(hSource, sourceName,sError);

		if (!pSource)
		{
			bPrepareSourceFailed = true;
			fprintf(stdout, "ERROR: %s\n", sError);
			continue;
		}


		pIndex->vSources.push_back(pSource);
		debugPrintf(1,"\nsource %s added to index %s",sourceName,sIndexName.c_str());
	}

	if (bPrepareSourceFailed)
	{
		fprintf(stdout, "ERROR: index '%s': failed to configure some sources, will not index.\n", sIndexName);
		return NULL;
	}

	if (!pIndex->vSources.size())
	{
		fprintf(stdout, "ERROR: index '%s': no sources to index; skipping.\n", sIndexName);
		return NULL;
	}
	return pIndex;
}
int main(int argc, char *argv[])
{

	bool bVerbose = true;
	g_sExecName = argv[0];
	g_bVerbose = 1;
	g_bPrintProgress = 1;
	int iFailed = 0, iLoaded = 0;
	//load config
	const char *sOptConfig = NULL;
	CSphConfigParser cp;
	MyConfig &hConf = cp.m_tConf;
	sOptConfig = cp.loadConfig(sOptConfig, g_bVerbose);

	if (!hConf("source"))
	{
		diePrintf("no indexes found in config file '%s'", sOptConfig);
	}
	if (hConf("indexer") && hConf["indexer"]("indexer"))
	{

		CSphConfigSection &hIndexer = hConf["indexer"]["indexer"];
		g_iMemLimit = hIndexer.GetSize("mem_limit", g_iMemLimit);
		g_iWriteBuffer = hIndexer.GetSize("write_buffer", 1024 * 1024);
	}
	if (hConf("common") && hConf["common"]("common"))
	{
		CSphConfigSection &hCommon = hConf["common"]["common"];
	}
	std::vector<MyIndex *> dIndexes;
	hConf["index"].IterateStart();
	while (hConf["index"].IterateNext())
	{

		//create index
		MyIndex* pIndex  = PrepareIndex(hConf["index"].IterateGet(), hConf["index"].IterateGetKey().c_str(), hConf["source"], g_bVerbose);

		if (pIndex)
		{
			//	pIndex->load_data("./data");
			dIndexes.push_back(pIndex);
			iLoaded++;
		}else
		{
			iFailed++;
		}
		debugPrintf(1, "\n%d indexes config sections has been loaded, %d has failed!", iLoaded, iFailed);
	}
	assert(!dIndexes.empty());
	//load only first index!!
	
	if (bVerbose)
	{
		fprintf(stdout, "loading data for index '%s'...\n", dIndexes[0]->m_sIndexName.c_str());
		fflush(stdout);
	}

	dIndexes[0]->load_data(true);

	doSearch(dIndexes[0],true);

	for (auto p : dIndexes) {
		safeDelete(p);
	}
	return 0;
}
