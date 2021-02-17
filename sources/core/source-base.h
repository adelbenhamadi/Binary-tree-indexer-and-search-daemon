//***************************************************************************************/
//*    snippet of this code are adapted from Sphinxsearch source code
//*    Author: Copyright (c) 2001-2016, Andrew Aksyonoff / Copyright (c) 2008-2016, Sphinx Technologies Inc
//*    Date: ?
//*    Code version: 2.95
//*    Availability: http://www.sphinxsearch.com
//*
//***************************************************************************************///

#ifndef _source_base_h_
#define _source_base_h_


#include <map>
#include <unordered_map>
#include <string>


namespace mynodes
{


	typedef std::unordered_map<did_t, sVector_t> SourceCollection_t;
	typedef SourceCollection_t::iterator SourceIterator_t;

	struct MySourceSettings
	{
		int m_iMinPrefixLen;	 ///< min indexable prefix (0 means don't index prefixes)
		int m_iMinInfixLen;		 ///< min indexable infix length (0 means don't index infixes)
		int m_iMaxSubstringLen;	 ///< max indexable infix and prefix (0 means don't limit infixes and prefixes)
		int m_iBoundaryStep;	 ///< additional boundary word position increment
		bool m_bIndexExactWords; ///< exact (non-stemmed) word indexing flag
		int m_iOvershortStep;	 ///< position step on overshort token (default is 1)
		int m_iStopwordStep;	 ///< position step on stopword token (default is 1)
		bool m_bIndexSP;		 ///< whether to index sentence and paragraph delimiters
		bool m_bIndexFieldLens;	 ///< whether to index field lengths


		MySourceSettings() {};
	};


	struct MyIndexSettings : public MySourceSettings
	{

		bool m_bHtmlStrip;		
		bool m_bStemming;	
		bool m_bVerbose;
		bool m_bShowProgress;
		size_t m_iMemLimit;
		size_t m_iWriteBufferSize;

		//ctor
		MyIndexSettings() :
			m_bHtmlStrip(false),
			m_bVerbose(false),
			m_bShowProgress(true),
			m_iMemLimit(0),
			m_iWriteBufferSize(0)
		{}
	};


	/// source statistics
	struct MySourceStats
	{
		enum E_ProgressPhase
		{
			PRECOMPUTE,
			INDEX,					// indexing phase
			SORT,						// sorting phase
			MERGE,					// index merging phase
			PREREAD
		};

		E_ProgressPhase			m_ePhase;
		size_t			m_iDocumentsCount; // documents count
		size_t			m_iTotalBytes;		//tot memory size in bytes
		size_t			m_iBytes;
		size_t			m_iSortedHits;
		size_t			m_iSortedBytes;
		size_t			m_iTotalHits;
		size_t			m_iMergedHits;
		MySourceStats()
		{
			flat();
		}

		void flat()
		{
			m_iDocumentsCount = 0;
			m_iTotalBytes = 0;
			m_iBytes = 0;
		}
		void Reset(E_ProgressPhase ePhase) {
			m_ePhase = ePhase;
			switch (m_ePhase)
			{
			
			case INDEX:
				m_iDocumentsCount = 0;
				m_iBytes = 0;
				m_iTotalHits = 0;
				break;
			case SORT:
				printf("\nSorting\t%llu items\n", m_iTotalHits);
				m_iSortedHits = 0;
				m_iSortedBytes = 0;
				
				break;
			case MERGE:
				m_iMergedHits = 0;
				break;
			case PREREAD:
				m_iTotalBytes = 0;
				m_iBytes = 0;
				break;
			}
		}
		void printProgress(bool bEndProgress = false) {
			switch (m_ePhase)
			{
			
			case INDEX:
				if (bEndProgress || m_iDocumentsCount % 1000 == 0)
				printf("\r\tIndexed %llu, %.2f MB", m_iDocumentsCount,
					float(m_iBytes) / 1048576.0f);
				break;
			case SORT:
				
				if (bEndProgress || m_iSortedHits % 1000000 == 0)
				printf("\r\tSorted: %.1f Mhits , %.1f %%", (float)m_iSortedHits / 1000000,
					 (float) m_iSortedHits / m_iTotalHits * 100.0);

				break;

			case MERGE:
				printf("merged %.1f KHits", float(m_iMergedHits) / 1000);
				break;

			case PREREAD:
				printf("read %.1f of %.1f MB, %.1f%% done",
					(float) m_iBytes / 1000000.0f, (float) m_iTotalBytes / 1000000.0f,
					(float) m_iBytes / m_iTotalBytes * 100.0 );
				break;

			default:

				printError("Progress Phase Error: (progress-phase-%d)", m_ePhase);
				break;
			}

		}
	};

	struct MySource
	{

		const char* m_sName;
		MySourceSettings m_tSettings;
		SourceCollection_t m_tDocuments;
		std::pair<did_t, sVector_t> m_tDoc;

		MySource(const char* sName)
			: m_sName(sName), m_iNullIds(0), m_iMaxIds(0) {};

		MySourceStats* getStats() { return &m_tStats; } ;
		void ResetStats() { m_tStats.flat(); }
		void appendSource(SourceCollection_t* src);
		bool updateStats();
		void setup(const MySourceSettings& tSettings);
		virtual bool load(size_t sz, char* sError) = 0;
		virtual void close() = 0;
		size_t count()
		{
			return m_tDocuments.size();
		}

		~MySource()
		{}

	protected:
		
		MySourceStats m_tStats;
		
	protected:

		int m_iNullIds;
		int m_iMaxIds;
	};

	typedef std::vector<MySource*> SourcePointer_t;

} // namespace mynodes

#endif
