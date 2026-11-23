/**
 *  SF2 Event redefinition
 */

#include "runtime/editor/interfaces/ievent_definition.h"

namespace Editor
{
	struct SF2_OrderEvent : public EventDefinition
	{
		SF2_OrderEvent()
		{
			m_Param.m_Value = { -0x20, 0x1f, 0xa0 };
			m_Param.m_Sentinels.m_Count = 3;
			m_Param.m_Sentinels.m_Types[0] = SentinelType::Empty;
			m_Param.m_Sentinels.m_Values[0] = 0x7f;
			m_Param.m_Sentinels.m_Types[1] = SentinelType::Loop;
			m_Param.m_Sentinels.m_Values[1] = 0xff;
			m_Param.m_Sentinels.m_Types[2] = SentinelType::Stop;
			m_Param.m_Sentinels.m_Values[2] = 0xfe;
			m_Payload.m_Value = { 0, 127, 0 };
			using T = std::pair<const unsigned char, std::string>;
			m_SentinelDisplayFormat = { T { 0x7f, "...." }, T { 0xfe, "stop" }, T { 0xff, "loop" } };
		};
	};
	struct SF2_SequenceEvent : public EventDefinition
	{
		SF2_SequenceEvent()
		{
			m_Param.m_Value = { 0x00, 0x1f, 0xa0 };
			m_Param.m_Sentinels.m_Count = 2;
			m_Param.m_Sentinels.m_Types[0] = SentinelType::Empty;
			m_Param.m_Sentinels.m_Values[0] = 0x80;
			m_Param.m_Sentinels.m_Types[1] = SentinelType::Mark;
			m_Param.m_Sentinels.m_Values[1] = 0x90;
			m_Command.m_Value = { 0x00, 0x3f, 0xc0 };
			m_Command.m_Sentinels.m_Count = 1;
			m_Command.m_Sentinels.m_Types[0] = SentinelType::Empty;
			m_Command.m_Sentinels.m_Values[0] = 0x80;
			m_Payload.m_Value = { 0x00, 0x5e, 0x01 };
			m_Payload.m_Sentinels.m_Count = 3;
			m_Payload.m_Sentinels.m_Types[0] = SentinelType::Empty; // no operation type
			m_Payload.m_Sentinels.m_Values[0] = 0x00;
			m_Payload.m_Sentinels.m_Types[1] = SentinelType::Mark; // "+++"
			m_Payload.m_Sentinels.m_Values[1] = 0x7e;
			m_Payload.m_Sentinels.m_Types[2] = SentinelType::Stop;
			m_Payload.m_Sentinels.m_Values[2] = 0x7f;
			using T = std::pair<const unsigned char, std::string>;
			m_SentinelDisplayFormat = { T { 0x00, "---" }, T { 0x7e, "+++" }, T { 0x80, "--" }, T { 0x90, "**" } };
		}
	};

}
