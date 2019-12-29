#ifndef MYNODES_H
#define MYNODES_H


#include "utils.h"

#define MAX_RESULT_LIMIT 1000000
#define ENABLE_PRE_SUFFIX 0

namespace mynodes {

const int ciDEBUG_LEVEL = 0;

const string_t csKEY_ZERO ="*";

struct Document{
    size64_t id;
    string_t text;
};
typedef std::map<size64_t ,std::vector<string_t>> DocCollection;

struct Node{
	string_t left;
	string_t right;
	Node(){left = csKEY_ZERO;right = csKEY_ZERO;}
    Node(const string_t &l, const string_t &r){	left = l;right = r;}
} ;
typedef std::pair<const string_t ,Node > NodeCollectionIterator;
typedef std::list<size64_t> LeafItemsVector;
struct Leaf{
	LeafItemsVector  m_vItems;
    /*Leaf(){ items = std::map<size64_t ,size64_t>();}*/
	bool addItem(size64_t iDoc){
		m_vItems.push_back(iDoc);
		debugPrintf("\nLeaf.addItem()  iDoc %llu",iDoc);
		return true;
	}
	size64_t count(){
		return m_vItems.size();
	}

} ;
struct NodeCollection{
	std::map<string_t ,Node > m_vCollection;
	bool find(string_t key){
        std::map<string_t ,Node >::iterator it = m_vCollection.find(key);
		bool r =/*is_string(key) && */ it != m_vCollection.end();
		return r;
	}
	size64_t count(){
		return m_vCollection.size();
	}
	void add(string_t &n,const string_t &left,const string_t &right){
        assert(!n.empty());
        debugPrintf("\nNodeCollection.add() %s , Left: %s, Right: %s",n.c_str(),left.c_str(),right.c_str());
		if(!find(n)){
            Node node =  Node(left,right) ;
			m_vCollection.emplace(n, node);

			debugPrintf("\nnew Node: %s",n.c_str());

		}
        else if(!left.empty()){

			string_t firstChild =m_vCollection[n].left;
			debugPrintf("\nfirstChild  = %s",firstChild.c_str());
			if( firstChild == csKEY_ZERO){
				m_vCollection.at(n).left = left;
				debugPrintf("\nnode $n --> left = %s",left.c_str());
			}
			else if(firstChild != left)
			{
			string_t mostright = get_most_right(firstChild,left);
			debugPrintf("\nmostright %s",mostright.c_str());
			assert(find(mostright));
			if( left != mostright){
				m_vCollection.at(mostright).right = left;
				debugPrintf("\nnode %s --> right = %s",mostright.c_str(),left.c_str());
			}
		  }
		}

	}
	void add2(string_t &word,const string_t &left,const string_t &right){
        assert(!word.empty());
        assert(!find(word));
        Node node = Node(left,right);
        m_vCollection.emplace(word,node) ;
        debugPrintf("\nnew Node: %s <-- %s --> %s",left.c_str(),word.c_str(),right.c_str());
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
	size64_t count(){
		return m_vCollection.size();
	}
	void add(string_t n){
        Leaf leaf ;
		m_vCollection.emplace(n, leaf);
		debugPrintf("\nLeafCollection.add()  for %s",n.c_str());
	}
};

class myNodes{
private:
	NodeCollection m_tPrefixNodes;//holds collection of nodes for prefix indexes
	NodeCollection m_tSuffixNodes;//holds collection of nodes for suffix indexes
    NodeCollection m_tPreSuffixNodes;//holds collection of nodes for both prefix and suffix indexes

	LeafCollection m_tLeaves;
	DocCollection m_tDocuments;
    size_t m_iCurrentCol = 0;


 public:
    size64_t m_iResultCount;
    LeafItemsVector m_tResultItems;
    int m_iCurrentMode;
	myNodes();
	bool doIndex();
	bool append_source(DocCollection &src);
	//routines for printing
	void print_nodes(const bool verbose);
	void print_leaves(const bool verbose);
	void print_documents(const bool verbose);
	//io routines
	bool load_data(const char *dir);
	bool save_data(const char *dir);
	string_t get_document(const size64_t ind);

	void doSearch(string_t sToSearch);
private:
	bool create_node(string_t &word,const string_t &left);
	bool create_node(string_t &word,const string_t &left,const string_t &right,const  int iMode,const size64_t doci);
	bool follow(const string_t &sFirst,const string_t &sSecond,const  int iFollowMode,const bool right,LeafItemsVector *pMergedLeafItems);
	void clearResults();
	void doSearch(const string_t &l,const string_t &r,const int iFollowMode);
	//io methods for nodes,leaves and documents
	bool save_nodes(const char* dir);
	bool save_leaves(const char* dir);
	bool save_documents(const char *dir);
	bool load_nodes(const char *dir);
	bool load_leaves(const char *dir);
	bool load_documents(const char *dir);

};//end class

}

#endif /* end of MYNODES_H */
