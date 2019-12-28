#ifndef MYNODES_H
#include "mynodes.h"
#endif

#include <limits.h>

#include <vector>

#include <chrono>

#include "utils.h"
/*
 using std::map;
 using std::swap;
 using std::binary_search;
 using std::lower_bound;
 using std::upper_bound;
 */

namespace mynodes {

myNodes::myNodes() {

	//initialize collections
	m_tPrefixNodes = NodeCollection();
	m_tSuffixNodes = NodeCollection();
#if ENABLE_PRE_SUFFIX
	m_tPreSuffixNodes = NodeCollection();
#endif
	m_tLeaves = LeafCollection();

	m_iResultCount = 0;
	m_iCurrentMode = 1;

}

bool myNodes::append_source(DocCollection &src) {
	assert(!src.empty());
	m_tDocuments = src;
	return true;
}
bool myNodes::doIndex() {

	std::vector<string_t> arr;

	myprintf("\n start processing documents :%d  documents\n---------\n",
			m_tDocuments.size());
	int co = 1;
	time_point_t t_s = STD_NOW;
	for (auto &doc : m_tDocuments) {
		size64_t docId = doc.first;
		string_t document = doc.second[m_iCurrentCol];
		normalize_word(document);

		if (document.empty()) {
			continue;
		}

		arr = explode(document, ' ');
		std::vector<string_t>::size_type sz = arr.size();
		for (unsigned i = 0; i < sz; i++) {
			string_t word = arr[i];

			if (word.empty()) {
				continue;
			}
			m_iCurrentMode = 1;
			if (!m_tPrefixNodes.find(word))
				create_node(word, csKEY_ZERO);
			//create_node(word,key_zero,key_zero,1,-1);

			m_iCurrentMode = 0;
			if (!m_tSuffixNodes.find(word))
				create_node(word, csKEY_ZERO);
			//create_node(word,key_zero,key_zero,0,docId);

			if (!m_tLeaves.find(word))
				m_tLeaves.add(word);
			m_tLeaves.m_vCollection[word].addItem(docId);

		}

		if (co % 1000 == 0) {
			myprintf("\r %d m_tDocuments processed", co);
			fflush(stdout);
		}
		co++;

	}
	std::chrono::high_resolution_clock::time_point te = STD_NOW;
	float duration = std::chrono::duration_cast<std::chrono::microseconds>(
			te - t_s).count();
	myprintf("\r %d documents processed in %8.4f seconds", co - 1,
			duration / 1000000);
	return true;
}
//optimized version of create_node()
bool myNodes::create_node(string_t &word, const string_t &left) {
	string_t parent;
	switch (m_iCurrentMode) {
	case 0:
		m_tSuffixNodes.add(word, left, csKEY_ZERO);
		parent = word.substr(1);
		break;
	case 1:
		m_tPrefixNodes.add(word, left, csKEY_ZERO);
		parent = word.substr(0, word.length() - 1);
		break;
	}

	if (!parent.empty()) {
		create_node(parent, word);
	}
	return true;
}

bool myNodes::create_node(string_t &word, const string_t &left,
		const string_t &right, const int iMode, const size64_t iDoc) {
	string_t parent;
	switch (iMode) {

	case 1:
		m_tPrefixNodes.add(word, left, right);
		parent = word.substr(0, word.length() - 1);
		break;
#if ENABLE_PRE_SUFFIX
	case 2:
		assert(iDoc >= 0);
		m_tPreSuffixNodes.add(word, left, right);
		if (!m_tLeaves.find(word))
			m_tLeaves.add(word);

		m_tLeaves.m_vCollection[word].addItem(iDoc);
		parent = word.substr(0, word.length() - 1);
		break;
#endif
	case 0:
		m_tSuffixNodes.add(word, left, right);
#if ENABLE_PRE_SUFFIX
		//create prefix node for suffix word
		create_node(word, csKEY_ZERO, csKEY_ZERO, 2, iDoc);
		debugPrintf("\ncreate_node prefix for suffix %s", word.c_str());
#endif
		parent = word.substr(1);
		break;
	}

	//
	if (!parent.empty()) {
		create_node(parent, word, csKEY_ZERO, iMode, iDoc);

	}
	return true;
}

bool myNodes::follow(const string_t &sFirst, const string_t &sSecond,
		const int iFollowMode, const bool right,LeafItemsVector *pMergedLeafItems) {
	NodeCollection* tNodeCol;
	switch (iFollowMode) {
	case 0:     //suffix searching like *ty

		tNodeCol = &m_tSuffixNodes;
		break;
	case 1:     //prefix searching like az*

		tNodeCol = &m_tPrefixNodes;
		break;
	case 2:     //infix searching like az*ty
		tNodeCol = &m_tPrefixNodes;
		//nodcol = &preSuffixNodes;
		break;
	case 3:     //exact search like azerty
		tNodeCol = &m_tPrefixNodes;
		break;
	case 4:     //infix search like *ert* TODO

		break;
	}
	if (!tNodeCol->find(sFirst))
		return false;

	debugPrintf("\n follow %s-%s",sFirst.c_str(),sSecond.c_str());
	//check before merging
	bool bCanMerge = m_tLeaves.find(sFirst);
	if (iFollowMode == 2) {
		bCanMerge &= hasSuffix(sFirst, sSecond);
	}
	if (bCanMerge) {
		debugPrintf(" %s(%llu) ",sFirst.c_str(),m_tLeaves.m_vCollection[sFirst].count());
		//merge results
		LeafItemsVector::iterator it = pMergedLeafItems->begin();
		pMergedLeafItems->insert(it,
				m_tLeaves.m_vCollection[sFirst].m_vItems.begin(),
				m_tLeaves.m_vCollection[sFirst].m_vItems.end());

		if (ciDEBUG_LEVEL > 2) {
			pMergedLeafItems->sort();
			pMergedLeafItems->unique();
			m_iResultCount = pMergedLeafItems->size();
			myprintf("\nbefore merge:");
			for (it = m_tLeaves.m_vCollection[sFirst].m_vItems.begin();
					it != m_tLeaves.m_vCollection[sFirst].m_vItems.end(); ++it) {
				myprintf(" %llu", (size64_t )*it);

			}
			myprintf("\nafter merge:");
			for (it = pMergedLeafItems->begin();
					it != pMergedLeafItems->end(); ++it) {
				myprintf(" %llu", (size64_t )*it);
			}
		}

	}
	//no need to follow for exact searching
	if (iFollowMode == 3)
		return bCanMerge;
	//continue following left and right
	string_t sTmp = tNodeCol->m_vCollection[sFirst].left;

	if (!sTmp.empty())
		follow(sTmp, sSecond, iFollowMode, true,pMergedLeafItems);
	if (right) {
		sTmp = tNodeCol->m_vCollection[sFirst].right;

		if (!sTmp.empty())
			follow(sTmp, sSecond, iFollowMode, true,pMergedLeafItems);
	}
	return true;
}
void myNodes::doSearch(const string_t &l, const string_t &r,const int iFollowMode) {
	m_tResultItems.clear();
	follow(l, r, iFollowMode, false,&m_tResultItems);
	m_tResultItems.sort();
	m_tResultItems.unique();

}
void myNodes::doSearch(string_t sKey) {
	const string_t sStar = "*";
	string_t sTmp;
	std::size_t pos = sKey.find(sStar);
	if (pos == STD_NPOS) {
		doSearch(sKey,sTmp,3);
	}
	else if(pos == 0) {
		sKey = sKey.substr(pos+1,STD_NPOS);
		doSearch(sKey,sTmp,0);
	} else if(pos >0 && pos <sKey.size()-1) {
		sTmp = sKey.substr(pos+1,STD_NPOS);
		sKey = sKey.substr(0,pos);
		doSearch(sKey,sTmp,2);
	} else if(pos ==sKey.size()-1) {
		sKey = sKey.substr(0,sKey.length()-1);
		doSearch(sKey,sTmp,1);
	}
}
void myNodes::print_nodes(const bool verbose) {
	auto c = m_tPrefixNodes.count();
	myprintf("\n%llu prefix Nodes", c);
	if (verbose) {
		for (auto &item : m_tPrefixNodes.m_vCollection) {

			myprintf("\n %s <-- %s --> %s", item.second.left.c_str(),
					item.first.c_str(), item.second.right.c_str());
		}
	}
	c = m_tSuffixNodes.count();
	myprintf("\n%llu suffix Nodes", c);
	if (verbose) {
		for (auto &item : m_tSuffixNodes.m_vCollection) {

			myprintf("\n %s <-- %s --> %s", item.second.left.c_str(),
					item.first.c_str(), item.second.right.c_str());
		}
	}
#if ENABLE_PRE_SUFFIX
	c= m_tPreSuffixNodes.count();
	myprintf("\n%d pre-suffix Nodes",c);
	if(verbose) {
		for(auto &item: m_tPreSuffixNodes.m_vCollection) {

			myprintf("\n %s <-- %s --> %s" , item.second.left.c_str() , item.first.c_str() , item.second.right.c_str());
		}
	}
#endif
}
void myNodes::print_leaves(const bool verbose) {
	auto c = m_tLeaves.count();
	myprintf("\n%llu Leaves", c);
	if (verbose) {
		for (auto &item : m_tLeaves.m_vCollection) {

			myprintf("\n %s => ", item.first.c_str());
			for (auto &i : item.second.m_vItems) {
				myprintf("%llu ", i);
			}
		}
	}
}

void myNodes::print_documents(const bool verbose) {
	auto c = m_tDocuments.size();
	myprintf("\n%d m_tDocuments", c);
	if (verbose) {
		for (auto &doc : m_tDocuments) {
			//string_t content = doc.second[iCurrentCol];
			myprintf("\n %llu =>", doc.first);
			for (auto& v : doc.second)
				myprintf("\t%s", v.c_str());
		}
	}
}

bool myNodes::save_nodes(const char *dir) {
	char filename[PATH_MAX];
	realpath(dir, filename);
	strcat(filename, "/nodes.bin");

	//open file
	if (std::FILE* fp = std::fopen(filename, "wb")) {
		fixBufferSize(fp);

		//write count
		size64_t co = m_tPrefixNodes.m_vCollection.size();
		std::fwrite(&co, sizeof(size64_t), 1, fp);
		//write collection

		for (auto &it : m_tPrefixNodes.m_vCollection) {

			writeStringToBinFile(it.first, fp);
			writeStringToBinFile(it.second.left, fp);
			writeStringToBinFile(it.second.right, fp);

		}
		//write count
		co = m_tSuffixNodes.m_vCollection.size();
		std::fwrite(&co, sizeof(size64_t), 1, fp);
		//write collection

		for (auto &it : m_tSuffixNodes.m_vCollection) {

			writeStringToBinFile(it.first, fp);
			writeStringToBinFile(it.second.left, fp);
			writeStringToBinFile(it.second.right, fp);

		}

		std::fclose(fp);
	} else {
		perror("Error opening file for writing");
		return false;
	}

	return true;
}

bool myNodes::load_nodes(const char *dir) {
	char filename[PATH_MAX];
	realpath(dir, filename);
	strcat(filename, "/nodes.bin");

	//open file
	if (std::FILE* fp = std::fopen(filename, "r")) {
		fixBufferSize(fp);
		//read count
		size64_t co;
		string_t word, left, right;
		std::fread(&co, sizeof(size64_t), 1, fp);
		for (size64_t i = 0; i < co; i++) {

			word = readStringFromBinFile(fp);
			left = readStringFromBinFile(fp);
			right = readStringFromBinFile(fp);
			if (ciDEBUG_LEVEL > 2)
				myprintf("\nreading node: %s <-- %s --> %s", left.c_str(),
						word.c_str(), right.c_str());
			m_tPrefixNodes.add2(word, left, right);

		}
		std::fread(&co, sizeof(size64_t), 1, fp);
		for (size64_t i = 0; i < co; i++) {

			word = readStringFromBinFile(fp);
			left = readStringFromBinFile(fp);
			right = readStringFromBinFile(fp);
			debugPrintf("\nadding node: %s <-- %s --> %s",left.c_str(),word.c_str(),right.c_str());
			m_tSuffixNodes.add2(word, left, right);

		}
		std::fclose(fp);
	} else {
		perror("Error opening file for reading");
		return false;
	}
	return true;
}

bool myNodes::save_leaves(const char *dir) {
	char filename[PATH_MAX];
	realpath(dir, filename);
	strcat(filename, "/leaves.bin");

	//open file
	if (std::FILE* fp = std::fopen(filename, "wb")) {
		fixBufferSize(fp);
		//write count
		size64_t co = m_tLeaves.m_vCollection.size();
		std::fwrite(&co, sizeof(size64_t), 1, fp);
		//write collection
		size_t sz = 0;
		for (auto &it : m_tLeaves.m_vCollection) {

			writeStringToBinFile(it.first, fp);
			co = it.second.m_vItems.size();
			sz = std::fwrite(&co, sizeof(size64_t), 1, fp);
			assert(sz == 1);
			if (co > 0) {
				size64_t arr[co];
				std::copy(it.second.m_vItems.begin(), it.second.m_vItems.end(), arr);
				sz = std::fwrite((char*) &arr[0], co * sizeof(size64_t), 1, fp);

				assert(sz == 1);
			} debugPrintf("\nwriting %s => %llu items",it.first.c_str(),co);

		}
		std::fclose(fp);
	} else {
		perror("Error opening file for writing");
		return false;
	}

	return true;
}

bool myNodes::load_leaves(const char *dir) {
	char filename[PATH_MAX];
	realpath(dir, filename);
	strcat(filename, "/leaves.bin");

	//open file
	if (std::FILE* fp = std::fopen(filename, "r")) {
		fixBufferSize(fp);

		//read count
		string_t word;
		size_t sz, ico;
		size64_t leavesCount;
		std::fread(&leavesCount, sizeof(size64_t), 1, fp);
		for (size64_t i = 0; i < leavesCount; i++) {
			word = readStringFromBinFile(fp);
			m_tLeaves.add(word);

			//read items count
			sz = std::fread(&ico, sizeof(size64_t), 1, fp);
			assert(sz == 1);

			size64_t arr[ico];
			arr[ico]= {0};
			sz = std::fread((char*) &arr[0], ico * sizeof(size64_t), 1, fp);
			assert(sz == 1);
			m_tLeaves.m_vCollection[word].m_vItems.insert(
					m_tLeaves.m_vCollection[word].m_vItems.begin(), arr, arr + ico);

		}
		std::fclose(fp);
	} else {
		perror("Error opening file for reading");
		return false;
	}
	return true;
}

bool myNodes::save_documents(const char *dir) {
	char filename[PATH_MAX];
	realpath(dir, filename);
	strcat(filename, "/m_tDocuments.bin");

	//open file
	if (std::FILE* fp = std::fopen(filename, "wb")) {
		fixBufferSize(fp, 8192 * 8);

		size64_t docid, co;
		size_t co2;
		//write count
		co = m_tDocuments.size();
		std::fwrite(&co, sizeof(size64_t), 1, fp);
		for (auto &doc : m_tDocuments) {
			docid = doc.first;
			std::fwrite(&docid, sizeof(size64_t), 1, fp);
			co2 = doc.second.size();
			std::fwrite(&co2, sizeof(size_t), 1, fp);
			for (auto& v : doc.second)
				writeStringToBinFile(v.c_str(), fp);

		}
		std::fclose(fp);
	} else {
		perror("Error opening file for writing");
		return false;
	}

	return true;
}
bool myNodes::load_documents(const char *dir) {
	char filename[PATH_MAX];
	realpath(dir, filename);
	strcat(filename, "/m_tDocuments.bin");

	//open file
	if (std::FILE* fp = std::fopen(filename, "r")) {
		fixBufferSize(fp);
		//read count
		string_t content;
		size64_t co, docid;
		size_t co2;
		std::fread(&co, sizeof(size64_t), 1, fp);
		for (size64_t i = 0; i < co; i++) {
			std::fread(&docid, sizeof(size64_t), 1, fp);
			std::fread(&co2, sizeof(size_t), 1, fp);
			for (unsigned int j = 0; j < co2; j++) {
				content = readStringFromBinFile(fp);
				m_tDocuments[docid].push_back(content);
				if (ciDEBUG_LEVEL > 2)
					myprintf("\nreading doc %llu => %s ", docid,
							content.c_str());
			}

		}

		std::fclose(fp);
	} else {
		perror("Error opening file for reading");
		return false;
	}
	return true;
}
bool myNodes::save_data(const char* dir) {
	time_point_t t_s = STD_NOW;
	save_leaves(dir);
	print_time(" Leaves saved in",t_s);

	t_s = STD_NOW;
	save_nodes(dir);
	print_time(" Nodes saved in",t_s);

	t_s = STD_NOW;
	save_documents(dir);
	print_time(" Documents saved in",t_s);
	return true;
}
bool myNodes::load_data(const char* dir) {
	time_point_t t_s = STD_NOW;
	load_leaves(dir);
	print_time(" Leaves loaded in",t_s);

	t_s = STD_NOW;
	load_nodes(dir);
	print_time(" Nodes loaded in",t_s);

	t_s = STD_NOW;
	load_documents(dir);
	print_time(" Documents loaded in",t_s);
	return true;
}
string_t myNodes::get_document(const size64_t ind) {
	string_t doc;
	for (auto& v : m_tDocuments[ind])
		doc = doc + "\t" + v;
	return doc;
}
void myNodes::clearResults() {
	m_iResultCount = 0;
	m_tResultItems.clear();
}

}
