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
	constexpr uint8_t  MAX_SEQUENCE_COUNT = 128;
	constexpr uint8_t  MAX_SEQUENCE_INDEX = 127;
	constexpr uint16_t MARK_ITEM		  = 256;

} // namespace Editor
