#ifndef MYNODES_H
#define MYNODES_H


#include "utils.h"

#define MAX_RESULT_LIMIT 1000000
#define ENABLE_INFIX 0

namespace mynodes {

static bool			g_bVerbose		= false;
static bool			g_bShowProgress		= true;
static  u64_t 		g_iMaxLeafSize =100000;
static  int 		g_iMinKeywordSize = 3;

const string_t csKEY_ZERO ="*";
typedef std::map<u64_t ,sVector_t> SourceCollection_t;
typedef SourceCollection_t::iterator SourceIterator_t;

struct Document_t{
    u64_t m_iDocId;
    sVector_t m_vList;
};
/// source statistics
struct MySourceStats
{
	size_t			m_iDocumentsCount;		// documents count
	size_t			m_iTotalSizeBytes;		//tot memory size in bytes

	MySourceStats ()
	{
		flat ();
	}

	void flat ()
	{
		m_iDocumentsCount = 0;
		m_iTotalSizeBytes = 0;
	}
};
struct MySource{
	private:
	MySourceStats m_tStats;
	public:
	SourceCollection_t* m_pDocuments;

    //def ctor
    MySource(){}
	//cor
	MySource(SourceCollection_t* src){
		m_pDocuments = src;
	}
	size_t count(){
		return m_pDocuments->size();
	}
	MySourceStats* getStats(){
		return &m_tStats;
	}
	virtual bool updateStats ();

	//dtor
	~MySource(){}
};
struct Node{
	string_t left;
	string_t right;
	Node(){left = csKEY_ZERO;right = csKEY_ZERO;}
    Node(const string_t &l, const string_t &r){	left = l;right = r;}
} ;
typedef std::pair<const string_t ,Node > NodeCollectionIterator_t;
typedef std::list<u64_t> LeafItemsVector_t;
struct Leaf{
	LeafItemsVector_t  m_vItems;
    /*Leaf(){ items = std::map<u64_t ,u64_t>();}*/
	bool addItem(u64_t iDoc){
		m_vItems.emplace_back(iDoc);
		debugPrintf(5,"\nLeaf.addItem()  iDoc %llu",iDoc);
		return true;
	}
	u64_t count(){
		return m_vItems.size();
	}
	void reserve(int n){
	 //   m_vItems.reserve(n);
	}

} ;
struct NodeCollection{
	std::map<string_t ,Node > m_vCollection;
	/*NodeCollection(){	collection = std::map<string_t ,Node >();}*/
	bool find(string_t key){
        std::map<string_t ,Node >::iterator it = m_vCollection.find(key);
		bool r =/*is_string(key) && */ it != m_vCollection.end();
		return r;
	}
	u64_t count(){
		return m_vCollection.size();
	}
	void add(string_t &n,const string_t &left,const string_t &right){
        assert(!n.empty());
        debugPrintf(4,"\nNodeCollection.add() %s , Left: %s, Right: %s",n.c_str(),left.c_str(),right.c_str());
		if(!find(n)){
            Node node =  Node(left,right) ;
			m_vCollection.emplace(n, node);

			debugPrintf(5,"\nnew Node: %s",n.c_str());

		}
        else if(!left.empty()){

			string_t firstChild =m_vCollection[n].left;
			debugPrintf(6,",\nfirstChild  = %s",firstChild.c_str());
			if( firstChild == csKEY_ZERO){
				m_vCollection.at(n).left = left;
				debugPrintf(6,"\nnode $n --> left = %s",left.c_str());
			}
			else if(firstChild != left)
			{
			string_t mostright = get_most_right(firstChild,left);
			debugPrintf(6,",\nmostright %s",mostright.c_str());
			assert(find(mostright));
			if( left != mostright){
				m_vCollection.at(mostright).right = left;
				debugPrintf(6,"\nnode %s --> right = %s",mostright.c_str(),left.c_str());
			}
		  }
		}

	}
	void add2(string_t &word,const string_t &left,const string_t &right){
        assert(!word.empty());
        assert(!find(word));
        Node node = Node(left,right);
        m_vCollection.emplace(word,node) ;
        debugPrintf(5,"\nnew Node: %s <-- %s --> %s",left.c_str(),word.c_str(),right.c_str());
	}
	inline string_t get_most_right(string_t &n,const string_t &compareto ){
		//assert(find(n));
		string_t right = n;
		string_t t = csKEY_ZERO;
		while(right != csKEY_ZERO){
			//debugPrintf("\nright node:  %s",right.c_str());
			t = right;
			right = m_vCollection.at(right).right;
			//debugPrintf("\nright node ->  %s, %s", right.c_str(),compareto.c_str());

			if(right == compareto ){
				t = compareto;
			 break;
			}
		}
		assert((right == csKEY_ZERO || t == compareto ) && t != csKEY_ZERO);
		return t;

	}
};

struct LeafCollection{
	std::map<string_t ,Leaf > m_vCollection;
	LeafCollection(){
		m_vCollection = std::map<string_t ,Leaf >();
	}
	bool find(string_t key){
        std::map<string_t ,Leaf >::iterator it = m_vCollection.find(key);
		bool r =/*is_string(key) && */ it != m_vCollection.end();
		return r;
	}
	u64_t count(){
		return m_vCollection.size();
	}
	void add(string_t n){
        Leaf leaf ;
		m_vCollection.emplace(n, leaf);
		debugPrintf(5,"\nLeafCollection.add()  for %s",n.c_str());

	}
	/*Leaf add(string_t n){
        Leaf leaf ;
		m_vCollection.emplace(n, leaf);
		debugPrintf(5,"\nLeafCollection.add()  for %s",n.c_str());
		return leaf;
	}*/
};


struct MyIndexStats
{
	size_t			m_iDocumentsCount; 		//doc count
	size_t 			m_iDocumentSizeBytes; 	//doc memory size in bytes
	size_t 			m_iPrefixCount; 		//prefix nodes count
	size_t 			m_iPrefixSizeBytes; 	//pref memory size in bytes
	size_t 			m_iSuffixCount; 		//suffix nodes count
	size_t 			m_iSuffixSizeBytes; 	//suff memory size in bytes
	size_t 			m_iLeavesCount; 		//leaves count
	size_t 			m_iLeavesSizeBytes; 	//leaves memory size in bytes

