#include "utils.h"
#include "mynodes.h"


using namespace mynodes;

void ShowProgress ( const MyIndexProgress* pProgress, bool bEndProgressPhase )
{
	if ( !g_bVerbose && !g_bShowProgress  )	return;
	fprintf ( stdout, "%s%c", pProgress->BuildMessage(), bEndProgressPhase ? '\n' : '\r' );
	fflush ( stdout );
}
bool load_data_to_src(const size_t limit, DocCollection_t &src) {
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;

	struct serverConnectionConfig mysqlD;
	mysqlD.server = "localhost";
	mysqlD.user = "root";
	mysqlD.password = "*";
	mysqlD.database = "mynodes_db";

	// connect to the mysql database
	conn = mysql_connection_setup(mysqlD);
	char qry[1024];
	//qry = "select count(*) from abonne_ft_v3";
	if (limit == 0) {
		sprintf(qry,
				"select id,body from tmp");
		} else {
		sprintf(qry,
				"select id,body from tmp limit 0,%d",limit);
		}

	printf("Query: \"%s\" \n", qry);

	res = mysql_perform_query(conn, qry);
	unsigned long num_fields = mysql_num_fields(res);

	u64_t i = 0;
	DocCollection_t::iterator it = src.begin();
	sVector_t cols;
	//cols.reserve(num_fields);

	while ((row = mysql_fetch_row(res)) != NULL) {
		i = (u64_t) atoll(row[0]);

		cols.clear();

		for (unsigned int j = 1; j < num_fields; j++)
			if (row[j])
				cols.push_back(row[j]);
			else
				cols.push_back("\0");

		src.insert(it, std::pair<u64_t, sVector_t>(i, cols));

		// myprintf("\n inserted %llu => %s",i,(char*)row[1]);
	}

	/* clean up the database result set */
	mysql_free_result(res);
	/* clean up the database link */
	mysql_close(conn);

	return true;
}

int main(int argc, char *argv[]) {

	int bTesting = 2;
	g_bVerbose = 0;
	g_bShowProgress = 1;
	size_t iDocLimit = 20000;
	int iMaxWordsPerDoc = 50;

	int i;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--limit") == 0 && (i + 2) < argc) {
			iDocLimit = atoi(argv[i + 2]);

		} else if (strcasecmp(argv[i], "--testing") == 0 && (i + 2) < argc) {
			bTesting = atoi(argv[i + 2]);

		} else {
			break;
		}

		if (i != argc || argc < 2) {
			if (argc > 1) {
				fprintf( stdout,
						"ERROR: malformed or unknown option near '%s'.\n",
						argv[i]);

			} else {
				fprintf( stdout,
						"Usage: myindexer [OPTIONS] \n"
								"\n"
								"Options are:\n"
								"--quiet\t\t\tbe quiet, only print errors\n"
								"--g_bVerbose\t\tg_bVerbose indexing issues report\n"
								"--noprogress\t\tdo not display progress\n"
								"--limit\t\t\tonly index <limit> documents, zero or default: index all documents\n"
								"--testing\t\t\ttesting mode for debugging\n"
								"Examples:\n"
								"myindexer --limit 1000\t\tindex 1000 documents\n");
			}

			return 1;
		}
	}
	//init myNodes
	MyNodes mynodes;
	DocCollection_t source;
	mynodes.SetProgressCallback ( ShowProgress );


    if (!bTesting) {
		load_data_to_src(iDocLimit, source);
	}else
	if (bTesting==1) {
       myprintf("\nin Testing mode..");
       source[0].push_back("azerty appe binne serre cires") ;
       source[1].push_back("natte ,beese,fury zeeyu\\karim .kaaem ;iopee") ;
       source[2].push_back("appe/needle: facees! wallet$") ;
       source[3].push_back("businees�;:/*,fury*arom%acdc]\"begin fuueryu* saaleetre&") ;
	}else if(bTesting==2){

       std::string tmp;
       for(u64_t i=0;i<iDocLimit;i++){
        tmp = "";
        sVector_t doc ;
         for(int j=0;j<iMaxWordsPerDoc;j++){
           tmp += random_string(j)  ;

         }
        doc.push_back(tmp);
        if(g_bVerbose && i % 1000 == 0){
            myprintf("\n random doc: %s\r\n",tmp.c_str());
        }

        source.insert(source.begin(), std::pair<u64_t, sVector_t>(i, doc));
        if( i % 1000 == 0){
            myprintf("\r\t%lu random documents generated",i);
            fflush(stdout);
        }
       }

	}
	//myprintf("\nSource : %d documents loaded!", source.size());

	mynodes.appendSource(source);
	mynodes.buildIndex();
	if (!bTesting)
		mynodes.save_data("data");

	mynodes.print_documents(g_bVerbose || (iDocLimit > 0 && iDocLimit <= 10));
	mynodes.print_nodes(g_bVerbose);
	mynodes.print_leaves(g_bVerbose);

	//searching
	myprintf("\n\nbegin searching\n");

	char word[32];
	size_t climit = 0;
	timePoint_t t_s;

	while (1) {
		myprintf("\nPlease enter a word to search for (press ! to quit): ");
		scan_stdin("%s %d", 2, word, &climit);
		if (strcasecmp(word, "!") == 0)
			return 0;

		myprintf("\n-----------------\n");
		if (climit == 0) {
			climit = MAX_RESULT_LIMIT;
		}
		strToLower(word);
		t_s = STD_NOW;

		mynodes.doSearch((string_t) word);

		size_t i = 1;

		for (LeafItemsVector_t::iterator it =
				mynodes.m_tResultItems.begin();
				i <= climit && it != mynodes.m_tResultItems.end();
				++it) {
			std::string str = mynodes.get_document(*it);
			str = str.substr(0,100);
			myprintf("\n%d %llu \t%s", i++, *it,str.c_str());
		}
		myprintf("\n-----------------\n");
		myprintf("\n %llu docs found for %s\n", (u64_t ) (mynodes.m_tResultItems.size()), ((string_t) word).c_str());

		print_time("searching done in", t_s);

	}

}

