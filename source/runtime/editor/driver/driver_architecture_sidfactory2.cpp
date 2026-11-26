#include "driver_architecture_sidfactory2.h"

#include "runtime/editor/driver/driver_info.h"
#include "runtime/emulation/cpumemory.h"

#include "runtime/editor/auxilarydata/auxilary_data_collection.h"
#include "runtime/editor/auxilarydata/auxilary_data_songs.h"

#include "foundation/base/assert.h"

namespace Editor
{
	DriverArchitectureSidFactory2::DriverArchitectureSidFactory2()
		: IDriverArchitecture( &od, &sd )
	{}

	void DriverArchitectureSidFactory2::SetEmptySequence( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress )
	{
		FOUNDATION_ASSERT( inCPUMemory != nullptr );
		FOUNDATION_ASSERT( inCPUMemory->IsLocked() );

		unsigned char empty_sequence_data[] = { 0x80, 0x00, 0x7f };
		inCPUMemory->SetData( inAddress, empty_sequence_data, sizeof( empty_sequence_data ) );
	}


	void DriverArchitectureSidFactory2::SetEmptyOrderList( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress )
	{
		FOUNDATION_ASSERT( inCPUMemory != nullptr );
		FOUNDATION_ASSERT( inCPUMemory->IsLocked() );

		unsigned char empty_orderlist_data[] = { 0xa0, 0x00, 0xff };
		inCPUMemory->SetData( inAddress, empty_orderlist_data, sizeof( empty_orderlist_data ) );
	}

	void DriverArchitectureSidFactory2::PostInitSetPlaybackIndices( std::vector< PlayMarkerInfo >& inPlayMarkerInfoList,
																	Emulation::CPUMemory*		   inCPUMemory,
																	const DriverInfo&			   inDriverInfo )
	{
		const auto&			driver_common = inDriverInfo.GetDriverCommon();
		const auto&			music_data	  = inDriverInfo.GetMusicData();

		const unsigned char track_count	  = music_data.m_TrackCount;

		if ( track_count == inPlayMarkerInfoList.size() ) {
			const unsigned short order_list_index_address		= driver_common.m_OrderListIndexAddress;
			const unsigned short sequence_index_address			= driver_common.m_SequenceIndexAddress;
			const unsigned short tick_counter_address			= driver_common.m_TickCounterAddress;
			const unsigned short current_sequence_address		= driver_common.m_CurrentSequenceAddress;
			const unsigned short current_transpose_address		= driver_common.m_CurrentTransposeAddress;
			const unsigned short current_event_duration_address = driver_common.m_CurrentSequenceEventDurationAddress;
			const unsigned short next_instrument_address		= driver_common.m_NextInstrumentAddress;

			for ( int i = 0; i < music_data.m_TrackCount; ++i ) {
				( *inCPUMemory )[ order_list_index_address + i ]  = inPlayMarkerInfoList[ i ].m_OrderListIndex;
				( *inCPUMemory )[ sequence_index_address + i ]	  = inPlayMarkerInfoList[ i ].m_SequenceIndex;
				( *inCPUMemory )[ tick_counter_address + i ]	  = inPlayMarkerInfoList[ i ].m_TickCounters;
				( *inCPUMemory )[ current_sequence_address + i ]  = inPlayMarkerInfoList[ i ].m_CurrentSequence;
				( *inCPUMemory )[ current_transpose_address + i ] = inPlayMarkerInfoList[ i ].m_CurrentTranspose;
				( *inCPUMemory )[ current_event_duration_address + i ] =
					inPlayMarkerInfoList[ i ].m_CurrentSequenceEventDuration;
				( *inCPUMemory )[ driver_common.m_SequenceInUseAddress + i ] =
					inPlayMarkerInfoList[ i ].m_SequenceInUse ? 1 : 0;

				if ( inPlayMarkerInfoList[ i ].m_NextInstrumentAddress >= 0x80 )
					( *inCPUMemory )[ next_instrument_address + i ] = inPlayMarkerInfoList[ i ].m_NextInstrumentAddress;
			}
		}
	}

	unsigned char DriverArchitectureSidFactory2::GetHighestSequenceIndexUsed(
		const Editor::DriverInfo&				  inDriverInfo,
		const Emulation::IMemoryRandomReadAccess& inMemoryReader ) const noexcept
	{
		unsigned char highest_sequence_index = 0;

		if ( inDriverInfo.HasParsedHeaderBlock( DriverInfo::HeaderBlockID::ID_MusicData ) ) {
			// Get song count
			const unsigned char song_count = inDriverInfo.GetAuxilaryDataCollection().GetSongs().GetSongCount();

			// Get Music data descriptor
			const DriverInfo::MusicData& music_data	  = inDriverInfo.GetMusicData();

			// Find highest used sequence index
			unsigned short				 order_list_1 = music_data.m_OrderListTrack1Address;

			for ( unsigned char i = 0; i < music_data.m_TrackCount * song_count; ++i ) {
				unsigned short order_list_address =
					order_list_1 + static_cast< unsigned short >( i ) * music_data.m_OrderListSize;
				for ( unsigned short j = 0; j < music_data.m_OrderListSize; ++j ) {
					unsigned char value = inMemoryReader[ order_list_address + j ];
					if ( value < 0x80 ) {
						if ( value > highest_sequence_index ) highest_sequence_index = value;
					} else {
						if ( value == 0xff ) break;
					}
				}
			}
		}

		return highest_sequence_index;
	}


