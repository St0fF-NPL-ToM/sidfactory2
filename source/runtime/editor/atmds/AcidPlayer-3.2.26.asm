/*
 ******************************************************************************
 *  SIDFactoryII Driver Assembler for ATPLR-3.2'26
 ******************************************************************************
 *  SF2 prepends a "driver" with a few chunks of description data.
 *  This data separates one driver of a special category from others.
 *
 *  Up until now, there was only one driver architecture "SF2". Now, with
 *  ATMDS3 running up, it needs its driver, too - purpose of this file.
 ******************************************************************************
 *  About ATMDS3 drivers:   Development started in late 1997 after I returned
 *  ---------------------   from a year as an exchange student to the USA.
 *
 *  The basic architecture is not very different from SF2 architecture.  The
 *  music data describing events and playback order consists of a Track/Sector
 *  hierarchy which is exactly the same as in SF2.
 *
 *  Only ATMDS did not have the concept of "command lists".  Therefore, all
 *  possible commands are kind of scattered thru both layers.  Meaning:
 *
 *  ATMDS supports a few OrderList-Commands other than "Loop" and "Stop", namely
 *      Volume = X
 *      Speed  = X
 *      Song   = X
 *  these could have been made sector commands or participants in a command table,
 *  but I didn't think that far in the 90's.
 *  Sequences also support commands.  But not only one, they support ALL.  Sounds
 *  a bit blunt ... special commands are simply bytes or even 2-byte-blocks inside
 *  a sequence.  So, all of them could follow one another, you could use all
 *  commands in a single sequence event (blasting player rastertime, as every
 *  command takes about one rasterline to be played). But rarely more than 2
 *  commands were needed.
 ******************************************************************************
 *  This leads to the missing command tables of ATMDS - a big NO-GO for SF2,
 *  which actually REQUIRES a command and an instrument table to exist.
 *
 *  The C++ side of the ATMDS3 architecture will produce a OrderCommands list
 *  and a SequenceCommands list upon song preparation.  It scans the loaded
 *  music data for command uses to create this additional abstraction layer.
 *
 *  Also: the driver itself needs ~2kB, pretty much standard. The data in edit-
 *  layout on the other hand needs nearly half the C64 memory.  On the vanilla
 *  machine, the driver started @0900, and all room between driver and music
 *  was editor code and data.  This will be empty space in the file produced.
 */

#define HAS_DATA           // ATTN: this define is crucial for data production!
#import "SF2_driver.asm"
#import "ATMDS3_data.asm"
#import "ATPLR-3.2-SF2.asm"

// added a "dry run" for determining size of driver code (in case it changes)
// (I should have simply counted by hand, though.)
.segment Dry [start=$f80,virtual]
SF2_driverCalls( 0, 0 )
.print "determined driver size: $" + toHexString( Driver_Size )

// assemble the player into a segment
.segment Player [ start=$1000 ]
assemble_ATP3( ATMDS3_emptyData(), true )

// prepend the driver
.segment Driver [ start=$1000 - Driver_Size ]
SF2_driverCalls( pinit, pplay )
.segmentout [segments="Player"]

// Prepare our Driver Descriptions:
// Driver-Commons
.const DB = DriverBlock( "Driver", $1000 - Driver_Size, code_end, "", $403c, $b4c0,
    initAddress, stopAddress, updateAddress, SIDchannelOffsetAddress, DriverStateAddress,
    pc, ptpos, psnum, 0, pspos, ptrp, pdur, pinum, pnxtcom, pnoteok, ptie, p_tick, panschlag, $7f
 )
// Table-Definitions
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
