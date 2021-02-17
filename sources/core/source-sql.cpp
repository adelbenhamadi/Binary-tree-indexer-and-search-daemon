//***************************************************************************************/
//*    snippet of this code are udapted from Sphinxsearch source code
//*    Author: Copyright (c) 2001-2016, Andrew Aksyonoff / Copyright (c) 2008-2016, Sphinx Technologies Inc
//*    Date: ?
//*    Code version: 2.95
//*    Availability: http://www.sphinxsearch.com
//*
//***************************************************************************************///

#include "../helpers/utility.h"
#include "./source-sql.h"

#include <cstdio>

namespace mynodes
{


#define LOC_SQL_ERROR(_msg) \
    {                       \
        sSqlError = _msg;   \
        break;              \
    }
#define LOC_ERROR(_msg, _arg)        \
    {                                \
        sprintf(sError, _msg, _arg); \
        return false;                \
    }
#define LOC_ERROR2(_msg, _arg, _arg2)      \
    {                                      \
        sprintf(sError _msg, _arg, _arg2); \
        return false;                      \
    }

     bool MySource_SQL::setup(const MySourceParams_SQL &tParams, char *sError)
    {
        // checks
        assert(&tParams && "\nassert: empty source params!");
        assert(tParams.m_sQuery && "\nassert: empty query param!");

        m_tParams = tParams;

//set defaults
#define LOC_FIX_NULL(_arg) \
    if (!m_tParams._arg)   \
        m_tParams._arg = "";
        LOC_FIX_NULL(m_sHost);
        LOC_FIX_NULL(m_sUser);
        LOC_FIX_NULL(m_sPass);
        LOC_FIX_NULL(m_sDB);
#undef LOC_FIX_NULL

      
        return true;
    }
    bool MySource_SQL::connect(char *sError)
    {
        // do not connect again
        if (m_bSqlConnected)
            return true;

        // try to connect
        if (!SqlConnect())
        {
            sprintf_s(sError,256, "Mysql error: %s", SqlError());
            return false;
        }

        m_bSqlConnected = true;
        return true;
    }

    void MySource_SQL::disconnect()
    {
        // safeDeleteArray ( m_pReadFileBuffer );
        //m_tHits.m_dData.Reset();

        if (m_iNullIds)
            printInfo("\nsource %s: skipped %d document(s) with zero/NULL ids", m_sName, m_iNullIds);

        if (m_iMaxIds)
            printInfo("\nsource %s: skipped %d document(s) with DOCID_MAX ids", m_sName, m_iMaxIds);

        m_iNullIds = 0;
        m_iMaxIds = 0;

        if (m_bSqlConnected)
            SqlDisconnect();
        m_bSqlConnected = false;
    }

    void MySource_SQL::PostIndex()
    {
        //todo
    }
    // issue main rows fetch query
    bool MySource_SQL::IterateStart(char *sError)
    {
        assert(m_bSqlConnected);

        m_iNullIds = false;
        m_iMaxIds = false;

        // run pre-queries
        //TODO

        for (;;)
        {
            m_tParams.m_iRangeStep = 0;
            // normal query; just issue
            if (!SqlQuery(m_tParams.m_sQuery))
            {
                sprintf_s(sError,256, "Mysql error: %s", SqlError());
                return false;
            }
           
            break;
        }
        return true;
    }

    BYTE **MySource_SQL::NextDocument(char *sError)
    {
        assert(m_bSqlConnected);
        return NULL;
    }

    const int *MySource_SQL::GetFieldLengths() const
    {
        return 0;// m_dFieldLengths;
    }

} // namespace mynodes
