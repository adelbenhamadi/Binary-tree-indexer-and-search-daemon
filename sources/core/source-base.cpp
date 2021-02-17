//***************************************************************************************/
//*    snippet of this code are udapted from Sphinxsearch source code
//*    Author: Copyright (c) 2001-2016, Andrew Aksyonoff / Copyright (c) 2008-2016, Sphinx Technologies Inc
//*    Date: ?
//*    Code version: 2.95
//*    Availability: http://www.sphinxsearch.com
//*
//***************************************************************************************///


#include "../helpers/utility.h"
#include "./source-base.h"

namespace mynodes
{


    void MySource::appendSource(SourceCollection_t *src)
    {
        assert(src && "append null source!");
        m_tDocuments = *src;
    }

    bool MySource::updateStats()
    {
        MySourceStats *pstats = getStats();
        pstats->flat();

        for (auto &doc : m_tDocuments)
        {
            pstats->m_iDocumentsCount++;
            for (auto &str : doc.second)
            {
                pstats->m_iBytes += (int)strlen(str.c_str());
            }
        };
        return true;
    }
    void MySource::setup(const MySourceSettings &tSettings)
    {
        m_tSettings.m_iMinPrefixLen = std::max(tSettings.m_iMinPrefixLen, 0);
        m_tSettings.m_iMinInfixLen = std::max(tSettings.m_iMinInfixLen, 0);
        m_tSettings.m_iMaxSubstringLen = std::max(tSettings.m_iMaxSubstringLen, 0);
        m_tSettings.m_iBoundaryStep = std::max(tSettings.m_iBoundaryStep, -1);
        m_tSettings.m_bIndexExactWords = tSettings.m_bIndexExactWords;
        m_tSettings.m_iOvershortStep = std::min(std::max(tSettings.m_iOvershortStep, 0), 1);
        m_tSettings.m_iStopwordStep = std::min(std::max(tSettings.m_iStopwordStep, 0), 1);
        m_tSettings.m_bIndexSP = tSettings.m_bIndexSP;
        m_tSettings.m_bIndexFieldLens = tSettings.m_bIndexFieldLens;
    }



} // namespace mynodes
