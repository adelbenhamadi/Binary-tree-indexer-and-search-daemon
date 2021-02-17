#include "queryBuilder.h"
struct CSphDocInfo
{
	SphDocID_t		m_iDocID;		///< document ID
	int				m_iRowitems;	///< row items count
	CSphRowitem* m_pRowitems;	///< row data

	/// ctor. clears everything
	CSphDocInfo()
		: m_iDocID(0)
		, m_iRowitems(0)
		, m_pRowitems(NULL)
	{
	}

	/// copy ctor. just in case
	CSphDocInfo(const CSphDocInfo& rhs)
		: m_iRowitems(0)
		, m_pRowitems(NULL)
	{
		*this = rhs;
	}

	/// dtor. frees everything
	~CSphDocInfo()
	{
		SafeDeleteArray(m_pRowitems);
	}

	/// reset
	void Reset(int iNewRowitems)
	{
		m_iDocID = 0;
		if (iNewRowitems != m_iRowitems)
		{
			m_iRowitems = iNewRowitems;
			SafeDeleteArray(m_pRowitems);
			if (m_iRowitems)
				m_pRowitems = new CSphRowitem[m_iRowitems];
		}
	}

	/// assignment
	const CSphDocInfo& operator = (const CSphDocInfo& rhs)
	{
		m_iDocID = rhs.m_iDocID;

		if (m_iRowitems != rhs.m_iRowitems)
		{
			SafeDeleteArray(m_pRowitems);
			m_iRowitems = rhs.m_iRowitems;
			if (m_iRowitems)
				m_pRowitems = new CSphRowitem[m_iRowitems]; // OPTIMIZE! pool these allocs
		}

		if (m_iRowitems)
		{
			assert(m_iRowitems == rhs.m_iRowitems);
			memcpy(m_pRowitems, rhs.m_pRowitems, sizeof(CSphRowitem) * m_iRowitems);
		}

		return *this;
	}

public:
	/// get attr by item index
	SphAttr_t GetAttr(int iItem) const
	{
		assert(iItem >= 0 && iItem < m_iRowitems);
		return m_pRowitems[iItem];
	}

	/// get attr by bit offset/count
	SphAttr_t GetAttr(int iBitOffset, int iBitCount) const
	{
		assert(iBitOffset >= 0 && iBitOffset < m_iRowitems * ROWITEM_BITS);
		assert(iBitCount > 0 && iBitOffset + iBitCount <= m_iRowitems * ROWITEM_BITS);
		return sphGetRowAttr(m_pRowitems, iBitOffset, iBitCount);
	}

	/// get float attr
	float GetAttrFloat(int iItem) const
	{
		assert(iItem >= 0 && iItem < m_iRowitems);
		return sphDW2F(m_pRowitems[iItem]);
	};

public:
	/// set attr by item index
	void SetAttr(int iItem, SphAttr_t uValue)
	{
		assert(iItem >= 0 && iItem < m_iRowitems);
		m_pRowitems[iItem] = uValue;
	}

	/// set attr by bit offset/count
	void SetAttr(int iBitOffset, int iBitCount, SphAttr_t uValue) const
	{
		assert(iBitOffset >= 0 && iBitOffset < m_iRowitems * ROWITEM_BITS);
		assert(iBitCount > 0 && iBitOffset + iBitCount <= m_iRowitems * ROWITEM_BITS);
		sphSetRowAttr(m_pRowitems, iBitOffset, iBitCount, uValue);
	}

	/// set float attr
	void SetAttrFloat(int iItem, float fValue) const
	{
		assert(iItem >= 0 && iItem < m_iRowitems);
		m_pRowitems[iItem] = sphF2DW(fValue);
	};
};


struct QueryMatch : public CSphDocInfo
{
	int		m_iWeight;	///< my computed weight
	int		m_iTag;		///< my index tag

	QueryMatch() : m_iWeight(0), m_iTag(0) {}
	bool operator == (const QueryMatch& rhs) const { return (m_iDocID == rhs.m_iDocID); }
};
struct Query {

};
template<class Q>
int QueryRequestBuilder_t<Q>::QueryLength(const char* sIndexes, const Q& q) const
{
	int iReqSize = 96 + 2 * sizeof(SphDocID_t) + 4 * q.m_iWeights
		+ strlen(q.m_sSortBy.cstr())
		+ strlen(q.m_sQuery.cstr())
		+ strlen(sIndexes)
		+ strlen(q.m_sGroupBy.cstr())
		+ strlen(q.m_sGroupSortBy.cstr())
		+ strlen(q.m_sGroupDistinct.cstr())
		+ strlen(q.m_sComment.cstr());
	ARRAY_FOREACH(j, q.m_dFilters)
	{
		const CSphFilter& tFilter = q.m_dFilters[j];
		iReqSize += 12 + strlen(tFilter.m_sAttrName.cstr()); // string attr-name; int type; int exclude-flag
		switch (tFilter.m_eType)
		{
		case SPH_FILTER_VALUES:
			iReqSize += 4 + 4 * tFilter.m_dValues.GetLength(); // int values-count; int[] values
			break;

		case SPH_FILTER_RANGE:
		case SPH_FILTER_FLOATRANGE:
			iReqSize += 8; // int/float min-val,max-val
			break;
		}
	}
	if (q.m_bGeoAnchor)
		iReqSize += 16 + strlen(q.m_sGeoLatAttr.cstr()) + strlen(q.m_sGeoLongAttr.cstr()); // string lat-attr, long-attr; float lat, long
	ARRAY_FOREACH(i, q.m_dIndexWeights)
		iReqSize += 8 + strlen(q.m_dIndexWeights[i].m_sName.cstr()); // string index-name; int index-weight
	ARRAY_FOREACH(i, q.m_dFieldWeights)
		iReqSize += 8 + strlen(q.m_dFieldWeights[i].m_sName.cstr()); // string field-name; int field-weight
	return iReqSize;
}


