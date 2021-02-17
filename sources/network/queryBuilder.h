#pragma once
#include "network.h"

#include <vector>



template<class Q>
struct QueryRequestBuilder_t 
{
	QueryRequestBuilder_t(const std::vector<Q>& dQueries, int iStart, int iEnd) : m_dQueries(dQueries), m_iStart(iStart), m_iEnd(iEnd) {}
	virtual void		Build(const char* sIndexes, NetWriteBuffer& tOut) const;

protected:
	int					QueryLength(const char* sIndexes, const Q& q) const;
	void				Send(const char* sIndexes, NetWriteBuffer& tOut, const Q& q) const;

protected:
	const std::vector<Q>				& m_dQueries;
	int									m_iStart;
	int									m_iEnd;
};
