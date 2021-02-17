#ifndef _MYNODES_H_
#define _MYNODES_H_
#include "../helpers/utility.h"
#include "source-base.h"
#include "source-mysql.h"
#include "../helpers/stemmer_en.h"
#include "../helpers/crc32.h"

#include <fstream>

#ifdef _MSC_VER
#include <thread>
#include <mutex>
#include <condition_variable>
#include <shared_mutex>
#include <future>

#else
#ifdef MINGW_STDTHREADS_GENERATED_STDHEADERS
#include <mingw.thread.h>
#include <mingw.mutex.h>
#include <mingw.condition_variable.h>
#include <mingw.shared_mutex.h>
#include <mingw.future.h>
#endif
#endif // _MSVC_VER

namespace mynodes {
	

	std::ofstream openOutputFile(const char* cFile,char *sError);
	std::ifstream openInputFile(const char* cFile,char* sError);

	struct Hasher_t {
	private:
		std::hash<string_t> m_hasher;
	public:
		tid_t hash(string_t w,bool bStem = false) {
			if (bStem) stem(w);
			return crc32str(w.c_str());
		}
		tid_t hash(char *w, bool bStem = false) {
			if (bStem) stem(w);
			return crc32str(w);
		}
		size_t hash64(string_t w, bool bStem = false) {
			if (bStem) stem(w);
			return m_hasher(w);
		}
		void stem(string_t& w) { 
			BYTE* wc = (BYTE*)w.c_str();
			stemmer::stem_en(wc);
			w = string_t((char*)wc);
		}
		void stem(char *wc) {
			stemmer::stem_en((BYTE*)wc);
		}
	};
	
	struct FileWriter {
		FileWriter(string_t sfilename) {
			_lastError = const_cast<char*>("");
			_stream = openOutputFile(sfilename.c_str(),_lastError);
		}
		~FileWriter() {
			close();
		}
		
		size_t write(void* pBuf, size_t size,bool bFatal = true) {
			
			if (_stream.write((char*)pBuf, size)) {
				_bytes += size;
				//flush();
				return size;

			}else if(bFatal)
			{
				diePrintf("\nI/O Error: writing data to disk failed, aborting");
			}
			return 0;
		}
		//use carefully pBuf can't be used until this job done
		size_t asyncWrite(char* pBuf, size_t size, bool bFatal = true) {
			std::future<size_t> asyncJob = std::async(std::launch::async,
				&FileWriter::write, this, pBuf, size, bFatal);
			asyncJob.get();
		}
		void asyncFlush() {			
				std::future<void> asyncJob = std::async(std::launch::async, &FileWriter::flush, this);
				asyncJob.get();
			}
		void flush() {
				_stream.flush();	
		}
		bool writeString(const string_t& word) {
			if (_stream && _stream.is_open()) {
				utility::writeStringToBinFile(word, _stream);
				return true;
			}
			return false;
		}
		size_t position() {
			if (_stream && _stream.is_open()) return _stream.cur;
			return 0;
		}
		
