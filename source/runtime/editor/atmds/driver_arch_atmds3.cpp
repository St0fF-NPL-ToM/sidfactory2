#include "driver_arch_atmds3.h"

#include "runtime/editor/driver/driver_info.h"
#include "runtime/emulation/cpumemory.h"

#include "foundation/base/assert.h"

#include <algorithm>
#include <set>

namespace Editor
{
#define SN SentinelType::NoSentinel

	ATMDS3TrackEvents::ATMDS3TrackEvents()
		: IEventDefinition()
	{
		m_Param.m_Value						= { -0x20, 0x1f, 0xa0, 0x00 };
		m_Param.m_Sentinels.m_Count			= 4;
		m_Param.m_Sentinels.m_Types[ 0 ]	= SentinelType::Empty;
		m_Param.m_Sentinels.m_Types[ 1 ]	= SentinelType::Loop;
		m_Param.m_Sentinels.m_Types[ 2 ]	= SentinelType::Stop;
		m_Param.m_Sentinels.m_Types[ 3 ]	= SentinelType::Start;
		m_Param.m_Sentinels.m_Values[ 0 ]	= 0x00;
		m_Param.m_Sentinels.m_Values[ 1 ]	= 0xfe;
		m_Param.m_Sentinels.m_Values[ 2 ]	= 0xff;
		m_Param.m_Sentinels.m_Values[ 3 ]	= 0xf0;
		m_Command.m_Value					= { 0, 0x3f, 0, 0x7f };
		m_Command.m_Sentinels.m_Count		= 1;
		m_Command.m_Sentinels.m_Types[ 0 ]	= SentinelType::Empty;
		m_Command.m_Sentinels.m_Values[ 0 ] = 0x7f;
		m_Payload.m_Value					= { 0x00, 0x5f, 0x00, 0x00 };
		m_Payload.m_Sentinels.m_Count		= 0;
		using T								= std::pair< const unsigned char, std::string >;
		m_SentinelDisplayFormat = { T{ 0x7f, "...." }, T{ 0xfe, "loop" }, T{ 0xff, "stop" }, T{ 0xf0, "SNGx" } };
	}

