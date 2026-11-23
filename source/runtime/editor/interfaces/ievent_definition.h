#pragma once
/**
 *  SF2 event definition interface
 *  ==============================
 *  Here, we implement how a driver declares what types of events it is able to consume.
 */

#include <map>
#include <string>
#include <cstdint>

namespace Editor
{

	struct IEvent
	{ uint8_t m_Param, m_Command, m_Payload, m_Reserved{}; }; // modern computers like sizes to be powers of 2

	enum class SentinelType : uint8_t { NoSentinel, Empty, Loop, Stop, Mark };
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
	};

	struct SentinelDef
	{
		uint8_t		 m_Count, m_Values[ 7 ];
		SentinelType m_Reserved, m_Types[ 7 ];

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
	};

	struct FieldDefinition
	{
		ValueContext m_Value;	  // Context of the basic field value
		SentinelDef	 m_Sentinels; // Set of allowed sentinels in this field
		// For checking if this field actually participates …
		bool		 IsUsed() const noexcept { return m_Value.m_Minimum < m_Value.m_Maximum; }
		uint8_t		 GetDefault() const noexcept
		{
			if ( auto esp = m_Sentinels.SentinelPos( SentinelType::Empty ); esp != -1 )
				return m_Sentinels.m_Values[ esp ];
			else return m_Value.m_Default != INT16_MIN ? m_Value.m_Default : m_Value.m_Offset;
		}
	};

	struct EventDefinition
	{
		FieldDefinition					 m_Param; // basic value context and sentinels accepted by the parameter
		FieldDefinition					 m_Command;
		FieldDefinition					 m_Payload;
		std::map< uint8_t, std::string > m_SentinelDisplayFormat;

		SentinelType					 WhichSentinel( uint8_t v, const FieldDefinition& f ) const noexcept
		{ return f.m_Sentinels.Which( v ); }

		bool IsParamSentinel( SentinelType s, const IEvent& inEvent ) const noexcept
		{ return WhichSentinel( inEvent.m_Param, m_Param ) == s; }
		bool IsCommandSentinel( SentinelType s, const IEvent& inEvent ) const noexcept
		{ return WhichSentinel( inEvent.m_Command, m_Command ) == s; }
		bool IsPayloadSentinel( SentinelType s, const IEvent& inEvent ) const noexcept
		{ return WhichSentinel( inEvent.m_Payload, m_Payload ) == s; }

		uint8_t GetSentinel( SentinelType sen, Semantic sem ) const noexcept
		{
			if ( sem == Semantic::Parameter && m_Param.IsUsed() ) return m_Param.m_Sentinels.GetSentinel( sen );
			else if ( sem == Semantic::Command && m_Command.IsUsed() ) return m_Command.m_Sentinels.GetSentinel( sen );
			else if ( sem == Semantic::Payload && m_Payload.IsUsed() ) return m_Payload.m_Sentinels.GetSentinel( sen );
			else return 0u;
		}
		IEvent MakeEmptyEvent() const noexcept
		{
			IEvent ev;
			ClearEvent( ev );
			return ev;
		}
		void ClearEvent( IEvent& evt ) const noexcept
		{
			evt.m_Param	  = m_Param.GetDefault();
			evt.m_Command = m_Command.GetDefault();
			evt.m_Payload = m_Payload.GetDefault();
		}
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
	};

} // namespace Editor