		size_t bytes() {
			return _bytes;
		}
		bool opened() {
			if (_stream) return _stream.is_open();
			return false;
		}
		bool eof() {
			if (_stream && _stream.is_open()) return _stream.eof();
			return true;
		}
		void close() {
			if (_stream && _stream.is_open()) _stream.close();
			_bytes = 0;
		}
	private:
		std::ofstream _stream;
		size_t _bytes;
		char* _lastError;
	};
	struct FileReader {
		FileReader(string_t filename) {
			_lastError = const_cast<char*>("");
			_filename = filename.c_str();
			_stream = openInputFile(_filename, _lastError);
			_bytes = 0;
		}
		~FileReader() {
			close();
		}
		char* getLastError() {
			//snprintf(_lastError, 256, "I/O error: File %s, %s", _filename, strerror(errno));
			return _lastError;
		}
		size_t read(char* pBuf, size_t size) {
			if (!checkState()) return 0;

			if (!_stream.is_open()) {
				
				diePrintf("\nI/O read error: stream is closed!");
				return 0;
			}
			if (eof()) {
				debugPrintf(10,"\nI/O read warning: reach eof");
				return 0;
			}
			if (_stream.read(pBuf, size)) {
				_bytes += _stream.gcount();
				return _stream.gcount();
			}
			
			debugPrintf(10,"\nI/O read warning: stream read error!");
			
			return 0;
		}
		bool checkState() {
			if (_stream.fail()) {	
				 if ((_stream.rdstate() & std::ifstream::failbit) != 0)
					 debugPrintf(10, "\nstream error: failbit");
				else if ((_stream.rdstate() & std::ifstream::badbit) != 0)
					 debugPrintf(10, "\nstream error: badbit");
			//try to clear rdstate:
				 _stream.clear();				
				return _stream.good();
			}
			return true;
		}
		string_t readString() {
			
				return utility::readStringFromBinFile(_stream);
		}
		size_t position() {
			if (checkState() && _stream.is_open()) return _stream.cur;
			return 0;
		}
		void seek(size_t pos) {
			if (checkState() && _stream.is_open()) {
				_stream.seekg(pos, _stream.beg);
			}
		}
		void seekRel(size_t pos) {
			if (checkState() && _stream.is_open()) {
				_stream.seekg(pos, _stream.cur);
			}
		}
		size_t bytes() {
			return _bytes;
		}
		size_t length() {
			if (!checkState() || !_stream.is_open()) return 0;
			_stream.seekg(0, _stream.end);
			size_t lg = _stream.tellg();
			_stream.seekg(0, _stream.beg);
			return lg;
		}
		void reset() {
			if (checkState()) {
				_stream.seekg(0, _stream.beg);
			}
			_bytes = 0;
		}
		bool opened() {
			if (checkState()) { return _stream.is_open(); }
			return false;
		}
		bool eof() {
			if ((_stream.rdstate() & std::ifstream::eofbit) != 0) {
				//printError("\nstream warning: eofbit");
				return true;
			}
			return false;
		}
		void close() {
			if (checkState() && _stream.is_open()) {
				_stream.close();
			}
			_bytes = 0;
		}
	private:
		std::ifstream _stream;
		size_t _bytes;
		const char* _filename;
		char* _lastError;
	};
	

	struct Document_t {
		did_t m_iDocId;
		sVector_t m_vList;
	};
	struct Expression_t {
		string_t value;
		Expression_t() { value = ""; }
		Expression_t(string_t& v)
			:value(v) {};
		void parse() {};
	};

	struct Node  {
		string_t left;
		string_t right;
		Node():left(csKEY_ZERO),right(csKEY_ZERO) {}
		Node(const string_t& l, const string_t& r) :left(l), right(r) {}
	};
	
	struct LeafItem {
		did_t did;
		tid_t tokenId;
		uint16_t hits;
		uint16_t position;
		LeafItem():did(0),tokenId(0),position(0) {}
		LeafItem(const did_t& id, const tid_t& tid, const uint16_t& h, const uint16_t& p) 
			:did(id), tokenId(tid),hits(h), position(p) {}
		LeafItem(const did_t& id) {
			did = id;
			hits = 1;
			position = 0;
			tokenId = 0;
			
		}
		bool operator==(const LeafItem& other) const { return this->did == other.did; };
	};
	class LeafItemCompare
	{
	public:
		bool operator() (LeafItem lh, LeafItem  rh)
		{
			return (lh.tokenId > rh.tokenId 
				|| (lh.tokenId == rh.tokenId && lh.did < rh.did)
				|| (lh.did == rh.did && lh.position < rh.position)
				);
		}
	};
	//[](LeafItem& lh, LeafItem& rh) { return lh.tokenId > rh.tokenId;  }
	class MinHeapCompare
	{
	public:
		bool operator() (std::pair<LeafItem, int> lh, std::pair<LeafItem, int> rh)
		{
			return LeafItemCompare().operator()(lh.first, rh.first);
			//return (lh.first.tokenId > rh.first.tokenId || (lh.first.tokenId == rh.first.tokenId && lh.first.did > rh.first.did));
		}
	};

	struct BinBlock {
		unsigned int mSize;
		unsigned mCurr;
		unsigned int mOffset;
		
		BinBlock(unsigned int sz, unsigned int offset)
			: mSize(sz)
			, mCurr(0)
			, mOffset(offset)
			, mChunckCount(0)
			, mChunckIdx(0)
			, mChunckPos(0)
			, mChunckSize(0)
			, mLastChunckSize(0){}
		
