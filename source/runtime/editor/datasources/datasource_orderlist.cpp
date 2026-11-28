#include "datasource_orderlist.h"
#include "foundation/base/assert.h"
#include "runtime/editor/interfaces/idriver_architecture.h"
#include "runtime/emulation/cpumemory.h"

namespace Editor
{
	DataSourceOrderList::DataSourceOrderList( Emulation::CPUMemory* inCPUMemory, const Editor::DriverInfo& inDriverInfo,
											  const Editor::DriverState& inDriverState, unsigned short inSourceAddress,
											  int inBlockSize )
		: IDataSourceEvents( *inDriverInfo.GetDriverArchitecture()->m_OrderEvents, inCPUMemory, inDriverInfo,
							 inDriverState, inSourceAddress, inBlockSize, MAX_ORDER_EVENTS )
	{}

	bool DataSourceOrderList::CanIncreaseSize() const
	{ return m_Length < static_cast< unsigned int >( MAX_ORDER_EVENTS ); }

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
		for ( int i = 0; i < MAX_ORDER_EVENTS; ++i ) {
			if ( IsEndIndex( i ) ) {
				m_Length = i + 1;
				return;
			}
		}

		m_Length = MAX_ORDER_EVENTS - 1;
	}

	unsigned char DataSourceOrderList::GetLoopIndex() const
	{
		if ( m_Length > 0 ) return m_Events[ m_Length - 1 ].m_Payload;

		return 0;
	}

	void DataSourceOrderList::SetLoopIndex( unsigned char inLoopIndex )
	{
		if ( m_Length > 0 ) {
			const bool isEnd				   = inLoopIndex == m_Length - 1;

			m_Events[ m_Length - 1 ].m_Param   = m_EventDef.GetSentinel( SentinelType::Loop, Semantic::Parameter );
			m_Events[ m_Length - 1 ].m_Command = m_EventDef.GetSentinel( SentinelType::Empty, Semantic::Command );
			if ( inLoopIndex < m_Length - 1 ) {
				m_Events[ m_Length - 1 ].m_Payload = inLoopIndex;
			} else {
				FOUNDATION_ASSERT( inLoopIndex == m_Length - 1 );

				if ( !m_EventDef.IsParamSentinel( SentinelType::Stop, m_Events[ m_Length - 1 ] ) ) {
					m_Events[ m_Length - 1 ].m_Param =
						m_EventDef.GetSentinel( SentinelType::Stop, Semantic::Parameter );
					m_Events[ m_Length - 1 ].m_Payload = static_cast< unsigned int >( m_Length - 1 );
				} else {
					m_Events[ m_Length - 1 ].m_Payload = 0;
				}
			}
		}
	}

	DataSourceOrderList::PackedDataEventPosition DataSourceOrderList::GetIndexInPackedData( int inIndex ) const
	{
		auto resa = m_EventDef.GetNextIndexPackedPosition( inIndex, m_Data, m_PackedSize );
		return { resa.m_IsValid, resa.m_Results[ 0 ], resa.m_Results[ 1 ] };
	}

	void DataSourceOrderList::Flush() noexcept
	{
		ClearEvents(); // resets all entries to their default value according to description
		m_Length = 2;  // we want one sequence entry and a loop-back afterwards
		SetSentinel( 1, SentinelType::Loop, Semantic::Parameter );
		SetValue( 0u, 1, Semantic::Payload );
	}
} // namespace Editor
