//***************************************************************************************/
//*    snippet of this code are adapted from Sphinxsearch source code
//*    Author: Copyright (c) 2001-2016, Andrew Aksyonoff / Copyright (c) 2008-2016, Sphinx Technologies Inc
//*    Date: ?
//*    Code version: 2.95
//*    Availability: http://www.sphinxsearch.com
//*
//***************************************************************************************///

#ifndef _source_mysql_h_
#define _source_mysql_h_

#include "./source-base.h"
#include "./source-sql.h"
#include <mysql.h>

namespace mynodes
{

	struct MySource_MySQL : MySource_SQL
	{
		explicit MySource_MySQL(const char *sName);
		bool setup(const MySourceParams_SQL& tParams, char *sError);
		bool load(size_t sz,char *sError);
		void close() { 
			disconnect(); 
			m_tDocuments.clear();

		}

	protected:
		MYSQL_RES *m_pMysqlResult;
		MYSQL_FIELD *m_pMysqlFields;
		unsigned int m_iFields;
		MYSQL_ROW m_tMysqlRow;
		MYSQL *m_pMysqlDriver;
		unsigned long *m_pMysqlLengths;

		const char *m_sMysqlUsock;
		int m_iMysqlConnectFlags;
		const char *m_sSslKey;
		const char *m_sSslCert;
		const char *m_sSslCA;



	protected:
		bool SqlQuery(const char *sQuery);
		bool SqlIsError();
		const char *SqlError();

		int SqlNumFields();
		bool SqlFetchRow();
		bool SqlConnect();
		void SqlDisconnect();
	/*
		virtual DWORD SqlColumnLength(int iIndex);
		virtual const char *SqlColumn(int iIndex);
		virtual const char *SqlFieldName(int iIndex);
		*/
	};

} // namespace mynodes
#endif