		void updateChuncks(unsigned co) {
			if (co) {
				mChunckCount = co;
				mChunckSize = mSize / co;
				mLastChunckSize = mSize - ((co-1) * mChunckSize);
				mpChunck = new LeafItem[mLastChunckSize];
			}
			else {
				printError("\nchunckCount cannot be zero!");
			}
			mChunckIdx = 0;
			mChunckPos = 0;
		}
		bool newChunck() {
			if (mChunckIdx) {
				return mChunckPos == currentChunckSize();
			}
			return true;
		}
		unsigned currentChunckSize() {
			return  mChunckIdx < mChunckCount ? mChunckSize : mLastChunckSize;
		}
		bool read(FileReader* pFile) {
			if (newChunck()) {
				if (mChunckIdx == mChunckCount) return false;
				mChunckIdx++;
				unsigned sz = currentChunckSize();
				pFile->seek(getOffset() * sizeof(LeafItem));
				pFile->read((char*)mpChunck, sz * sizeof(LeafItem));
				mChunckPos = 0;
				mCurr += sz;
			}
			return true;
		}
		
		LeafItem* more(bool bReset = false) {
			if (bReset) mChunckPos = 0;
			return &mpChunck[mChunckPos++];
		}
		~BinBlock() {
			if (mLastChunckSize>0) {
				delete[] mpChunck;
			}
		}
		unsigned getOffset() {
			return mOffset + mCurr;
		}
		bool eob() { return mCurr > mSize; }
	private:
		unsigned mChunckSize;
		unsigned mLastChunckSize;
		unsigned mChunckCount;
		unsigned int mChunckIdx;
		unsigned int mChunckPos;
		LeafItem* mpChunck;
	};


	typedef std::vector<LeafItem> LeafItemsVector_t;
	class Leaf  {
		Leaf(const Leaf&) = delete;
		const Leaf& operator=(const Leaf&) = delete;
	public:
		size_t m_iCount;
		LeafItemsVector_t m_vLeafItems;
		did_t m_iLastAddedId;
		Leaf():m_iCount(0),m_iLastAddedId(0) {/* printf("\nnew Leaf");*/ }

		size_t size() {
			return m_vLeafItems.size();
		}

		/*inline bool findItemAndUpdateHits(did_t& id) {
			for (LeafItemsVector_t::iterator it = m_vLeafItems.begin(); it != m_vLeafItems.end(); ++it) {
				LeafItem& wh = *it;
				if (wh.did == id) {
					//wh.hits++;
					debugPrintf(5, "\ndocId: %llu hits-> %llu", id, wh.hits);
					return true;
				}
			}
			return false;
		}*/
		inline const LeafItem& findItem(did_t& id) {
			for (LeafItemsVector_t::const_iterator it = m_vLeafItems.begin(); it != m_vLeafItems.end(); ++it) {
				if (it->did == id) {
					return (*it);
				}
			}
			return NULL;
		}


	};
	class NodeCollection {
	public:
		std::unordered_map<string_t, Node > m_vNodes;
		inline bool find(const string_t& key) {
			//return m_vNodes.find(key) != m_vNodes.end();
			 return m_vNodes.count(key)>0;
		}
		size_t count() {
			return m_vNodes.size();
		}
		inline void add0(const string_t& n, const string_t& left, const string_t& right) {
			assert(!n.empty());
			//debugPrintf(1, "\nadd:  %s <-- %s --> %s", left.c_str(), n.c_str(), right.c_str());
			assert(!find(n));
		    m_vNodes.emplace(n, Node(left, right));

		}
		inline void add(const string_t& n, const string_t& left, const string_t& right) {
			assert(!n.empty());
			debugPrintf(10, "\nadd:  %s <-- %s --> %s", left.c_str(), n.c_str(), right.c_str());
			if (!find(n)) {
				//m_vNodes.emplace(n, Node(left, right));

				m_vNodes.emplace(std::piecewise_construct,
					std::forward_as_tuple(n),  // key
					std::forward_as_tuple(Node(left, right)));
				//m_vNodes.insert(std::make_pair(n, Node(left, right)));
				//m_vNodes[n] = Node(left, right);
				debugPrintf(10, "\tnew : %s", n.c_str());

			}
			else if (!left.empty()) {

				string_t& n_leftChild = m_vNodes[n].left;
				debugPrintf(10, ",\nfirstChild  = %s", n_leftChild.c_str());
				if (n_leftChild == csKEY_ZERO) {
					n_leftChild = left;
					debugPrintf(10, "\t %s <--", left.c_str());
					return;
				}
				if (n_leftChild != left)
				{
					const string_t& mostright = get_most_right(n_leftChild, left);
					debugPrintf(10, ",\tmostright = %s", mostright.c_str());
					assert(find(mostright));
					if (left != mostright) {
						m_vNodes.at(mostright).right = left;
						debugPrintf(10, "\t left : %s <--", left.c_str());
					}
					return;
				}
			}

		}