	std::vector< int > DriverArchitectureSidFactory2::GetSequenceUsageCount(
		const Editor::DriverInfo&				  inDriverInfo,
		const Emulation::IMemoryRandomReadAccess& inMemoryReader ) const noexcept
	{
		if ( inDriverInfo.HasParsedHeaderBlock( DriverInfo::HeaderBlockID::ID_MusicData ) ) {
			// Output collection
			const int		   SequenceCount = static_cast< int >( inDriverInfo.GetMusicData().m_SequenceCount );
			std::vector< int > usage_count( SequenceCount, 0 );

			// Get Music data descriptor
			const DriverInfo::MusicData& music_data	  = inDriverInfo.GetMusicData();

			// Find highest used sequence index
			unsigned short				 order_list_1 = music_data.m_OrderListTrack1Address;

			// Get song count
			const unsigned char song_count = inDriverInfo.GetAuxilaryDataCollection().GetSongs().GetSongCount();

			for ( unsigned char i = 0; i < music_data.m_TrackCount * song_count; ++i ) {
				unsigned short order_list_address =
					order_list_1 + static_cast< unsigned short >( i ) * music_data.m_OrderListSize;
				for ( unsigned short j = 0; j < music_data.m_OrderListSize; ++j ) {
					unsigned char value = inMemoryReader[ order_list_address + j ];

					if ( value < SequenceCount ) usage_count[ value ]++;
					else if ( value >= 0xfe ) break;
				}
			}
			return usage_count;
		}
		return std::vector< int >();
	}


	unsigned char DriverArchitectureSidFactory2::GetFirstUnusedSequenceIndex(
		const Editor::DriverInfo&				  inDriverInfo,
		const Emulation::IMemoryRandomReadAccess& inMemoryReader ) const noexcept
	{
		std::vector< int > sequence_usage_count = GetSequenceUsageCount( inDriverInfo, inMemoryReader );

		for ( size_t i = 0; i < sequence_usage_count.size(); ++i ) {
			if ( sequence_usage_count[ i ] == 0 ) return static_cast< unsigned char >( i );
		}
		return 0xff;
	}

	unsigned char DriverArchitectureSidFactory2::GetFirstEmptySequenceIndex(
		const Editor::DriverInfo&				  inDriverInfo,
		const Emulation::IMemoryRandomReadAccess& inMemoryReader ) const noexcept
	{
		if ( inDriverInfo.HasParsedHeaderBlock( DriverInfo::HeaderBlockID::ID_MusicData ) ) {
			const DriverInfo::MusicData& music_data			  = inDriverInfo.GetMusicData();
			const std::vector< int >	 sequence_usage_count = GetSequenceUsageCount( inDriverInfo, inMemoryReader );

			for ( unsigned short i = 0; i < static_cast< unsigned short >( sequence_usage_count.size() ); ++i ) {
				const unsigned short sequence_address = music_data.m_Sequence00Address + i * music_data.m_SequenceSize;

				const bool			 is_empty		  = inMemoryReader[ sequence_address ] == 0x80
														&& inMemoryReader[ sequence_address + 1 ] == 0x00
														&& inMemoryReader[ sequence_address + 2 ] == 0x7f;

				if ( is_empty && sequence_usage_count[ i ] == 0 ) return static_cast< unsigned char >( i );
			}
		}
		return 0xff;
	}


	unsigned char DriverArchitectureSidFactory2::GetHighestInstrumentIndexUsed(
		const Editor::DriverInfo&				  inDriverInfo,
		const Emulation::IMemoryRandomReadAccess& inMemoryReader ) const noexcept
	{
		unsigned char highest_instrument_index = 0;

		if ( inDriverInfo.HasParsedHeaderBlock( DriverInfo::HeaderBlockID::ID_MusicData ) ) {
			// Get Music data descriptor
			const DriverInfo::MusicData& music_data = inDriverInfo.GetMusicData();

			// Find highest used sequence index
			unsigned short				 sequence_0 = music_data.m_Sequence00Address;

			for ( unsigned char i = 0; i < music_data.m_SequenceCount; ++i ) {
				unsigned short order_list_address =
					sequence_0 + static_cast< unsigned short >( i ) * music_data.m_SequenceSize;

				for ( unsigned short j = 0; j < music_data.m_SequenceSize; ++j ) {
					const unsigned char value = inMemoryReader[ order_list_address + j ];
					if ( value >= 0x80 ) {
						if ( value >= 0xa0 && value < 0xc0 ) {
							const unsigned char instrument_index = value & 0x1f;
							if ( instrument_index > highest_instrument_index )
								highest_instrument_index = instrument_index;
						}
					} else {
						if ( value == 0x7f ) break;
					}
				}
			}
		}
		return highest_instrument_index;
	}


