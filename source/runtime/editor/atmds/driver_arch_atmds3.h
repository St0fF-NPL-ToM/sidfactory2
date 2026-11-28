#pragma once
/**
 *		SF2: driver_arch_ATMDS3
 *		=======================
 *		Adding a non-JCH/Laxity driver architecture to SIDFactoryII.
 *
 *		ATMDS 3.2 was mostly programmed in 1998 and 1999.  Due to St0fF's psychological state he
 *		couldn't "regard the tracker as a finished product", due to a few unfixed, known bugs.
 *
 *		Anyhow, ATMDS3.2 was not as far developed regarding optimal Order and Sequence Structure
 *		compared to JCH's NewPlayer architecture.  It also did not know about a command table,
 *		that indirection step was taken in ATMDS 4's unreleased drivers.
 *
 *		So - OrderLists in ATMDS are called Tracks.  They do contain the same information, as SF2
 *		Orderlists, but also a few more.
 *		- global volume, playback speed, and current song selection can be encoded, which reaches
 *		  far beyond the Commands of SF2.
 *
 *		Sectors - which would be Sequences in SF2 - on the other hand do not know about a command
 *		table.  There are a few built-in commands a Sector knows, such as:
 *		- Glide, TuneAD, TuneSR, TunePP, TuneFP, TuneFCF, Switch
 *		These will need to be transmogrified into some kind of a command table.
 *
 *		Command Tables:
 *		We have 3 extra Order - Commands, of which one has kind of "Loop"-Semantics (SNG=x).
 *		The semantics of a song change induced by a song is not implemented in SF2.  And - let's
 *		face reality: that feature of ATMDS3 was nonsense, although I did use it quite often.
 *		Then we have the extra Sequence Commands.  ATMDS does allow many commands in one Event.
 *		Thus, all 5 Tuning-Commands and the Glide Command need to be "presentable" at once.  This
 *		Glide-Command also brings its target note as a secondary parameter.
 *
 *		So, ATMDS3 - Architecture will provide SF2 with the following command tables:
 *			MainCommandTable: 	columns: bitmask | AD | SR | PP | FP | FF | GLDfx | TgtNote
 *			OrderCommandTable:	columns: bitmask | VOL | SPD | SNG
 *
 *		Internal State:	bool m_CommandsCreated
 *		Internal Func:  void GenerateCommandTable
 *			triggered by: PostInitSetPlaybackIndices?
 *
 *		Command Tables in emulated memory: should be positioned outside the original ATMDS data
 *		range, e.g. ending at $4000.  In "my DDK" I just set it to ($4000-10*128), ColumnMajor.
 *
 */
#include "ATMDS3_Constants.h"
#include "runtime/editor/interfaces/idriver_architecture.h"
#include "runtime/editor/interfaces/ievent_definition.h"

namespace Editor
{
	enum class ATMDS3_CMD : uint8_t {
		none	= 0,
		uses_AD = ( 1 << 0 ),
		uses_SR = ( 1 << 1 ),
		uses_PP = ( 1 << 2 ),
		uses_FP = ( 1 << 3 ),
		uses_FF = ( 1 << 4 ),
		uses_G	= ( 1 << 5 ),
		uses_V	= ( 1 << 6 ),
		uses_S	= ( 1 << 7 )
	};

	struct ATMDS3TrackCmd
	{
		uint8_t m_c0{}, m_c1{};
		ATMDS3TrackCmd() = default;
		ATMDS3TrackCmd( ATMDS3_CMD use, unsigned char val ) { set( use, val ); }
		void set( ATMDS3_CMD use, unsigned char val )
		{ // clang-format off
			switch( use ) {
				case ATMDS3_CMD::uses_V:	m_c0 |= val | uint8_t(ATMDS3_CMD::uses_V);	break;
				case ATMDS3_CMD::uses_S:	m_c1 = val;	m_c0 |= uint8_t(ATMDS3_CMD::uses_S); break;
				default: return;								// Wrong values are ignored!
			}
		} // clang-format on
	};