	EventsPackResult ATMDS3TrackEvents::Pack( const IDataSourceEvents& inData, uint8_t* od ) const noexcept
	{
		int				   out{}, trp{};
		std::vector< int > rowStarts( 1, out );

		for ( int index = 0; index < inData.GetLength(); ++index ) {
			// Finde Kommando, falls eines Referenziert wurde:
			if ( inData.HasCommand( index ) ) {
				const auto cid = inData.GetCommand( index );
				const auto cmd = // unconventional callback into the architecture class
					static_cast< DriverArchitectureATMDS3* >( inData.GetDriverInfo().GetDriverArchitecture() )
						->m_OrderCommands[ cid ];
				if ( uint8_t( cmd.m_Uses ) & uint8_t( ATMDS3_CMD::uses_V ) ) od[ out++ ] = ( cmd.m_VOL & 0x0f ) | 0xe0;
				if ( uint8_t( cmd.m_Uses ) & uint8_t( ATMDS3_CMD::uses_S ) ) od[ out++ ] = ( cmd.m_SPD & 0x1f ) | 0x60;
			}
			// Untersuchung nach Steuerung vs. Transposition
			if ( inData.HasParameter( index ) ) {
				const auto p = inData.GetParameter( index );
				if ( trp != p ) {
					trp			= p;
					od[ out++ ] = ( p >= 0 ? p | 0xa0 : 0x80 - p - 1 ); // 2er-Komplement bei negativen Werten!
				}
				// Da es keinen Sentinel als Parameter gab, gibt es eine Sequenz
				od[ out++ ] = inData.GetPayload( index );
				rowStarts.push_back( out );
			} else {
				switch ( inData.GetSentinel( index, Semantic::Parameter ) ) {
					case SentinelType::Loop:
						od[ out++ ] = 0xfe;
						od[ out++ ] = rowStarts[ inData.GetPayload( index ) ];
						break;
					case SentinelType::Stop: od[ out++ ] = 0xff; break;
					case SentinelType::Start: od[ out++ ] = 0xf0 | inData.GetPayload( index ); break;
					default: // SentinelType::Empty
						od[ out++ ] = inData.GetPayload( index );
						rowStarts.push_back( out );
						break;
				}
			}
		}
		return { od, out };
	}
	EventsUnpackResult ATMDS3TrackEvents::Unpack( IDataSourceEvents& o, uint8_t* str ) const noexcept
	{
		// Lies den Eingabestrom eines Tracks … ACHTUNG: Repeat nicht vergessen!
		int					 index = 0, repeat = 0, pos = 0;
		std::map< int, int > rowStarts( { 0, 0 } );

		while ( true ) {
			// Beginne Row Element:
			auto&	evt = o.ClearEvent( index );
			uint8_t C	= str[ pos++ ];
			if ( C == 0xff ) { // STOP
				o.SetSentinel( index, SentinelType::Stop, Semantic::Parameter );
				break;
			} else if ( C == 0xfe ) {
				o.SetSentinel( index, SentinelType::Loop, Semantic::Parameter );
				o.SetValue( rowStarts[ str[ pos++ ] ], index, Semantic::Payload );
				break;
			} else if ( C >= 0xf0 && C < 0xf4 ) {
				o.SetSentinel( index, SentinelType::Start, Semantic::Parameter );
				o.SetValue( C & 3, index, Semantic::Payload );
				break;
			}
		}
	}
	EventsPackedRowPos ATMDS3TrackEvents::GetNextIndexPackedPosition( int inIndex, uint8_t* m_Data,
													   uint16_t m_PackedSize ) const noexcept
	{
		int			  count		= 0;
		unsigned char transpose = 0;
		unsigned char sequence	= 0;

		for ( int i = 0; i < static_cast< int >( m_PackedSize ); ++i ) {
			if ( m_Data[ i ] < 0x60 ) {
				sequence = m_Data[ i ];

				++count;
				if ( count == inIndex + 1 ) {
					const unsigned char next_event = m_Data[ i + 1 ];
					const int return_index = next_event == 0xff
												 ? m_Data[ i + 2 ]
												 : ( i + 1 ); // If the next event is a loop mark, the next next event
															  // is the loop offset into the orderlist data.

					return { true, false, static_cast< unsigned char >( return_index ), sequence, transpose };
				}
			} else {
				int transpose_value = static_cast< int >( ( m_Data[ i ] & 0x7f ) - 0x20 );
				transpose			= static_cast< unsigned int >( transpose_value & 0xff );
			}
		}
		return { false, false, 0, 0, 0 };
	}


	void DriverArchitectureATMDS3::SetEmptySequence( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress )
	{
		FOUNDATION_ASSERT( inCPUMemory != nullptr );
		FOUNDATION_ASSERT( inCPUMemory->IsLocked() );

		unsigned char empty_sequence_data[] = { 0x90, 0xfe, 0xff }; // dur=10,w8,end
		inCPUMemory->SetData( inAddress, empty_sequence_data, sizeof( empty_sequence_data ) );
	}


	void DriverArchitectureATMDS3::SetEmptyOrderList( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress )
	{
		FOUNDATION_ASSERT( inCPUMemory != nullptr );
		FOUNDATION_ASSERT( inCPUMemory->IsLocked() );

		unsigned char empty_orderlist_data[] = { 0, 0xfe, 0 }; // as the others do it: empty seq, loop it.
		inCPUMemory->SetData( inAddress, empty_orderlist_data, sizeof( empty_orderlist_data ) );
	}

