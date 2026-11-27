/*
 *  SIDFactoryII Driver Development Kit
 *  ===================================
 *  Type-, function- and macro definitions
 */
#importonce
.label DriverStateAddress = $03
.macro SF2_driverCalls( initAddr, playAddr ) {
    .if ( playAddr == 0 ) {
        @startDriverAddress:
    }
    @initAddress:   sta $04
                    ldx #$80
                    stx DriverStateAddress
                    jmp initAddr
    @stopAddress:   lda #$40
                    sta DriverStateAddress
        !:          and #0
                    sta $d404
                    sta $d40b
                    sta $d412
                    rts
    @updateAddress: bit DriverStateAddress
                    bvs !-
                    jmp playAddr
    @SIDchannelOffsetAddress:
                    .byte 0, 7, 14
    @SF2_AUXILIARY_DATA_POINTER:    .word 0
    @SF2_SAFEGUARD_EMPTY_JUMP:      jmp pinit
    .if ( playAddr == 0 ) {
        .const @Driver_Size = * - startDriverAddress
    }
}
/* SF2-Driver::Block-ID */ .enum {
    ID_Descriptor = 1,
    ID_DriverCommon = 2,
    ID_DriverTables = 3,
    ID_DriverInstrumentDescriptor = 4,
    ID_MusicData = 5,
    ID_TableColorRules = 6,
    ID_TableInsertDeleteRules = 7,
    ID_TableActionRules = 8,
    ID_DriverInstrumentDataDescriptor = 9,
    _IDBlock_Count = 10,
    ID_End = $ff
 }
/* SF2-Driver::TableTypes */ .enum { Generic_Table = 0, Instrument_Table = $80, Command_Table = $81, No_Table = $ff }
/* SF2-Driver::TableLayout */ .enum { RowMajor = 0, ColumnMajor = 1 }
/* SF2-Driver::TableProps */ .enum { EnableInsertDelete = 1, LayoutAddVertically = 2, IndexAsContinuousMemory = 4 };

/* SF2-Driver::Description Object */ .struct DriverBlock {
    /* 1st: the compiled driver segment */ SegmentName, SegmentStart, SegmentEnd,
    /* 2nd: the compiled data segment */ DataBlockName, DataBlockStart, DataBlockEnd,
    /* 3rd: necessary code pointers */ InitAddress, StopAddress, UpdateAddress,
    /* SF2-Requirements */
    SIDchannelOffsetAddress, DriverStateAddress, TickCounterAddress, OrderListIndexAddress,
    SequenceIndexAddress, SequenceInUseAddress, CurrentSequenceAddress, CurrentTransposeAddress,
    CurrentSequenceEventDurationAddress, NextInstrumentAddress, NextCommandAddress, NextNoteAddress,
    NextNoteIsTiedAddress, TempoCounterAddress, TriggerSyncAddress, NoteEventTriggerSyncValue
 }
/* SF2-Driver::Tables Object */ .struct DriverTable {
    Type, ID, Name, TextFieldWidth, DataLayout, Properties, InsDelID, CrActID, ColorID, Address, Columns, Rows, visRows
 }
/* SF2-Driver::Music Object */ .struct MusicData {
    TrackCount, TrackPtrsLow, TrackPtrsHi,
    SequenceCount, SequencePtrsLow, SequencePtrsHi,
    OrderListSize, OrderListAddr,
    SequencesSize, SequencesAddr
 }
/* SF2-Driver::ColorRule */  .struct ColorRule {  EvalCellIndex, EvalCellMask, ConditionalValue, ConditionalColor }
/* SF2-Driver::InsDelRule */ .struct InsDelRule { TableID, TargetCellIndex, EvalCellIndex, EvalCellMask, ConditionalValue }
/* SF2-Driver::ActionRule */ .struct ActionRule { ApplicableCell, TargetTableID, TargetIndexCell, TargetIndexMask, EvalCellIndex, EvalCellMask, ConditionalValue }
/* SF2-Driver::InstrumentData */ .struct InstrumentData { TableID, InstrumentDataPointerPosition, PointerAndValue, InstrumentDataConditionalValuePosition, ConditionValueAndValue, ConditionEqualityValue, TableDataType, TableJumpMarkerValuePosition, TableJumpMarkerValue, TableJumpDestinationIndexPosition }

.function startBlock( id ) {    .return List().add( id, 0 ) }
.function addString( block, S ) {
    .for ( var i=0; i < S.size(); i++ ) .eval block.add( S.charAt( i ) )
    .eval block.add( 0 )
 }
.function addWords( block, W ) {
    .for ( var i=0; i<W.size(); i++ ) {
        .eval block.add( <( W.get( i ) ), >( W.get( i ) ) )
    }
 }
.macro dump( block, bn ) {
    .memblock bn
    .eval block.set( 1, block.size() - 2 )
    .fill block.size(), block.get( i )
 }

