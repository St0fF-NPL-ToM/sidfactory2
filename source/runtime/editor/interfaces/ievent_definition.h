#pragma once
/**
 *  SF2 event definition interface
 *  ==============================
 *  Here, we implement how a driver declares what types of events it is able to consume.
 */

#include <cstring>
#include <map>
#include <string>
#include <memory>
#include <cstdint>

namespace Editor
{
	struct EventsPackResult
	{
		EventsPackResult()
			: m_Data( nullptr )
			, m_DataLength( -1 )
		{}

		EventsPackResult( uint8_t* inData, int inDataLength )
			: m_Data( inData )
			, m_DataLength( inDataLength )
		{}

		std::shared_ptr< unsigned char > m_Data;
		int								 m_DataLength;
	};

	struct EventsUnpackResult
	{
		unsigned m_Length;
		unsigned m_PackedSize;
		uint8_t	 m_LastParameterSet;
		uint8_t	 m_LastCommandSet;
		bool	 m_PackingErrorState;
	};

	struct EventsPackedRowPos
	{
		bool	m_IsValid, m_NextIsEnd;
		uint8_t m_Results[ 4 ];
	};


	struct IEvent
	{ uint8_t m_Param, m_Command, m_Payload, m_Reserved{}; }; // modern computers like sizes to be powers of 2

	enum class SentinelType : uint8_t { NoSentinel, Empty, Loop, Stop, Start, Legato, Mark, Mark2, Mark3, Mark4 };
	enum class Semantic : uint8_t { Parameter, Command, Payload };

	struct ValueContext
	{
		// a value has its range and possibly a storage offset and maybe a default value (if not the offset)
		int16_t m_Minimum{}, m_Maximum{}, m_Offset{}, m_Default{ INT16_MIN };

		// values are actually in use: m_Minimum < m_Maximum.
		bool	valid( uint16_t value ) const noexcept
		{
			if ( m_Minimum >= m_Maximum ) return true; // on unused fields any value is valid.
			return ( m_Minimum + m_Offset <= value && m_Maximum + m_Offset >= value );
		}
		ValueContext()						= default;
		ValueContext( const ValueContext& ) = default;
		ValueContext( int16_t a, int16_t b, int16_t c )
			: m_Minimum( a )
			, m_Maximum( b )
			, m_Offset( c )
		{}
		ValueContext( int16_t a, int16_t b, int16_t c, int16_t d )
			: ValueContext( a, b, c )
		{ m_Default = d; }
	};

	struct SentinelDef
	{
		uint8_t		 m_Count{}, m_Values[ 7 ]{};
		SentinelType m_Reserved{}, m_Types[ 7 ]{};

		// let definition check if value is a sentinel
		SentinelType Which( uint8_t value ) const noexcept
		{
			for ( int i{}; i < m_Count; ++i )
				if ( m_Values[ i ] == value ) return m_Types[ i ];
			return SentinelType::NoSentinel;
		}
		int SentinelPos( SentinelType sen ) const noexcept
		{
			for ( int i{}; i < m_Count; ++i )
				if ( m_Types[ i ] == sen ) return i;
			return -1;
		}
		uint8_t GetSentinel( SentinelType sen ) const noexcept
		{
			if ( auto sp = SentinelPos( sen ); sp != -1 ) return m_Values[ sp ];
			return 0u;
		}
		SentinelDef() = default;
		SentinelDef( const SentinelDef& o ) { *this = o; }
		SentinelDef& operator=( const SentinelDef& o ) { return memcpy( this, &o, sizeof( SentinelDef ) ), *this; }
	};

	struct FieldDefinition
	{
		ValueContext m_Value{};		// Context of the basic field value
		SentinelDef	 m_Sentinels{}; // Set of allowed sentinels in this field
		// For checking if this field actually participates …
		bool		 IsUsed() const noexcept { return m_Value.m_Minimum < m_Value.m_Maximum; }
		uint8_t		 GetDefault() const noexcept
		{
			if ( auto esp = m_Sentinels.SentinelPos( SentinelType::Empty ); esp != -1 )
				return m_Sentinels.m_Values[ esp ];
			else return m_Value.m_Default != INT16_MIN ? m_Value.m_Default : m_Value.m_Offset;
		}
		FieldDefinition()						  = default;
		FieldDefinition( const FieldDefinition& ) = default;
	};

	struct IDataSourceEvents;
	struct DriverState;
	struct DriverInfo;

