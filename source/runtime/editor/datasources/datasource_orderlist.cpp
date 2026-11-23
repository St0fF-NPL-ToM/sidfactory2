#include "datasource_orderlist.h"
#include "foundation/base/assert.h"
#include "runtime/editor/interfaces/idriver_architecture.h"
#include "runtime/emulation/cpumemory.h"
#include <cstring>

namespace Editor
{
	DataSourceOrderList::DataSourceOrderList( const IDriverArchitecture* inArchitecture,
											  Emulation::CPUMemory*		 inCPUMemory,
											  unsigned short			 inSourceAddress,
											  int						 inBlockSize )
		: DataSourceEmulationMemory( inCPUMemory, inSourceAddress, inBlockSize )
		, m_Length( 0 )
		, m_PackedSize( 0 )
	{
		m_Events		 = new Entry[ MaxEntryCount ];
		m_InternalBuffer = new unsigned char[ MaxEntryCount * 2 + 1 ];

		ClearEntries();

		PullDataFromSource();
	}

	DataSourceOrderList::~DataSourceOrderList()
	{
		delete[] m_Events;
		delete[] m_InternalBuffer;
	}


	DataSourceOrderList::Entry& DataSourceOrderList::operator[]( int inIndex )
	{
		FOUNDATION_ASSERT( inIndex < MaxEntryCount );
		FOUNDATION_ASSERT( m_Events != nullptr );

		return m_Events[ inIndex ];
	}


	const DataSourceOrderList::Entry& DataSourceOrderList::operator[]( int inIndex ) const
	{
		FOUNDATION_ASSERT( inIndex < MaxEntryCount );
		FOUNDATION_ASSERT( m_Events != nullptr );

		return m_Events[ inIndex ];
	}


	bool DataSourceOrderList::PushDataToSource()
	{
		FOUNDATION_ASSERT( m_CPUMemory != nullptr );
		FOUNDATION_ASSERT( m_CPUMemory->IsLocked() );
		FOUNDATION_ASSERT( m_Data != nullptr );

		m_CPUMemory->SetData( m_SourceAddress, m_Data, m_DataSize );

		return true;
	}


	unsigned int DataSourceOrderList::GetPackedSize() const
	{ return m_PackedSize; }


	unsigned int DataSourceOrderList::GetLength() const
	{ return m_Length; }


	bool DataSourceOrderList::CanIncreaseSize() const
	{ return m_Length < static_cast< unsigned int >( MaxEntryCount ); }

	void DataSourceOrderList::IncreaseSize()
	{
		FOUNDATION_ASSERT( CanIncreaseSize() );

		++m_Length;
	}

	void DataSourceOrderList::DecreaseSize()
	{
		FOUNDATION_ASSERT( m_Length > 2 );

		--m_Length;
	}


	void DataSourceOrderList::ComputeLength()
	{
		for ( int i = 0; i < MaxEntryCount; ++i ) {
			if ( m_Arch->m_OrderEvents.IsParamSentinel( SentinelType::Stop, m_Events[ i ] )
				 || m_Arch->m_OrderEvents.IsParamSentinel( SentinelType::Loop, m_Events[ i ] ) )
			{
				m_Length = i + 1;
				return;
			}
		}

		m_Length = MaxEntryCount - 1;
	}


	unsigned char DataSourceOrderList::GetLoopIndex() const
	{
		if ( m_Length > 0 ) return m_Events[ m_Length - 1 ].m_Payload;

		return 0;
	}


	void DataSourceOrderList::SetLoopIndex( unsigned char inLoopIndex )
	{
		const auto& a = m_Arch->m_OrderEvents;
		if ( m_Length > 0 ) {
			const bool isEnd				   = inLoopIndex == m_Length - 1;

			m_Events[ m_Length - 1 ].m_Param   = a.GetSentinel( SentinelType::Loop, Semantic::Parameter );
			m_Events[ m_Length - 1 ].m_Command = a.GetSentinel( SentinelType::Empty, Semantic::Command );
			if ( inLoopIndex < m_Length - 1 ) {
				m_Events[ m_Length - 1 ].m_Payload = inLoopIndex;
			} else {
				FOUNDATION_ASSERT( inLoopIndex == m_Length - 1 );

				if ( !a.IsParamSentinel( SentinelType::Stop, m_Events[ m_Length - 1 ] ) ) {
					m_Events[ m_Length - 1 ].m_Param   = a.GetSentinel( SentinelType::Stop, Semantic::Parameter );
					m_Events[ m_Length - 1 ].m_Payload = static_cast< unsigned int >( m_Length - 1 );
				} else {
					m_Events[ m_Length - 1 ].m_Payload = 0;
				}
			}
		}
	}


	DataSourceOrderList::PackResult DataSourceOrderList::Pack() const
	{ return m_Arch->PackOrderList( *this, m_InternalBuffer ); }


	void DataSourceOrderList::SendPackedDataToBuffer( const PackResult& inPackResult )
	{
		FOUNDATION_ASSERT( inPackResult.m_DataLength <= m_DataSize );

		memset( m_Data, 0, m_DataSize );
		memcpy( m_Data, &*inPackResult.m_Data, inPackResult.m_DataLength );

		m_PackedSize = inPackResult.m_DataLength;
	}


	DataSourceOrderList::PackedDataEventPosition DataSourceOrderList::GetIndexInPackedData( int inIndex ) const
	{ return m_Arch->GetOrderListIndex( inIndex, m_Data, m_PackedSize ); }


	void DataSourceOrderList::PullDataFromSource()
	{
		FOUNDATION_ASSERT( m_CPUMemory != nullptr );
		FOUNDATION_ASSERT( m_Data != nullptr );

		m_CPUMemory->Lock();
		m_CPUMemory->GetData( m_SourceAddress, m_Data, m_DataSize );
		m_CPUMemory->Unlock();

		ClearEntries();
		Unpack();
	}

	void DataSourceOrderList::ClearEntries()
	{
		for ( int i = 0; i < MaxEntryCount; ++i ) m_Arch->m_OrderEvents.ClearEvent( m_Events[ i ] );
	}

	void DataSourceOrderList::Unpack()
	{ m_Arch->UnpackOrderList( *this, m_Data ); }

	bool DataSourceOrderList::IsEndIndex( int inIndex ) const
	{
		return ( m_Arch->m_OrderEvents.IsParamSentinel( SentinelType::Loop, m_Events[ inIndex ] )
				 || m_Arch->m_OrderEvents.IsParamSentinel( SentinelType::Stop, m_Events[ inIndex ] ) );
	};
} // namespace Editor
