#if !KICKASSEMBLY
#pragma once
#else
#importonce
#endif
/**
 *  SIDFactoryII - ATMDS 3.2 constant definitions
 *  =============================================
 *  This will probably become a lot more than simple constant definitions, as I have to understand
 *  SF2's layouting resulting from the table definitions.
 */

#if !KICKASSEMBLY
#include <cstdint>

namespace ATMDS3
{
	// openly exported driver assembly call addresses
	constexpr uint16_t driver_init_address		 = 0x1000;
	constexpr uint16_t driver_play_address		 = 0x1003;
	constexpr uint16_t driver_set_volume_address = 0x1006;
	constexpr uint16_t driver_file_text_address	 = 0x1020;
	constexpr uint16_t driver_file_text_length	 = 0x20;
#endif
// driver table ids - sorted for "output renter rtl-ttb order", adapted to JS enum convention of declaring Name = Value for KickAss
#define _TableID  { TRACK_CMD = 0, MSP = 1, SONG = 2, INSTRUMENT = 3, SECTOR_CMD = 4, WAVE = 5, PULSE = 6, FILTER = 7, GLIDE = 8 }
#if KICKASSEMBLY
    .enum _TableID;
#else
	enum class TableID : uint8_t _TableID;
#endif
#undef _TableID
	/* Now, Table sizing and placement happens in SF2 according to some table settings. Desired Arrangement:

		TrackCmds:  Multi:  Song:
		00: XV TC   00: XY  00: TC 0V "der Name von Titel #1 im Daten- "
		01: XV TC   01: XY  01: TC 0V "Titel #2                 satz..."
		02: XV TC   02: XY  02: TC 0V "Titel #3     oder einfach       "
		03: XV TC   03: XY  03: TC 0V "Titel #4     irgendwelcher Text."

		Instruments:
		00: AD SR WP PP FP V1 V2 V3 HR H1 H2 "wurstgerät zum dran treten      "
		01: AD SR WP PP FP V1 V2 V3 HR H1 H2 "wurstgerät zum drauf hauen      "
		02: AD SR WP PP FP V1 V2 V3 HR H1 H2 "wurstgerät mit schmackes        "
		03: AD SR WP PP FP V1 V2 V3 HR H1 H2 "                ...             "
		04: AD SR WP PP FP V1 V2 V3 HR H1 H2 "                   ...          "
		05: AD SR WP PP FP V1 V2 V3 HR H1 H2 "                      ...       "
		06: AD SR WP PP FP V1 V2 V3 HR H1 H2 "                         ...    "
		07: AD SR WP PP FP V1 V2 V3 HR H1 H2 "                            ... "

		Commands:                    Wave:      Pulse:     Filter:    Glide:
		00: TY AD SR PP FP FF GD GT  00: WC TP  00: P1 P2  00: F1 F2  00: GL GH
		01: TY AD SR PP FP FF GD GT  01: WC TP  01: P1 P2  01: F1 F2  01: GL GH
		02: TY AD SR PP FP FF GD GT  02: WC TP  02: P1 P2  02: F1 F2  02: GL GH
		03: TY AD SR PP FP FF GD GT  03: WC TP  03: P1 P2  03: F1 F2  03: GL GH
		04: TY AD SR PP FP FF GD GT  04: WC TP  04: P1 P2  04: F1 F2  04: GL GH
		05: TY AD SR PP FP FF GD GT  05: WC TP  05: P1 P2  05: F1 F2  05: GL GH
		06: TY AD SR PP FP FF GD GT  06: WC TP  06: P1 P2  06: F1 F2  06: GL GH
		07: TY AD SR PP FP FF GD GT  07: WC TP  07: P1 P2  07: F1 F2  07: GL GH
	*/
#if !KICKASSEMBLY
} // namespace ATMDS3
#endif
