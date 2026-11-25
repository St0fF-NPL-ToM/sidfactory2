#pragma once

#include "datasource_emulation_memory.h"
#include "runtime/editor/interfaces/ievent_definition.h"
#include "runtime/editor/datasources/idatasource_events.h"
#include "runtime/editor/global_constants.h"

#include <memory>

namespace Emulation
{
	class CPUMemory;
}

namespace Editor
{
	class IDriverArchitecture;

	class DataSourceOrderList : public IDataSourceEvents
	{
	  public:
		struct PackedDataEventPosition
		{
			bool		  m_IsValid;
			unsigned char m_Index;
			unsigned char m_Sequence;
			unsigned char m_CurrentTranspose;
		};

		DataSourceOrderList( Emulation::CPUMemory* inCPUMemory, const Editor::DriverInfo& inDriverInfo,
							 const Editor::DriverState& inDriverState, unsigned short inSourceAddress,
							 int inBlockSize );
		virtual ~DataSourceOrderList() = default;

		uint8_t		  GetSequenceId( int inIndex ) const noexcept { return IDataSourceEvents::GetPayload( inIndex ); }

		bool		  CanIncreaseSize() const;
		void		  IncreaseSize();
		void		  DecreaseSize();
		void		  ComputeLength();

		unsigned char GetLoopIndex() const;
		void		  SetLoopIndex( unsigned char inLoopIndex );

		PackedDataEventPosition GetIndexInPackedData( int inIndex ) const;

		void Flush() noexcept;
	};
} // namespace Editor