	struct ATMDS3SectorCmd
	{
		ATMDS3_CMD m_Uses{ ATMDS3_CMD::none };
		uint8_t	   m_AD, m_SR, m_PP, m_FP, m_FF, m_G, m_T;
		ATMDS3SectorCmd() = default;
		ATMDS3SectorCmd( ATMDS3_CMD use, unsigned char val ) { set( use, val ); }
		void set( ATMDS3_CMD use, unsigned char val )
		{ // clang-format off
			switch( use ) {
				case ATMDS3_CMD::uses_AD:	m_AD = val;	break;
				case ATMDS3_CMD::uses_SR:	m_SR = val;	break;
				case ATMDS3_CMD::uses_PP:	m_PP = val;	break;
				case ATMDS3_CMD::uses_FP:	m_FP = val;	break;
				case ATMDS3_CMD::uses_FF:	m_FF = val;	break;
				case ATMDS3_CMD::uses_G:	m_G  = val; break;
				case ATMDS3_CMD::none:		m_T  = val; break;	// abusing cmd "none" for glide target setting!
				default: return;								// Wrong values are ignored!
			}
			m_Uses = ATMDS3_CMD( int(use) + int(m_Uses) );
		} // clang-format on
		bool operator()( ATMDS3_CMD use ) const noexcept { return bool( uint8_t( m_Uses ) & uint8_t( use ) ); }
		void clear() { memset( this, 0, sizeof( ATMDS3SectorCmd ) ); }
		bool operator==( const ATMDS3SectorCmd& o ) const noexcept
		{
			if ( m_Uses == o.m_Uses ) { // same uses must be compared
				for ( int bit = 0; bit <= 5; ++bit )
					if ( o( ATMDS3_CMD( 1 << bit ) ) && ( ( &m_AD )[ bit ] != ( &o.m_AD )[ bit ] ) ) return false;
				if ( o( ATMDS3_CMD::uses_G ) ) return m_T == o.m_T;
				return true;
			}
			return false;
		}
	};

	struct ATMDS3TrackEvents : public IEventDefinition
	{
		ATMDS3TrackEvents();
		uint8_t			   GetTrackCommandIndex( uint8_t volume, uint8_t speed ) const noexcept;
		EventsPackResult   Pack( const IDataSourceEvents& inData, uint8_t* ouTernalBuffer ) const noexcept override;
		EventsUnpackResult Unpack( IDataSourceEvents& outData, uint8_t* inTernalBuffer ) const noexcept override;
		EventsPackedRowPos GetNextIndexPackedPosition( int inIndex, uint8_t* inData,
													   uint16_t inDataSize ) const noexcept override;
		std::vector< ATMDS3TrackCmd > m_OrderCommands;
	};
	struct ATMDS3SectorEvents : public IEventDefinition
	{
		ATMDS3SectorEvents();
		uint8_t			   GetSectorCommandIndex( const ATMDS3SectorCmd& cmd ) const noexcept;
		EventsPackResult   Pack( const IDataSourceEvents& inData, uint8_t* ouTernalBuffer ) const noexcept override;
		EventsUnpackResult Unpack( IDataSourceEvents& outData, uint8_t* inTernalBuffer ) const noexcept override;
		EventsPackedRowPos GetNextIndexPackedPosition( int inIndex, uint8_t* inData,
													   uint16_t inDataSize ) const noexcept override;
		bool MayPushData( IDataSourceEvents&, const DriverInfo&, const DriverState& ) const noexcept override;
		std::vector< ATMDS3SectorCmd > m_SequenceCommands;
	};

	class DriverArchitectureATMDS3 final : public IDriverArchitecture
	{
		friend class ATMDS3TrackEvents;
		friend class ATMDS3SectorEvents;

	  public:
		static constexpr uint8_t Descriptor = 0xac;
		// Static constants cannot be made inherited. So we give it another virtual …
		DriverArchitectureATMDS3();
		virtual ~DriverArchitectureATMDS3() = default;

		uint8_t GetID() const noexcept override { return Descriptor; }

		void	SetEmptySequence( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress ) override;
		void	SetEmptyOrderList( Emulation::CPUMemory* inCPUMemory, unsigned short inAddress ) override;
		void PostInitSetPlaybackIndices( std::vector< PlayMarkerInfo >& inPlayMarkerInfoList,
										 Emulation::CPUMemory* inCPUMemory, const DriverInfo& inDriverInfo ) override;

		std::vector< uint16_t > GetOrderListsLength( DRIVER_UTIL_PARAMS ) const noexcept override;
		std::vector< int >		GetSequenceUsageCount( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint16_t				GetEndOfMusicDataAddress( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint16_t GetSequenceLength( uint8_t inSequenceIndex, DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetFirstUnusedSequenceIndex( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetFirstEmptySequenceIndex( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetHighestSequenceIndexUsed( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetHighestInstrumentIndexUsed( DRIVER_UTIL_PARAMS ) const noexcept override;
		uint8_t	 GetHighestCommandIndexUsed( DRIVER_UTIL_PARAMS ) const noexcept override;

	  private:
		void			   GenerateCommandTables( Emulation::CPUMemory* inCPUMemory, const DriverInfo& inDriverInfo );
		uint8_t			   GetTrackCommandIndex( uint8_t volume, uint8_t speed ) const noexcept;
		uint8_t			   GetSectorCommandIndex( const ATMDS3SectorCmd& cmd ) const noexcept;

		ATMDS3TrackEvents  m_TrackEventDef;
		ATMDS3SectorEvents m_SectorEventDef;
	};
} // namespace Editor
