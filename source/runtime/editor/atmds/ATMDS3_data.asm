/**************************************************************************************************
/*
/*	ATMDS 3.2 Music Parser
/*	...	dies ist eigentlich kein Assembler-File, eher Javascript für KickAssembler.
/*
/*	Am Ende wird dies vermutlich der letzte Musik-Packer für die ATMDS 3.2 Reihe.  Die vorherigen
/*	Versuche in Pascal und ACME waren ja so lálá ...
/*
/*************************************************************************************************/
#import "ATMDS3_datahelfer.asm"
/**************************************************************************************************
/*	Zuerst definiere ich das Speicher-Layout:	Da ich es hier mit "bei Zuweisung typisierten" Var-
/*	-----------------------------------------	iablen zu tun habe, geht das noch recht einfach.
/*************************************************************************************************/
// Gesamtpaket: im C64 von $403c-$b400
.struct verbosity { tabs, secs, trks, sort }
.var v = verbosity( false, 0, true, true ).lock()
.var dmp = createFile( "sectors.txt" )
.function bytesString( startText, bytes ) {
	.var t = startText
	.for ( var i=0; i<bytes.size(); i++ ) .eval t += " " + toHexString( bytes.get ( i ), 2 )
	.return t
	}
.struct ATMDS_mem {	SpeedTable, TuneData, GlideTable,
					AD, SR, AP, PP, FP, V1, V2, V3, HR, H1, H2,
					WaveTable, TransTable, Pulse1, Pulse2, Filter1, Filter2,
					Tracks,	Sectors	}
// TuneDatas: C64-eigene Zeiger auf die Tracks.  Dazu "Speed" und "Volume".  Leider fehlen die
// Multiplay-Informationen hier, die stecken in der SpeedTable und werden nur vom Editor verwendet.
.struct ATMDS_td { Trk1Id, Trk2Id, Trk3Id, Speed, Volume }
.macro tune_datas( td ) {
	.word $4800 + 256*td.get( 0 ), $4800 + 256*td.get( 1 ), $4800 + 256*td.get( 2 )
	.byte td.get( 3 ), td.get( 4 )	}

// leere Editordaten
.function ATMDS3_emptyTracks() {
	.var l = List( 3*4 )
	.for( var id=0; id < 3*4; id++ ) {
		.eval l.set( id, e100( $ff ) )
	}
	.return l	}
.function ATMDS3_emptySectors() {
	.var l = List( $60 )
	.for( var id=0; id < $60; id++ ) {
		.eval l.set( id, e100( $fe ) )
	}
	.return l	}

.function ATMDS3_emptyData() {
	.return ATMDS_mem(
		List().add( 0, 0, 0, 0 ),
		List().add( ATMDS_td( 0, 1, 2, 4, 15 ), ATMDS_td( 3, 4, 5, 4, 15 ), ATMDS_td( 6, 7, 8, 4, 15 ), ATMDS_td( 9, 10, 11, 4, 15 ) ), e20(), e20(), e20(), e20(), e20(), e20(), e20(), e20(), e20(), e20(), e20(), e20(),
		e100(), e100(), e100(), e100(), e100(), e100(),
		ATMDS3_emptyTracks(),
		ATMDS3_emptySectors() )
}
.function ATMDS3_load_file( filename ) {
	.var m = ATMDS_mem()
	.var f = LoadBinary( filename )
	.var sa = fread_w( f, 0 )
	.var data = List();
	.if ( sa >= $4000 && sa <= $4040 ) {
		.print "ATMDS3_read_file(" + filename + ") ... seems 2B an editor-dump."
		// in diesem Falle sollten wir von einem Memory-Dump des Trackers ausgehen
		.var off = $403c - sa + 2
		.var addr = $403c
		.var fs = f.getSize() - off
		.while ( fs > 0 ) {
			.eval data.add( f.uget( off++ ) )
			.eval fs--
			.eval addr++
		}
		.while ( addr < $5400 ) {
			.eval data.add( 0 )
			.eval addr++
		}
		.while ( addr < $b400 ) {
			.eval data.add( $fe )
			.eval addr++
		}
	} else .if ( sa > $413c && sa < $b500 ) {
		.var addr = fread_w( f, 2 )	// Zieladresse
		.var ctrl = f.uget( 4 )		// Controlbyte für RLE
		.var pos = 5
		.var end = f.size()
		// vermutlich ein CUNP-File - also RLE-Encodiert
		.print "ATMDS3_read_file(" + filename + ") ... seems 2B a 'CUNP'-file, CTRL: " + ctrl
		.if ( addr > $403c) .for ( var i=0; i < addr-$403c; i++ ) .eval data.add( 0 )
		.while ( pos < end ) {
			.var b = f.uget( pos++ )
			.if ( b == ctrl ) {
				.var cnt = f.uget( pos++ )
				.if ( cnt > 0 ) {
					.eval b = f.uget( pos++ )
					.for ( var x = cnt; x > 0; x-- ) .eval data.add( b )
				} else .eval pos = end
			} else .eval data.add( b )
		}
		.print "...unpacked $" + toHexString( data.size(), 4 ) + " Bytes."
	} else {
		// Andere Arten, Musikdateien zu laden ...
	}
	.if ( data.size() == $b400 - $403c ) {
		.eval m.set( "SpeedTable", List().add( data.get( 0 ), data.get( 1 ), data.get( 2 ), data.get( 3 ) ) )
		.var off = 4
		// Tunedata zusammenbauen
		.var td = List() ; .for ( var i=0; i<4; i++ ) {
			.var dat = ATMDS_td( >( read_w( data, off ) - $4800 ), >( read_w( data, off+2 ) - $4800 ), >( read_w( data, off+4 ) - $4800 ), data.get( off+6 ), data.get( off+7 ) )
			.eval off = off + 8
			.eval td.add( dat )
		}
		.eval m.set( "TuneData", td )
		.eval m.GlideTable = List()
		.for ( var i=0; i<$20; i++ ) .eval m.GlideTable.add( read_wo( data, off+i, $20 ) )
		.eval off = off + $40
		.eval m.AD			= read_20( data, off ) ;		.eval off += $20
		.eval m.SR			= read_20( data, off ) ;		.eval off += $20
		.eval m.AP			= read_20( data, off ) ;		.eval off += $20
		.eval m.PP			= read_20( data, off ) ;		.eval off += $20
		.eval m.FP			= read_20( data, off ) ;		.eval off += $20
		.eval m.V1			= read_20( data, off ) ;		.eval off += $20
		.eval m.V2			= read_20( data, off ) ;		.eval off += $20
		.eval m.V3			= read_20( data, off ) ;		.eval off += $20
		.eval m.HR			= read_20( data, off ) ;		.eval off += $20
		.eval m.H1			= read_20( data, off ) ;		.eval off += $20
		.eval m.H2			= read_20( data, off ) ;		.eval off += $20
		.eval m.WaveTable 	= read_page_full( data, off ) ;	.eval off += $100
		.eval m.TransTable 	= read_page_full( data, off ) ;	.eval off += $100
		.eval m.Pulse1 		= read_page_full( data, off ) ;	.eval off += $100
		.eval m.Pulse2 		= read_page_full( data, off ) ;	.eval off += $100
		.eval m.Filter1 	= read_page_full( data, off ) ;	.eval off += $100
		.eval m.Filter2 	= read_page_full( data, off ) ;	.eval off += $100
		// Theoretisch sollten jetzt die 12 Tracks folgen
		.eval m.Tracks = List()
		.for ( var i=0; i<12; i++ ) {
			.eval m.Tracks.add( read_page_full( data, off ) )
			.eval off = off + $100
		}
		// Nun kommen noch so lange Sektoren, wie da eben Sektoren kommen.
		.eval m.Sectors = List()
		.for ( var i=0; i < $60; i++ ) {
			.eval m.Sectors.add( read_page_full( data, off ) )
			.eval off += 256
		}
	}
	.errorif ( m.get( "Sectors" ).size() < $60 ), "not enough Sektors for a complete ATMDS 3 Music."
	.return m
}
.function ATMDS3_Track_bytes( t ) {
	.var i = t.size()-1
	.while ( i >= 0 && t.get(i) < $fe ) .eval i--
	.if ( i > 0 && t.get( i ) == $fe ) .eval i++
	.return i+1
}

