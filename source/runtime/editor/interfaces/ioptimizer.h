#pragma once
/**
 *  SF2 optimizer interface
 *  =======================
 *  Here, we declare how a driver optimizes its data and feeds back information into SF2.
 */

#include "runtime/editor/driver/driver_info.h"
#include "runtime/editor/datasources/datasource_orderlist.h"
#include "runtime/editor/datasources/datasource_sequence.h"
#include "runtime/editor/datasources/datasource_table.h"

namespace Editor
{
	// The Optimizer Interface is an optional driver interface to SF2.  SF2 itself has optimizers
	// for its own drivers, but driver manufacturers would need to provide their own.
	struct IOptimizer
	{
		// to be created by its respective Driver Architecture class!
		IOptimizer( DriverInfo& inDriverInfo )
			: m_DriverInfo( inDriverInfo )
		{}
		virtual ~IOptimizer()																	 = default;

		// The actual interface is simply a call to optimize given data structure in
		// memory.  This interface may be enhanced like the original Optimizer does,
		// by sending the Event Data Sources down the optimizer, too (although it
		// could simply assign its own sources and create its own intermediate
		// representation)
		virtual void								Execute( Emulation::CPUMemory* inCPUMemory ) = 0;

		virtual const std::vector< unsigned char >& GetUsedSequenceIndices() const noexcept		 = 0;
		virtual const std::vector< unsigned char >& GetUsedInstrumentIndices() const noexcept	 = 0;
		virtual const std::vector< unsigned char >& GetUsedCommandIndices() const noexcept		 = 0;

		// In case the optimizer takes a Memory image and creates a new one: read only
		// operation would be the case.  Read only operation needs means to retrieve
		// final result.
		virtual bool								ReadOnlyOperation() const noexcept { return false; }
		virtual EventsPackResult					GetResult() { return {}; }

		// Driver info (references need to be constructed / thus inside the interface)
		DriverInfo&									m_DriverInfo;
	};
} // namespace Editor
