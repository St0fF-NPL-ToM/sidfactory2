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

		// Now, here comes the part SF2 was not regarding as driver interface, which indeed is, though.
		std::vector< uint16_t > GetOrderListsLength( DRIVER_UTIL_PARAMS ) const noexcept override;
		std::vector< int >		GetSequenceUsageCount( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint16_t				GetEndOfMusicDataAddress( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint16_t GetSequenceLength( uint8_t inSequenceIndex, DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetFirstUnusedSequenceIndex( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetFirstEmptySequenceIndex( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetHighestSequenceIndexUsed( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetHighestInstrumentIndexUsed( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetHighestCommandIndexUsed( DRIVER_UTIL_PARAMS ) const noexcept override;
	};
} // namespace Editor