/*	Grundsätzliche Ideen:
	Alle diese Tabellen haben irgendwo einen Aufbau, der aus Grundeinheiten des Taktes besteht.
	Deshalb sollten sie entsprechend auch zerlegbar sein.  Das wäre hier die Aufgabe.  Mit dieser
	Lösung würde sich ein "Packen" auch nicht mehr lohnen, da eh nur inkludiert wird, was ver-
	wendet wird.
	Tracks: Wir suchen immer nach dem nächsten Sektor (oder Sprung/Ende) - alles davor ist sozu-
	sagen "Einstellungen für den Sektor treffen" - wie Transpose und Repeat, oder aber Einstell-
	ungen für den Song - Vol, Spd, Sng. - wobei "Sng" eher in die Kategorie von Sprung/Ende ge-
	hört.
	Ein Track-Item erhält also einen Transpose-Wert und einen Repeat-Wert einen Song- und einen
	Spd-Wert.  Damit haben wir das Track schon zerlegt.
*/
.struct Song {	Voice1Track, Voice2Track, Voice3Track, TickSpeed, InitialVolume, MultiplaySpeed, MultiplayRatio }
.enum { playable, controlEnd, controlJump, controlSong }
.struct TrackBlock { Type, Transpose, Repeat, Sector, Speed, VolumeOrControl }
.function ATMDS3_interpretTrack( mem ) {
	// mem ist eine List() aus 256 Bytes!
	.var pos = 0
	.var trk = List()
	.while ( pos < 256 ) {
		.var it = TrackBlock()
		.while ( it.Type == null ) {
			.var b = mem.get( pos++ )
			.if ( b < $60 ) {			.eval it.Sector = b
										.eval it.Type = playable
			} else .if ( b < $80 ) {	.eval it.Speed = ( b & 31 )
			} else .if ( b < $c0 ) {	.eval it.Transpose = ( b & 15 ) * (b < $a0 ? 1 : -1)
			} else .if ( b < $e0 ) {	.eval it.Repeat = ( b & $1f )
			} else .if ( b < $f0 ) {	.eval it.VolumeOrControl = ( b & 15 )
			} else .if ( b < $f4 ) {	.eval it.VolumeOrControl = ( b & 3 )
										.eval it.Type = controlSong
			} else .if ( b == $fe ) {	.eval it.VolumeOrControl = mem.get( pos++ )
										.eval it.Type = controlJump
			} else .if ( b == $ff ) {	.eval it.Type = controlEnd	}
			.errorif ( pos > 255 && it.Type == null ), "Konnte keinen Block vor Ende des Tracks finden."
		}
		.eval trk.add( it )
		.if ( it.Type != playable ) .eval pos = 256
	}
	// Letzter Eintrag sollte vom Typ "control" sein.  Ist er das nicht, ist das Track vmtl. leer
	// oder mit Junk gefüllt.
	.if ( trk.get( trk.size() - 1 ).Type == playable ) .eval trk = List()
	.return trk
}

.enum { note, wait, skip, off, end }				// SectorBar.Type
.var SBT = Hashtable().put( note, "note", wait, "wait", skip, "skip", off, "off", end, "end" )
.enum { ad, sr, pp, fp, ff, tie, glide }	// BarModifier.Type
.var BMT = Hashtable().put( ad, "ad", sr, "sr", pp, "pp", fp, "fp", ff, "ff", tie, "tie", glide, "glide" )
.struct BarModifier { Type, Parameter, Target }
.struct SectorBar { Type, Instrument, Duration, Note, Modifiers }
.struct PState { Ins, Dur, Tie }
.struct Sector{ InitialPState, Bars, OldData }
.function barStr( b ) {
	.var res = SBT.get( b.Type ) + ": " + b.Note + ", i=" + b.Instrument + ", d=" + b.Duration
	.for (var i=0; i<b.Modifiers.size(); i++ ) {
		.eval res += (i==0?", MOD( ":", ") + BMT.get(b.Modifiers.get( i ).Type) + "=" + b.Modifiers.get( i ).Parameter + " (" + b.Modifiers.get( i ).Target + ")"
	}
	.return res + (b.Modifiers.size() > 0 ? " )" : "" )
}
.function ATMDS3_interpretSector( mem, secnum, initialState ) {
	.var pos = 0
	.var lastIns = initialState.Ins
	.var lastDur = initialState.Dur
	.var tieState = initialState.Tie
	.if ( v.secs > 0 ) .print "... interpret sector #" + toHexString( secnum, 2 ) + ", initial state: ins=" + lastIns + ", dur=" + lastDur + ", tie=" + tieState
	.var bars = List()
	.while ( pos < 256 ) {
		// Der Sektor-Interpreter ...
		.var p0 = pos
		.var bar = SectorBar()
		.eval bar.Modifiers = List()
		.while ( bar.Type == null && pos < 256 ) {
			.var b = mem.get( pos++ )
			.if ( b < $60 ) {			.eval bar.Note = b ; .eval bar.Type = note
			} else .if ( b < $80 ) {	// Glide
				.eval bar.Note = mem.get( pos++ )
				.eval bar.Modifiers.add( BarModifier( glide, ( b & $1f ), mem.get( pos++ ) ) )
				.eval bar.Type = note
			} else .if ( b < $c0 ) {	.eval bar.Duration = lastDur = ( b & $3f )
			} else .if ( b < $e0 ) {	.eval bar.Instrument = lastIns = ( b & $1f )
			} else .if ( b == $f6 ) {	.eval bar.Modifiers.add( BarModifier( fp, mem.get( pos++ ), null ) )
			} else .if ( b == $f7 ) {	.eval bar.Modifiers.add( BarModifier( pp, mem.get( pos++ ), null ) )
			} else .if ( b == $f8 ) {	.eval bar.Modifiers.add( BarModifier( ff, mem.get( pos++ ), null ) )
			} else .if ( b == $f9 ) {	.eval bar.Modifiers.add( BarModifier( sr, mem.get( pos++ ), null ) )
			} else .if ( b == $fa ) {	.eval bar.Modifiers.add( BarModifier( ad, mem.get( pos++ ), null ) )
			} else .if ( b == $fb ) {	.eval bar.Modifiers.add( BarModifier( tie, null, null ) ) ; .eval tieState = !tieState
			} else .if ( b == $fc ) {	.eval bar.Type = off
			} else .if ( b == $fd ) {	.eval bar.Type = skip
			} else .if ( b == $fe ) {	.eval bar.Type = wait
			} else .if ( b == $ff ) {	.eval bar.Type = end	}
		}
		.var err = ""
		.if ( bar.Type != end ) {
			.if ( bar.Duration == null ) {
				.if ( lastDur < 0 )	.eval err = "bar without Duration"
				else  				.eval bar.Duration = lastDur
			}
			.if ( bar.Instrument == null ) {
				.if ( lastIns < 0 ) .eval err = "bar without Instrument"
				else 				.eval bar.Instrument = lastIns
			}
		}
		.eval bars.add( bar )
		.if ( v.secs > 1 ) {
			.var bts = ""
			.for ( var i=p0; i<pos; i++ ) .eval bts += toHexString( mem.get( i ), 2 )+" "
			.while ( bts.size() < 20 ) .eval bts += " "
			.print "        #" + toHexString( bars.size()-1, 2 ) + ": " + bts + "=> " + barStr( bar )
		}
		.if ( err.size() > 0 ) {
			.if ( v.secs > 0 ) .print "    error reading sector: '" + err + "' @ pos=$" + toHexString( pos, 2 ) + ", bar#" + toHexString( bars.size(), 2 )
		}
		.if ( bar.Type == end ) .eval pos = 256
	}
	.var res = Sector( initialState, bars, mem )
	.eval initialState.Ins = lastIns ; .eval initialState.Dur = lastDur ; .eval initialState.Tie = tieState
	.return res
}

.struct Instrument { AD, SR, WP, PP, FP, V1, V2, V3, HR, H1, H2 }
.function ATMDS3_readInstrument( mem, i ) {
	.return Instrument(	mem.AD.get( i ), mem.SR.get( i ),
			mem.AP.get( i ), mem.PP.get( i ), mem.FP.get( i ),
			mem.V1.get( i ), mem.V2.get( i ), mem.V3.get( i ),
			mem.HR.get( i ), mem.H1.get( i ), mem.H2.get( i ) )
}

