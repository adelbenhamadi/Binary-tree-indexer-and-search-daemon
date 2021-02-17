//***************************************************************************************/
//*    snippet of this code are adapted from Sphinxsearch source code
//*    Author: Copyright (c) 2001-2016, Andrew Aksyonoff / Copyright (c) 2008-2016, Sphinx Technologies Inc
//*    Date: ?
//*    Code version: 2.95
//*    Availability: http://www.sphinxsearch.com
//*
//***************************************************************************************///

#include "../helpers/utility.h"
#include "./source-mysql.h"

namespace mynodes
{
   // static size_t itot = 0;
   MySource_MySQL::MySource_MySQL(const char *sName)
       : MySource_SQL(sName)
   {
   }
   bool MySource_MySQL::setup(const MySourceParams_SQL& tParams, char *sError)
   {
      if (!MySource_SQL::setup(tParams, sError))
         return false;

      if (!connect(sError))
         return false;

      //execute query
      assert(m_pMysqlDriver && "null connection!");
      const char* qry = m_tParams.m_sQuery;
      if (!SqlQuery(qry))
      {
          sprintf_s(sError, 64, "Cannot query database! %s", SqlError());
          return false;
      };
      m_iFields = mysql_num_fields(m_pMysqlResult);
      m_sMysqlUsock = tParams.m_sUsock;
      m_iMysqlConnectFlags = tParams.m_iFlags;
      m_sSslKey = tParams.m_sSslKey;
      m_sSslCert = tParams.m_sSslCert;
      m_sSslCA = tParams.m_sSslCA;


      return true;
   }
   bool MySource_MySQL::load(size_t sz,char *sError)
   {
     
      
      did_t iDocid = 0;

     // SourceIterator_t it = m_tDocuments.begin();
      m_tDocuments.clear();
      size_t iDocCount = 0;
      sVector_t cols;
      while (iDocCount++ < sz &&  SqlFetchRow()  )
      {
         iDocid = (did_t)atoll(m_tMysqlRow[0]);
         cols.clear();
         for (unsigned int j = 1; j < m_iFields; j++)
         {
            if (m_tMysqlRow[j])
               cols.push_back(m_tMysqlRow[j]);
            else
               cols.push_back("\0");
         }
        // m_tDoc = std::make_pair(iDocid, cols);
         m_tDocuments.emplace(std::pair<did_t, sVector_t>(iDocid, cols));
        
         
      }
      //itot += m_tDocuments.size();
     // debugPrintf(10, "\n%llu documents inserted, m_pMysqlResult.rowcount= %llu",itot, m_pMysqlResult->row_count);

      return m_tDocuments.size()>0;
   }
   bool MySource_MySQL::SqlConnect()
   {

       m_pMysqlDriver = mysql_init(NULL);
      if (!m_pMysqlDriver)
      {
         return false;
      }
      if (!m_sSslKey || !m_sSslCert || !m_sSslCA)
         mysql_ssl_set(m_pMysqlDriver, m_sSslKey, m_sSslCert, m_sSslCA, NULL, NULL);

      m_iMysqlConnectFlags |= CLIENT_MULTI_RESULTS; // we now know how to handle this
      if (!mysql_real_connect(
              m_pMysqlDriver,
              m_tParams.m_sHost,
              m_tParams.m_sUser,
              m_tParams.m_sPass,
              m_tParams.m_sDB,
              m_tParams.m_iPort,
              m_tParams.m_sUsock,
              m_tParams.m_iFlags))
      {
         return false;
      }
      return true;
   }
   void MySource_MySQL::SqlDisconnect()
   {
      debugPrintf(3, "SQL-DISCONNECT\n");
      // clean up the database result set
      mysql_free_result(m_pMysqlResult);
      // clean up the database link
      mysql_close(m_pMysqlDriver);
   }

   bool MySource_MySQL::SqlQuery(const char *sQuery)
   {
      if (mysql_query(m_pMysqlDriver, sQuery))
      {
         debugPrintf(3, "SQL-QUERY: %s: FAIL\n", sQuery);
         return false;
      }
      debugPrintf(3, "SQL-QUERY: %s: ok\n", sQuery);

      m_pMysqlResult = mysql_use_result(m_pMysqlDriver);
      m_pMysqlFields = NULL;
      return true;
   }

   bool MySource_MySQL::SqlIsError()
   {
      return mysql_errno(m_pMysqlDriver) != 0;
   }

   const char *MySource_MySQL::SqlError()
   {
      return mysql_error(m_pMysqlDriver);
   }

   int MySource_MySQL::SqlNumFields()
   {
      if (!m_pMysqlResult)
         return -1;

      return mysql_num_fields(m_pMysqlResult);
   }

   bool MySource_MySQL::SqlFetchRow()
   {
      if (!m_pMysqlResult){
         return false;
      }
      m_tMysqlRow = mysql_fetch_row(m_pMysqlResult);
      if (!m_tMysqlRow)
          return false;
      return true;
     
   }



} // namespace mynodes

