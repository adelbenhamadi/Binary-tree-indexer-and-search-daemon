#ifndef MYNODES_H
#define MYNODES_H


#include "utils.h"

#define MAX_RESULT_LIMIT 1000000
#define ENABLE_PRE_SUFFIX 0

namespace mynodes {
const u64_t g_iMaxLeafSize =100000;
const int g_iMinKeywordSize = 3;
const string_t csKEY_ZERO ="*";

struct Document{
    u64_t id;
    string_t text;
};
typedef std::map<u64_t ,sVector_t> DocCollection;


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

class myNodes{
private:
	NodeCollection m_tPrefixNodes;
	NodeCollection m_tSuffixNodes;
    NodeCollection m_tPreSuffixNodes;

	LeafCollection m_tLeaves;
	DocCollection m_tDocuments;
    size_t m_iCurrentCol = 0;


 public:
    u64_t m_iResultCount;
    LeafItemsVector_t m_tResultItems;
    int m_iCurrentMode;
	myNodes();
	bool buildIndex();
	bool append_source(DocCollection &src);

	void print_nodes(const bool verbose);

	void print_leaves(const bool verbose);

	void print_documents(const bool verbose);
	bool load_data(const char *dir);
	bool save_data(const char *dir);
	string_t get_document(const u64_t ind);

	void doSearch(string_t sToSearch);
	void doSearch(const string_t &l,const string_t &r,const int iFollowMode);
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