.struct WaveLine { WaveForm, Gate, Sync, Ring, DrumMode, Offset, nextId }
.function ATMDS3_readWaveProg( mem, pos ) {
	// Ich lese die Wavetable Zeile für Zeile aus und erstelle daraus je eine WaveLine.
	// Sobald ein Loop oder End gefunden wird, wird nur noch die entsprechende nextId
	// (im Eintrag davor) geändert (bei end: -1)
	.var tp = pos
	.var lines = List()
	.var ids = Hashtable()
	.var go = true
	.while ( go ) {
		.var wv = mem.WaveTable.get( tp )
		.var tv = mem.TransTable.get( tp )
		.eval ids.put( tp++, lines.size() )
		.var ni = ids.keys().size()
		.if ( mem.WaveTable.get( tp ) >= $90 ) {
			.eval ni = ( mem.WaveTable.get( tp ) == $91 ? -1 : ids.get( mem.TransTable.get( tp ) ) )
			// Spezialfall, weil das mit einfachen Tabellen möglich war/ist, mit den Programmen
			// wiederum eher nicht - kann im 4er Player dann via Unterfunktionen nachgestellt werden!
			.if ( ni == null ) {
				// Hier liegt der Spezialfall vor, dass ein Instrument Teile eines anderen Instru
				// -mentes mit verwendet.  Sozusagem wie ein Unterfunktionsloop im Ausklang oder so.
				.eval ni = ids.keys().size()
				.eval tp = mem.TransTable.get( tp )
			} else .eval go = false	// Loop oder End. ni ist korrekt gesetzt
		}
		.eval lines.add( WaveLine( wv & $f0, ((wv & 1)!=0), ((wv & 2)!=0), ((wv & 4)!=0), ((wv & 8)!=0), tv, ni ) )
	}
	.return lines
}
.struct TableId { PrgLine, Program }
.struct PulseLine { AddValue, Count, nextId }
.struct Pulse { InitialPulse, Lines }
.function ATMDS3_readPulseProg( mem, pos, ids, prg ) {
	.var t1 = mem.Pulse1
	.var t2 = mem.Pulse2
	.eval ids.put( pos, TableId( -1, prg ) )
	.var lns = List()
	.var ini = ( t1.get( pos ) << 8 ) + t2.get( pos );
	.if ( v.tabs ) .print "... read pulse_program #$" + toHexString( prg, 2 ) + " - $" + toHexString( pos, 2 ) + "#pos : " + toHexString( t1.get( pos ), 2 ) + toHexString( t2.get( pos ), 2 ) + "     => init: pulse=$" + toHexString( ini, 4 )
	.var pp = pos+1
	.var do = true
	.while ( do ) {
		.var nxtLn = t2.get( pp+1 )
		// Pulse-Funktion: nach der Init wird immer erst einmal Count Mal Addiert. Dann wird zu
		// Ptr gesprungen. => Abbruchbedingungen - loop im gleichen Program, end durch Ptr(0)
		// ... also erst einmal diese Zeile als eingelesen markieren:
		.eval ids.put( pp, TableId( lns.size(), prg ) )
		// Ich speichere ja keine Tabellenzeiger mehr, sondern line-ids -> umrechnen!
		.var nxtId = nxtLn == 0 ? -1 : lns.size()+1
		// Daraufhin die Abbruchbedingungen prüfen
		.if ( nxtLn == 0 || (ids.containsKey( nxtLn ) && ids.get( nxtLn ).Program == prg) ) {
			.if ( nxtLn != 0 ) .eval nxtId = ids.get( nxtLn ).PrgLine
			.eval do = false
		}
		.var pl = PulseLine( ( t1.get( pp ) << 8 ) + t2.get( pp ), t1.get( pp + 1 ), nxtId )
		.if ( v.tabs ) .print "                               " + toHexString( pp, 2 ) + ":   " + toHexString( t1.get( pp + 1 ), 2 ) + "." + toHexString( t1.get( pp ), 2 ) + toHexString( t2.get( pp ), 2 ) + "::" + toHexString( t2.get( pp + 1 ), 2 ) + " => line #$" + toHexString( lns.size(), 2 ) + ": add=$" + toHexString( pl.AddValue, 4 ) + ", cnt=" + toHexString( pl.Count, 2 ) + ", nxt=" + pl.nextId
		.eval lns.add( pl )
		.eval pp = nxtLn
	}
	.return Pulse( ini, lns )
}
.struct FilterInit { Type, Resonance, Cutoff }
.struct FilterLine { AddValue, Count, Resonance, nextId }
.struct Filter { Init, Lines }
.function ATMDS3_readFilterProg( mem, pos, ids, prg ) {
	.var t1 = mem.Filter1
	.var t2 = mem.Filter2
	.var ini = FilterInit( t2.get( pos ) & $0f, ( t2.get( pos ) & $f0 ) >> 4, t1.get( pos ) << 8 )
	.if ( v.tabs ) .print "... read filter_program #$" + toHexString( prg, 2 ) + " - $" + toHexString( pos, 2 ) + "#pos : " + toHexString( t1.get( pos ), 2 ) + toHexString( t2.get( pos ), 2 ) + "     => init: T=" + ini.Type + ", R=" + ini.Resonance + ", frq=$" + toHexString( ini.Cutoff, 4 )
	.var lns = List()
	.var fp = pos+1
	.var do = true
	.while ( do ) {
		.var nxtLn = t2.get( fp+1 )
		.eval ids.put( fp, TableId( lns.size(), prg ) )
		.var nxtId = nxtLn == 0 ? -1 : lns.size()+1
		.if ( nxtId == -1 || ( ids.containsKey( nxtLn ) && ids.get( nxtLn ).Program == prg ) ) {
			.if ( nxtId != -1 ) .eval nxtId = ids.get( nxtLn ).PrgLine
			.eval do = false
		}
		.var fl = FilterLine( ( t1.get( fp ) << 8 ) + ( t2.get( fp ) & $f0 ), t1.get( fp+1 ), t2.get( fp ) & $f, nxtId )
		.if ( v.tabs ) .print "                                " + toHexString( fp, 2 ) + ":   " + toHexString( t1.get( fp+1 ), 2 ) + "." + toHexString( t1.get( fp ), 2 ) + toHexString( t2.get( fp ), 2 ) + "::" + toHexString( t2.get( fp+1 ), 2 ) + " => line #$" + toHexString( lns.size(), 2 ) + ": add=$" + toHexString( fl.AddValue, 4 ) + ", r=$" + toHexString( fl.Resonance ) + ", cnt=" + toHexString( fl.Count,2 ) + ", nxt=" + fl.nextId
		.eval lns.add( fl )
		.eval fp = nxtLn
	}
	.eval ids.put( pos, TableId( -1, prg ) )
	.return Filter( ini, lns )
}

.struct ATMDS3_Music {	SongList,	// beinhaltet schon die Tracks
					  	SectorList, GlideList,
						InstrumentList,
						WaveList, PulseList, FilterList
}