	struct IEventDefinition
	{
		FieldDefinition					 m_Param; // basic value context and sentinels accepted by the parameter
		FieldDefinition					 m_Command;
		FieldDefinition					 m_Payload;
		std::map< uint8_t, std::string > m_SentinelDisplayFormat;

		virtual EventsPackResult   Pack( const IDataSourceEvents& inData, uint8_t* ouTernalBuffer ) const noexcept = 0;
		virtual EventsUnpackResult Unpack( IDataSourceEvents& outData, uint8_t* inTernalBuffer ) const noexcept	   = 0;
		virtual EventsPackedRowPos GetNextIndexPackedPosition( int inIndex, uint8_t* inData,
															   uint16_t inDataSize ) const noexcept				   = 0;
		virtual bool MayPushData( IDataSourceEvents&, const DriverInfo&, const DriverState& ) const noexcept
		{ return true; }

		// Functionality provided by "The Event Definition Interface":
		SentinelType WhichSentinel( uint8_t v, const FieldDefinition& f ) const noexcept
		{ return f.m_Sentinels.Which( v ); }

		const FieldDefinition& field( Semantic s ) const noexcept
		{
			switch ( s ) { // clang-format off
				case Semantic::Parameter: 	return m_Param;
				case Semantic::Command: 	return m_Command;
				case Semantic::Payload:		return m_Payload;
			} // clang-format on … the following is never reached …
			return m_Payload; // but another return is necessary to silence gcc/clang
		}

		bool IsParamSentinel( SentinelType s, const IEvent& inEvent ) const noexcept
		{ return WhichSentinel( inEvent.m_Param, m_Param ) == s; }
		bool IsCommandSentinel( SentinelType s, const IEvent& inEvent ) const noexcept
		{ return WhichSentinel( inEvent.m_Command, m_Command ) == s; }
		bool IsPayloadSentinel( SentinelType s, const IEvent& inEvent ) const noexcept
		{ return WhichSentinel( inEvent.m_Payload, m_Payload ) == s; }

		uint8_t GetSentinel( SentinelType sen, Semantic sem ) const noexcept
		{
			const auto& f = field( sem );
			if ( f.IsUsed() ) return f.m_Sentinels.GetSentinel( sen );
			return 0u;
		}
		IEvent MakeEmptyEvent() const noexcept
		{
			IEvent ev;
			ClearEvent( ev );
			return ev;
		}
		IEvent& ClearEvent( IEvent& evt ) const noexcept
		{
			evt.m_Param	  = m_Param.GetDefault();
			evt.m_Command = m_Command.GetDefault();
			evt.m_Payload = m_Payload.GetDefault();
			return evt;
		}
		bool HasParameter( const IEvent& e ) const noexcept
		{ return ( m_Param.IsUsed() && m_Param.m_Sentinels.Which( e.m_Param ) == SentinelType::NoSentinel ); }
		bool HasCommand( const IEvent& e ) const noexcept
		{ return ( m_Command.IsUsed() && m_Command.m_Sentinels.Which( e.m_Command ) == SentinelType::NoSentinel ); }
		bool HasPayload( const IEvent& e ) const noexcept
		{
			// When does an event have a payload?  Is a loop pointer payload?
			//	- a loop pointer is accompanying data to the loop parameter
			//	- an event cannot have a payload in case it is a loop or stop-event
			const auto ps = WhichSentinel( e.m_Param, m_Param );
			if ( ps != SentinelType::Loop && ps != SentinelType::Stop ) {
				// - if payload parameter itself is not a sentinel and valid
				return ( WhichSentinel( e.m_Payload, m_Payload ) == SentinelType::NoSentinel )
					   && m_Payload.m_Value.valid( e.m_Payload );
			}
			return false;
		}
		const char* GetSentinelText( SentinelType t, Semantic s ) const noexcept
		{
			const auto sv = field( s ).m_Sentinels.GetSentinel( t );
			if ( !m_SentinelDisplayFormat.contains( sv ) ) return nullptr;
			else return m_SentinelDisplayFormat.at( sv ).c_str();
		}
		uint8_t GetMinimum( Semantic s ) const noexcept { return field( s ).m_Value.m_Minimum; }
		uint8_t GetMaximum( Semantic s ) const noexcept { return field( s ).m_Value.m_Maximum; }
		uint8_t GetOffset( Semantic s ) const noexcept { return field( s ).m_Value.m_Offset; }
		uint8_t GetDefault( Semantic s ) const noexcept { return field( s ).m_Value.m_Offset; }
	};

} // namespace Editor
