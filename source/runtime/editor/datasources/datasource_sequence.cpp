#include "datasource_sequence.h"
#include "foundation/base/assert.h"
#include "runtime/editor/driver/driver_info.h"
#include "runtime/editor/driver/driver_state.h"
#include "runtime/editor/interfaces/idriver_architecture.h"
#include "runtime/emulation/cpumemory.h"
#include <cstring>

namespace Editor
{
	DataSourceSequence::DataSourceSequence( Emulation::CPUMemory*	   inCPUMemory,
											const Editor::DriverInfo&  inDriverInfo,
											const Editor::DriverState& inDriverState,
											unsigned short			   inSourceAddress,
											int						   inBlockSize,
											const unsigned char		   inSequenceIndex )
		: IDataSourceEvents( inDriverInfo.GetDriverArchitecture()->m_SequenceEvents, inCPUMemory, inDriverInfo,
							 inDriverState, inSourceAddress, inBlockSize, MAX_SEQUENCE_EVENTS, inSequenceIndex )
	{}

	// made public ...
	DataSourceSequence::DataSourceSequence( const DataSourceSequence& inOther )
		: IDataSourceEvents( inOther )
	{}

	//------------------------------------------------------------------------------------------------------------------

	void DataSourceSequence::operator=( const DataSourceSequence& inRhs )
	{
		for ( int i = 0; i < MaxEventCount; ++i ) m_Events[ i ] = inRhs.m_Events[ i ];

		m_Length = inRhs.m_Length;
	}

	//------------------------------------------------------------------------------------------------------------------

	void DataSourceSequence::SetLength( unsigned int inLength )
	{
		FOUNDATION_ASSERT( inLength <= MaxEventCount );
		m_Length = inLength;
	}

	//------------------------------------------------------------------------------------------------------------------

	DataSourceSequence::PackedDataEventPosition
	DataSourceSequence::GetEventPositionInPackedData( int inEventPosition ) const
	{
		auto r = m_EventDef.GetRowIndex( inEventPosition, m_Data, m_PackedSize );
		return { r.m_IsValid, r.m_Results[ 0 ], r.m_Results[ 1 ], r.m_Results[ 2 ], r.m_Results[ 3 ], r.m_NextIsEnd };
	}
} // namespace Editor
