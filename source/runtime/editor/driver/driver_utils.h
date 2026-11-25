#pragma once

#include "runtime/editor/driver/driver_info.h"

#include <vector>
#include <cstdint>
#include <algorithm>

namespace Emulation
{
	class IMemoryRandomReadAccess;
	class CPUMemory;
} // namespace Emulation

namespace Utility
{
	class C64FileWriter;
}

namespace Editor
{
	class DataSourceTable;

	struct SIDWriteInformation
	{
		unsigned char m_AddressLow;
		unsigned char m_CycleOffset;
	};

	namespace DriverUtils
	{
		std::shared_ptr< DataSourceTable > CreateTableDataSource( const DriverInfo::TableDefinition& inTableDefinition,
																  Emulation::CPUMemory*				 inCPUMemory );
		unsigned char	   GetHighestSequenceIndexUsed( const Editor::DriverInfo&				  inDriverInfo,
														const Emulation::IMemoryRandomReadAccess& inMemoryReader );
		std::vector< int > GetSequenceUsageCount( const Editor::DriverInfo&					inDriverInfo,
												  const Emulation::IMemoryRandomReadAccess& inMemoryReader );
		unsigned char	   GetFirstUnusedSequenceIndex( const Editor::DriverInfo&				  inDriverInfo,
														const Emulation::IMemoryRandomReadAccess& inMemoryReader );
		unsigned char	   GetFirstEmptySequenceIndex( const Editor::DriverInfo&				 inDriverInfo,
													   const Emulation::IMemoryRandomReadAccess& inMemoryReader );
		unsigned char	   GetHighestInstrumentIndexUsed( const Editor::DriverInfo&					inDriverInfo,
														  const Emulation::IMemoryRandomReadAccess& inMemoryReader );
		unsigned char	   GetHighestCommandIndexUsed( const Editor::DriverInfo&				 inDriverInfo,
													   const Emulation::IMemoryRandomReadAccess& inMemoryReader );
		unsigned char	   GetHighestTableRowUsedIndex( const Editor::DriverInfo::TableDefinition& inTableDefinition,
														const Emulation::IMemoryRandomReadAccess&  inMemoryReader );
		std::vector< unsigned short > GetOrderListsLength( const Editor::DriverInfo&				 inDriverInfo,
														   const Emulation::IMemoryRandomReadAccess& inMemoryReader );
		unsigned short GetSequenceLength( unsigned short inSequenceIndex, const Editor::DriverInfo& inDriverInfo,
										  const Emulation::IMemoryRandomReadAccess& inMemoryReader );
		unsigned short GetEndOfMusicDataAddress( const Editor::DriverInfo&				   inDriverInfo,
												 const Emulation::IMemoryRandomReadAccess& inMemoryReader );
		unsigned short GetEndOfFileAddress( const Editor::DriverInfo&				  inDriverInfo,
											const Emulation::IMemoryRandomReadAccess& inMemoryReader );

		std::vector< SIDWriteInformation > GetSIDWriteInformationFromDriver( Emulation::CPUMemory& inCPUMemory,
																			 const DriverInfo&	   inDriverInfo );

		// Plays the song on a copy of the memory until every track has started over once or the driver stops.
		// Returns the play time in milliseconds, or 0 when that takes longer than inMaxMilliseconds
		unsigned int GetSongLengthInMilliseconds( Emulation::CPUMemory& inCPUMemory, const DriverInfo& inDriverInfo,
												  unsigned char inSongIndex, bool inPAL,
												  unsigned int inMaxMilliseconds );

		void		 InsertIRQ( const Editor::DriverInfo& inDriverInfo, Utility::C64FileWriter& inFileWriter );
	} // namespace DriverUtils

	// DriverReport replaces the driver_utilities query functions basing on internal driver knowledge.
	// The driver designer may want to apply some kind of better dirty-state-tracking.
	// The class is misplaced in this header file, though, it consumes above interfaces, thus
	// for a start it may live in here ...
	class DriverReport
	{
	  protected: // internal storage to cache data
		union SeqDat
		{
			uint16_t value;
			struct
			{
				uint8_t usecnt;
				uint8_t size;
			} d;
		};
		std::vector< SeqDat >					  m_SequenceData;
		std::vector< uint16_t >					  m_OrderListLengths;
		uint8_t									  m_MaxInsUsed, m_MaxCommandUsed;
		bool									  m_Dirty{ true };
		const Editor::DriverInfo&				  m_Driver;
		const Emulation::IMemoryRandomReadAccess& m_Reader;

		DriverReport&							  AcquireReport();

	  public:
		DriverReport( const Editor::DriverInfo& inDriverInfo, const Emulation::IMemoryRandomReadAccess& inMemoryReader )
			: m_Driver( inDriverInfo )
			, m_Reader( inMemoryReader )
		{}
		// please make it dirty yourself!
		void	MakeDirty() { m_Dirty = true; }
		// queries
		uint8_t GetHighestSequenceIndexUsed();
		std::vector< int > GetSequenceUsageCount();
		uint8_t GetFirstUnusedSequenceIndex();
		uint8_t GetFirstEmptySequenceIndex();
		uint8_t						   GetHighestInstrumentIndexUsed() { return AcquireReport().m_MaxInsUsed; }
		uint8_t						   GetHighestCommandIndexUsed() { return AcquireReport().m_MaxCommandUsed; }
		const std::vector< uint16_t >& GetOrderListsLength() { return AcquireReport().m_OrderListLengths; }
		uint16_t					   GetSequenceLength( uint16_t inSequenceIndex )
		{ return AcquireReport().m_SequenceData[ inSequenceIndex ].d.size; }
		uint16_t GetEndOfMusicDataAddress();
		uint16_t GetEndOfFileAddress();
	};
} // namespace Editor
