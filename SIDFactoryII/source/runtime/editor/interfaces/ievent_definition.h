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
	enum class SentinelType : uint8_t
	{
		Empty,
		Loop,
		Stop,
		Mark
	};

	struct ValueContext
	{
		// a value has its range and possibly a display offset
		int16_t m_Minimum {}, m_Maximum {}, m_Offset {}, m_Reserved {};
		// values are actually in use: m_Minimum < m_Maximum.
	};
	struct SentinelDef
	{
		uint8_t m_Count, m_Values[7];
		SentinelType m_Reserved, m_Types[7];
	};

	struct FieldDefinition
	{
		ValueContext m_Value; // Context of the basic field value
		SentinelDef m_Sentinels; // Set of allowed sentinels in this field
		// For checking if this field actually participates …
		bool isUsed() const { return m_Value.m_Minimum < m_Value.m_Maximum; }
	};

	struct EventDefinition
	{
		FieldDefinition m_Param; // basic value context and sentinels accepted by the parameter
		FieldDefinition m_Command;
		FieldDefinition m_Payload;
		std::map<uint8_t, std::string> m_SentinelDisplayFormat;
	};
}