		void add_old(string_t& word, const string_t& left, const string_t& right) {
			assert(!word.empty());
			assert(!find(word));
		
			m_vNodes[word] = Node(left, right);
			//m_vNodes.emplace(word, Node(left, right));
			debugPrintf(10, "\nnew Node: %s <-- %s --> %s", left.c_str(), word.c_str(), right.c_str());
		}
		
		inline string_t get_most_right(string_t& n, const string_t& compareto) {
			//assert(find(n));
			debugPrintf(10, ",\nget_most_right %s , compareto: %s", n.c_str(), compareto.c_str());
			string_t right = n;
			string_t t = csKEY_ZERO;
			while (right != csKEY_ZERO) {
				//debugPrintf(6,"\nright node:  %s",right.c_str());
				t = right;
				right = m_vNodes[right].right;
				debugPrintf(10, "\nright node ->  %s , compareto: %s", right.c_str(), compareto.c_str());

				if (right == compareto) { t = compareto; break; }
			}
			assert((right == csKEY_ZERO || t == compareto) && t != csKEY_ZERO);
			return t;

		}
	};

	class LeafCollection {
	public:
		size_t m_iLeavesCount = 0;
		std::unordered_map<tid_t, Leaf > m_vDict;
		//std::unordered_map<string_t, Leaf > m_vCollection;
		LeafCollection() {
			m_vDict = std::unordered_map<tid_t, Leaf >();
			//m_vCollection = std::unordered_map<string_t, Leaf >();
		}
		bool find(tid_t key) {
		
			return m_vDict.find(key) != m_vDict.end();
			//return m_vCollection.find(key) != m_vCollection.end();
			/* std::unordered_map<string_t ,Leaf >::iterator it = m_vNodes.find(key);
			 bool r =  it != m_vNodes.end();
			 return r;
			 */
		}
		
		inline void add(tid_t s) {
		
			m_vDict.emplace(std::piecewise_construct,
				std::forward_as_tuple( s ),
				std::forward_as_tuple());
			/*m_vCollection.emplace(std::piecewise_construct,
				std::forward_as_tuple(s),  // args for key
				std::forward_as_tuple());  // args for mapped value
				*/
		   //m_vCollection.emplace(s,Leaf());


		}
		//bool write(std::ofstream& input, bool bClose);
		void reset(){
			m_vDict.clear();
			m_iLeavesCount = 0;
			//m_vCollection.clear();
		}
	};


	class MyIndexStats
	{
		MyIndexStats(const MyIndexStats&) = delete;
		const MyIndexStats& operator=(const MyIndexStats&) = delete;
	public:
		size_t			m_iDocumentsCount; 		//doc count
		size_t 			m_iDocumentSizeBytes; 	//doc memory size in bytes
		size_t 			m_iPrefixCount; 		//prefix nodes count
		size_t 			m_iPrefixSizeBytes; 	//pref memory size in bytes
		size_t 			m_iSuffixCount; 		//suffix nodes count
		size_t 			m_iSuffixSizeBytes; 	//suff memory size in bytes
		size_t 			m_iLeavesCount; 		//leaves count
		size_t 			m_iLeavesSizeBytes; 	//leaves memory size in bytes

		//ctor
		MyIndexStats() {
			flat();
		}
		void flat() {
			m_iDocumentsCount = 0;
			m_iDocumentSizeBytes = 0;
			m_iPrefixCount = 0;
			m_iPrefixSizeBytes = 0;
			m_iSuffixCount = 0;
			m_iSuffixSizeBytes = 0;
			m_iLeavesCount = 0;
			m_iLeavesSizeBytes = 0;


		}
		size_t ToalSizeBytes() {
			return m_iDocumentSizeBytes + m_iPrefixSizeBytes + m_iSuffixSizeBytes + m_iLeavesSizeBytes;
		}
	};

	class MyProgressPrinter
	{
	public:
		enum E_Phase
		{
			PHASE_COLLECT,					// collecting phase
			PHASE_PRECOMPUTE,
			PHASE_INDEX,					// indexing phase
			PHASE_SORT,						// sorting phase
			PHASE_MERGE,					// index merging phase
			PHASE_PREREAD
		};

