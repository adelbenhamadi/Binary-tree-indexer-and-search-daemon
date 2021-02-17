//***************************************************************************************/
//*    snippet of this code are adapted from Sphinxsearch source code
//*    Author: Copyright (c) 2001-2016, Andrew Aksyonoff / Copyright (c) 2008-2016, Sphinx Technologies Inc
//*    Date: ?
//*    Code version: 2.95
//*    Availability: http://www.sphinxsearch.com
//*
//***************************************************************************************///

#include "config.h"
#include "helpers/utility.h"
#include "core/mynodes.h"
#include "helpers/config-parser.h"

using namespace mynodes;
using namespace mynodes::globals;

bool compareHits(LeafItem a, LeafItem b) { return a.hits > b.hits; }

bool SetupIndex(const CSphConfigSection& hIndex, MyIndexSettings& tSettings, char* sError)
{
	// misc settings
	tSettings.m_iMinPrefixLen = std::max(hIndex.GetInt("min_prefix_len"), 0);
	tSettings.m_iMinInfixLen = std::max(hIndex.GetInt("min_infix_len"), 0);
	tSettings.m_iMaxSubstringLen = std::max(hIndex.GetInt("max_substring_len"), 0);

	bool bWordDict = (strcmp(hIndex.GetStr("dict", "keywords"), "keywords") == 0);

	// html stripping
	if (hIndex("html_strip"))
	{
		tSettings.m_bHtmlStrip = hIndex.GetInt("html_strip") != 0;
		//tSettings.m_sHtmlIndexAttrs = hIndex.GetStr ( "html_index_attrs" );
		//tSettings.m_sHtmlRemoveElements = hIndex.GetStr ( "html_remove_elements" );
	}

	//stemming
	tSettings.m_bStemming = false;
	if (hIndex("stemmer")) {
		if (hIndex["stemmer"] == "en")  tSettings.m_bStemming = true;
	}
	

	return true;
}

