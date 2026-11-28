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

	uint8_t ATMDS3TrackEvents::GetTrackCommandIndex( uint8_t volume, uint8_t speed ) const noexcept
	{
		ATMDS3TrackCmd cmd;
		if ( volume != 0xff ) cmd.set( ATMDS3_CMD::uses_V, volume & 15 );
		if ( speed != 0xff ) cmd.set( ATMDS3_CMD::uses_S, speed );
		const uint16_t v = cmd.m_c0 | ( cmd.m_c1 << 8 );
		for ( uint8_t res = 0; res < m_OrderCommands.size(); ++res )
			if ( auto c = m_OrderCommands[ res ]; v == ( c.m_c0 | ( c.m_c1 << 8 ) ) ) return res;

		return m_OrderCommands.size(); // should never reach here!
	}

	EventsPackResult ATMDS3TrackEvents::Pack( const IDataSourceEvents& inData, uint8_t* od ) const noexcept
	{
		int				   out{}, trp{};
		std::vector< int > rowStarts( 1, out );

		for ( int index = 0; index < inData.GetLength(); ++index ) {
			// Finde Kommando, falls eines Referenziert wurde:
			if ( inData.HasCommand( index ) ) {
				const auto cid = inData.GetCommand( index );
				const auto cmd = m_OrderCommands[ cid ];
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
		std::map< int, int > rowStarts;
		bool				 trpC{ true }; // always tell SF2 to start with TRP #0
		rowStarts[ 0 ] = 0;

		while ( pos < MAX_ORDER_PACKED_SIZE ) {
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
				if ( vol >= 0 || spd >= 0 ) evt.m_Command = lc = GetTrackCommandIndex( vol, spd );
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
		m_Param.m_Value						= { 0, 0x1f, 0xc0 };
		m_Param.m_Sentinels.m_Count			= 3;
		m_Param.m_Sentinels.m_Types[ 0 ]	= SentinelType::Empty;
		m_Param.m_Sentinels.m_Types[ 1 ]	= SentinelType::Legato;
		m_Param.m_Sentinels.m_Types[ 2 ]	= SentinelType::Stop;
		m_Param.m_Sentinels.m_Values[ 0 ]	= 0x80;
		m_Param.m_Sentinels.m_Values[ 1 ]	= 0xfb;
		m_Param.m_Sentinels.m_Values[ 2 ]	= 0xff;
		m_Command.m_Value					= { 0, 0x3f, 0 };
		m_Command.m_Sentinels.m_Count		= 1;
		m_Command.m_Sentinels.m_Types[ 0 ]	= SentinelType::Empty;
		m_Command.m_Sentinels.m_Values[ 0 ] = 0xfe;
		m_Payload.m_Value					= { 0, 0x5f, 0 };
		m_Payload.m_Sentinels.m_Count		= 3;
		m_Payload.m_Sentinels.m_Types[ 0 ]	= SentinelType::Empty;
		m_Payload.m_Sentinels.m_Types[ 1 ]	= SentinelType::Gate;
		m_Payload.m_Sentinels.m_Types[ 2 ]	= SentinelType::Mark; // st0ff command
		m_Payload.m_Sentinels.m_Values[ 0 ] = 0xfe;
		m_Payload.m_Sentinels.m_Values[ 1 ] = 0xfd;
		m_Payload.m_Sentinels.m_Values[ 2 ] = 0xfc; // st0ff command
		using T								= std::pair< const unsigned char, std::string >;
		m_SentinelDisplayFormat = { T{ 0x7f, "...." }, T{ 0xfe, "loop" }, T{ 0xff, "stop" }, T{ 0xf0, "SNGx" } };
	}

	uint8_t ATMDS3SectorEvents::GetSectorCommandIndex( const ATMDS3SectorCmd& cmd ) const noexcept
	{
		for ( uint8_t res = 0; res < m_SequenceCommands.size(); ++res )
			if ( auto* c = &m_SequenceCommands[ res ]; memcmp( c, &cmd, sizeof( ATMDS3SectorCmd ) ) == 0 ) return res;

		return m_SequenceCommands.size(); // should never reach here!
	}

	EventsPackResult ATMDS3SectorEvents::Pack( const IDataSourceEvents& inData, uint8_t* ouTernalBuffer ) const noexcept
	{
		uint8_t	 dur{ 0xff }, ins{ 0xff }, ldur{ dur }, lins{ ins };
		bool	 legato_on{ false }, llegato_on{ false };
		uint8_t* tgt = ouTernalBuffer;

		for ( int i{}; i < inData.GetLength(); ++i ) {
			// Build an ATMDS3-Sector row from a SF2 editor row description
			const auto is = inData.GetSentinel( i, Semantic::Parameter );
			if ( is == SentinelType::Stop ) *( ++tgt ) = 0xff;
			else {
				// → find out duration of this row
				const auto evt = inData.GetSentinel( i, Semantic::Payload );
				if ( inData.GetLength() - i < 2 ) dur = 0; // catch the case where we cannot check further
				else {
					bool more = true;
					int	 j{};
					while ( more && j < 0x40 ) {
						// cases: - this is a note and the following is a wait
						//		  - this is some Sentinel and the following is the same
						auto nevt = inData.GetSentinel( i + ++j, Semantic::Payload );
						more	  = ( evt == SentinelType::NoSentinel && nevt == SentinelType::Empty )
									|| ( evt == nevt && evt != SentinelType::NoSentinel );
						more &= inData.GetSentinel( i + j, Semantic::Command ) == SentinelType::Empty;
					}
					dur = j - 1;
				}
				if ( dur != ldur ) *( tgt++ ) = 0x80 | dur; // store duration change

				// Check instrument column
				if ( is == SentinelType::NoSentinel ) { // instrument set
					ins = ( inData.GetValue( i, Semantic::Parameter ) & 0x1f );
					if ( ins != lins ) *( tgt++ ) = 0xc0 | ins, lins = ins;
				} else if ( is == SentinelType::Legato ) {
					if ( !llegato_on ) { // Legato is a system state in ATMDS and not a "modifier"
						*( tgt++ ) = 0xfb;
						legato_on  = true;
					}
				}
				// Check command column
				bool has_glide{ false };
				if ( inData.GetSentinel( i, Semantic::Command ) != SentinelType::Empty ) {
					const auto& cmd = m_SequenceCommands.at( inData.GetCommand( i ) );
					if ( cmd( ATMDS3_CMD::uses_AD ) ) {
						*( tgt++ ) = 0xfa;
						*( tgt++ ) = cmd.m_AD;
					}
					if ( cmd( ATMDS3_CMD::uses_SR ) ) {
						*( tgt++ ) = 0xf9;
						*( tgt++ ) = cmd.m_SR;
					}
					if ( cmd( ATMDS3_CMD::uses_FF ) ) {
						*( tgt++ ) = 0xf8;
						*( tgt++ ) = cmd.m_FF;
					}
					if ( cmd( ATMDS3_CMD::uses_PP ) ) {
						*( tgt++ ) = 0xf7;
						*( tgt++ ) = cmd.m_PP;
					}
					if ( cmd( ATMDS3_CMD::uses_FP ) ) {
						*( tgt++ ) = 0xf6;
						*( tgt++ ) = cmd.m_FP;
					}
					if ( cmd( ATMDS3_CMD::uses_G ) ) {
						has_glide  = true;
						*( tgt++ ) = 0x60 | cmd.m_G;
						*( tgt++ ) = cmd.m_T;
					}
				}
				// Finally, our payload
				if ( evt == SentinelType::NoSentinel ) {
					// we do play a note - special Handling for Legato is required!
					const auto pd = inData.GetPayload( i );
					// As legato is an osc state and not an event modifier, it needs
					// to get reset in case it was enabled and is not used, now.
					if ( !has_glide && llegato_on != legato_on ) {
						*( tgt++ ) = 0xfb;
						llegato_on = legato_on;
						legato_on  = false;
					}
					*( tgt++ ) = pd; // finally, write the note, concluding this row-set
				} else {
					*( tgt++ ) = inData.GetAllSentinels( Semantic::Payload ).GetSentinel( evt );
				}
			}
		}
		return { ouTernalBuffer, int( tgt - ouTernalBuffer ) };
	}
	EventsUnpackResult ATMDS3SectorEvents::Unpack( IDataSourceEvents& outData, uint8_t* inTernalBuffer ) const noexcept
	{
		unsigned		dur{}, row{};
		uint8_t *		src{ inTernalBuffer }, D{}, lcs{ 0xff }, lis{ 0xff };
		bool			rowDone{ false }, leg{ false }, cmd{ false }, hasNote{ false };
		ATMDS3SectorCmd tmpCmd;

		outData.ClearEvent( row );
		do {
			D = *( src++ );
			if ( D < 0xff ) {
				if ( D > 0xfb ) { // pause …
					static const SentinelType dings[] = { SentinelType::Mark, SentinelType::Gate, SentinelType::Empty };
					outData.SetSentinel( row, dings[ D - 0xfc ], Semantic::Payload );
					// this marks the end of the row
					rowDone = true;
				} else if ( D == 0xfb ) {
					leg = !leg;
				} else if ( D > 0xf0 || ( ( D & 0xe0 ) == 0x60 ) ) { // any command
					cmd = true;
					if ( D < 0x80 ) {
						tmpCmd.set( ATMDS3_CMD::uses_G, D & 0x1f );
						tmpCmd.set( ATMDS3_CMD::none, *( src++ ) );
					} else if ( D == 0xf6 ) {
						tmpCmd.set( ATMDS3_CMD::uses_FP, *( src++ ) );
					} else if ( D == 0xf7 ) {
						tmpCmd.set( ATMDS3_CMD::uses_PP, *( src++ ) );
					} else if ( D == 0xf8 ) {
						tmpCmd.set( ATMDS3_CMD::uses_FF, *( src++ ) );
					} else if ( D == 0xf9 ) {
						tmpCmd.set( ATMDS3_CMD::uses_SR, *( src++ ) );
					} else if ( D == 0xfa ) {
						tmpCmd.set( ATMDS3_CMD::uses_AD, *( src++ ) );
					}
				} else if ( D >= 0xc0 ) {
					outData.SetValue( lis = ( D & 0x1f ), row, Semantic::Parameter );
				} else if ( D >= 0x80 ) dur = D & 0x3f;
				else if ( D < 0x60 ) {
					outData.SetValue( D, row, Semantic::Payload );
					rowDone = hasNote = true;
				}
				if ( rowDone ) {
					if ( cmd ) { // if there were commands - add unified item
						lis = GetSectorCommandIndex( tmpCmd );
						outData.SetValue( lis, row, Semantic::Command );
						tmpCmd.clear();
						cmd = false;
					}
					if ( leg && hasNote ) outData.SetSentinel( row, SentinelType::Legato, Semantic::Parameter );
					for ( int n{}; n <= dur; ++n ) outData.ClearEvent( ++row );
					hasNote = rowDone = false;
				}
			}
		} while ( *src != 0xff );
		outData.SetSentinel( row, SentinelType::Stop, Semantic::Parameter );
		return { row, unsigned( src - inTernalBuffer ), lis, lcs, false };
	}
	EventsPackedRowPos ATMDS3SectorEvents::GetNextIndexPackedPosition( int inIndex, uint8_t* inData,
																	   uint16_t inDataSize ) const noexcept
	{
		unsigned		row{};
		uint8_t *		src{ inData }, dur{}, ins{ 0xff }, cmd{ 0xff }, D{};
		bool			rowDone{ false }, hasNote{ false }, hasCmd{ false };
		ATMDS3SectorCmd tmpCmd;

		do {
			D = *( src++ );
			if ( D < 0xff ) { // END does not count, pauses mark the end of the row
				if ( D > 0xfb ) rowDone = true;
				else if ( D > 0xf0 || ( ( D & 0xe0 ) == 0x60 ) ) { // any command or legato
					if ( D != 0xfb ) {
						hasCmd = true; // we need to return the last command set …
						if ( D < 0x80 ) {
							tmpCmd.set( ATMDS3_CMD::uses_G, D & 0x1f );
							tmpCmd.set( ATMDS3_CMD::none, *( src++ ) );
						} else if ( D == 0xf6 ) {
							tmpCmd.set( ATMDS3_CMD::uses_FP, *( src++ ) );
						} else if ( D == 0xf7 ) {
							tmpCmd.set( ATMDS3_CMD::uses_PP, *( src++ ) );
						} else if ( D == 0xf8 ) {
							tmpCmd.set( ATMDS3_CMD::uses_FF, *( src++ ) );
						} else if ( D == 0xf9 ) {
							tmpCmd.set( ATMDS3_CMD::uses_SR, *( src++ ) );
						} else if ( D == 0xfa ) {
							tmpCmd.set( ATMDS3_CMD::uses_AD, *( src++ ) );
						}
					}
				} else if ( D >= 0xc0 ) ins = D & 0x1f;
				else if ( D >= 0x80 ) dur = D & 0x3f;
				else if ( D < 0x60 ) rowDone = hasNote = true;
				if ( rowDone ) {
					if ( hasCmd ) {
						cmd = GetSectorCommandIndex( tmpCmd );
						tmpCmd.clear();
						hasCmd = false;
					}
					++row;
					hasNote = rowDone = false;
					row += dur;
				}
			}
		} while ( ( row <= inIndex ) && ( *src != 0xff ) );
		return { row > inIndex, *src == 0xff, uint8_t( src - inData ), ins, cmd };
	}
	bool ATMDS3SectorEvents::MayPushData( IDataSourceEvents&, const DriverInfo&, const DriverState& ) const noexcept
	{ return true; } // TODO: this actually is pretty wrong and will destabilize the player.


	DriverArchitectureATMDS3::DriverArchitectureATMDS3()
		: IDriverArchitecture( &m_TrackEventDef, &m_SectorEventDef )
	{}

	void DriverArchitectureATMDS3::SetEmptySequence( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress )
	{
		FOUNDATION_ASSERT( inCPUMemory != nullptr );
		FOUNDATION_ASSERT( inCPUMemory->IsLocked() );

		unsigned char empty_sequence_data[] = { 0x8f, 0xfe, 0xff }; // dur=10,w8,end
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
		if ( m_TrackEventDef.m_OrderCommands.empty() || m_SectorEventDef.m_SequenceCommands.empty() )
			GenerateCommandTables( inCPUMemory, inDriverInfo );

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
						addRC( ATMDS3_CMD::uses_S, tmp[ pos++ ] & 0x1f, m_TrackEventDef.m_OrderCommands );
					// VOL command?
					if ( ( tmp[ pos ] > 0xe0u ) && ( tmp[ pos ] < 0xf0u ) )
						addRC( ATMDS3_CMD::uses_V, tmp[ pos++ ] & 0x0f, m_TrackEventDef.m_OrderCommands );
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
									addRC( ATMDS3_CMD::uses_G, ts[ sp++ ] & 0x1f, m_SectorEventDef.m_SequenceCommands );
									addRC( ATMDS3_CMD::none, ts[ sp++ ], m_SectorEventDef.m_SequenceCommands );
									// after the glide-fx, sp points to the respective note
								}
								if ( ts[ sp ] == 0xf6u )
									addRC( ATMDS3_CMD::uses_FP, ts[ ++sp ], m_SectorEventDef.m_SequenceCommands );
								if ( ts[ sp ] == 0xf7u )
									addRC( ATMDS3_CMD::uses_PP, ts[ ++sp ], m_SectorEventDef.m_SequenceCommands );
								if ( ts[ sp ] == 0xf8u )
									addRC( ATMDS3_CMD::uses_FF, ts[ ++sp ], m_SectorEventDef.m_SequenceCommands );
								if ( ts[ sp ] == 0xf9u )
									addRC( ATMDS3_CMD::uses_SR, ts[ ++sp ], m_SectorEventDef.m_SequenceCommands );
								if ( ts[ sp ] == 0xfau )
									addRC( ATMDS3_CMD::uses_AD, ts[ ++sp ], m_SectorEventDef.m_SequenceCommands );
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
				inCPUMemory->SetData( t.m_Address, m_TrackEventDef.m_OrderCommands.data(),
									  m_TrackEventDef.m_OrderCommands.size() * sizeof( ATMDS3TrackCmd ) );
			else if ( t.m_ID == static_cast< uint8_t >( ATMDS3::TableID::SECTOR_CMD ) )
				inCPUMemory->SetData( t.m_Address, m_SectorEventDef.m_SequenceCommands.data(),
									  m_SectorEventDef.m_SequenceCommands.size() * sizeof( ATMDS3SectorCmd ) );
	}

	uint8_t DriverArchitectureATMDS3::GetTrackCommandIndex( uint8_t volume, uint8_t speed ) const noexcept
	{ return m_TrackEventDef.GetTrackCommandIndex( volume, speed ); }

	uint8_t DriverArchitectureATMDS3::GetSectorCommandIndex( const ATMDS3SectorCmd& cmd ) const noexcept
	{ return m_SectorEventDef.GetSectorCommandIndex( cmd ); }


	std::vector< uint16_t >
	DriverArchitectureATMDS3::GetOrderListsLength( const DriverInfo&						 di,
												   const Emulation::IMemoryRandomReadAccess& mem ) const noexcept
	{
		std::vector< uint16_t > res;
		auto&					md = di.GetMusicData();
		uint16_t				t0 = md.m_OrderListTrack1Address;
		for ( int s = 0; s < 4; s++ )
			for ( int t = 0; t < md.m_TrackCount; ++t )
				for ( int l = 0; l < 256; ++l ) // Tracks / Orders are easy: may not contain FE/FF except at END
					if ( auto b = mem[ t0 + 256 * ( s * md.m_TrackCount + t ) + l ]; b >= 0xfe )
						res.push_back( l + ( b == 0xfe ? 2 : 1 ) ), l = 255;
		return res;
	}

	std::vector< int >
	DriverArchitectureATMDS3::GetSequenceUsageCount( const DriverInfo&						   di,
													 const Emulation::IMemoryRandomReadAccess& mem ) const noexcept
	{
		auto&			   md = di.GetMusicData();
		std::vector< int > res( md.m_SequenceCount, 0 );
		uint16_t		   t0 = md.m_OrderListTrack1Address;
		for ( int s = 0; s < 4; s++ )
			for ( int t = 0; t < md.m_TrackCount; ++t )
				for ( int l = 0; l < 256; ++l )
					if ( auto b = mem[ t0 + 256 * ( s * md.m_TrackCount + t ) + l ]; b < md.m_SequenceCount )
						res[ b ]++;
					else if ( b >= 0xfe ) l = 255; // end this track
		return res;
	}

	uint16_t
	DriverArchitectureATMDS3::GetEndOfMusicDataAddress( const DriverInfo&						  di,
														const Emulation::IMemoryRandomReadAccess& mem ) const noexcept
	{
		return 0xb4c0; // in case the sequence pointer tables are located in editor layout, they're behind the last seq
	}

	uint16_t DriverArchitectureATMDS3::GetSequenceLength( uint8_t inSequenceIndex, const DriverInfo& di,
														  const Emulation::IMemoryRandomReadAccess& mem ) const noexcept
	{
		auto&	 md = di.GetMusicData();
		uint16_t s0 = md.m_Sequence00Address + md.m_SequenceSize * inSequenceIndex;
		for ( int l = 0; l < md.m_SequenceSize; ++l ) {
			const auto C = mem.GetByte( s0 + l );
			if ( C == 0xff ) return l + 1;
			else if ( C >= 0xf6 && C < 0xfb ) ++l; // Commands with additional parameters
			else if ( C >= 0x60 && C < 0x80 )
				++l; // which could be 0xff's
					 // any other byte value is consumed "as is" and does not mean "end".
		}
		return 256;
	}

	uint8_t DriverArchitectureATMDS3::GetFirstUnusedSequenceIndex(
		const DriverInfo& di, const Emulation::IMemoryRandomReadAccess& mem ) const noexcept
	{
		auto sc = GetSequenceUsageCount( di, mem );
		for ( int i{}; i < sc.size(); ++i )
			if ( sc[ i ] == 0 ) return i;
		return 0x60;
	}

	uint8_t
	DriverArchitectureATMDS3::GetFirstEmptySequenceIndex( const DriverInfo&							di,
														  const Emulation::IMemoryRandomReadAccess& mem ) const noexcept
	{
		// so what is an empty sequence??? I'd say anything containing: dur,pause,end or dur,off,end
		auto&	 md = di.GetMusicData();
		uint16_t s0 = md.m_Sequence00Address;
		uint8_t	 tmp[ 3 ];
		for ( int s{}; s < md.m_SequenceCount; ++s ) {
			mem.GetData( s0 + md.m_SequenceSize * s, tmp, 3 );
			// Mask out duration value (using 0xc0 to receive 0x80 for dur, and masking
			// the reset-indicator on the pause to get a pause/gate difference)
			if ( ( tmp[ 0 ] & 0xc0 ) == 0x80	/* DUR masked out */
				 && ( tmp[ 1 ] / 0xfd ) == 0xfc /* mask away the reset bit to receive a pause */
				 && tmp[ 2 ] == 0xff )
				return s;
		}
		return 0xff;
	}

	uint8_t DriverArchitectureATMDS3::GetHighestSequenceIndexUsed(
		const DriverInfo& di, const Emulation::IMemoryRandomReadAccess& mem ) const noexcept
	{
		auto sc = GetSequenceUsageCount( di, mem );
		for ( int i = sc.size() - 1; i > 0; --i )
			if ( sc[ i ] > 0 ) return i;
		return 0xff;
	}

	uint8_t DriverArchitectureATMDS3::GetHighestInstrumentIndexUsed(
		const DriverInfo& di, const Emulation::IMemoryRandomReadAccess& mem ) const noexcept
	{
		uint8_t ins = 0xff;
		auto&	md	= di.GetMusicData();
		auto	sc	= GetSequenceUsageCount( di, mem );
		for ( int i = sc.size() - 1; i > 0; --i )
			if ( sc[ i ] > 0 ) { // check all used sequences
				uint16_t s0 = md.m_Sequence00Address + md.m_SequenceSize * i;
				for ( int l = 0; l < md.m_SequenceSize; ++l ) {
					const auto C = mem.GetByte( s0 + l );
					if ( C == 0xff ) l = md.m_SequenceSize;
					else if ( C >= 0xf6 && C < 0xfb ) ++l; // Commands with additional parameters
					else if ( C >= 0x60 && C < 0x80 ) ++l; // which could be 0xff's
					else if ( C >= 0xc0 && C < 0xe0 ) {
						if ( ins == 0xff ) ins = C & 0x1f;
						else ins = std::min( ins, uint8_t( C & 0x1f ) );
					}
				}
			}
		return ins;
	}

	uint8_t
	DriverArchitectureATMDS3::GetHighestCommandIndexUsed( const DriverInfo&							di,
														  const Emulation::IMemoryRandomReadAccess& mem ) const noexcept
	{ return m_SectorEventDef.m_SequenceCommands.size() - 1; }

} // namespace Editor