.function ATMDS3_interpretObjects( mem ) {
	/*
		Hier geht es darum, die Daten aus dem Speicherabbild zu Objekten zu machen, die sich am
		Ende ggf. leichter zusammensetzen lassen.

		Zuerst werden die Songs untersucht - genauer gesagt ihre Tracks.  Damit "tokenisiere"
		ich die genutzten Tracks und erstelle die Song-Liste.
	*/
	.print "interpret ATMDS3 editor memory dump ..."
	// lokale Variablen:
		.var songs			= List()	;	.var song_cnt		= 0
		.var sectors		= List( 96 );	.var sector_cnt		= 0
		.var glides			= List( 32 );	.var glide_cnt		= 0
		.var instruments	= List( 32 );	.var instrument_cnt	= 0
		.var waves 			= List();
		.var pulses			= List();
		.var filters		= List();
		.var up 			= 0
		.var down 			= $5f

	/*	Schleifen zum Präparieren der Elemente: Songs + Tracks
		*/
		.for ( var song=0; song<4; song++ ) {
			.var td = mem.TuneData.get( song )
			.var msp = mem.SpeedTable.get( song )
			.var t1 = ATMDS3_interpretTrack( mem.Tracks.get( td.Trk1Id ) )
			.var t2 = ATMDS3_interpretTrack( mem.Tracks.get( td.Trk2Id ) )
			.var t3 = ATMDS3_interpretTrack( mem.Tracks.get( td.Trk3Id ) )
			.if ( t1.size() + t2.size() + t3.size() > 0 ) {
				.eval songs.add( Song( t1, t2, t3, td.Speed, td.Volume, msp & 15, (msp >> 4) & 15 ) )
				.eval song_cnt++
			}
		}
		.print "... found " + song_cnt + " songs."
	/*	Die Sektoren:
		Die Tracks sind schon da, also gehe ich sie durch und erstelle "on the fly" die passenden
		Sektoren.	*/
		.for ( var song=0; song<songs.size(); song++ ) {
			.for ( var t=0; t<3; t++ ) {
				.var trk = songs.get( song ).get( t )
				.var st = PState( 0, 0, false )
				.for ( var tp=0; tp < trk.size(); tp++ ) {
					.var trkblk = trk.get( tp )
					// Sektor gefunden ... welche ID, schon hinzugefügt?
					.if ( trkblk.Type == playable ) {
						.var se = sectors.get( trkblk.Sector )
 						.if ( null == se ) {
							// neuer Sektor gefunden
							.eval sectors.set( trkblk.Sector, ATMDS3_interpretSector( mem.Sectors.get( trkblk.Sector ), trkblk.Sector, st ) )
							.if ( sectors.get( trkblk.Sector ).Bars.size() > 0 ) .eval sector_cnt++
						} else {
							// Sektor wurd schon einmal gefunden -> ggf. sind nun die Initial States nicht mehr gleich!
							.if ( st.Ins != se.InitialPState.Ins ) .eval sectors.get( trkblk.Sector ).InitialPState.Ins = -1
							.if ( st.Dur != se.InitialPState.Dur ) .eval sectors.get( trkblk.Sector ).InitialPState.Dur = -1
							.if ( st.Tie != se.InitialPState.Tie ) .error "initial Tie States differ - sector #$" + toHexString( trkblk.Sector, 2 ) + " must sound pretty strange sometimes."
						}
					}
				}
			}
		}
		.print "... found " + sector_cnt + " ($" + toHexString( sector_cnt, 2 ) + ") sectors."
	/*	Sektor-IDs ausdünnen.  Sprich: übersprungene Sektoren überschreiben ...
		*/
		.while ( up < down ) {
			.while ( up < down && null != sectors.get( up ) ) .eval up++
			.while ( up < down && null == sectors.get( down ) ) .eval down--
			.if ( up < down ) {
				.print "... replace Sector IDs " + toHexString( down, 2 ) + " => " + toHexString( up, 2 ) + ( v.secs > 0 ? ":" : "." )
				// up ist leer, down aber voll ... einfach vertauschen, ja ...
				.eval sectors.set( up, sectors.get( down ) )
				.eval sectors.set( down, null )
				// jetzt aber noch durch die Tracks durchmarschieren
				// und alle Referenzen ändern ...
				.for ( var s=0; s<songs.size(); s++ ) .for ( var t=0; t<3; t++) {
					.var trk = songs.get( s ).get( t )
					.for ( var r=0; r<trk.size(); r++ ) {
						.var row = trk.get( r );
						.if ( row.Type == playable && row.Sector == down ) {
							.eval row.Sector = up
							.eval trk.set( r, row )
							.if ( v.secs > 0 ) .print "    -> in Song #" + s + " : voice" + (t+1) + " : row$" + toHexString( r, 2 )
						}
					}
				}
			} else .if ( v.secs > 0 ) .print "... no (more) sector ID replacements neccessary."
		}
	/*	Glide-Tabelle:	Durchsuchen der Sektoren nach Glide-Modifiern.
		*/
		.for ( var s = 0; s < $60; s++ ) .if ( null != sectors.get( s ) ) {
			.var sec = sectors.get( s )
			.for ( var b=0; b<sec.Bars.size(); b++ )
				.if ( null != sec.Bars.get( b ).Modifiers )
					.for ( var m=0; m<sec.Bars.get( b ).Modifiers.size(); m++ ) {
						.var mod = sec.Bars.get( b ).Modifiers.get( m )
						// noch unbenutzten Glide-Effekt gefunden ?
						.if ( mod.Type == glide && glides.get( mod.Parameter ) == null ) {
							.eval glides.set( mod.Parameter, mem.GlideTable.get( mod.Parameter ) )
							.eval glide_cnt++
				}
			}
		}
		.print "... found " + glide_cnt + " glide-effects."
		.eval up = 0 ; .eval down = $1f
		.while ( up < down ) {
			.while ( up < down && null != glides.get( up ) ) .eval up++
			.while ( up < down && null == glides.get( down ) ) .eval down--
			.if ( up < down ) {
				.print "... replace Glide FXs " + down + " => " + up + ( v.tabs ? ":" : "." )
				// up ist leer, down aber voll ... einfach vertauschen, ja ...
				.eval glides.set( up, glides.get( down ) )
				.eval glides.set( down, null )
				// jetzt aber noch durch die Sektoren durchmarschieren
				// und alle Referenzen ändern ...
				.for ( var s=0; s<$60; s++ ) {
					.var sec = sectors.get( s )
					.if ( null != sec ) {
						.for ( var b=0; b<sec.size(); b++ ) {
							.var bar = sec.get( b ); .var m=0
							.if ( bar.Type == note && bar.Modifiers.size() > 0 ) {
								.while ( m < bar.Modifiers.size() )
									.if ( bar.Modifiers.get( m ).Type == glide && bar.Modifiers.get( m ).Parameter == down ) {
										.eval bar.Modifiers.get( m ).Parameter = up
										.if ( v.tabs ) .print "    -> in sector " + s + ", bar #" + b + " ..."
										.eval m++
									} else { .eval m++ }
							}
						}
						// Originalsektor aktualisieren
						.for ( var p=0; p<sec.OldData.size(); p++ ) {
							.var b = sec.OldData.get( p )	// Glide aktualisieren
							.if ( b == $60 | down ) {		.eval sec.OldData.set( p, $60 | up )
							} else .if ( b >= $f6 && b < $fb ) .eval p++ // Parameter überspringen
						}
					}
				}
			} else .if ( v.tabs ) .print "... no (more) glide FX replacements neccessary."
		}
	/*	die Instrumente einsammeln:
		*/
		.for ( var s = 0; s < $60; s++ ) .if ( null != sectors.get( s ) ) {
			.var sec = sectors.get( s ).Bars
			.for ( var b=0; b<sec.size(); b++ ) {
				.var ba = sec.get( b )
				.if ( ba.Type == note ) {
					.var bi = ba.Instrument
					.if ( null != bi && bi > -1 && null == instruments.get( bi ) ) {
						.eval instruments.set( bi, ATMDS3_readInstrument( mem, bi ) )
						.eval instrument_cnt++
					}
				}
			}
		}
		.print "... found " + instrument_cnt + " instruments."
		.eval up = 0 ; .eval down = $1f
		.while ( up < down ) {
			.while ( up < down && null != instruments.get( up ) ) .eval up++
			.while ( up < down && null == instruments.get( down ) ) .eval down--
			.if ( up < down ) {
				.print "... replace Instrument IDs " + down + "=>" + up + ( v.tabs ? ":" : "." )
				.eval instruments.set( up, instruments.get( down ) )
				.eval instruments.set( down, null )
				// jetzt aber noch durch die Sektoren durchmarschieren
				// und alle Referenzen ändern ...
				.for ( var s=0; s<$60; s++ ) {
					.var sec = sectors.get( s )
					.if ( sec != null ) {
						.for ( var b=0; b<sec.size(); b++ ) {
							.var bar = sec.get( b ); .var m=0
							.if ( bar.Type == note && bar.Instrument == down ) {
								.eval sectors.get( s ).get( b ).Instrument = up
								.if ( v.tabs ) .print "    -> in sector " + s + ", bar #" + b + " ..."
							}
						}
						// Originaldaten anpassen
						.for ( var p=0; p<sec.OldData.size(); p++ ) {
							.var b = sec.OldData.get( p )
							.if ( b == $c0 | down ) {	.eval sectors.get( s ).OldData.set( p, $c0 | up )
							} else .if ( b >= $f6 && b < $fb ) .eval p++
						}
					}
				}
			} else .if ( v.tabs ) .print "... no (more) instrument ID replacements neccessary."
		}
	/*	Bleiben noch die Tabellen übrig.  Wave-Programme werden nur von Instrumenten referenziert.
		Pulse und Filterprogramme hingegen können auch im Sektor durch Modifier referenziert werden
		*/
		.var wptrs = Hashtable()	;	.var pptrs = Hashtable()	;	.var fptrs = Hashtable()
		.var pprg = 0	;	.var fprg = 0
		// Instrumente nach Programmen durchforsten und gefundene Programme aufnehmen
		// Hierbei werden die Tabellenzeiger gleich durch ID-Referenzen auf die jeweils
		// erstellten Programme ersetzt!
		.for ( var i=0; i<instrument_cnt; i++ ) {
			.var id = waves.size()
			.var wp = instruments.get( i ).WP
			.if ( !wptrs.containsKey( wp ) ) {
				.eval waves.add( ATMDS3_readWaveProg( mem, wp ) )
				.eval wptrs.put( wp, id )
			} else .eval id = wptrs.get( wp )
			.eval instruments.get( i ).WP = id
			.eval id = pulses.size()
			.var pp = instruments.get( i ).PP
			.if ( pp > 0 && pp < $ff ) {
				.if ( !pptrs.containsKey( pp ) ) {
					.eval pulses.add( ATMDS3_readPulseProg( mem, pp, pptrs, pprg++ ) )
					.if ( v.tabs ) .print "... pulse #" + (pprg-1) + ": " + pulses.get( pprg-1 ).Lines.size() + " Lines, used by Instrument #" + i
				} else .eval id = pptrs.get( pp ).Program
			} else .eval id = ( pp == 0 ? -1 : -2 )
			.eval instruments.get( i ).PP = id
			.eval id = filters.size()
			.var fp = instruments.get( i ).FP
			.if ( fp > 0 && fp < $ff ) {
				.if ( !fptrs.containsKey( fp ) ) {
					.eval filters.add( ATMDS3_readFilterProg( mem, fp, fptrs, fprg++ ) )
					.if ( v.tabs ) .print "... filter #" + (fprg-1) + ": " + filters.get( fprg-1 ).Lines.size() + " Lines, used by Instrument #" + i
				} else .eval id = fptrs.get( fp ).Program
			} else .eval id = ( fp == 0 ? -1 : -2 )
			.eval instruments.get( i ).FP = id
		}
		// Sektoren nach Programmen durchforsten und gefundene Programme aufnehmen und IDs
		// entsprechend ersetzen
		.for ( var s=0; s<sector_cnt; s++ ) {
			.var sec = sectors.get( s ).Bars
			.for ( var b=0; b<sec.size(); b++ ) {
				.var bar = sec.get( b )
				.for ( var m=0; m<bar.Modifiers.size(); m++ ) {
					.var mod = bar.Modifiers.get( m )
					.if ( mod.Type == pp ) {
						.var pp = mod.Parameter
						.var id = pulses.size()
						.if ( pp > 0 && pp < $ff ) {
							.if ( !pptrs.containsKey( pp ) ) {
								.eval pulses.add( ATMDS3_readPulseProg( mem, pp, pptrs, pprg++ ) )
								.if ( v.tabs ) .print "... pulse #" + (pprg-1) + ": " + pulses.get( pprg-1 ).Lines.size() + " Lines, used in Sector #" + toHexString( s,2 ) + ":" + toHexString( b,2 )
							} else .eval id = pptrs.get( pp ).Program
						} else .eval id = ( pp == 0 ? -1 : -2 )
						.eval mod.Parameter = id
					} else .if ( mod.Type == fp ) {
						.var fp = mod.Parameter
						.var id = filters.size()
						.if ( fp > 0 && fp < $ff ) {
							.if ( !fptrs.containsKey( fp ) ) {
								.eval filters.add( ATMDS3_readFilterProg( mem, fp, fptrs, fprg++ ) )
								.if ( v.tabs ) .print "... filter #" + (fprg-1) + ": " + filters.get( fprg-1 ).Lines.size() + " Lines, used in Sector #" + toHexString( s,2 ) + ":" + toHexString( b,2 )
							} else .eval id = fptrs.get( fp ).Program
						} else .eval id = ( fp == 0 ? -1 : -2 )
						.eval mod.Parameter = id
					}
				}
			}
		}
		.print "... found " + waves.size() + " Wave Table Programs."
		.print "... found " + pulses.size() + " Pulse Table Programs."
		.print "... found " + filters.size() + " Filter Table Programs."
	/*	Alle Objekte sollten jetzt erstellt sein.  Damit kann die Musik zusammen gebaut werden!
		*/
	.return ATMDS3_Music( songs, sectors, glides, instruments, waves, pulses, filters )
}

