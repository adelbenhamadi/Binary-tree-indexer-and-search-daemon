#ifndef _hash_h_
#define _hash_h_


#include "utility.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>



bool fileIsReadable ( const char * sPath, char * sError );

DWORD sphCRC32 ( const void * s );

/// simple dynamic hash
/// implementation: fixed-size bucket + chaining
/// keeps the order, so Iterate() return the entries in the order they was inserted
/// WARNING: slow copy
template < typename T, typename KEY, typename HASHFUNC, int LENGTH >
class CSphOrderedHash
{
protected:
	struct HashEntry_t
	{
		KEY				m_tKey;				///< key, owned by the hash
		T 				m_tValue;			///< data, owned by the hash
		HashEntry_t *	m_pNextByHash;		///< next entry in hash list
		HashEntry_t *	m_pPrevByOrder;		///< prev entry in the insertion order
		HashEntry_t *	m_pNextByOrder;		///< next entry in the insertion order
	};


protected:
	HashEntry_t *	m_dHash [ LENGTH ];		///< all the hash entries
	HashEntry_t *	m_pFirstByOrder;		///< first entry in the insertion order
	HashEntry_t *	m_pLastByOrder;			///< last entry in the insertion order
	int				m_iLength;				///< entries count

protected:
	/// find entry by key
	HashEntry_t * FindByKey ( const KEY & tKey ) const
	{
		unsigned int uHash = ( (unsigned int) HASHFUNC::Hash ( tKey ) ) % LENGTH;
		HashEntry_t * pEntry = m_dHash [ uHash ];

		while ( pEntry )
		{
			if ( pEntry->m_tKey==tKey )
				return pEntry;
			pEntry = pEntry->m_pNextByHash;
		}
		return NULL;
	}

	HashEntry_t * AddImpl ( const KEY &tKey )
	{
		unsigned int uHash = ( (unsigned int) HASHFUNC::Hash ( tKey ) ) % LENGTH;

		// check if this key is already hashed
		HashEntry_t * pEntry = m_dHash[uHash];
		HashEntry_t ** ppEntry = &m_dHash[uHash];
		while ( pEntry )
		{
			if ( pEntry->m_tKey==tKey )
				return nullptr;

			ppEntry = &pEntry->m_pNextByHash;
			pEntry = pEntry->m_pNextByHash;
		}

		// it's not; let's add the entry
		assert ( !pEntry );
		assert ( !*ppEntry );

		pEntry = new HashEntry_t;
		pEntry->m_tKey = tKey;
		pEntry->m_pNextByHash = NULL;
		pEntry->m_pPrevByOrder = NULL;
		pEntry->m_pNextByOrder = NULL;

		*ppEntry = pEntry;

		if ( !m_pFirstByOrder )
			m_pFirstByOrder = pEntry;

		if ( m_pLastByOrder )
		{
			assert ( !m_pLastByOrder->m_pNextByOrder );
			assert ( !pEntry->m_pNextByOrder );
			m_pLastByOrder->m_pNextByOrder = pEntry;
			pEntry->m_pPrevByOrder = m_pLastByOrder;
		}
		m_pLastByOrder = pEntry;

		m_iLength++;
		return pEntry;
	}

public:
	/// ctor
	CSphOrderedHash ()
		: m_pFirstByOrder ( NULL )
		, m_pLastByOrder ( NULL )
		, m_iLength ( 0 )
		, m_pIterator ( NULL )
	{
		for ( int i=0; i<LENGTH; i++ )
			m_dHash[i] = NULL;
	}

	/// dtor
	~CSphOrderedHash ()
	{
		Reset ();
	}

	/// reset
	void Reset ()
	{
		assert ( ( m_pFirstByOrder && m_iLength ) || ( !m_pFirstByOrder && !m_iLength ) );
		HashEntry_t * pKill = m_pFirstByOrder;
		while ( pKill )
		{
			HashEntry_t * pNext = pKill->m_pNextByOrder;
			safeDelete ( pKill );
			pKill = pNext;
		}

		for ( int i=0; i<LENGTH; i++ )
			m_dHash[i] = 0;

		m_pFirstByOrder = NULL;
		m_pLastByOrder = NULL;
		m_pIterator = NULL;
		m_iLength = 0;
	}

