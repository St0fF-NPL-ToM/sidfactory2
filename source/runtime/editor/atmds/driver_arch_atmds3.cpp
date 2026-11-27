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
				if ( uint8_t( cmd.m_c0 ) & uint8_t( ATMDS3_CMD::uses_V ) ) od[ out++ ] = ( cmd.m_c0 & 0x0f ) | 0xe0;
				if ( uint8_t( cmd.m_c0 ) & uint8_t( ATMDS3_CMD::uses_S ) ) od[ out++ ] = ( cmd.m_c1 & 0x1f ) | 0x60;
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
				// Hier müssten jetzt die Repeat's eingepflegt werden - als einfaches RLE … allerdings machen
				// Repeats den Song unübersichtlich und sind deshalb in der Edit-Phase nicht wirklich nützlich.
				// Später vielleicht … Stichwort: Optimizer
			} else {
				const auto X = inData.GetPayload( index );
				switch ( inData.GetSentinel( index, Semantic::Parameter ) ) {
					case SentinelType::Loop:
						od[ out++ ] = 0xfe;
						od[ out++ ] = rowStarts[ X ];
						break;
					case SentinelType::Stop: od[ out++ ] = 0xff; break;
					case SentinelType::Start: od[ out++ ] = 0xf0 | X; break;
					default: // SentinelType::Empty
						od[ out++ ] = X;
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
		unsigned			 index{}, repeat{}, pos{};
		int					 vol{ -1 }, spd{ -1 }, trp{ 0 }, lc{ -1 };
		std::map< int, int > rowStarts( { 0, 0 } );
		bool				 trpC{ true }; // always tell SF2 to start with TRP #0

		while ( true ) {
			uint8_t C = str[ pos++ ];
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
			} else if ( C >= 0xe0 ) {
				vol = C & 0x0f;
			} else if ( C >= 0xc0 ) {
				repeat = C & 0x1f;
			} else if ( C >= 0x80 ) {
				const auto nt = C >= 0xa0 ? C & 0x1f : 0xa0 - C;
				trpC		  = nt != trp;
				trp			  = nt;
			} else if ( C >= 0x60 ) {
				spd = C & 0x1f;
			} else {								   // eine Sequenz - also ein Event …
				auto& evt	  = o.ClearEvent( index ); // sollte überall "Empty" Sentinels setzen …
				// Sequenz selbst
				evt.m_Payload = C;
				// Transpose-Änderung?
				if ( trpC ) o.SetValue( trp, index, Semantic::Parameter ), trpC = false;
				// Kommando zusammenbauen?
				if ( vol >= 0 || spd >= 0 )
					evt.m_Command = lc = // unconventional callback into the architecture class
						static_cast< DriverArchitectureATMDS3* >( o.GetDriverInfo().GetDriverArchitecture() )
							->GetTrackCommandIndex( vol, spd );
				++index;
				for ( int i = 0; i < repeat; ++i, ++index ) o[ index ] = evt;
				repeat = 0;
			}
		}
		return { index, pos, static_cast< uint8_t >( trp ), static_cast< uint8_t >( lc ), false };
	}

	EventsPackedRowPos ATMDS3TrackEvents::GetNextIndexPackedPosition( int inIndex, uint8_t* m_Data,
																	  uint16_t m_PackedSize ) const noexcept
	{
		for ( uint8_t pos{}, trp{}, seq{}, row{}, rep{}; pos < m_PackedSize; ) {
			// get "next" always means find "end of row"
			auto C = m_Data[ pos++ ];
			if ( C > 0x5f ) { // all commands are no sectors. But may be ends…
				if ( ( C >= 0x80 ) && ( C < 0xc0 ) ) {
					// Transpose command - I'm not sure to trust C++ uint8 math, though …
					trp = C - 0xa0 >= 0 ? C - 0xa0 : 0x80 - C;
				} else if ( ( C >= 0xc0 ) && ( C < 0xe0 ) ) {
					rep = C & 0x1f;		  // Repeat Command
				} else if ( C >= 0xf0 ) { // End Of Order markers …
					// End Song:
					if ( C == 0xfe ) pos = m_Data[ pos ]; // a loop simply loops on
					else break;
				}
				C = m_Data[ pos++ ];
			} else { // Sequence data: row ends here.
				seq = C;
				if ( row == inIndex || row + rep + 1 > inIndex )
					return { true, pos < m_PackedSize && m_Data[ pos ] >= 0xfe, pos, seq, trp };
				row += 1 + rep;
				rep = 0;
			}
		}
		return { false, false, 0, 0, 0 };
	}


	ATMDS3SectorEvents::ATMDS3SectorEvents()
		: IEventDefinition()
	{
		m_Param.m_Value = {0,0x1f,0xc0};
		m_Param.m_Sentinels.m_Count = 3;
		m_Param.m_Sentinels.m_Types[0] = SentinelType::Empty;
		m_Param.m_Sentinels.m_Types[1] = SentinelType::Legato;
		m_Param.m_Sentinels.m_Types[2] = SentinelType::Stop;
		m_Param.m_Sentinels.m_Values[0] = 0x80;
		m_Param.m_Sentinels.m_Values[1] = 0xfb;
		m_Param.m_Sentinels.m_Values[2] = 0xff;
		m_Command.m_Value = {0,0x3f,0};
		m_Command.m_Sentinels.m_Count = 1;
		m_Command.m_Sentinels.m_Types[0] = SentinelType::Empty;
		m_Command.m_Sentinels.m_Values[0] = 0xfe;
		m_Payload.m_Value = {0,0x5f,0};
		m_Payload.m_Sentinels.m_Count = 3;
		m_Payload.m_Sentinels.m_Types[0] = SentinelType::Empty;
		m_Payload.m_Sentinels.m_Types[1] = SentinelType::Gate;
		m_Payload.m_Sentinels.m_Types[2] = SentinelType::Mark;	// st0ff command
		m_Payload.m_Sentinels.m_Values[0] = 0xfe;
		m_Payload.m_Sentinels.m_Values[1] = 0xfd;
		m_Payload.m_Sentinels.m_Values[2] = 0xfc;	// st0ff command
		using T								= std::pair< const unsigned char, std::string >;
		m_SentinelDisplayFormat = { T{ 0x7f, "...." }, T{ 0xfe, "loop" }, T{ 0xff, "stop" }, T{ 0xf0, "SNGx" } };
	}
	EventsPackResult ATMDS3SectorEvents::Pack( const IDataSourceEvents& inData, uint8_t* ouTernalBuffer ) const noexcept
	{}
	EventsUnpackResult ATMDS3SectorEvents::Unpack( IDataSourceEvents& outData, uint8_t* inTernalBuffer ) const noexcept
	{}
	EventsPackedRowPos ATMDS3SectorEvents::GetNextIndexPackedPosition( int inIndex, uint8_t* inData,
																	   uint16_t inDataSize ) const noexcept
	{}
	bool ATMDS3SectorEvents::MayPushData( IDataSourceEvents&, const DriverInfo&, const DriverState& ) const noexcept {}


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
		if ( m_OrderCommands.empty() || m_SequenceCommands.empty() ) GenerateCommandTables( inCPUMemory, inDriverInfo );

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
				const auto oa = inDriverInfo.GetMusicData().m_OrderListTrack1Address;
				const auto tp = inCPUMemory->GetWord( oa + song * 8 + track * 2 );
				const auto sa = inDriverInfo.GetMusicData().m_Sequence00Address;

				// then get the track into easily accessible local mem
				inCPUMemory->GetData( tp, tmp, 256 );

				// parse one track only regarding commands!
				int	 rowCmd = -1;
				auto addRC	= [ & ]( ATMDS3_CMD cmd, unsigned char val, auto& tgt )
				{
					if ( rowCmd < 0 ) rowCmd = tgt.size(), tgt.emplace_back( cmd, val );
					else tgt[ rowCmd ].set( cmd, val );
				};
				for ( int pos = 0; pos < 256; pos++ ) {
					// SPD command?
					if ( ( tmp[ pos ] > 0x60u ) && ( tmp[ pos ] < 0x80u ) )
						addRC( ATMDS3_CMD::uses_S, tmp[ pos++ ] & 0x1f, m_OrderCommands );
					// VOL command?
					if ( ( tmp[ pos ] > 0xe0u ) && ( tmp[ pos ] < 0xf0u ) )
						addRC( ATMDS3_CMD::uses_V, tmp[ pos++ ] & 0x0f, m_OrderCommands );
					// Sector → reset row command
					if ( tmp[ pos ] < 0x60u ) {
						rowCmd = -1;
						if ( sectors_parsed.count( tmp[ pos ] ) == 0 ) {
							unsigned char ts[ 256 ]; // acquire the sector
							inCPUMemory->GetData( sa + 0x100 * tmp[ pos ], ts, 256 );
							// now, parse the sector
							for ( int sr{}, sp{}; sp < 256; sp++ ) {
								if ( ts[ sp ] < 0x60u || ts[ sp ] >= 0xfcu ) // row-end
								{
									sr++, rowCmd = -1;
									if ( ts[ sp ] == 0xffu ) break; // end marker
								}
								if ( ts[ sp ] < 0x80u ) { // glide table index and target note
									addRC( ATMDS3_CMD::uses_G, ts[ sp++ ] & 0x1f, m_SequenceCommands );
									addRC( ATMDS3_CMD::none, ts[ sp++ ], m_SequenceCommands );
									// after the glide-fx, sp points to the respective note
								}
								if ( ts[ sp ] == 0xf6u ) addRC( ATMDS3_CMD::uses_FP, ts[ ++sp ], m_SequenceCommands );
								if ( ts[ sp ] == 0xf7u ) addRC( ATMDS3_CMD::uses_PP, ts[ ++sp ], m_SequenceCommands );
								if ( ts[ sp ] == 0xf8u ) addRC( ATMDS3_CMD::uses_FF, ts[ ++sp ], m_SequenceCommands );
								if ( ts[ sp ] == 0xf9u ) addRC( ATMDS3_CMD::uses_SR, ts[ ++sp ], m_SequenceCommands );
								if ( ts[ sp ] == 0xfau ) addRC( ATMDS3_CMD::uses_AD, ts[ ++sp ], m_SequenceCommands );
								// all other commands are single byte, thus we can ignore them.
							}
							sectors_parsed.emplace( tmp[ pos ] );
						}
					}
					// Song/Jump/Stop
					if ( tmp[ pos ] >= 0xF0u ) break;
				}
			}
		}
		// Command Tables sollten nun vollständig gebaut sein ... also übertragen.
		const auto& td = inDriverInfo.GetTableDefinitions();
		for ( const auto& t: td )
			if ( t.m_ID == static_cast< uint8_t >( ATMDS3::TableID::TRACK_CMD ) )
				inCPUMemory->SetData( t.m_Address, m_OrderCommands.data(),
									  m_OrderCommands.size() * sizeof( ATMDS3TrackCmd ) );
			else if ( t.m_ID == static_cast< uint8_t >( ATMDS3::TableID::SECTOR_CMD ) )
				inCPUMemory->SetData( t.m_Address, m_SequenceCommands.data(),
									  m_SequenceCommands.size() * sizeof( ATMDS3SectorCmd ) );
	}

	uint8_t DriverArchitectureATMDS3::GetTrackCommandIndex( uint8_t volume, uint8_t speed ) const noexcept
	{
		ATMDS3TrackCmd cmd;
		if ( volume != 0xff ) cmd.set( ATMDS3_CMD::uses_V, volume & 15 );
		if ( speed != 0xff ) cmd.set( ATMDS3_CMD::uses_S, speed );
		const uint16_t v = cmd.m_c0 | ( cmd.m_c1 << 8 );
		for ( uint8_t res = 0; res < m_OrderCommands.size(); ++res )
			if ( auto c = m_OrderCommands[ res ]; v == ( c.m_c0 | ( c.m_c1 << 8 ) ) ) return res;

		return m_OrderCommands.size(); // should never reach here!
	}

	uint8_t DriverArchitectureATMDS3::GetSectorCommandIndex( const ATMDS3SectorCmd& cmd ) const noexcept
	{
		for ( uint8_t res = 0; res < m_SequenceCommands.size(); ++res )
			if ( auto* c = &m_SequenceCommands[ res ]; memcmp( c, &cmd, sizeof( ATMDS3SectorCmd ) ) == 0 ) return res;

		return m_SequenceCommands.size(); // should never reach here!
	}

} // namespace Editor