void DoSearch(MyIndex* pIndex, bool b_showResultDocs)
{

	//searching
	printInfo("\n\nbegin searching\n");
	char word[64];
	size_t climit = 0;
	timePoint_t t_s1, t_s2;
	Expression_t hExpression;// = Expression_t(word);
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
		hExpression.value = (string_t)word;

		t_s1 = STD_NOW;
		pIndex->doSearch(hExpression);

		if (pIndex->m_tResultItems.size() > 0) {
			t_s2 = STD_NOW;
			std::sort(pIndex->m_tResultItems.begin(), pIndex->m_tResultItems.end(), compareHits);
			auto last = std::unique(pIndex->m_tResultItems.begin(), pIndex->m_tResultItems.end());
			pIndex->m_tResultItems.erase(last, pIndex->m_tResultItems.end());
			utility::print_time("sorting results...in ", t_s2);

			size_t i = 1;
			u64_t lastDid = -1;
			printInfo("\n-----------------\n");
			for (LeafItemsVector_t::iterator it = pIndex->m_tResultItems.begin();
				i <= climit && lastDid != it->did && it != pIndex->m_tResultItems.end(); ++it)
			{

				lastDid = it->did;
				if (b_showResultDocs) {
					std::string doc = pIndex->get_document(pIndex->vSources[0], *it);
					//only first 100 char
					doc = doc.substr(0, 200);
					printInfo("\n%d.[%llu %d hits] \n%s", i++, it->did, it->hits, doc.c_str());
				}
				else {
					printInfo("\n%d.%llu [%d]", i++, it->did, it->hits);
				}
			}
		}
		printInfo("\n-----------------\n");
		printInfo("\n %llu docs found for %s\n", (u64_t)(pIndex->m_tResultItems.size()), ((string_t)word).c_str());

		utility::print_time("searching done in", t_s1);
	}
	return;
}
MySource* PrepareSourceMySQL(const CSphConfigSection& hSource, const char* sSourceName, char* sError)
{
	assert(hSource["type"] == "mysql");

#define LOC_GET(_key) \
				hSource.Exists(_key) ? hSource[_key].cstr() : NULL;

	MySourceParams_SQL tParams;

	tParams.m_sQuery = LOC_GET("sql_query");
	tParams.m_sQueryPre = LOC_GET("sql_query_pre");
	tParams.m_sQueryPost = LOC_GET("sql_query_post");
	tParams.m_sQueryRange = LOC_GET("sql_query_range");
	//tParams.m_sQueryPostIndex	= LOC_GET ( "sql_query_post_index" );
	//tParams.m_sGroupColumn		= LOC_GET ( "sql_group_column" );
	//tParams.m_sDateColumn		= LOC_GET ( "sql_date_column" );
	tParams.m_sHost = LOC_GET("sql_host");
	tParams.m_sUser = LOC_GET("sql_user");
	tParams.m_sPass = LOC_GET("sql_pass");
	tParams.m_sDB = LOC_GET("sql_db");
	tParams.m_sUsock = LOC_GET("sql_sock");

#undef LOC_GET
#define LOC_GET(_arg,_key) \
		if ( hSource.Exists(_key) && hSource[_key].intval() ) \
			_arg = hSource[_key].intval();

	LOC_GET(tParams.m_iPort, "sql_port");
	LOC_GET(tParams.m_iRangeStep, "sql_range_step");

#undef LOC_GET

	MySource_MySQL* pSrcMySQL = new MySource_MySQL(sSourceName);
	bool res = pSrcMySQL->setup(tParams, sError);

	if (!res) {
		safeDelete(pSrcMySQL);
		sprintf_s(sError, 256, "source %s cannot be setup!", sSourceName);

	}
	else {

	}


	return pSrcMySQL;
}
MyIndex* PrepareIndex(const CSphConfigSection& hIndex, const string_t sIndexName,
	const MyConfigType& hSources, bool bVerbose)
{
	//TODO check index type

	// progress bar
	if (bVerbose)
	{
		fprintf(stdout, "indexing index '%s'...\n", sIndexName.c_str());
		fflush(stdout);
	}

	// check config
	if (!hIndex("path"))
	{
		fprintf(stdout, "ERROR: index '%s': key 'path' not found.\n", sIndexName.c_str());
		return NULL;
	}

	char* sError = const_cast<char*>("");
	MyIndexSettings tSettings;
	if (!SetupIndex(hIndex, tSettings, sError))
		diePrintf("index '%s': %s", sIndexName.c_str(), sError);
	string_t sIndexPath;
	sIndexPath = hIndex["path"].strval();
	//if (g_bRotate)	sIndexPath += ".tmp";

	//create indexer
	MyIndex* pIndex = new MyIndex(sIndexName, sIndexPath);
	assert(pIndex);

	tSettings.m_bVerbose = bVerbose;
	pIndex->m_tSettings = tSettings;
	// parse and prepare all sources

	bool bGotAttrs = false;
	bool bPrepareSourceFailed = false;
	timePoint_t t_s = STD_NOW;

	for (CSphVariant* pSourceName = hIndex("source"); pSourceName; pSourceName = pSourceName->m_pNext)
	{
		auto sourceName = pSourceName->cstr();
		if (!hSources(sourceName))
		{
			fprintf(stdout, "ERROR: index '%s': source '%s' not found.\n", sIndexName.c_str(), sourceName);
			continue;
		}
		const CSphConfigSection& hSource = hSources[sourceName];

		MySource* pSource = PrepareSourceMySQL(hSource, sourceName, sError);
		if (!pSource)
		{
			bPrepareSourceFailed = true;
			fprintf(stdout, "ERROR: %s\n", sError);
			continue;
		}


		pIndex->vSources.push_back(pSource);
		debugPrintf(1, "\nsource %s added to index %s", sourceName, sIndexName.c_str());
	}

	if (bPrepareSourceFailed)
	{
		fprintf(stdout, "ERROR: index '%s': failed to configure some sources, will not index.\n", sIndexName.c_str());
		return NULL;
	}

	if (!pIndex->vSources.size())
	{
		fprintf(stdout, "ERROR: index '%s': no sources to index; skipping.\n", sIndexName.c_str());
		return NULL;
	}
	else {
		float duration = TIME_MilS(STD_NOW - t_s);
		//int64_t iTotalDocs = 0;
		//int64_t iTotalBytes = 0;
		fprintf(stdout, "\nData collected in %8.3f sec", duration / 1000);
	}
	return pIndex;
}
bool DoIndex(MyIndex* pIndex, bool bVerbose) {
	timePoint_t t_s = STD_NOW;
	bool bOK = false;
	//build index
	bOK = pIndex->build();

	if (!bOK) {
		fprintf(stdout, "ERROR: index '%s': %s.\n", pIndex->m_sIndexName.c_str(), pIndex->GetLastError());
		safeDelete(pIndex);
		return false;
	}

	pIndex->Unlock();

	// do report
	if (bVerbose)
	{

		int64_t iTotalDocs = 0;
		int64_t iTotalBytes = 0;

		for (auto pSource : pIndex->vSources)
		{
			MySourceStats* sourceStats = pSource->getStats();
			iTotalDocs += sourceStats->m_iDocumentsCount;
			iTotalBytes += sourceStats->m_iBytes;
		}
		float duration = TIME_MilS(STD_NOW - t_s);

		//fprintf(stdout, "\nTotal: %llu docs, %llu bytes\n", iTotalDocs, iTotalBytes);

		fprintf(stdout, "\nDone in %8.3f sec, %3.2f MB/sec, %d docs/sec\n",
			duration / 1000, 							 // sec
			(float)((iTotalBytes / 1048576) * 1000 / duration),													 // bytes/sec
			(int)(iTotalDocs * 1000 / duration)
		); // docs/sec
	}

	// cleanup
	return true;


}

