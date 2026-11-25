#pragma once
/**
 *  SF2 global constants
 *  ====================
 *  A lot of constants are used over and over within SIDFactoryII, kind of ignoring the core
 *  guideline "keep single sources of truth".
 */
#include <cstdint>

namespace Editor
{
	constexpr uint8_t  MAX_SEQUENCE_COUNT		= 128;
	constexpr uint8_t  MAX_SEQUENCE_INDEX		= 127;
	constexpr uint16_t MARK_ITEM				= 256;
	constexpr int	   MAX_ORDER_EVENTS			= 256;
	constexpr int	   MAX_ORDER_PACKED_SIZE	= 256;
	constexpr int	   MAX_SEQUENCE_PACKED_SIZE = 256;
	constexpr int	   MAX_SEQUENCE_EVENTS		= 1024;
} // namespace Editor
