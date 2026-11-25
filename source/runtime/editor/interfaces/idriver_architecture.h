/**
 *	SIDFactoryII driver architecture interface
 *	==========================================
 *	This is how a driver explains itself and makes it's C64 counterpart
 *	generate music with a set of data.
 *
 *	It describes the translation layer between Editor Events and actual
 *	driver data encoding and placement.
 *
 *	-	Initialize the architecture itself
 *	-	Read (unpack) / Write (Pack) special data pakets:
 *		-	OrderLists
 *		-	Sequences
 *	-	Query data structure:
 *		-	Sequence / Order count
 *		-	Sequence / Order termination
 *		-
 */

#pragma once

#include "ievent_definition.h"
#include "ioptimizer.h"
#include <vector>

namespace Emulation
{
	class CPUMemory;
	class IMemoryRandomReadAccess;
} // namespace Emulation

namespace Editor
{
	class DriverInfo;
	class IDriverArchitecture
	{
	  public:
		struct PlayMarkerInfo
		{
			unsigned char m_OrderListIndex;
			unsigned char m_CurrentSequence;
			unsigned char m_CurrentTranspose;
			unsigned char m_SequenceIndex;
			unsigned char m_CurrentSequenceEventDuration;
			unsigned char m_TickCounters;
			unsigned char m_NextInstrumentAddress;
			bool		  m_SequenceInUse;
		};
		// This way a derived CTor can do the Event-Description init. See SF2 arch for details.
		IDriverArchitecture( IEventDefinition&& OrderEventDef, IEventDefinition&& SequenceEventDef )
			: m_OrderEvents( OrderEventDef )
			, m_SequenceEvents( SequenceEventDef )
		{}
		virtual ~IDriverArchitecture() {}
		// Static constants cannot be made inherited. So we give it another virtual …
		virtual uint8_t GetID() const noexcept { return 0xff; }

		// Default SF2 Architectural interface
		using CPU																			   = Emulation::CPUMemory;
		virtual void SetEmptySequence( CPU* inCPUMemory, unsigned short inAddress )			   = 0;
		virtual void SetEmptyOrderList( CPU* inCPUMemory, unsigned short inAddress )		   = 0;
		virtual void PostInitSetPlaybackIndices( std::vector< PlayMarkerInfo >& inPlayMarkerInfoList,
												 CPU*							inCPUMemory,
												 const DriverInfo&				inDriverInfo ) = 0;

		// Driver Utility architectural functionality:
		// → all these functions operate on driver byte data, so they are certainly driver_arch_dependent …
		// → all these functions operate on a certain driver state / driver info, thus the call signature will
		//	  always contain input memory and a driver info containing the current states.

#define DRIVER_UTIL_PARAMS const DriverInfo&, const Emulation::IMemoryRandomReadAccess&

		virtual std::vector< uint16_t > GetOrderListsLength( DRIVER_UTIL_PARAMS ) const noexcept		 = 0;
		virtual std::vector< int >		GetSequenceUsageCount( DRIVER_UTIL_PARAMS ) const noexcept		 = 0;
		virtual uint16_t				GetEndOfMusicDataAddress( DRIVER_UTIL_PARAMS ) const noexcept	 = 0;
		virtual uint16_t GetSequenceLength( uint8_t inSequenceIndex, DRIVER_UTIL_PARAMS ) const noexcept = 0;
		virtual uint8_t	 GetFirstUnusedSequenceIndex( DRIVER_UTIL_PARAMS ) const noexcept				 = 0;
		virtual uint8_t	 GetFirstEmptySequenceIndex( DRIVER_UTIL_PARAMS ) const noexcept				 = 0;
		virtual uint8_t	 GetHighestSequenceIndexUsed( DRIVER_UTIL_PARAMS ) const noexcept				 = 0;
		virtual uint8_t	 GetHighestInstrumentIndexUsed( DRIVER_UTIL_PARAMS ) const noexcept				 = 0;
		virtual uint8_t	 GetHighestCommandIndexUsed( DRIVER_UTIL_PARAMS ) const noexcept				 = 0;

		virtual std::unique_ptr< IOptimizer > GetOptimizer( DriverInfo& ) const noexcept { return {}; }

		const IEventDefinition&				  m_OrderEvents;
		const IEventDefinition&				  m_SequenceEvents;
	};
} // namespace Editor