	/// add new entry
	/// returns true on success
	/// returns false if this key is already hashed
	bool Add ( T&& tValue, const KEY & tKey )
	{
		// check if this key is already hashed
		HashEntry_t * pEntry = AddImpl ( tKey );
		if ( !pEntry )
			return false;
		pEntry->m_tValue = std::move ( tValue );
		return true;
	}

	bool Add ( const T & tValue, const KEY & tKey )
	{
		// check if this key is already hashed
		HashEntry_t * pEntry = AddImpl ( tKey );
		if ( !pEntry )
			return false;
		pEntry->m_tValue = tValue;
		return true;
	}

	/// add new entry
	/// returns the pointer to just inserted or previously cached (if dupe) value
	T & AddUnique ( const KEY & tKey )
	{
		unsigned int uHash = ( (unsigned int) HASHFUNC::Hash ( tKey ) ) % LENGTH;

		// check if this key is already hashed
		HashEntry_t * pEntry = m_dHash [ uHash ];
		HashEntry_t ** ppEntry = &m_dHash [ uHash ];
		while ( pEntry )
		{
			if ( pEntry->m_tKey==tKey )
				return pEntry->m_tValue;

			ppEntry = &pEntry->m_pNextByHash;
			pEntry = pEntry->m_pNextByHash;
		}

		// it's not; let's add the entry
		assert ( !pEntry );
		assert ( !*ppEntry );

		pEntry = new HashEntry_t;
		pEntry->m_tKey = tKey;
		pEntry->m_pNextByHash = NULL;
		pEntry->m_pPrevByOrder = NULL;
		pEntry->m_pNextByOrder = NULL;

		*ppEntry = pEntry;

		if ( !m_pFirstByOrder )
			m_pFirstByOrder = pEntry;

		if ( m_pLastByOrder )
		{
			assert ( !m_pLastByOrder->m_pNextByOrder );
			assert ( !pEntry->m_pNextByOrder );
			m_pLastByOrder->m_pNextByOrder = pEntry;
			pEntry->m_pPrevByOrder = m_pLastByOrder;
		}
		m_pLastByOrder = pEntry;

		m_iLength++;
		return pEntry->m_tValue;
	}

	/// delete an entry
	bool Delete ( const KEY & tKey )
	{
		unsigned int uHash = ( (unsigned int) HASHFUNC::Hash ( tKey ) ) % LENGTH;
		HashEntry_t * pEntry = m_dHash [ uHash ];

		HashEntry_t * pPrevEntry = NULL;
		HashEntry_t * pToDelete = NULL;
		while ( pEntry )
		{
			if ( pEntry->m_tKey==tKey )
			{
				pToDelete = pEntry;
				if ( pPrevEntry )
					pPrevEntry->m_pNextByHash = pEntry->m_pNextByHash;
				else
					m_dHash [ uHash ] = pEntry->m_pNextByHash;

				break;
			}

			pPrevEntry = pEntry;
			pEntry = pEntry->m_pNextByHash;
		}

		if ( !pToDelete )
			return false;

		if ( pToDelete->m_pPrevByOrder )
			pToDelete->m_pPrevByOrder->m_pNextByOrder = pToDelete->m_pNextByOrder;
		else
			m_pFirstByOrder = pToDelete->m_pNextByOrder;

		if ( pToDelete->m_pNextByOrder )
			pToDelete->m_pNextByOrder->m_pPrevByOrder = pToDelete->m_pPrevByOrder;
		else
			m_pLastByOrder = pToDelete->m_pPrevByOrder;

		// step the iterator one item back - to gracefully hold deletion in iteration cycle
		if ( pToDelete==m_pIterator )
			m_pIterator = pToDelete->m_pPrevByOrder;

		SafeDelete ( pToDelete );
		--m_iLength;

		return true;
	}

	/// check if key exists
	bool Exists ( const KEY & tKey ) const
	{
		return FindByKey ( tKey )!=NULL;
	}