int main(int argc, char* argv[])
{
	//std::cin.get();
	g_sExecName = argv[0];
	int bTesting = 0;
	g_bVerbose = 1;
	g_bPrintProgress = 1;
	size_t iDocLimit = 1000000;

	int i;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--limit") == 0 && (i + 2) < argc)
		{
			iDocLimit = atoi(argv[i + 2]);
		}
		else if (strcasecmp(argv[i], "--testing") == 0 && (i + 2) < argc)
		{
			bTesting = atoi(argv[i + 2]);
		}
		else
		{
			break;
		}

		if (i != argc || argc < 2)
		{
			if (argc > 1)
			{
				fprintf(stdout,
					"ERROR: malformed or unknown option near '%s'.\n",
					argv[i]);
			}
			else
			{
				fprintf(stdout,
					"Usage: myindexer [OPTIONS] \n"
					"\n"
					"Options are:\n"
					"--quiet\t\t\tbe quiet, only print errors\n"
					"--g_bVerbose\t\tg_bVerbose indexing issues report\n"
					"--noprogress\t\tdo not display progress\n"
					"--limit\t\t\tonly index <limit> documents, zero or default: index all documents\n"
					"--testing\t\t\ttesting mode for debugging\n"
					"Examples:\n"
					"indexer --limit 1000\t\tthis will index 1000 documents\n");
			}

			return 1;
		}
	}

	//load config
	const char* sOptConfig = NULL;
	CSphConfigParser cp;
	MyConfig& hConf = cp.m_tConf;
	sOptConfig = cp.loadConfig(sOptConfig, g_bVerbose);
	size_t iMemLimit = g_iMemLimit;

	if (!hConf("source"))
		diePrintf("no indexes found in config file '%s'", sOptConfig);

	if (hConf("indexer") && hConf["indexer"]("indexer"))
	{

		CSphConfigSection& hIndexer = hConf["indexer"]["indexer"];
		iMemLimit = hIndexer.GetSize64("mem_limit", g_iMemLimit);

	}
	if (hConf("common") && hConf["common"]("common"))
	{
		CSphConfigSection& hCommon = hConf["common"]["common"];
	}

	//index all
	int iIndexed = 0;
	int iFailed = 0;
	std::vector<MyIndex*> dIndexes;
	hConf["index"].IterateStart();
	while (hConf["index"].IterateNext())
	{

		MyIndex* pIndex = PrepareIndex(hConf["index"].IterateGet(), hConf["index"].IterateGetKey().c_str(), hConf["source"], g_bVerbose);
		if (pIndex)
		{
			pIndex->m_tSettings.m_iMemLimit = iMemLimit;
			if (DoIndex(pIndex, g_bVerbose)) {
				iIndexed++;
				if (!pIndex->save_data()) {
					fprintf(stdout, "ERROR: index '%s': cannot save data; skipping.\n", pIndex->m_sIndexName.c_str());
				}
				else {

					dIndexes.push_back(pIndex);
				}
			}
		}

		else
			iFailed++;
		debugPrintf(1, "\n%d indexes has been indexed, %d has failed!", iIndexed, iFailed);
	}
	if (iIndexed == 0) {
		printInfo("Nothing to search in!");
		exit(0);
	}
	//merge before search!

	//search
	assert(!dIndexes.empty());

	fprintf(stdout, "\n------------\nloading data for index '%s'...\n", dIndexes[0]->m_sIndexName.c_str());
	fflush(stdout);

	dIndexes[0]->load_data(g_bShowResultDocs);
	DoSearch(dIndexes[0], g_bShowResultDocs);

	for (auto p : dIndexes) {
		safeDelete(p);
	}
	return 0;
}
