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
#include "imemory_streaming.h"
#include <vector>

namespace Emulation
{
	class CPUMemory;
}

namespace Editor
{
	class DriverInfo;
	class IDriverArchitecture
		: public IOrderStreaming
		, public ISequenceStreaming
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
		IDriverArchitecture( EventDefinition&& OrderEventDef, EventDefinition&& SequenceEventDef )
			: m_OrderEvents( OrderEventDef )
			, m_SequenceEvents( SequenceEventDef )
		{}
		virtual ~IDriverArchitecture() {}

		virtual void	SetEmptySequence( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress )	 = 0;
		virtual void	SetEmptyOrderList( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress ) = 0;
		virtual void	PostInitSetPlaybackIndices( std::vector< PlayMarkerInfo >& inPlayMarkerInfoList,
													Emulation::CPUMemory*		   inCPUMemory,
													const DriverInfo&			   inDriverInfo )		 = 0;

		EventDefinition m_OrderEvents;
		EventDefinition m_SequenceEvents;
	};
} // namespace Editor
