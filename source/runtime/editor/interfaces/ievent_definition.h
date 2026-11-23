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

	enum class SentinelType : uint8_t { Empty, Loop, Stop, Mark };
	enum class Semantic : uint8_t { Parameter, Command, Payload };

	struct ValueContext
	{
		// a value has its range and possibly a display offset
		int16_t m_Minimum{}, m_Maximum{}, m_Offset{}, m_Reserved{};
		// values are actually in use: m_Minimum < m_Maximum.
	};
	struct SentinelDef
	{
		uint8_t		 m_Count, m_Values[ 7 ];
		SentinelType m_Reserved, m_Types[ 7 ];
	};

	struct FieldDefinition
	{
		ValueContext m_Value;	  // Context of the basic field value
		SentinelDef	 m_Sentinels; // Set of allowed sentinels in this field
		// For checking if this field actually participates …
		bool		 isUsed() const { return m_Value.m_Minimum < m_Value.m_Maximum; }
	};

	struct EventDefinition
	{
		FieldDefinition					 m_Param; // basic value context and sentinels accepted by the parameter
		FieldDefinition					 m_Command;
		FieldDefinition					 m_Payload;
		std::map< uint8_t, std::string > m_SentinelDisplayFormat;

		bool							 isMySentinel( SentinelType s, uint8_t v, const FieldDefinition& f ) const
		{
			for ( int i{}; i < f.m_Sentinels.m_Count; ++i )
				if ( f.m_Sentinels.m_Types[ i ] == s && f.m_Sentinels.m_Values[ i ] == v ) return true;
			return false;
		}
		bool IsParamSentinel( SentinelType s, const IEvent& inEvent ) const
		{ return isMySentinel( s, inEvent.m_Param, m_Param ); }
		bool IsCommandSentinel( SentinelType s, const IEvent& inEvent ) const
		{ return isMySentinel( s, inEvent.m_Command, m_Command ); }
		bool IsPayloadSentinel( SentinelType s, const IEvent& inEvent ) const
		{ return isMySentinel( s, inEvent.m_Payload, m_Payload ); }
		uint8_t GetSentinel( SentinelType sen, Semantic sem ) const noexcept
		{
			auto GetIt = [ & ]( const auto& f ) -> uint8_t
			{
				for ( int i{}; i < f.m_Sentinels.m_Count; ++i )
					if ( f.m_Sentinels.m_Types[ i ] == sen ) return f.m_Sentinels.m_Values[ i ];
				return 0u;
			};
			if ( sem == Semantic::Parameter && m_Param.isUsed() ) return GetIt( m_Param );
			else if ( sem == Semantic::Command && m_Command.isUsed() ) return GetIt( m_Command );
			else if ( sem == Semantic::Payload && m_Payload.isUsed() ) return GetIt( m_Payload );
			else return 0xff;
		}
		IEvent MakeEmptyEvent() const
		{
			return { GetSentinel( SentinelType::Empty, Semantic::Parameter ),
					 GetSentinel( SentinelType::Empty, Semantic::Command ),
					 GetSentinel( SentinelType::Empty, Semantic::Payload ) };
		}
		void ClearEvent( IEvent& evt ) const
		{
			evt.m_Param	  = GetSentinel( SentinelType::Empty, Semantic::Parameter );
			evt.m_Command = GetSentinel( SentinelType::Empty, Semantic::Command );
			evt.m_Payload = GetSentinel( SentinelType::Empty, Semantic::Payload );
		}
	};

} // namespace Editor
