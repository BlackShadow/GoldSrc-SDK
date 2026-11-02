#include "utlvector.h"
#include "utlsymbol.h"

class CScheme;

template int CUtlVector<CUtlSymbol>::InsertBefore( int elem, const CUtlSymbol& src );
template int CUtlVector<CScheme*>::InsertBefore( int elem, CScheme* const& src );
