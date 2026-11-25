#pragma once

#include "runtime/editor/interfaces/idriver_architecture.h"
#include "sf2_events.h"

namespace Editor
{
	class DriverArchitectureSidFactory2 final : public IDriverArchitecture
	{
	  public:
		DriverArchitectureSidFactory2()
			: IDriverArchitecture( SF2_OrderEvent(), SF2_SequenceEvent() )
		{}

		void				 SetEmptySequence( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress ) override;
		void				 SetEmptyOrderList( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress ) override;
		void				 PostInitSetPlaybackIndices( std::vector< PlayMarkerInfo >& inPlayMarkerInfoList,
														 Emulation::CPUMemory*			inCPUMemory,
														 const DriverInfo&				inDriverInfo ) override;

		static unsigned char GetDescriptorType() { return 0x00; }
	};
} // namespace Editor
