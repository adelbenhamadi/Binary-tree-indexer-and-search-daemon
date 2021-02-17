#ifndef _config_parser_h_
#define _config_parser_h_

#include "./utility.h"
#include "./hash.h"


namespace mynodes{

struct CSphVariant
{
protected:
	std::string		m_sValue;
	int				m_iValue;
	int64_t			m_i64Value;
	float			m_fValue;

public:
	CSphVariant *	m_pNext;
	// tags are used for handling multiple same keys
	bool			m_bTag; // 'true' means override - no multi-valued; 'false' means multi-valued - chain them
	int				m_iTag; // stores order like in config file

public:
	/// default ctor
	CSphVariant ()
		: m_iValue ( 0 )
		, m_i64Value ( 0 )
		, m_fValue ( 0.0f )
		, m_pNext ( NULL )
		, m_bTag ( false )
		, m_iTag ( 0 )
	{
	}

	/// ctor from C string
	CSphVariant ( const char * sString, int iTag )
		: m_sValue ( sString )
		, m_iValue ( sString ? atoi ( sString ) : 0 )
		, m_i64Value ( sString ? (int64_t) strtoull ( sString, NULL, 10 ) : 0 )
		, m_fValue ( sString ? (float)atof ( sString ) : 0.0f )
		, m_pNext ( NULL )
		, m_bTag ( false )
		, m_iTag ( iTag )
	{
	}

	/// copy ctor
	CSphVariant ( const CSphVariant & rhs )
	{
		m_pNext = NULL;
		*this = rhs;
	}

	/// default dtor
	/// WARNING: automatically frees linked items!
	~CSphVariant ()
	{
		safeDelete ( m_pNext );
	}

	const char * cstr() const { return m_sValue.c_str(); }

	const std::string & strval () const { return m_sValue; }
	int intval () const	{ return m_iValue; }
	int64_t int64val () const { return m_i64Value; }
	float floatval () const	{ return m_fValue; }

	/// default copy operator
	const CSphVariant & operator = ( const CSphVariant & rhs )
	{
		assert ( !m_pNext );
		if ( rhs.m_pNext )
			m_pNext = new CSphVariant ( *rhs.m_pNext );

		m_sValue = rhs.m_sValue;
		m_iValue = rhs.m_iValue;
		m_i64Value = rhs.m_i64Value;
		m_fValue = rhs.m_fValue;
		m_bTag = rhs.m_bTag;
		m_iTag = rhs.m_iTag;

		return *this;
	}

	bool operator== ( const char * s ) const { return m_sValue==s; }
	bool operator!= ( const char * s ) const { return m_sValue!=s; }
};
/// config section (hash of variant values)
class CSphConfigSection : public CSphSmallStringHash_T < CSphVariant >
{
public:
	CSphConfigSection ()
		: m_iTag ( 0 )
	{}

	/// get integer option value by key and default value
	int GetInt ( const char * sKey, int iDefault=0 ) const
	{
		CSphVariant * pEntry = (*this)( sKey );
		return pEntry ? pEntry->intval() : iDefault;
	}

	/// get float option value by key and default value
	float GetFloat ( const char * sKey, float fDefault=0.0f ) const
	{
		CSphVariant * pEntry = (*this)( sKey );
		return pEntry ? pEntry->floatval() : fDefault;
	}

	/// get string option value by key and default value
	const char * GetStr ( const char * sKey, const char * sDefault="" ) const
	{
		CSphVariant * pEntry = (*this)( sKey );
		return pEntry ? pEntry->strval().c_str() : sDefault;
	}

	/// get size option (plain int, or with K/M prefix) value by key and default value
	int		GetSize ( const char * sKey, int iDefault ) const;
	int64_t GetSize64 ( const char * sKey, int64_t iDefault ) const;

	int m_iTag;
};

/// config section type (hash of sections)
typedef CSphSmallStringHash_T < CSphConfigSection >	MyConfigType;

/// config (hash of section types)
typedef CSphSmallStringHash_T < MyConfigType >	MyConfig;

class CSphConfigParser
{
public:
	MyConfig		m_tConf;
	CSphConfigParser ()
	: m_sFileName ( "" )
	, m_iLine ( -1 )
	,m_iWarnings(0)
	{}
	~CSphConfigParser(){};
	bool			Parse ( const char * sFileName, const char * pBuffer = NULL );
	const char *	loadConfig ( const char * sOptConfig, bool bQuiet);
protected:
	std::string		m_sFileName;
	int				m_iLine;
	std::string		m_sSectionType;
	std::string		m_sSectionName;
	char			m_sError [ 1024 ];

	int					m_iWarnings;
	static const int	WARNS_THRESH	= 5;

protected:
	bool			IsPlainSection ( const char * sKey );
	bool			IsNamedSection ( const char * sKey );
	bool			AddSection ( const char * sType, const char * sSection );
	void			AddKey ( const char * sKey, char * sValue );
	bool			ValidateKey ( const char * sKey );
	char *			GetBufferString ( char * szDest, int iMax, const char * & szSource );
};

bool TryToExec ( char * pBuffer, const char * szFilename, std::vector<char> & dResult, char * sError, int iErrorLen );

}

#endif // _config_parser_h_
