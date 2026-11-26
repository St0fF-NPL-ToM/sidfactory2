#pragma once

#include "runtime/editor/interfaces/idriver_architecture.h"
#include "runtime/editor/optimize/optimizer.h"
#include "sf2_events.h"

namespace Editor
{
	class DriverArchitectureSidFactory2 final : public IDriverArchitecture
	{
	  public:
		DriverArchitectureSidFactory2();
		static constexpr uint8_t Descriptor = 0x00;
		uint8_t					 GetID() const noexcept override { return Descriptor; }
		void SetEmptySequence( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress ) override;
		void SetEmptyOrderList( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress ) override;
		void PostInitSetPlaybackIndices( std::vector< PlayMarkerInfo >& inPlayMarkerInfoList,
										 Emulation::CPUMemory*			inCPUMemory,
										 const DriverInfo&				inDriverInfo ) override;

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

		std::unique_ptr< IOptimizer > GetOptimizer( DriverInfo& di ) const noexcept override
		{ return std::make_unique< Optimizer >( di ); }

	  private:
		SF2_OrderEvent	  od;
		SF2_SequenceEvent sd;
	};
} // namespace Editor
