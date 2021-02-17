//***************************************************************************************/
//*    snippet of this code are udapted from Sphinxsearch source code
//*    Author: Copyright (c) 2001-2016, Andrew Aksyonoff / Copyright (c) 2008-2016, Sphinx Technologies Inc
//*    Date: ?
//*    Code version: 2.95
//*    Availability: http://www.sphinxsearch.com
//*
//***************************************************************************************///

#ifndef _source_sql_h_
#define _source_sql_h_

#include "./source-base.h"


namespace mynodes{

	// source params
	struct MySourceParams_SQL
	{
		// query params
		const char *m_sQuery;
		const char *m_sQueryPre;
		const char *m_sQueryPost;
		const char *m_sQueryRange;

		const char *m_sQueryKilllist;
		int64_t		m_iRangeStep;
		int64_t		m_iRefRangeStep;
		bool		m_bPrintQueries;

		std::vector<string_t> m_dQueryPre;
		std::vector<string_t> m_dQueryPost;
		std::vector<string_t> m_dQueryPostIndex;
		std::vector<string_t> m_dFileFields;

		int m_iRangedThrottle;
		int m_iMaxFileBufferSize;
		
		// connection params
		const char *m_sHost;
		const char *m_sUser;
		const char *m_sPass;
		const char *m_sDB;
		int			m_iPort;

		const char *m_sUsock;
		const char *m_sSslKey;
		const char *m_sSslCert;
		const char *m_sSslCA;
		int m_iFlags;

		/// ctor
		MySourceParams_SQL()
		: m_iRangeStep ( 1024 )
		, m_iRefRangeStep ( 1024 )
		, m_bPrintQueries ( false )
		, m_iRangedThrottle ( 0 )
		, m_iMaxFileBufferSize ( 0 )
		, m_iPort ( 0 )
		{};
	};

	struct MySource_SQL : public MySource
	{
		MySourceParams_SQL m_tParams;

		MySource_SQL(const char *sName)
			: MySource(sName)
			 , m_bSqlConnected(false)
		{
		}
		virtual bool setup(const MySourceParams_SQL &tParams, char *sError);
		virtual bool connect(char *sError);
		virtual bool load(size_t sz, char* sError) = 0;
		virtual void close() = 0;
		virtual void disconnect();
		virtual void PostIndex();
		//new
		virtual bool IterateStart(char *sError);
		virtual BYTE **NextDocument(char *sError);
		virtual const int *GetFieldLengths() const;
		

	private:
		bool m_bSqlConnected;

	
	protected:
		bool RunQueryStep(const char *sQuery, char *sError);

		virtual bool SqlQuery(const char *sQuery) = 0;
		virtual bool SqlIsError() = 0;
		virtual const char *SqlError() = 0;
		virtual int SqlNumFields() = 0;
		virtual bool SqlFetchRow() = 0;

		virtual bool SqlConnect() = 0;
		virtual void SqlDisconnect() = 0;

	};


}//end mynodes


#endif