	/// get value pointer by key
	T * operator () ( const KEY & tKey ) const
	{
		HashEntry_t * pEntry = FindByKey ( tKey );
		return pEntry ? &pEntry->m_tValue : NULL;
	}

	/// get value reference by key, asserting that the key exists in hash
	T & operator [] ( const KEY & tKey ) const
	{
		HashEntry_t * pEntry = FindByKey ( tKey );
		assert ( pEntry && "hash missing value in operator []" );

		return pEntry->m_tValue;
	}

	/// get pointer to key storage
	const KEY * GetKeyPtr ( const KEY & tKey ) const
	{
		HashEntry_t * pEntry = FindByKey ( tKey );
		return pEntry ? &pEntry->m_tKey : NULL;
	}

	/// copying
	const CSphOrderedHash<T,KEY,HASHFUNC,LENGTH> & operator = ( const CSphOrderedHash<T,KEY,HASHFUNC,LENGTH> & rhs )
	{
		if ( this!=&rhs )
		{
			Reset ();

			rhs.IterateStart ();
			while ( rhs.IterateNext() )
				Add ( rhs.IterateGet(), rhs.IterateGetKey() );
		}
		return *this;
	}

	/// copying ctor
	CSphOrderedHash<T,KEY,HASHFUNC,LENGTH> ( const CSphOrderedHash<T,KEY,HASHFUNC,LENGTH> & rhs )
		: m_pFirstByOrder ( NULL )
		, m_pLastByOrder ( NULL )
		, m_iLength ( 0 )
		, m_pIterator ( NULL )
	{
		for ( int i=0; i<LENGTH; i++ )
			m_dHash[i] = NULL;
		*this = rhs;
	}

	/// length query
	int GetLength () const
	{
		return m_iLength;
	}

public:
	/// start iterating
	void IterateStart () const
	{
		m_pIterator = NULL;
	}

	/// start iterating from key element
	bool IterateStart ( const KEY & tKey ) const
	{
		m_pIterator = FindByKey ( tKey );
		return m_pIterator!=NULL;
	}

	/// go to next existing entry
	bool IterateNext () const
	{
		m_pIterator = m_pIterator ? m_pIterator->m_pNextByOrder : m_pFirstByOrder;
		return m_pIterator!=NULL;
	}

	/// get entry value
	T & IterateGet () const
	{
		assert ( m_pIterator );
		return m_pIterator->m_tValue;
	}

	/// get entry key
	const KEY & IterateGetKey () const
	{
		assert ( m_pIterator );
		return m_pIterator->m_tKey;
	}

	/// go to next existing entry in terms of external independed iterator
	bool IterateNext ( void ** ppCookie ) const
	{
		HashEntry_t ** ppIterator = reinterpret_cast < HashEntry_t** > ( ppCookie );
		*ppIterator = ( *ppIterator ) ? ( *ppIterator )->m_pNextByOrder : m_pFirstByOrder;
		return ( *ppIterator )!=NULL;
	}

	/// get entry value in terms of external independed iterator
	static T & IterateGet ( void ** ppCookie )
	{
		assert ( ppCookie );
		HashEntry_t ** ppIterator = reinterpret_cast < HashEntry_t** > ( ppCookie );
		assert ( *ppIterator );
		return ( *ppIterator )->m_tValue;
	}

	/// get entry key in terms of external independed iterator
	static const KEY & IterateGetKey ( void ** ppCookie )
	{
		assert ( ppCookie );
		HashEntry_t ** ppIterator = reinterpret_cast < HashEntry_t** > ( ppCookie );
		assert ( *ppIterator );
		return ( *ppIterator )->m_tKey;
	}


private:
	/// current iterator
	mutable HashEntry_t *	m_pIterator;
};

/// string hash function
struct CSphStrHashFunc
{
	static int Hash ( const std::string & sKey ){
		return sKey.empty() ? 0 : sphCRC32 ( sKey.c_str() );
	};
};
/// small hash with string keys
template < typename T >
class CSphSmallStringHash_T : public CSphOrderedHash < T, std::string, CSphStrHashFunc, 256 > {};



#endif