	void DriverArchitectureATMDS3::PostInitSetPlaybackIndices( std::vector< PlayMarkerInfo >& inPlayMarkerInfoList,
															   Emulation::CPUMemory*		  inCPUMemory,
															   const DriverInfo&			  inDriverInfo )
	{
		if ( m_CreatedCommands.empty() ) GenerateCommandTables( inCPUMemory, inDriverInfo );

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

			for ( int i = 0; i < 7 * music_data.m_TrackCount; i += 7 ) {
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

	void DriverArchitectureATMDS3::GenerateCommandTables( Emulation::CPUMemory* inCPUMemory,
														  const DriverInfo&		inDriverInfo )
	{
		std::set< unsigned char > sectors_parsed;
		char					  tmp[ 256 ];
		// parse all songs, all tracks, all used sequences …
		for ( int song = 0; song < 4; song++ ) {
			for ( int track = 0; track < 3; track++ ) {
				// use the song track data pointer to retrieve the track address
				const auto tp = inCPUMemory->GetWord( 0x4040 + song * 8 + track * 2 );

				// then get the track into easily accessible local mem
				inCPUMemory->GetData( tp, tmp, 256 );

				// parse one track only regarding commands!
				int	 rowCmd = -1;
				auto addRC	= [ & ]( ATMDS_cmd::cmd_use cmd, unsigned char val )
				{
					if ( rowCmd < 0 ) rowCmd = m_CreatedCommands.size(), m_CreatedCommands.emplace_back( cmd, val );
					else m_CreatedCommands.back().set( cmd, val );
				};
				for ( int pos = 0; pos < 256; pos++ ) {
					// SPD command?
					if ( ( tmp[ pos ] > 0x60u ) && ( tmp[ pos ] < 0x80u ) )
						addRC( ATMDS_cmd::uses_S, tmp[ pos++ ] & 0x1f );
					// VOL command?
					if ( ( tmp[ pos ] > 0xe0u ) && ( tmp[ pos ] < 0xf0u ) )
						addRC( ATMDS_cmd::uses_V, tmp[ pos++ ] & 0x0f );
					// Sector → reset row command
					if ( tmp[ pos ] < 0x60u ) {
						if ( sectors_parsed.count( tmp[ pos ] ) == 0 ) {
							unsigned char ts[ 256 ]; // acquire the sector
							inCPUMemory->GetData( 0x5400 + 0x100 * tmp[ pos ], ts, 256 );
							// now, parse the sector
							for ( int sr{}, sp{}; sp < 256; sp++ ) {
								if ( ts[ sp ] < 0x60u || ts[ sp ] >= 0xfcu ) // row-end
								{
									sr++, rowCmd = -1;
									if ( ts[ sp ] == 0xffu ) break; // end marker
								}
								if ( ts[ sp ] < 0x80u ) // glide
								{
									addRC( ATMDS_cmd::uses_G, ts[ sp++ ] & 0x1f ); // glide table index
									addRC( ATMDS_cmd::none, ts[ sp++ ] );		   // target note
									// after the glide-fx, sp points to the respective note
								}
								if ( ts[ sp ] == 0xf6u ) addRC( ATMDS_cmd::uses_FP, ts[ ++sp ] );
								if ( ts[ sp ] == 0xf7u ) addRC( ATMDS_cmd::uses_PP, ts[ ++sp ] );
								if ( ts[ sp ] == 0xf8u ) addRC( ATMDS_cmd::uses_FF, ts[ ++sp ] );
								if ( ts[ sp ] == 0xf9u ) addRC( ATMDS_cmd::uses_SR, ts[ ++sp ] );
								if ( ts[ sp ] == 0xfau ) addRC( ATMDS_cmd::uses_AD, ts[ ++sp ] );
							}
							sectors_parsed.emplace( tmp[ pos ] );
						}
					}
					// Song/Jump/Stop
					if ( tmp[ pos ] >= 0xF0u ) break;
				}
			}
		}
		// Command Table sollte nun vollständig gebaut sein ... also übertragen.
		inCPUMemory->SetData( 0x4000 - 10 * 128, m_CreatedCommands.data(),
							  m_CreatedCommands.size() * sizeof( ATMDS_cmd ) );
	}

} // namespace Editor