template<class Q>
void QueryRequestBuilder_t<Q>::Send(const char* sIndexes, NetWriteBuffer& tOut, const Q& q) const
{
	tOut.Write<int>(0); // offset is 0
	tOut.Write<int>(q.m_iMaxMatches); // limit is MAX_MATCHES
	tOut.Write<int>((DWORD)q.m_eMode); // match mode
	tOut.Write<int>((DWORD)q.m_eRanker); // ranking mode
	tOut.Write<int>(q.m_eSort); // sort mode
	tOut.Write<std::string>(q.m_sSortBy.cstr()); // sort attr
	tOut.Write<std::string>(q.m_sQuery.cstr()); // query
	tOut.Write<int>(q.m_iWeights);
	for (int j = 0; j < q.m_iWeights; j++)
		tOut.Write<int>(q.m_pWeights[j]); // weights
	tOut.Write<std::string>(sIndexes); // indexes
	tOut.Write<int>(USE_64BIT); // id range bits
	tOut.SendDocid(q.m_iMinID); // id/ts ranges
	tOut.SendDocid(q.m_iMaxID);
	tOut.Write<int>(q.m_dFilters.GetLength());
	ARRAY_FOREACH(j, q.m_dFilters)
	{
		const CSphFilter& tFilter = q.m_dFilters[j];
		tOut.Write<std::string>(tFilter.m_sAttrName.cstr());
		tOut.Write<int>(tFilter.m_eType);
		switch (tFilter.m_eType)
		{
		case SPH_FILTER_VALUES:
			tOut.Write<int>(tFilter.m_dValues.GetLength());
			ARRAY_FOREACH(k, tFilter.m_dValues)
				tOut.Write<int>(tFilter.m_dValues[k]);
			break;

		case SPH_FILTER_RANGE:
			tOut.Write<DWORD>(tFilter.m_uMinValue);
			tOut.Write<DWORD>(tFilter.m_uMaxValue);
			break;

		case SPH_FILTER_FLOATRANGE:
			tOut.SendFloat(tFilter.m_fMinValue);
			tOut.SendFloat(tFilter.m_fMaxValue);
			break;
		}
		tOut.Write<int>(tFilter.m_bExclude);
	}
	tOut.Write<int>(q.m_eGroupFunc);
	tOut.Write<std::string>(q.m_sGroupBy.cstr());
	tOut.Write<int>(q.m_iMaxMatches);
	tOut.Write<std::string>(q.m_sGroupSortBy.cstr());
	tOut.Write<int>(q.m_iCutoff);
	tOut.Write<int>(q.m_iRetryCount);
	tOut.Write<int>(q.m_iRetryDelay);
	tOut.Write<std::string>(q.m_sGroupDistinct.cstr());
	tOut.Write<int>(q.m_bGeoAnchor);
	if (q.m_bGeoAnchor)
	{
		tOut.Write<std::string>(q.m_sGeoLatAttr.cstr());
		tOut.Write<std::string>(q.m_sGeoLongAttr.cstr());
		tOut.SendFloat(q.m_fGeoLatitude);
		tOut.SendFloat(q.m_fGeoLongitude);
	}
	tOut.Write<int>(q.m_dIndexWeights.GetLength());
	ARRAY_FOREACH(i, q.m_dIndexWeights)
	{
		tOut.Write<std::string>(q.m_dIndexWeights[i].m_sName.cstr());
		tOut.Write<int>(q.m_dIndexWeights[i].m_iValue);
	}
	tOut.Write<DWORD>(q.m_uMaxQueryMsec);
	tOut.Write<int>(q.m_dFieldWeights.GetLength());
	ARRAY_FOREACH(i, q.m_dFieldWeights)
	{
		tOut.Write<std::string>(q.m_dFieldWeights[i].m_sName.cstr());
		tOut.Write<int>(q.m_dFieldWeights[i].m_iValue);
	}
	tOut.Write<std::string>(q.m_sComment.cstr());
}

template<class Q>
void QueryRequestBuilder_t<Q>::Build(const char* sIndexes, NetWriteBuffer& tOut) const
{
	int iReqLen = 4; // int num-queries
	for (int i = m_iStart; i <= m_iEnd; i++)
		iReqLen += QueryLength(sIndexes, m_dQueries[i]);

	tOut.Write<DWORD>(SPHINX_SEARCHD_PROTO);
	tOut.Write<WORD>(SEARCHD_COMMAND_SEARCH); // command id
	tOut.Write<WORD>(VER_COMMAND_SEARCH); // command version
	tOut.Write<int>(iReqLen); // request body length

	tOut.Write<int>(m_iEnd - m_iStart + 1);
	for (int i = m_iStart; i <= m_iEnd; i++)
		Send(sIndexes, tOut, m_dQueries[i]);
}
