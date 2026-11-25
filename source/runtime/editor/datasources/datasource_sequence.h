#pragma once

#include "idatasource_events.h"

#include <cstdint>
#include <memory>

namespace Emulation
{
	class CPUMemory;
}

namespace Editor
{
	class DataSourceSequence : public IDataSourceEvents
	{
	  public:
		struct PackedDataEventPosition
		{
			bool		  m_IsValid;
			unsigned char m_Index;
			unsigned char m_RemainingTicks;
			unsigned char m_CurrentDurationValue;
			unsigned char m_CurrentInstrument;
			bool		  m_NextIsEndOfSequence;
		};

		DataSourceSequence( Emulation::CPUMemory*	   inCPUMemory,
							const Editor::DriverInfo&  inDriverInfo,
							const Editor::DriverState& inDriverState,
							unsigned short			   inSourceAddress,
							int						   inBlockSize,
							unsigned char			   inSequenceIndex );
		DataSourceSequence( const DataSourceSequence& inOther );
		virtual ~DataSourceSequence() = default;

		void					operator=( const DataSourceSequence& inRhs );

		void					SetLength( unsigned int inLength );

		unsigned char			GetLastInstrumentSet() const { return m_LastParameterSet; }

		PackedDataEventPosition GetEventPositionInPackedData( int inEventPosition ) const;
	};
} // namespace Editor