	//ctor
	MyIndexStats(){
		flat();
	}
	void flat(){
		m_iDocumentsCount = 0;
		m_iDocumentSizeBytes = 0;
		m_iPrefixCount = 0;
		m_iPrefixSizeBytes = 0;
		m_iSuffixCount = 0;
		m_iSuffixSizeBytes = 0;
		m_iLeavesCount = 0;
		m_iLeavesSizeBytes = 0;


	}
	size_t ToalSizeBytes(){
		return m_iDocumentSizeBytes+m_iPrefixSizeBytes+m_iSuffixSizeBytes+m_iLeavesSizeBytes;
	}
};

struct MyIndexProgress
{

enum My_phase
	{
		PHASE_COLLECT,					// collecting phase
		PHASE_PRECOMPUTE,
		PHASE_INDEX	,					// indexing phase
		PHASE_SORT,						// sorting phase
		PHASE_MERGE,					// index merging phase
		PHASE_PREREAD
	};

	My_phase			m_ePhase;		// current indexing phase

	int64_t			m_iDocumentsCount;	// PHASE_COLLECT: documents collected so far
	int64_t			m_iBytes;			// PHASE_COLLECT: bytes collected so far
	int64_t 		m_iBytesTotal; 		//total read

	int64_t		m_iHits;				// PHASE_SORT: hits sorted so far
	int64_t		m_iHitsTotal;			// PHASE_SORT: hits total

	int				m_iDone;

	typedef void ( *IndexingProgress_fn ) ( const MyIndexProgress * pStat, bool bEndProgressPhase );
	IndexingProgress_fn m_fnProgress;

	MyIndexProgress ()
		: m_ePhase ( PHASE_COLLECT )
		, m_iDocumentsCount ( 0 )
		, m_iBytes ( 0 )
		, m_iHits ( 0 )
		,m_iBytesTotal (0)
		, m_iHitsTotal ( 0 )
		, m_fnProgress ( NULL )
	{}

	/// builds a message to print
	/// WARNING, STATIC BUFFER, NON-REENTRANT
	const char * BuildMessage() const;

	void Show ( bool bEndProgressPhase ) const;
};


class MyNodes{
private:
	NodeCollection m_tPrefixNodes;
	NodeCollection m_tSuffixNodes;
	#if ENABLE_INFIX
    NodeCollection m_tInfixNodes;
	#endif
	LeafCollection m_tLeaves;
	MySource m_tSource;
    size_t m_iCurrentCol = 0;
	MyIndexProgress	m_tProgress;
	MyIndexStats m_tIndexStats;



 public:
    u64_t m_iResultCount;
    LeafItemsVector_t m_tResultItems;
    int m_iCurrentMode;

	MyNodes();
	bool buildIndex();
	bool appendSource(SourceCollection_t &src);

	void print_nodes(const bool verbose);

	void print_leaves(const bool verbose);

	void print_documents(const bool verbose);
	bool load_data(const char *dir);
	bool save_data(const char *dir);
	string_t get_document(const u64_t ind);

	void doSearch(string_t sToSearch);
	void doSearch(const string_t &l,const string_t &r,const int iFollowMode);
	void SetProgressCallback ( MyIndexProgress::IndexingProgress_fn pfnProgress );
private:
	bool create_node(string_t &word,const string_t &left);
	bool create_node(string_t &word,const string_t &left,const string_t &right,const  int iMode,const u64_t doci);
	bool follow(const string_t &sFirst,const string_t &sSecond,const  int iFollowMode,const bool right,LeafItemsVector_t *pMergedLeafItems);
	void clearResults();

	bool save_nodes(const char* dir);
	bool save_leaves(const char* dir);
	bool save_documents(const char *dir);

	bool load_nodes(const char *dir);
	bool load_leaves(const char *dir);
	bool load_documents(const char *dir);

};//end class


}

#endif /* end of MYNODES_H */
