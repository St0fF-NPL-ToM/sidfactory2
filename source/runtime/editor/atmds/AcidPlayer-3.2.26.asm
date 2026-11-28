/*
 * SIDFactoryII Driver Creator
 *
 *  Ok, also haben JCH, Laxity und Youth doch etwas geschlampt. Soeben sind mir
 *  die auxiliary Data aufgefallen, deren Adresse "HARDCODED" bei $0ffb liegt.
 *  Kein Wunder, dass sie sich noch einen JMP $0000 vor dem Player aufgehoben
 *  haben, das ist dann vermutlich der nächste Workaround.
 *
 *  Anyhow … ich muss das also nur in meinen Code integrieren → siehe SF2_driver.asm
 */

#import "SF2_driver.asm"
#import "ATMDS3_data.asm"
#import "ATPLR-3.2-SF2.asm"
#undef HAS_DATA

// added a "dry run" for determining size of driver code (in case it changes)
// (I should have simply counted by hand, though.)
.segment Dry [start=$f80,virtual]
SF2_driverCalls( 0, 0 )
.print "determined driver size: $" + toHexString( Driver_Size )

// assemble the player
.segment Player [ start=$1000 ]
assemble_ATP3( null, true )

// prepend the driver
// (MUST BE: else the idea of SF2, to always play a music packed, won't really
//  hold, as different page boundary crossings would occur automatically)
.segment Driver [ start=$1000 - Driver_Size ]
SF2_driverCalls( pinit, pplay )
.segmentout [segments="Player"]

// Prepare our Driver Descriptions
.const DB = DriverBlock( "Driver", $1000 - Driver_Size, code_end, "", $403c, $b4c0,
    initAddress, stopAddress, updateAddress, SIDchannelOffsetAddress, DriverStateAddress,
    pc, ptpos, psnum, 0, pspos, ptrp, pdur, pinum, pnxtcom, pnoteok, ptie, p_tick, panschlag, $7f
 )
// Type, ID, Name, TextFieldWidth, DataLayout, Properties, InsDelID, CrActID, ColorID, Address, Columns, Rows, visRows
.const TABS = List().add(
    DriverTable( Generic_Table, 0, "TrackCmds", 0, ColumnMajor, IndexAsContinuousMemory, $ff, $ff, $ff, code_end, 2, 32, 4 ),
    DriverTable( Generic_Table, 1, "Multi", 0, RowMajor, IndexAsContinuousMemory, $ff, $ff, $ff, speed_table, 1, 4, 4 ),
    DriverTable( Generic_Table, 2, "Song", 32, ColumnMajor, IndexAsContinuousMemory, $ff, $ff, $ff, tunedats, 8, 4, 4 ),
    DriverTable( Instrument_Table, 3, "INS AD SR WP PP FP V1 V2 V3 HR H1 H2", 32, ColumnMajor, LayoutAddVertically | IndexAsContinuousMemory, $ff, $ff, $ff, adtb, 11, 32, 8 ),
    DriverTable( Command_Table, 4, "Sequence Commands", 0, ColumnMajor, LayoutAddVertically | IndexAsContinuousMemory, $ff, $ff, $ff, code_end+64, 8, 64, 8 ),
    DriverTable( Generic_Table, 5, "Wave", 0, RowMajor, IndexAsContinuousMemory, $ff, $ff, $ff, wave, 2, 256, 8 ),
    DriverTable( Generic_Table, 6, "Pulse", 0, RowMajor, IndexAsContinuousMemory, $ff, $ff, $ff, ptb1, 2, 256, 8 ),
    DriverTable( Generic_Table, 7, "Filter", 0, RowMajor, IndexAsContinuousMemory, $ff, $ff, $ff, ftb1, 2, 256, 8 ),
    DriverTable( Generic_Table, 8, "Glide", 0, RowMajor, IndexAsContinuousMemory, $ff, $ff, $ff, gldtbl, 2, 32, 8 )
)
// TableID, InstrumentDataPointerPosition, PointerAndValue, InstrumentDataConditionalValuePosition,
// ConditionValueAndValue, ConditionEqualityValue, TableDataType, TableJumpMarkerValuePosition,
// TableJumpMarkerValue, TableJumpDestinationIndexPosition
.const INSDAT = List().add(
    InstrumentData( 5, 2, 255, 2, 0, 0, 1, 0, 144, 1 ),
    InstrumentData( 6, 3, 255, 3, 255, 255, 1, 0, 255, 1 ),
    InstrumentData( 7, 4, 255, 4, 255, 255, 1, 0, 255, 1 )
)
// prepend Driver Information Blocks
SF2_driver( "AcidPlayer-3.2.26", $ac, "3.2.26", DB, TABS,
    List().add( "ad", "sr", "wp", "pp", "fp", "v1", "v2", "v3", "hr", "hf1", "hf2" ),
    MusicData( 3, tunedats, tunedats+1, 96, secs_lo, secs_hi, 256, trk10, 256, sec00 ),
    List(), List(), List(), INSDAT )

/* be finished … */
.segment Default
.print "done."