		E_Phase			m_ePhase;		// current indexing phase

		int64_t			m_iDocumentsCount;	// PHASE_COLLECT: documents collected so far
		int64_t			m_iBytes;			// PHASE_COLLECT: bytes collected so far
		int64_t 		m_iBytesTotal; 		//total read

		int64_t		m_iHits;				// PHASE_SORT: hits sorted so far
		int64_t		m_iHitsTotal;			// PHASE_SORT: hits total

		int				m_iDone;

		typedef void (*Progress_fn) (const MyProgressPrinter* pProgressPrinter, bool bEndPhase);
		Progress_fn m_fnProgress;

		MyProgressPrinter()
			: m_ePhase(PHASE_COLLECT)
			, m_iDocumentsCount(0)
			, m_iBytes(0)
			, m_iHits(0)
			, m_iBytesTotal(0)
			, m_iHitsTotal(0)
			, m_fnProgress(NULL)
		{}

		/// builds a message to print
		/// WARNING, STATIC BUFFER, NON-REENTRANT
		const char* BuildMessage() const;
		void Reset(E_Phase ePhase, int iCount, int iBytes) {
			m_ePhase = ePhase;
			m_iDocumentsCount = iCount;
			m_iBytes = iBytes;
		}
		void Show(bool bEndProgressPhase) const;
	};

	struct IoStats {
		size_t m_iBytesIn;
		size_t m_iBytesOut;
		IoStats() : m_iBytesIn(0), m_iBytesOut(0) {}
		void reset() { m_iBytesIn = 0; m_iBytesOut = 0; }
	};


	class MyIndex  {
		MyIndex(const MyIndex&) = delete;
		const MyIndex& operator=(const MyIndex&) = delete;
	public:
		NodeCollection m_tPrefixNodes;
		NodeCollection m_tSuffixNodes;
#if ENABLE_INFIX
		NodeCollection m_tInfixNodes;
#endif
		NodeCollection* m_pSearchNodeCol;
		LeafCollection m_tLeaves;
	private:
		int m_iCurrentCol = 0;
		
		MyIndexStats m_tIndexStats;
		char m_sError[256];


	public:
		MyIndexSettings  m_tSettings;
		std::vector<MySource*> vSources;
		LeafItemsVector_t m_tResultItems;
		size_t m_iResultCount;
		IoStats m_tIoStats;
		int m_iCurrentMode;
		string_t m_sIndexName;
		string_t m_sIndexPath;
		Hasher_t m_hasher;

		MyIndex(const string_t& sName, const string_t& sPath);
		bool build();
		bool buildFromSource(MySource* pSource,size_t iMemLimit,size_t iWriteBufferSz);
		//std::thread createJob(MySource *ps, int i, bool b);
		string_t getIndexFilename(const char* cExt);
		
		bool load_data(bool b_loadDocs);
		bool save_data();
		string_t get_document(MySource* pSource, const LeafItem& wh);

		void doSearch(const Expression_t& hExpression);
		void doSearch(const string_t& l, const string_t& r, const int& iFollowMode);
		void assignProgressCallback(MyProgressPrinter::Progress_fn pfnProgress);
		void Setup(MySourceSettings& tSettings);
		void Unlock() {};
		Leaf& findLeaf(string_t& key) { 
			return m_tLeaves.m_vDict[ m_hasher.hash(key)];
			/*m_vCollection[key];*/ }
		bool findPrefix(string_t& key) { return m_tPrefixNodes.find(key); }
		bool findSuffix(string_t& key) { return m_tSuffixNodes.find(key); }
		char* GetLastError() {
			return m_sError;
		};
		~MyIndex() {
			for (auto pSource : vSources)
				safeDelete(pSource);
			//safeDelete( m_sError);
		}
	private:
		bool create_node(string_t& word, const string_t& left);
		bool create_node(string_t& word, const string_t& left, const string_t& right, const  int iMode, const did_t doci);
		bool follow(const string_t& sFirst, const string_t& sSecond, const  int& iFollowMode, const bool& right, LeafItemsVector_t* pMergedLeafItems);
		void clearResults();
	public:	
		bool save_nodes();
		bool save_dict();
		//bool save_leaves(const char* dir);
		bool save_documents(MySource* pSource);

		bool load_nodes();
		bool load_leaves();
		bool load_dict();
		bool load_documents(MySource* pSource);

	};//end class


}


#endif /* end of MYNODES_H */
