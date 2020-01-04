#include "utils.h"
#include "mynodes.h"


using namespace mynodes;

bool load_data_to_src(const size_t limit, DocCollection &src) {
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;

	struct serverConnectionConfig mysqlD;
	mysqlD.server = "localhost";
	mysqlD.user = "amor";
	mysqlD.password = "*";
	mysqlD.database = "scheduler10_prod";

	// connect to the mysql database
	conn = mysql_connection_setup(mysqlD);
	char qry[1024];
	//qry = "select count(*) from abonne_ft_v3";
	if (limit == 0) {
		sprintf(qry,
				"select id,tmp from logs");
		} else {
		sprintf(qry,
				"select id,tmp from logs limit 0,%d",limit);
		}

	printf("Query: \"%s\" \n", qry);

	res = mysql_perform_query(conn, qry);
	unsigned long num_fields = mysql_num_fields(res);

	u64_t i = 0;
	DocCollection::iterator it = src.begin();
	sVector_t cols;
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

	bool bTesting = 0;
	size_t iDocLimit = 1000000;

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
								"--verbose\t\tverbose indexing issues report\n"
								"--noprogress\t\tdo not display progress\n"
								"--limit\t\t\tonly index <limit> documents, zero or default: index all documents\n"
								"--testing\t\t\ttesting mode for debugging\n"
								"Examples:\n"
								"myindexer --limit 1000\t\tindex 1000 documents\n");
			}

			return 1;
		}
	}
	myNodes mynodes;

	DocCollection source;

	if (bTesting) {
		source[0].push_back("\\//aDEl aZERty let");
		source[0].push_back("NOT YET SEARCHABLE COLUMN 1");
		source[1].push_back("1245 LEt.. LEr AzouR$# LE");
		source[1].push_back("NOT YET SEARCHABLE COLUMN 1");
		source[1].push_back("NOT YET SEARCHABLE COLUMN 2");
		source[2].push_back("?aDEl LEs abs LEs LEr");
		source[3].push_back("!!LE aZERty ùaDEl");
		source[4].push_back("**LEt LEr AzouR==**// le");
		source[5].push_back("+-aDEl aZERty");

	} else {
		load_data_to_src(iDocLimit, source);
	}
	myprintf("\nSource : %d documents loaded!", source.size());

	mynodes.append_source(source);
	mynodes.buildIndex();
	if (!bTesting)
		mynodes.save_data("data");

	mynodes.print_documents(bTesting || (iDocLimit > 0 && iDocLimit <= 10));
	mynodes.print_nodes(bTesting);
	mynodes.print_leaves(bTesting);

	//searching
	myprintf("\n\nbegin searching\n");

	char word[32];
	size_t climit = 0;
	time_point_t t_s;

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
		myprintf("\n %llu docs found for %s\n", mynodes.m_tResultItems.size(), word);

		print_time("searching done in", t_s);

	}

}