// SF2 Auxiliary Data:
//
// I would like to hope that AuxData won't make up more than 4k.  If that counts, it could be
// placed @$3000 - why?  Longest packed music I saw was Insanity @$1AB2 Bytes in packed size.
// So - otherwise it will be put @b500 to not interfere with music data.
//
// Types of AuxData:
.enum {		Undefined,
			EditingPreferences,
			HardwarePreferences,
			PlayMarkers,
			TableText,
			Songs
}
.function AuxBlock( Type, Version, Content ) {
    .return List().add( Type, <Version, >Version, <Content.size(), >Content.size() ).addAll( Content )
}
.function buildSongData() {
    .return AuxBlock( Songs, 1, List().add( 4, 0, 0, 0, 0, 0 ) )
}
.function buildAuxData() {
    .var ad = buildSongData()
    .return ad.add( Undefined, 0, 0, 0, 0 )
}

/* finally a macro to push out the respective PRG file */
.macro SF2_driver(  Name, Type, VersionDottedString, DriverBlock, DriverTables, InstrumentInfos,
                    MusicData, ColorRules, InsDelRules, ActionRules, DriverInstrumentDataDescriptor ) {
    // prepare text output
    .encoding "screencode_mixed"

    /* building the basic blocks: */
    .var descriptor = startBlock( ID_Descriptor )
    .eval descriptor.add( Type )
    .eval addWords( descriptor, List().add( DriverBlock.DataBlockEnd - DriverBlock.SegmentStart ) )
    .eval addString( descriptor, Name )
    .eval addWords( descriptor, List().add( DriverBlock.SegmentStart, DriverBlock.SegmentEnd - DriverBlock.SegmentStart ) )
    // read version string into bytes
    .var s = 0
    .for ( var i=0; i < VersionDottedString.size(); i++ )
        .if ( VersionDottedString.charAt( i ) == '.') { // just found "non-standard" and "demo-driver"
            .eval descriptor.add( VersionDottedString.substring( s, i ).asNumber() | $80 )
            .eval s = i+1;
        } else .if ( i + 1 == VersionDottedString.size() ) {
            .eval descriptor.add( VersionDottedString.substring( s, VersionDottedString.size() ).asNumber() )
        }

    .var drivercommon = startBlock( ID_DriverCommon )
    .eval addWords( drivercommon, List().add(
        DriverBlock.InitAddress, DriverBlock.StopAddress, DriverBlock.UpdateAddress,
        DriverBlock.SIDchannelOffsetAddress, DriverBlock.DriverStateAddress, DriverBlock.TickCounterAddress,
        DriverBlock.OrderListIndexAddress, DriverBlock.SequenceIndexAddress, DriverBlock.SequenceInUseAddress,
        DriverBlock.CurrentSequenceAddress, DriverBlock.CurrentTransposeAddress,
        DriverBlock.CurrentSequenceEventDurationAddress, DriverBlock.NextInstrumentAddress,
        DriverBlock.NextCommandAddress, DriverBlock.NextNoteAddress, DriverBlock.NextNoteIsTiedAddress,
        DriverBlock.TempoCounterAddress, DriverBlock.TriggerSyncAddress    ) )
    .eval drivercommon.add( DriverBlock.NoteEventTriggerSyncValue, 0, 0, 0 )

    .var drivertables = startBlock( ID_DriverTables )
    .for ( var i=0; i < DriverTables.size(); i++ ) {
        .const t = DriverTables.get( i )
        .eval drivertables.add( t.Type, t.ID, t.TextFieldWidth )
        .eval addString( drivertables, t.Name )
        .eval drivertables.add( t.DataLayout, t.Properties, t.InsDelID, t.CrActID, t.ColorID )
        .eval addWords( drivertables, List().add( t.Address, t.Columns, t.Rows ) )
        .eval drivertables.add( t.visRows )
    }
    .eval drivertables.add( $ff )

    .var driverinstrumentdescriptor = startBlock( ID_DriverInstrumentDescriptor )
    .eval driverinstrumentdescriptor.add( InstrumentInfos.size() )
    .for ( var i=0; i < InstrumentInfos.size(); i++ )
        .eval addString( driverinstrumentdescriptor, InstrumentInfos.get( i ) )

    .var musicdata = startBlock( ID_MusicData )
    .eval musicdata.add( MusicData.TrackCount ) /* ¿ single-sid / multi-sid ? */
    .eval addWords( musicdata, List().add( MusicData.TrackPtrsLow, MusicData.TrackPtrsHi ) )
    .eval musicdata.add( MusicData.SequenceCount )
    .eval addWords( musicdata, List().add(
        MusicData.SequencePtrsLow, MusicData.SequencePtrsHi,
        MusicData.OrderListSize, MusicData.OrderListAddr,
        MusicData.SequencesSize, MusicData.SequencesAddr    ) )

    .var tablecolorrules = startBlock( ID_TableColorRules )
    .for ( var i=0; i < ColorRules.size(); i++ ) {
        .const c = ColorRules.get( i )
        .eval tablecolorrules.add( c.EvalCellIndex, c.EvalCellMask, c.ConditionalValue, c.ConditionalColor )
    }

    .var tableinsertdeleterules = startBlock( ID_TableInsertDeleteRules )
    .for ( var i=0; i < InsDelRules.size(); i++ ) {
        .const idr = InsDelRules.get( i )
        .eval tableinsertdeleterules.add(   idr.TableID, idr.TargetCellIndex, idr.EvalCellIndex,
                                            idr.EvalCellMask, idr.ConditionalValue )
    }

    .var tableactionrules = startBlock( ID_TableActionRules )
    .for ( var i=0; i < ActionRules.size(); i++ ) {
        .const a = ActionRules.get( i )
        .eval tableactionrules.add( a.ApplicableCell, a.TargetTableID, a.TargetIndexCell,
                                    a.TargetIndexMask, a.EvalCellIndex, a.EvalCellMask, a.ConditionalValue )
    }

    .var driverinstrumentdatadescriptor = List()
    .if ( DriverInstrumentDataDescriptor.size() > 0 ) {
        .eval driverinstrumentdatadescriptor = startBlock( ID_DriverInstrumentDataDescriptor )
        .byte 9,1+10*DriverInstrumentDataDescriptor.size(), DriverInstrumentDataDescriptor.size()
        .for ( var i=0; i < DriverInstrumentDataDescriptor.size(); i++ ) {
            .const idd = DriverInstrumentDataDescriptor.get( i )
            .byte idd.TableID, idd.InstrumentDataPointerPosition, idd.PointerAndValue, idd.InstrumentDataConditionalValuePosition
            .byte idd.ConditionValueAndValue, idd.ConditionEqualityValue, idd.TableDataType, idd.TableJumpMarkerValuePosition
            .byte idd.TableJumpMarkerValue, idd.TableJumpDestinationIndexPosition
        }
    }

    .var end = List().add( $ff, 0, 0 )  // Block::ID_End, no size

    // now that all of our basic SF2-descriptors are created, we can summarize their size and pack up the file.
    .var sz = descriptor.size() + drivercommon.size() + drivertables.size() + driverinstrumentdescriptor.size() + musicdata.size() + tablecolorrules.size() + tableinsertdeleterules.size() + tableactionrules.size() + driverinstrumentdatadescriptor.size() + end.size()
    .const descriptorStart = DriverBlock.SegmentStart - sz - 2 // 0x1337 not to forget!
    .print "Descriptor-Start: $" + toHexString( descriptorStart, 4 )

    /* finally, we got all data together to "render" a driver file */
    .segment Output_Driver [start=descriptorStart, outPrg=(Name + ".prg") ]
    .memblock "1337"
    .word $1337
    dump( descriptor, "descriptor" )
    dump( drivercommon, "drivercommon" )
    dump( drivertables, "drivertables" )
    dump( driverinstrumentdescriptor, "driverinstrumentdescriptor" )
    dump( musicdata, "musicdata" )
    dump( tablecolorrules, "tablecolorrules" )
    dump( tableinsertdeleterules, "tableinsertdeleterules" )
    dump( tableactionrules, "tableactionrules" )
    dump( driverinstrumentdatadescriptor, "driverinstrumentdatadescriptor" )
    dump( end, "end" )
    .print "Driver start: $" + toHexString(*,4)
    /* Player */
    * = DriverBlock.SegmentStart
    .segmentout [ segments=DriverBlock.SegmentName ]
    /* Data */
    .if ( DriverBlock.DataBlockName.size() > 0 ) {
        * = DriverBlock.DataBlockStart
        .segmentout [ segments=DriverBlock.DataBlockName ]
    }

    // .if ( AuxiliaryData != 0 ) {
    //     * = AuxiliaryData

    //     .var aux = buildAuxData()
    //     .print "Aux-Data: $" + toHexString(*, 4) + " ... " + aux

    //     .fill aux.size(), aux.get( i )
    // }
}

.segment SF2_IRQ [start=$0900,outPrg="SF2irq.prg"]
.byte 			$a9, $00, $20, $00, $10, $78, $a2, $00
.byte			$8e, $0e, $dc, $e8, $8e, $1a, $d0, $a9
.byte			$20, $8d, $14, $03, $a9, $c0, $8d, $15
.byte			$03, $a9, $32, $8d, $12, $d0, $58, $60
.byte			$a9, $1b, $8d, $11, $d0, $ea, $ea, $ea
.byte			$ea, $ea, $ea, $ee, $20, $d0, $20, $06
.byte			$10, $ce, $20, $d0, $6e, $19, $d0, $4c
.byte			$31, $ea