	unsigned char DriverArchitectureSidFactory2::GetHighestCommandIndexUsed(
		const Editor::DriverInfo&				  inDriverInfo,
		const Emulation::IMemoryRandomReadAccess& inMemoryReader ) const noexcept
	{
		unsigned char highest_command_index = 0;

		if ( inDriverInfo.HasParsedHeaderBlock( DriverInfo::HeaderBlockID::ID_MusicData ) ) {
			// Get Music data descriptor
			const DriverInfo::MusicData& music_data = inDriverInfo.GetMusicData();

			// Find highest used sequence index
			unsigned short				 sequence_0 = music_data.m_Sequence00Address;

			for ( unsigned char i = 0; i < music_data.m_SequenceCount; ++i ) {
				unsigned short order_list_address =
					sequence_0 + static_cast< unsigned short >( i ) * music_data.m_SequenceSize;

				for ( unsigned short j = 0; j < music_data.m_SequenceSize; ++j ) {
					const unsigned char value = inMemoryReader[ order_list_address + j ];
					if ( value >= 0x80 ) {
						if ( value >= 0xc0 ) {
							const unsigned char command_index = value & 0x3f;
							if ( command_index > highest_command_index ) highest_command_index = command_index;
						}
					} else {
						if ( value == 0x7f ) break;
					}
				}
			}
		}
		return highest_command_index;
	}


	std::vector< unsigned short > DriverArchitectureSidFactory2::GetOrderListsLength(
		const Editor::DriverInfo&				  inDriverInfo,
		const Emulation::IMemoryRandomReadAccess& inMemoryReader ) const noexcept
	{
		std::vector< unsigned short > order_list_length_list;

		if ( inDriverInfo.HasParsedHeaderBlock( DriverInfo::HeaderBlockID::ID_MusicData ) ) {
			// Get song count
			const unsigned char song_count = inDriverInfo.GetAuxilaryDataCollection().GetSongs().GetSongCount();

			// Get Music data descriptor
			const DriverInfo::MusicData& music_data	  = inDriverInfo.GetMusicData();

			// Find highest used sequence index
			unsigned short				 order_list_1 = music_data.m_OrderListTrack1Address;

			for ( unsigned char i = 0; i < music_data.m_TrackCount * song_count; ++i ) {
				unsigned short order_list_address =
					order_list_1 + static_cast< unsigned short >( i ) * music_data.m_OrderListSize;
				for ( int j = 0; j < music_data.m_OrderListSize; ++j ) {
					unsigned char value = inMemoryReader[ order_list_address + j ];
					if ( value == 0xff ) {
						// +2, because the value after $ff is used as the loop index of the order list!
						order_list_length_list.push_back( static_cast< unsigned short >( j ) + 2 );
						break;
					} else if ( value == 0xfe ) {
						order_list_length_list.push_back( static_cast< unsigned short >( j ) + 1 );
						break;
					}
				}
			}
		}

		return order_list_length_list;
	}


	unsigned short DriverArchitectureSidFactory2::GetSequenceLength(
		uint8_t									  inSequenceIndex,
		const Editor::DriverInfo&				  inDriverInfo,
		const Emulation::IMemoryRandomReadAccess& inMemoryReader ) const noexcept
	{
		FOUNDATION_ASSERT( inSequenceIndex < 0x7f );

		const unsigned short sequence_0_address = inDriverInfo.GetMusicData().m_Sequence00Address;
		const unsigned short sequence_address =
			sequence_0_address + inSequenceIndex * inDriverInfo.GetMusicData().m_SequenceSize;

		for ( unsigned short i = 0; i < inDriverInfo.GetMusicData().m_SequenceSize; ++i ) {
			if ( inMemoryReader[ sequence_address + static_cast< unsigned short >( i ) ] == 0x7f ) return i + 1;
		}

		return 0;
	}


	unsigned short DriverArchitectureSidFactory2::GetEndOfMusicDataAddress(
		const Editor::DriverInfo&				  inDriverInfo,
		const Emulation::IMemoryRandomReadAccess& inMemoryReader ) const noexcept
	{
		const unsigned char	 highest_sequence_index = GetHighestSequenceIndexUsed( inDriverInfo, inMemoryReader );

		// Get Music data descriptor
		const unsigned short sequence_data_address =
			inDriverInfo.GetMusicData().m_Sequence00Address
			+ ( inDriverInfo.GetMusicData().m_SequenceSize * ( highest_sequence_index + 1 ) );

		return sequence_data_address;
	}

} // namespace Editor