.enum { no_problem, acceptable, inacceptable }
.enum { Wave, Trans, P1, P2, F1, F2, SecL, SecH, AD, SR, WP, PP, FP, V1, V2, V3, HR, H1, H2, Track, Sector, Glide, TD, ST }
.const CTL = Hashtable().put( Wave, "Wave", Trans, "Trans", P1, "P1", P2, "P2", F1, "F1", F2, "F2", SecL, "SecL", SecH, "SecH", AD, "AD", SR, "SR", WP, "WP", PP, "PP", FP, "FP", V1, "V1", V2, "V2", V3, "V3", HR, "HR", H1, "H1", H2, "H2", Track, "Track", Sector, "Sector", Glide, "Glide", TD, "TD", ST, "ST" )
.struct Chunk { Type, Bytes, Id, PageBoundaryCrossings }
.function chunkStr( chunk ) {
	.var res = "t:" + CTL.get( chunk.Type ) + ", id:" + chunk.Id + ", size:" + chunk.Bytes.size()
	.for ( var i=0; i<chunk.Bytes.size(); i++ ) {
		.if ( null == chunk.Bytes.get( i ) ) .eval res += " bad byte @$" + toHexString(i,2)
	}
	.return res
}
.function listIns( l, p, v ) {
	.if ( p < l.size() ) {
		.if ( p < 0 ) .eval p = 0
		.var list = List()
		.for ( var i=0;i<p;i++ ) .eval list.add( l.get( i ) )
		.eval list.add( v )
		.for ( var i=p;i<l.size();i++ ) .eval list.add( l.get( i ) )
		.return list
	} else {
		.eval l.add( v )
		.return l
	}
}
.function makeAvailable( avail, sizes, size_index ) {
	.var si = 0 ; .var seeking = true
	.while ( seeking ) {
		.if ( si >= avail.size() ) {
			.eval avail.add( size_index )
			.eval seeking = false
		} else .if ( sizes.get( size_index ) >= sizes.get( avail.get( si ) ) ) {
			.eval avail = listIns( avail, si, size_index )
			.eval seeking = false
		} else .eval si++
	}
	.return avail
}
.function ATMDS3_pack( md, adr ) {
/* Grundsätzliche Ideen:
	- eine interpretierte Musik und die Pack-Startadresse wird geliefert.
	- zuerst müssen die Musikbausteine wieder zurück in Tabellen bzw. Speicherblöcke
		zerlegt werden.  Diese haben dann Längen und eine Art Priorität:
	- Page Boundary Crossings sind die Crux für rechenzeitarme Musikwiedergabe.  Je mehr Daten
		schlecht im Speicher positioniert sind, desto häufiger kommen extra-Zyklen zur Rechenzeit
		hinzu.
	- Hierbei lohnt es sich, etwas tiefgründiger nachzudenken.  Möglicherweise sind die Daten
		einfach nicht so perfekt zu packen, dass keine PageBoundary überschritten wird.  Das ist
		kaum zu verhindern.  Dennoch wirken sich die Übertritte unterschiedlich aus:
		-	Daten, die bei jedem Aufruf "angefasst" werden (könnten), sollten innerhalb ihrer
			jeweiligen Pages liegen ->	die 6 Tabellen beispielsweise.
		-	Daten, die regelmäßig bei rechenzeitintensiven Vorgängen genutzt werden sollten
			möglichst ebenso in ihre Page passen ->	Instrumenten-Tabellen
		-	Sektoren mit PBCs wirken sich unterschiedlich aus, je nachdem, wie weit hinten die
			Crossing ist.  Hier kann man ggf. einfach akzeptieren, wenn es zu crossings kommt.
		-	Genau so akzeptabel ist es bei Tracks - wobei ich mir noch nicht sicher bin, ob das
			stimmt.  Dennoch wird auf Tracks seltener zugegriffen, sodass ggf. dort eine PBC noch
			weniger Auswirkung zeigt.
		-	Zuletzt die Glide-Tabellen und die TuneDatas - diese sollten zuerst verwendet werden,
			um Pages zu "überfüllen", wenn sie nicht anders auffüllbar sind.
			-	allerdings könnten die Glide-Tabellen nicht existent sein - damit fallen sie als
				"Füllmaterial" dann wieder weg.  Die entsprechenden Speicherblöcke sollten dann
				als erstes "geschrieben" werden, damit sie zum Auffüllen nicht in Frage kommen
/**************************************************************************************************
	Arbeitsschritte:
	-	Speicherblöcke erstellen
	-	Speicherblöcke sortieren und ggf. Inhalte aktualisieren
	-	Speicherblöcke zurückgeben, sodass sie an den Player angehängt werden können.
*/
	.var chunks = List()	// Chunks placed ...
	.if ( md.getStructName() == "ATMDS_mem" ) {
		.eval chunks.add( Chunk( ST, md.SpeedTable, 0, no_problem ) )
		.var tdc = Chunk( TD, List(), 0, no_problem )
		.for ( var i=0;i<4;i++ ) {
			.var td = md.TuneData.get( i )
			.eval tdc.Bytes.add( 0, $48 + td.Trk1Id, 0, $48 + td.Trk2Id, 0, $48 + td.Trk3Id, td.Speed, td.Volume )
		}
		.eval chunks.add( tdc )
		.var gl = Chunk( Glide, List(), 0, no_problem )
		.var gh = Chunk( Glide, List(), 1, no_problem )
		.for ( var i=0;i<$20;i++ ) {
			.eval gl.Bytes.add( <md.GlideTable.get( i ) )
			.eval gh.Bytes.add( >md.GlideTable.get( i ) )
		}
		.eval chunks.add( gl, gh,	// Instrumente und Tabellen
			Chunk( AD, md.AD, 0, no_problem ), Chunk( SR, md.SR, 0, no_problem ),
			Chunk( WP, md.AP, 0, no_problem ), Chunk( PP, md.PP, 0, no_problem ),
			Chunk( FP, md.FP, 0, no_problem ), Chunk( V1, md.V1, 0, no_problem ),
			Chunk( V2, md.V2, 0, no_problem ), Chunk( V3, md.V3, 0, no_problem ),
			Chunk( HR, md.HR, 0, no_problem ),
			Chunk( H1, md.H1, 0, no_problem ), Chunk( H2, md.H2, 0, no_problem ),
			Chunk( Wave, md.WaveTable, 0, no_problem ), Chunk( Trans, md.TransTable, 0, no_problem ),
			Chunk( P1, md.Pulse1, 0, no_problem ), Chunk( P2, md.Pulse2, 0, no_problem ),
			Chunk( F1, md.Filter1, 0, no_problem ), Chunk( F2, md.Filter2, 0, no_problem )
		)
		.for ( var i=0; i<12; i++ ) {
			.eval chunks.add( Chunk( Track, md.Tracks.get( i ), i, no_problem ) )
		}
		.var sl = List()
		.var sh = List()
		.for ( var i=0; i<$60; i++ ) {
			.eval chunks.add( Chunk( Sector, md.Sectors.get( i ), i, no_problem ) )
			.eval sl.add( 0 )
			.eval sh.add( $54 + i )
		}
		.eval chunks.add( Chunk( SecL, sl, 0, no_problem ), Chunk( SecH, sh, 1, no_problem ) )
	} else .if ( md.getStructName() == "ATMDS3_Music" ) {
		.var c2p = List()			// Chunks to place
		.var p2p_wt = Hashtable()	// Wave-Program-ID zu Table-Pointer
		.var p2p_pt = Hashtable()	// Pulse-Program-ID zu Table-Pointer
		.var p2p_ft = Hashtable()	// Filter-Program-ID zu Table-Pointer
		.var td = Chunk( TD, List(), 0, no_problem )
		.var sl = Chunk( SecL, List(), 0, inacceptable )
		.var sh = Chunk( SecH, List(), 1, inacceptable )
		// Blöcke erstellen
		{	// Glide-Blöcke
			.var g = md.GlideList
			.var gl = Chunk( Glide, List(), 0, no_problem )
			.var gh = Chunk( Glide, List(), 1, no_problem )
			.var i=0
			.while ( i < g.size() && g.get( i ) != null ) {
				.eval gl.Bytes.add( <g.get( i ) )
				.eval gh.Bytes.add( >g.get( i ) )
				.eval i++
			}
			.if ( gl.Bytes.size() == 0 ) .eval chunks.add( gl, gh )	// keine Glides vorhanden
			else .eval c2p.add( gl, gh )
		}{	// Wave-Blöcke - die Wavetable aus Wave-Programmen zusammensetzen ...
			.var wt = Chunk( Wave, List(), 0, inacceptable )
			.var tt = Chunk( Trans, List(), 0, inacceptable )
			.var pos = 0
			.for ( var p=0; p<md.WaveList.size(); p++ ) {
				.var wp = md.WaveList.get( p )	// WaveProgram
				.eval p2p_wt.put( p, pos )		// Verknüpfung WaveProgramID->WavePosition
				.var map = Hashtable()
				.for ( var l=0; l<wp.size(); l++ ) {
					.var wl = md.WaveList.get( p ).get( l )		// WaveLine
					.if ( null != wl ) {
						.eval wt.Bytes.add( wl.WaveForm | (wl.DrumMode?8:0) | (wl.Ring?4:0) | (wl.Sync?2:0) | (wl.Gate?1:0) )
						.eval tt.Bytes.add( wl.Offset )
						.eval map.put( l, pos++ )
						.if ( l >= wl.nextId ) {
							.eval wt.Bytes.add( wl.nextId < 0 ? $91 : $90 )
							.eval tt.Bytes.add( wl.nextId < 0 ? 0 : map.get( l ) )
							.eval pos++
						}
					}
				}
			}
			.eval c2p.add( wt, tt )
		}{	// Pulse-Blöcke - auch die Pulse-Tabelle muss neu zusammen gesetzt werden
			// -> in Version 4 neu: dort werden die Positionen #0 auch verwendet
			//		=> die 2-Byte-Verschwendung hört dann auf
			.var p1 = Chunk( P1, List().add( 0 ), 1, inacceptable )
			.var p2 = Chunk( P2, List().add( 0 ), 2, inacceptable )
			.eval pos = 1
			.if ( v.tabs ) .print "... assembling Pulse Table: 0000"
			.for ( var p=0; p<md.PulseList.size(); p++ ) {
				.var pp = md.PulseList.get( p )
				.eval p2p_pt.put( p, pos )		// Verknüpfung PulseProgramID->PulsePosition
				.var map = Hashtable()			// die Positionen der Zeilen dieses Programmes
				.eval p1.Bytes.add( >pp.InitialPulse )
				.eval p2.Bytes.add( <pp.InitialPulse )
				.if ( v.tabs ) .print "    prg #$" + toHexString( p, 2 ) + " -> pos = $" + toHexString( pos, 2 ) + " - " + toHexString( p1.Bytes.get( pos ), 2 ) + toHexString( p2.Bytes.get( pos ), 2 )
				.eval pos++
				.for ( var l=0; l<pp.Lines.size(); l++ ) {
					.var pl = pp.Lines.get( l )
					.eval map.put( l, pos )
					.eval pos += 2
					.eval p1.Bytes.add( >pl.AddValue, pl.Count )
					.eval p2.Bytes.add( <pl.AddValue, pl.nextId > l ? pos : pl.nextId < 0 ? 0 : map.get( pl.nextId ) )
					.if ( v.tabs ) .print "                      $" + toHexString( pos - 2, 2 ) + " - " + toHexString( p1.Bytes.get(pos-1), 2 ) + "." + toHexString( p1.Bytes.get(pos-2), 2 ) + toHexString( p2.Bytes.get(pos-2), 2 ) + "::" + toHexString( p2.Bytes.get(pos-1), 2 )
				}
			}
			.eval c2p.add( p1, p2 )
		}{	// Filter-Blöcke - auch die Filter-Tabelle muss neu zusammen gesetzt werden
			.var f1 = Chunk( F1, List().add( 0 ), 1, inacceptable )
			.var f2 = Chunk( F2, List().add( 0 ), 2, inacceptable )
			.eval pos = 1
			.if ( v.tabs ) .print "... assembling Filter Table: 0000"
			.for ( var p=0; p<md.FilterList.size(); p++ ) {
				.var fp = md.FilterList.get( p )
				.eval p2p_ft.put( p, pos )		// Verknüpfung PulseProgramID->PulsePosition
				.var map = Hashtable()			// die Positionen der Zeilen dieses Programmes
				.eval f1.Bytes.add( >fp.Init.Cutoff )
				.eval f2.Bytes.add( fp.Init.Type | (fp.Init.Resonance << 4) )
				.if ( v.tabs ) .print "    prog #$" + toHexString( p, 2 ) + " -> pos = $" + toHexString( pos, 2 ) + " - " + toHexString( f1.Bytes.get( pos ), 2 ) + toHexString( f2.Bytes.get( pos ), 2 )
				.eval pos++
				.for ( var l=0; l<fp.Lines.size(); l++ ) {
					.var fl = fp.Lines.get( l )
					.eval map.put( l, pos )
					.eval pos += 2
					.eval f1.Bytes.add( >fl.AddValue, fl.Count )
					.eval f2.Bytes.add( (<fl.AddValue & $f0) | fl.Resonance, fl.nextId > l ? pos : fl.nextId < 0 ? 0 : map.get( fl.nextId ) )
					.if ( v.tabs ) .print "                       $" + toHexString( pos-2, 2 ) + " - " + toHexString( f1.Bytes.get( pos - 1 ), 2 ) + "." + toHexString( f1.Bytes.get( pos - 2 ), 2 ) + toHexString( f2.Bytes.get( pos - 2 ), 2 ) + "::" + toHexString( f2.Bytes.get( pos - 1 ), 2 )
				}
			}
			.eval c2p.add( f1, f2 )
		}{	// Instrumente-Blöcke (können erst nach den Tabellen-Blöcken gebaut werden,
			// weil sie diese schliesslich referenzieren)
			.var l = List().add( Chunk( AD, List(), 0, inacceptable ), Chunk( SR, List(), 0, inacceptable ), Chunk( WP, List(), 0, inacceptable ), Chunk( PP, List(), 0, inacceptable ), Chunk( FP, List(), 0, inacceptable ), Chunk( V1, List(), 0, inacceptable ), Chunk( V2, List(), 0, inacceptable ), Chunk( V3, List(), 0, inacceptable ), Chunk( HR, List(), 0, inacceptable ), Chunk( H1, List(), 0, inacceptable ), Chunk( H2, List(), 0, inacceptable ) )
			.var go = true ; .eval i=0
			.while ( go ) {
				.var ins = md.InstrumentList.get( i++ )
				.if ( null == ins ) .eval go = false
				else {
					.eval l.get(  0 ).Bytes.add( ins.AD )
					.eval l.get(  1 ).Bytes.add( ins.SR )
					.eval l.get(  2 ).Bytes.add( p2p_wt.get( ins.WP ) )
					.eval l.get(  3 ).Bytes.add( ins.PP < 0 ? ins.PP + 1 : p2p_pt.get( ins.PP ) )
					.eval l.get(  4 ).Bytes.add( ins.FP < 0 ? ins.FP + 1 : p2p_ft.get( ins.FP ) )
					.eval l.get(  5 ).Bytes.add( ins.V1 )
					.eval l.get(  6 ).Bytes.add( ins.V2 )
					.eval l.get(  7 ).Bytes.add( ins.V3 )
					.eval l.get(  8 ).Bytes.add( ins.HR )
					.eval l.get(  9 ).Bytes.add( ins.H1 )
					.eval l.get( 10 ).Bytes.add( ins.H2 )
				}
			}
			.eval c2p.addAll( l )
		}{	// Sektor-Blöcke (das Selbe, wie bei den Instrumenten)
			.if ( v.secs > 1 ) .print "... beginnig to reassemble sectors:"
			.for ( var s=0; s<$60; s++ ) {
				.var sec = md.SectorList.get( s )
				.if ( null != sec ) {
					.var st = "    sector #$" + toHexString( s, 2 ) + ":"
					.eval sl.Bytes.add( 0 ) ; .eval sh.Bytes.add( 0 )
					.var c = Chunk( Sector, List(), s, acceptable )
					.var dur = sec.InitialPState.Dur
					.var ins = sec.InitialPState.Ins
					.var tn = sec.InitialPState.Tie
					.for ( var b=0; b<sec.Bars.size(); b++ ) {
						.var bar = sec.Bars.get( b )
						.var is_glide = -1
						// Sektoren schreiben - worauf genau kam es nochmal an?
						// -> wenn's eine Note mit Anschlag ist, muss zuerst das Instrument feststehen
						// -> die Duration - wenn nötig, kommt zuletzt vor der Note / Pause
						// -> alle anderen Dinge sind von der Reihenfolge her frei, zuerst sollte man
						//	  aber die "kostspieligeren" Modifier schreiben, denn falls genug Zeit ist,
						//	  werden die dann einzeln behandelt.
						.for ( var m=0; m<bar.Modifiers.size(); m++ )
							.if ( bar.Modifiers.get( m ).Type == tie )
								.eval tn = !tn
						.var is_trigger = (bar.Type == note) && !tn
						// Zuerst das Instrument schreiben, wenn es gebraucht wird
						.if ( null != bar.Instrument && is_trigger && ins != bar.Instrument ) {
							.eval ins = bar.Instrument
							.eval c.Bytes.add( $c0 | ins )
							.eval st += " " + toHexString( $c0 | ins, 2 )
						}
						// Dann sind die Modifier an der Reihe
						.for ( var m=0; m<bar.Modifiers.size(); m++ ) {
							.var mod = bar.Modifiers.get( m )
							.if ( mod.Type == ff ) {			.eval c.Bytes.add( $f6, mod.Parameter )
																.eval st += " f6 " + toHexString( mod.Parameter, 2 )
							} else .if ( mod.Type == fp ) {		.eval c.Bytes.add( $f7, mod.Parameter < 0 ? mod.Parameter + 1 : p2p_ft.get( mod.Parameter ) )
																.eval st += " f7 " + toHexString( c.Bytes.get( c.Bytes.size()-1 ), 2 )
							} else .if ( mod.Type == pp ) {		.eval c.Bytes.add( $f8, mod.Parameter < 0 ? mod.Parameter + 1 : p2p_pt.get( mod.Parameter ) )
																.eval st += " f8 " + toHexString( c.Bytes.get( c.Bytes.size()-1 ), 2 )
							} else .if ( mod.Type == ad ) {		.eval c.Bytes.add( $f9, mod.Parameter )
																.eval st += " f9 " + toHexString( mod.Parameter, 2 )
							} else .if ( mod.Type == sr ) {		.eval c.Bytes.add( $fa, mod.Parameter )
																.eval st += " fa " + toHexString( mod.Parameter, 2 )
							} else .if ( mod.Type == tie ) {	.eval c.Bytes.add( $fb ) ; .eval st += " fb"
							} else .if ( mod.Type == glide ) {	.eval is_glide = m	}
						}
						// Jetzt noch die Dauer bei Bedarf (also bei Änderung)
						.if ( null != bar.Duration && dur != bar.Duration ) {
							.eval dur = bar.Duration
							.eval c.Bytes.add( $80 | dur )
							.eval st += " " + toHexString( $80 | dur, 2 )
						}
						// Zum Schluss der Trigger
						.if ( bar.Type == note ) {
							.if ( is_glide < 0 ) {
								.eval c.Bytes.add( bar.Note )
								.eval st += " " + toHexString( bar.Note, 2 )
							} else {
								.var gm = bar.Modifiers.get( is_glide )
								.eval c.Bytes.add( $60 + gm.Parameter, bar.Note, gm.Target )
								.eval st += " " + toHexString( $60 + gm.Parameter, 2 ) + " " + toHexString( bar.Note, 2 ) + " " + toHexString( gm.Target, 2 )
							}
							.eval st += "  "
						} else .if ( bar.Type == off ) {	.eval c.Bytes.add( $fc ) ; .eval st += " fc  "
						} else .if ( bar.Type == skip ) {	.eval c.Bytes.add( $fd ) ; .eval st += " fd  "
						} else .if ( bar.Type == wait ) {	.eval c.Bytes.add( $fe ) ; .eval st += " fe  "
						} else .if ( bar.Type == end ) {	.eval c.Bytes.add( $ff ) ; .eval st += " ff" }
					}
					.if ( v.secs > 1 ) .print st
					.if ( null != dmp ) {
						.eval dmp.writeln( st )
						.eval dmp.writeln( bytesString( "new data:", c.Bytes ) )
						.eval dmp.writeln( bytesString( "old data:", sec.OldData ) )
					}
					.eval c2p.add( c )
				}
			}
			.eval c2p.add( sl, sh )
		}{	// Track-Blöcke + TuneData-Block + Speedtable-Block
			.var st = Chunk( ST, List(), 0, no_problem )
			.for ( var s=0; s<md.SongList.size(); s++ ) {
				.var sng = md.SongList.get( s )
				.if ( null != sng ) {
					.var tId = st.Bytes.size() * 4
					// die Track-Adressen stehen noch nicht fest!
					.eval td.Bytes.add( 0, 0, 0, 0, 0, 0, sng.TickSpeed, sng.InitialVolume )
					.eval st.Bytes.add( sng.MultiplaySpeed + (sng.MultiplayRatio << 4) )
					.for ( var t=0; t<3; t++ ) {
						.var trk = sng.get( t )
						.var tc = Chunk( Track, List(), tId++, acceptable )
						.for ( var b=0; b<trk.size(); b++ ) {
							.var tb = trk.get( b )
							.if ( tb.Type == playable ) {
								.if ( tb.Transpose != null )		.eval tc.Bytes.add( ( ( tb.Transpose >= 0 ) ? $80 : $a0 ) + abs( tb.Transpose ) )
								.if ( tb.Repeat != null )			.eval tc.Bytes.add( ( tb.Repeat & 15 ) | $c0 )
								.if ( tb.VolumeOrControl != null )	.eval tc.Bytes.add( ( tb.VolumeOrControl & 15 ) | $e0 )
								.if ( tb.Speed != null )			.eval tc.Bytes.add( ( tb.Speed & 15 ) | $60 )
																	.eval tc.Bytes.add( tb.Sector )
							} else .if ( tb.Type == controlEnd ) {	.eval tc.Bytes.add( $ff )
							} else .if ( tb.Type == controlJump ) { .eval tc.Bytes.add( $fe, tb.VolumeOrControl )
							} else .if ( tb.Type == controlSong ) {	.eval tc.Bytes.add( ( tb.VolumeOrControl & 3 ) | $f0 )	}
						}
						.eval c2p.add( tc )
					}
				}
			}
			.eval c2p.add( td, st )
		}
		// Die Blöcke sind erstellt ... jetzt müssen sie sortiert und ausgegeben werden ...
		// Vermutlich sehr nützlich:
		//	- Listen der Prioritäten
		//	- Liste der Längen
		//	- Sortierung der Längen-Liste -> IDs longest to shortest
		.var prio1 = List() ; .var prio2 = List() ; .var prio3 = List()
		.var sizes = List() ; .var avail = List()
		.for ( var i=0; i<c2p.size(); i++ ) {
			.var c = c2p.get( i )
			.if ( c.PageBoundaryCrossings == inacceptable ) 	.eval prio1.add( i )
			else .if ( c.PageBoundaryCrossings == acceptable )	.eval prio2.add( i )
			else .if ( c.PageBoundaryCrossings == no_problem )	.eval prio3.add( i )
			// Sort Index into size sorted index list, add size to sizes list
			.var sz = c.Bytes.size()
			.eval sizes.add( sz )
			.eval avail = makeAvailable( avail, sizes, i )
		}
		// Und nun möchte ich Page für Page mit Chunks füllen, und dabei möglichst jede Page
		// exakt füllen, oder mit Prio3 bzw. Prio2 überfüllen.
		// Hierfür müsste eine Art Backtracking-Algorithmus eigentlich gut passen.  Ich bin mir
		// gerade nur nicht sicher, wie ich das in JavaScript machen soll.
		.if ( v.sort ) .print "... chunks produced: " + c2p.size() + " -> Start-Address = $" + toHexString( adr, 4) + @"\n    sizes: " + sizes + @"\n    prio1: " + prio1 + @"\n    prio2: " + prio2 + @"\n    prio3: " + prio3


		/*
			Praktisch müsste ich es hier mit dem klassischen Nummern-Summen-Problem zu tun haben.
			Das heisst, wenn ich eine Konstellation der Chunks ausrechnen kann, kann ich die dort
			getroffenen Entscheidungen noch back-track`end korrigieren um ggf. weitere Lösungsmög-
			lichkeiten zu erhalten.

			Naja, ich sollte vielleicht erst einmal mit der Suche nach einer Lösung anfangen ...
			... und vorher eine Art Rahmen abstecken.  Es startet immer bei "adr".  Gesucht werden
			möglichst genau so viele Bytes, um bis zur Grenze aufzufüllen.

			Algorithmus:	- Liste der Chunks an sich -> gibt die IDs der Chunks vor
							- Liste der nach Größe sortierten ChunkIDs (avail)
			Design einer Lösung:
			Hier wird ausschliesslich mit Listen und Hashtabellen gearbeitet.  Außerdem werden die
			Kriterien sozusagen "on the fly" verändert.  Sprich: die erste Page hat (vermutlich)
			weniger Platz übrig.
			Das bedeutet wiederum, eine Lösung besteht aus einer Sammlung an Pages mit Chunks drin.
				-> Hashtable( startAddress, List( ChunkIDs ) )

			*/
		.var pgAdr = adr
		.var pages = List()
		.var penalty = 0
		// Hier beginnt sozusagen die Schleife über die Pages...
		.while ( avail.size() > 0 && penalty < 1)
		{
			// eine Page beginnen
			.var pg = List() ; .var btc = 0
			.var bytes = 256 - <pgAdr
			.var sid = 0 ; .var id = 0
			.if ( v.sort ) .print "+ starting page $" + toHexString( >pgAdr ) + ", left:" + bytes + " Bytes."
			.while ( bytes > 0 && sid < avail.size() && sid > -1 ) {
				// findNextFittingSI( sid, bytes )
				.eval id = sid
				.while ( ( id < avail.size() ) && ( sizes.get( avail.get( id ) ) > bytes ) ) .eval id++;
				//
				.if ( id < avail.size() ) {
					.var cid = avail.get( id )
					.eval bytes -= sizes.get( cid )
					.eval pg.add( cid )
					.eval avail.remove( id ) // nicht mehr available!
					.if ( v.sort ) .print "  select(" + cid + "), left: " + bytes
					.eval sid = id
				} else {
					.eval sid = -1
					.while ( sid == -1 && pg.size() > 0 ) {
						.eval id = pg.get( pg.size() - 1 )
						.eval pg.remove( pg.size() - 1 )
						.eval avail = makeAvailable( avail, sizes, id )
						.eval bytes += sizes.get( id )
						// sid = findNextSizeId( id );
						.eval sid = 0
						.while ( sid < avail.size() && avail.get( sid ) != id ) .eval sid++
						// "size_id" von "id" gefunden -> die size_id des nächstkleineren chunks wird gesucht.
						.while ( sid < avail.size() && sizes.get( avail.get( sid ) ) == sizes.get( id ) ) .eval sid++
						.eval btc++
						.if ( v.sort ) .print "  deselect(" + id + "), left:" + pg
					}
				}
				// Abbruch abfangen: Bytes übrig, aber keine Chunks mehr da -> viel weiter zurück
				.if ( bytes > 0 && sid == avail.size() && avail.size() > 0 && btc < 100 ) {
					.var dl = List()
					.var cd = btc < pg.size() ? btc : pg.size() - 1;
					.for ( ; cd > -1; cd--, btc-- )
						.if ( pg.size() > 0 )
						{
							.eval id = pg.get( pg.size() - 1 )
							.eval pg.remove( pg.size() - 1 )
							.eval avail = makeAvailable( avail, sizes, id )
							.eval bytes += sizes.get( id )
							.eval dl.add( id )
						}
					// sid = findNextSizeId( id );
					.eval sid = 0
					.while ( sid < avail.size() && avail.get( sid ) != id ) .eval sid++
					// "size_id" von "id" gefunden -> die size_id des nächstkleineren chunks wird gesucht.
					.while ( sid < avail.size() && sizes.get( avail.get( sid ) ) == sizes.get( id ) ) .eval sid++
					.eval btc++;
					.if ( v.sort ) .print "  deselect(" + dl + "), left:" + bytes + ", selected: " + pg
				}
			}
			.errorif ( bytes > 0 && sid != avail.size() ), "! cannot find a solution to page $" + toHexString( >pgAdr, 2 )
			.eval pages.add( pg )
			.if ( v.sort ) .print "- page $"+ toHexString( >pgAdr, 2 ) + ": Chunk-IDs =" + pg
			.eval pgAdr = (pgAdr + 256) & $ff00
		}
		.print "... Music data has been relocated, so (hopefully) no data spans any page boundaries."
		.if ( pgAdr > adr && pages.size() > 0 ) {
			// Zuerst die Daten, die zu verändern sind - Adresstabellen ...
			.eval pgAdr = adr ; .var pa0 = adr
			.for ( var p = 0; p < pages.size(); p++ ) {
				.var pg = pages.get( p )
				.eval pa0 = pgAdr
				.for ( var c = 0; c < pg.size(); c++ ) {
					.var chk = c2p.get( pg.get( c ) )
					.if ( chk.Type == Sector ) {
						.eval sl.Bytes.set( chk.Id, <pgAdr )
						.eval sh.Bytes.set( chk.Id, >pgAdr )
					} else .if ( chk.Type == Track ) {
						.eval td.Bytes.set( chk.Id * 2, <pgAdr )
						.eval td.Bytes.set( chk.Id * 2 + 1, >pgAdr )
					}
					.eval chunks.add( chk )
					.eval pgAdr += chk.Bytes.size()
				}
			}
		}
	}
	// am Ende: die erstellten Chunks zurück geben ...
	.return chunks
}
