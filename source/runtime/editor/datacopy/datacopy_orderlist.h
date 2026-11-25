#pragma once

#include "runtime/editor/datasources/datasource_orderlist.h"
#include <vector>

namespace Editor
{
	class DataCopyOrderList
	{
	  public:
		DataCopyOrderList();
		DataCopyOrderList( const std::vector< DataSourceOrderList::Event >& inData );

		unsigned int					  GetEntryCount() const;
		const DataSourceOrderList::Event& operator[]( unsigned int inIndex ) const;

	  private:
		std::vector< DataSourceOrderList::Event > m_Data;
	};
} // namespace Editor
