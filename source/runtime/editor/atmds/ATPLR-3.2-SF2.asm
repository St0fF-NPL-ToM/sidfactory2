/******************************************************************************
/*	Acid Track Player 3.2+++			St0fF / Neoplasia ^ the0bsessedManiacs!
/******************************************************************************
/*	Changes 2026:
/*  - tried to make the player conform to SIDFactorII requirements to be able
/*	  to use that great editor with our player(s)
/******************************************************************************
/*	Changes 2025:
/*	-------------
/*	- make a macro-call out of the player itself, so it can be placed more free
/*	  in memory and included or written several times (like building a disk)
/*	- integrate the multispeed-ratio into the play-routine (get rid of MP+6)
/*	- remove the duration table, as in all music created by now, we did not
/*	  need the extended comparison range for hard restart timing
/*	- take measures against page boundary crossings, where possible
/*		- Freq Tables: check - all inside page #0
/*		- Variables: check - all inside page #1
/*		- Sector-Table: variable code placement needed!  Init needs to be
/*		  placed behind main code, so we have some bytes to juggle.
/*****************************************************************************/
#importonce
.macro assemble_ATP3( data, new_ticking ) {
	#if HAS_DATA
		.errorif ( data.getStructName() != "ATMDS_mem" && data.getStructName() != "ATMDS3_Music" ),	"Please call the 'assemble_ATP3'-macro with a valid data set! (Obtainable via 'ATMDS3_load_file()' or 'ATMDS3_emptyData()')"
	#else
	// Krill hates that about KackAssembler:
	// … repeatedly having to write out '.label' …
		.label @speed_table = $403c
		.label @tunedats = $4040
		.label @gldtbl = $4060
		.label @gldtbh = $4080
		.label @adtb = $40a0
		.label @srtb = $40c0
		.label @aptb = $40e0
		.label @pptb = $4100
		.label @fptb = $4120
		.label @v1tb = $4140
		.label @v2tb = $4160
		.label @v3tb = $4180
		.label @hrtb = $41a0
		.label @hf1tb = $41c0
		.label @hf2tb = $41e0
		.label @wave = $4200
		.label @trans = $4300
		.label @ptb1 = $4400
		.label @ptb2 = $4500
		.label @ftb1 = $4600
		.label @ftb2 = $4700
		.label @trk10 = $4800
		.label @trk20 = $4900
		.label @trk30 = $4a00
		.label @sec00 = $5400
		.label @secs_lo = $b400
		.label @secs_hi = $b460
	#endif
	.var sector0 = 0
	.var sector_tables = 0
	.var player_base = *
	.print "assemble_ATP3(" + data + ")"
	//	----
	//	Base:
	//	=====
	* = player_base "ATPLR 3.2'26"
		// NTSC not really supported ...
		// ... but for freqtable generation
		.var timing = "PAL"
		.var max_song = 3
	//
	//	Player:
	//	=======
		.zp { .label padr 	= $a0 }//	ZPLabel
		.label	sfl			= $d400//	SIDLabels
		.label	sfh			= $d401
		.label	spl			= $d402
		.label	sph			= $d403
		.label	sct			= $d404
		.label	sad			= $d405
		.label	ssr			= $d406
		.label	sffl		= $d415
		.label	sffh		= $d416
		.label	srf			= $d417
		.label	smv			= $d418
	//	=====================================
	//	=   Acid Track Player Version 3.2   =
	//	=C0ded by St0fF of NE0PLASIA in 1997=
	//	=  Improved and optimized in 1998!  =
	//	=  ... finalized or so in 2025 ...  =
	//	=====================================
				jmp pinit
				jmp pplay
	//	============================================
	//	Volume change func. (for fading out or such)
	//	============================================
	ptunevol:	and #$0f
				sta pvolume
				rts
	//	========================
	//	player-id + artist text:
	//	========================
		.encoding "screencode_mixed"
		.text "acidtrackplr v3.2"
		.text "'26"//	-> ID!
		.encoding "petscii_mixed"
		.text "!player by st0ff & music by you!"
	//	=================
	//	frequency tables:
	//	=================
		.var FreqTableCache = List()
		.var SIDfreq = 432.0 * pow( 256, 3 ) / (timing == "PAL" ? 985248.0 : 1022727.0 )
		// Fülle den Cache von C-0 bis der SID gesprengt wird.  Die erste Sprengung wird als $ffff ge-
		// speichert und damit eine Abbruchbedingung geschaffen (Und etwas, was ein Player nutzen kann).
		.var note = 0.0
		.var SID_Note_Frequency = note
		.while ( SID_Note_Frequency < 65535.0 ) {
			.eval SID_Note_Frequency = SIDfreq * pow( 2.0, ( (note - 9.0) / 12.0 ) - 4.0 )
			.if ( SID_Note_Frequency >= 65535.0 ) {
						.eval SID_Note_Frequency = 65535
			} else {	.eval note = note + 1.0		}
			.eval FreqTableCache.add( SID_Note_Frequency )
		}
		.errorif ( FreqTableCache.size() != 96 ), "SID frequency table calculation failed - too few entries."

		pfrqtb: .lohifill FreqTableCache.size(), FreqTableCache.get(i)
//	===========================================================================
//	player-variables - most need to be held for all 3 voices, thus ".fill 14,0"
//	===========================================================================
		first_variable:
		presvoice:	.by 0			; .watch presvoice,7,"b32"
		@panschlag:	.by 0			; .watch presvoice+7,7,"b32"
		ptrkrep:	.by 0			; .watch presvoice+14,7,"b32"
		@ptpos:		.by 0
		phf1:		.by 0
		phf2:		.by 0
		@ptrp:		.by 0
					.fill 14,0
		@psnum:		.by 0			; .watch psnum,4,"b32"
		@pspos:		.by 0			; .watch psnum+7,4,"b32"
		@ptie:		.by 0			; .watch psnum+14,4,"b32"
		padtuned:	.by 0
		psrtuned:	.by 0
		ppptuned:	.by 0
		@pnxtcom:	.by 0
					.fill 14,0
		@pinum:		.by 0			; .watch pinum,4,"b32"
		phr:		.by 0			; .watch pinum+7,4,"b32"
		pgatepos:	.by 0			; .watch pinum+14,4,"b32"
		parpp:		.by 0
		pvibval:	.by 0
		pvibw8:		.by 0
		pvibcnt:	.by 0
					.fill 14,0
		pviblo:		.by 0
		pvibhi:		.by 0
		pvibaddlo:	.by 0
		pvibaddhi:	.by 0
		ppupos:		.by 0
		ppulo:		.by 0
		ppuhi:		.by 0
					.fill 14,0
		ppucnt:		.by 0
		pgldon:		.by 0
		pgldlo:		.by 0
		pgldhi:		.by 0
		pgldeff:	.by 0
		@pdur:		.by 0
		penable:	.by 0
					.fill 14,0
		@pnoteok:	.by 0
		ptablecnt:	.by 0
		ptablespeed:.by 0
		pglddir:	.by 0
		pvibdir:	.by 0
	.if ( new_ticking ) {
		@pc:		.by 0
		.watch pc ; .watch pc+7 ; .watch pc+14
	} else {
		@pc1:		.by 0		; .watch pc1,2,"h16"
		@pc1h:		.by 0		; .watch pc1+7,2,"h16"
								  .watch pc1+14,2,"h16"
	}
					.fill 14,0
		last_variable:
//	==========================================
//	ATTN: special variables are the bit masks!
//	==========================================
	.encoding "petscii_upper"
		pfltoff:	.by 6	//,5,2 -> %110,%101,%011
		pflton:		.by 1	//,2,4 -> %001,%010,%100
		pfltcnt:	.by 0
	.text "+atp"
		.by 5,2
	.text "+3.2+"
		.by 3,4
//	===========================================================================
//	frame play function:
//	=>	now with an integrated multiplay-ratio
//		=> be done with the extra multiplay entry point!
//	===========================================================================
@pplay:	// Check: Filterpointer gesetzt?  Dann ist der Filter an.
				ldy pfltpnt:#0
				beq pnofilter
		// in der Zählphase? -> geh zählen!
				dec pfltcnt
				bne pdofilter
		// Sonst ist es Zeit, die Tabelle zu interpretieren ...
				lda ftb2 + 1,y	// t2+1 => Zeiger auf nächste Position.
				sta pfltpnt
				beq pnofilter	// Abschalten des "Sweepens" inklusive...
				tay				// neue Position
				lda ftb2,y		// Resonanz im low_nybble 1.Byte Tabelle #2
				asl
				asl
				asl
				asl
				sta pfltres
				lda ftb1 + 1,y	// Zähler im 2. Byte Tabelle #1
				sta pfltcnt
	pdofilter:	lda ftb2,y		// Weiter mit dem 1.B T#2 - low adder
				and #%11100000
				clc
				adc pfltfrql:#0
				sta pfltfrql
				lda pfltfrqh:#0
				adc ftb1,y		// und 1.B T#1 - high adder
				sta pfltfrqh
				sta sffh
				lda pfltfrql	// Wer hat den SID designed?
				asl
				rol
				rol
				rol
				sta sffl
	pnofilter:	lda padr		// zp shadowing
				sta padrback0
				lda padr+1
				sta padrback1
//	=============================
//	=                    		=
//	=	Mainmusicroutine ...	=
//	=                    		=
//	=============================
//	Change 2025: remove JSR, use sbx, integrate MP-ratio
//	so finally it is one big chunk of spaghetti c0de ...
				ldx #14
	pwork_loop: lda mp_ratio_cnt:#0		// Beide springen bei bne
				ora penable,x			// ergo kombinierbar!
				bne ptvp1
				lda panschlag,x
				bpl ptrkttn
				and #$0f
				beq ptrackget
				jmp pnotrackwork
//	================================
//	=                              =
//	=Trackinterpreter 4 atp 3.2 ...=
//	=                              =
//	================================
//	Prioritaeten:           Zeiten:
//
//	 (8) - Sector         ->43/42/41
//	 (7) - Transpose      ->53/52/51
//	 (6) - Repeat         ->46/45/44
//	 (5) - Loop           ->52/52/52
//	 (4) - Max.Volume     ->47/46/45
//	 (3) - Song           ...ausser
//	 (2) - Speed          Konkurrenz...
//	 (1) - End            ->46/45/44
//	---------------------------------------
	.break
	ptrackget:	ldy ptpos,x
				inc ptpos,x
				cpx #7
				beq ptk2
				bcs ptk3
	ptk1:		lda $4000,y
				bcc ptkgot
	ptk2:		lda $4100,y
				bcs ptkgot
	ptk3:		lda $4200,y
	ptkgot:		bpl ptestifsec
		// transpose + repeat
				cmp #$e0
				bcc ptestiftrprep
		// loop, song, end
				cmp #$fe
				bne ptestifsong
	ploop:		iny
				cpx #7
				beq ptk21
				bcc ptk11
	ptk31:		lda $4200,y
				bcs ptkgot1
	ptk21:		lda $4100,y
				bcs ptkgot1
	ptk11:		lda $4000,y
	ptkgot1:	sta ptpos,x
//	==================================================================
//	Test: ist noch genug Zeit?  Hier war damals der Zähler schöner ...
//	==================================================================
	ptrktest:	.break
.if ( new_ticking ) {
				lda pc,x		// Bar Counter nicht 0 ? -> Zeit!
				beq ptrackget
} else {
				lda pc1h,x
				bne ptrkttn
				lda pc1,x
				cmp #5
				bcc ptrackget
}
	ptrkttn:	jmp ptestnew
//	===================
	ptrkend:	sta penable,x
	ptvp1:		jmp pvoiceplay
//	=====================
	pnewsong:	and #3
				asl
				asl
				asl
				tay
				jsr pgetdata
	.if ( new_ticking ) {
				sta p_ticks
	} else {
				ldx #14
				sty newdur
	}
				and #0
				sta ptpos
				sta ptpos+7
				sta ptpos+14
				beq ptrktest
//	===================
	ptestifsong:bcs ptrkend
				cmp #$f0
				bcs pnewsong
	pmaxvol:	and #$0f
				sta pvolume
				bpl ptrktest
//	===================
	prepeat:	and #$1f
				sta ptrkrep,x
				bpl ptrktest
//	===================
	ptestiftrprep:
				cmp #$c0
				bcs prepeat
	ptransp:	cmp #$a0		// Was hab ich denn hier genau gemacht?
				and #$1f		// hmm, die Beschränkung, dann *-1 wenn größer/gleich $a0?
				bcc !+
				eor #$ff		// falsch/richtig korrespondierende Werte aus Insanity:
				adc #0			// a2/82 ,  a5/83  ,9d/a3 ,a7/87  ,a0/80
			!:	sta ptrp,x		//	+2		+5		-3		+7		+0
				jmp ptrktest
//	===================
	ptestifsec:	cmp #$60
				bcc psector
	pspeedx:	and #$1f
	.if ( new_ticking ) {
				sta p_ticks
	} else {
				sta newdur
	}
				bpl ptrktest
//	===================
	psector:	sta psnum,x
				lda #$ff
				sta panschlag,x
//	=================================
//	=                               =
//	=Sectorinterpreter 4 atp 3.2 ...=
//	=                               =
//	=================================
//	Prioritaeten:            Zeit:
//
//	 (6) - End             ->63/81 !
//	 (5) - Ins=xx          ->85/81 -
//	 (4) - Dur=xx          ->60/56 -
//	 (3) - Note            ->52/52
//	 (3) - Gld-xx          ->52/52
//	 (3) - W8              ->67/67
//	 (3) - Skip            ->67/67
//	 (3) - st0FF           ->67/67
//	 (2) - ad=xx           ->73/74 !
//	 (2) - sr=xx           ->71/76 !
//	 (2) - ff=xx           ->75/77 !
//	 (2) - pp=xx           ->78/70 -
//	 (2) - fp=xx           ->76/71 -
//	 (1) - swTIE           ->70/65 -
//	---------------------------------------
	pnotrackwork:	.break
				ldy psnum,x
				lda secs_lo,y
				sta padr
				lda secs_hi,y
				sta padr+1
	pregetsec:	ldy pspos,x
	psecgetagain:
				inc pspos,x
				lda (padr),y
				bpl psecng
				cmp #$ff
				bne psecnoend
			// sector end
				and #0
				sta pspos,x
				lda ptrkrep,x
				bne prepsec
				lda #$f0
				sta panschlag,x
				jmp ptrktest
		prepsec:dec ptrkrep,x
				bpl pgetnext
			//	===================
	psecnoend:	cmp #$f6
				bcs psecnoinsdur
				cmp #$c0
				bcc psecdur
	psecins:	and #$1f
				sta pinum,x
				tay
				lda hrtb,y
				sta phr,x
				lda hf1tb,y
				sta phf1,x
				lda hf2tb,y
				sta phf2,x
			//	=================
	pgetnext:	.break
	.if ( new_ticking ) {
				lda pc,x
				bne ptestnew
				lda p_tick
	} else {
				lda pc1h,x
				bne ptestnew
				lda pc1,x
	}
				cmp #4
				bcc pregetsec
				jmp ptestnew
			//	===================
	psecdur:	and #$3f
				sta pdur,x
				bpl pgetnext
			//	===================
	psecnoinsdur:
				cmp #$fb
				bcc psecasfpf
				beq psectie
	psecw8coms:	sta pnxtcom,x
				and #0
				sta panschlag,x
				beq ptestnew
			//	===================
	psecng:		sta pnxtcom,x
				lda #1
				sta panschlag,x
				bne ptestnew
			//	===================
	psectie:	lda ptie,x
				eor #$ff
				sta ptie,x
				jmp pgetnext
			//	===================
	psecasfpf:	iny
				cmp #$f9
				beq psecsr
				bcs psecad
				cmp #$f7
				bcc psecfp
				bne psecff
	psecpp:		lda (padr),y
				sta ppptuned,x
				bcs psecgninc
			//	====================
	psecad:		lda (padr),y
				sta padtuned,x
				bcs psecgninc
			//	====================
	psecsr:		lda (padr),y
				sta psrtuned,x
				bcs psecgninc
			//	====================
	psecfp:		lda (padr),y
				sta pfltpnttuned
				bcc psecgninc
			//	====================
	psecff:		lda (padr),y
				sta pfltfrqtuned
			//	========================
	psecgninc:	iny
				tya
				sta pspos,x
	.if ( new_ticking ) {
				lda pc,x
				bne ptestnew
	} else {
				lda  pc1h,x
				bne  ptestnew
				lda  pc1,x
				cmp #4
				bcs  ptestnew
	}
				jmp psecgetagain
	ptestnew:
	.if ( new_ticking ) {
				ldy @p_tick: #0		// Tick nicht 0 - dann werden die Bars nicht
				bne pteston			// herunter gezählt
				dec pc,x			// Bars herunterzählen
				bpl ptvp			// kein Anschlag?
				jmp pnewframe
		.watch @p_tick
	} else {
				dec  pc1,x
				bne  pteston
				lda  pc1h,x
				bne  *+5
				jmp  pnewframe
				dec  pc1h,x
	}
	ptvp:		jmp pvoiceplay
	pteston:	.break
	.if ( new_ticking ) {
				lda pc,x		// Y enthält Tick -> >0 - nichts steht an!
	} else {	lda pc1h,x	}
				ora ptie,x		// Tie ist negativ, wenn "getied" wird
				bne ptvp
				lda panschlag,x
				beq ptvp
		// jetzt die Hard-Restart-Frames testen, weil ein Anschlag bevor steht
	.if ( new_ticking ) {
				tya
	} else {	lda  pc1,x	}
				cmp phf2,x		// HF2: more important!
				beq phardframe2
				cmp phf1,x
				bne ptvp
//	------------------------------------------------------------------------------
//	4 verschdl. HF's, jeder soll nur den nötigen C0de bekommen ... harte Aufgabe!
//	------------------------------------------------------------------------------
	phardframe1:lda phr,x
				beq patc2hr1
				cmp #2
				beq pdmc4hr1
				bcs ptvp
	patc1hr1:	lda #$ff
	patc2hr1:	sta sad,x
				and #0
				sta ssr,x
	pdmc4hr1:	lda #%11110110
				sta pgatepos,x
				lda presvoice,x//	gate bei
				and #%00001110//	voiceres=1
				sta presvoice,x//	loeschen
				bpl ptvp
			//	========
	phardframe2:lda #9	//	R=1
				sta presvoice,x//	---
				ldy pinum,x
			//	---APtab-INIT
				lda aptb,y
				sta parpp,x
				lda v2tb,y
				and #$0f
				sta ptablespeed,x
				sta ptablecnt,x
			//	---VIBRATO-INIT!
				lda v1tb,y
				sta pvibval,x
				lda v3tb,y
				sta pvibw8,x
				sta pvibcnt,x
				lda v2tb,y
				sta pvibdir,x
				lsr
				lsr
				lsr
				lsr
				and #7
				sta pcalcvibn
			//	---PULSE-INIT!
				lda ppptuned,x
				bne !+
				lda pptb,y
			!:	cmp #$ff
				beq pgoonvib
				sta ppupos,x
				tay
				beq pgoonvib
				lda ptb2,y
				sta ppulo,x
				lda ptb1,y
				sta ppuhi,x
				lda ptb1+2,y
				sta ppucnt,x
				inc ppupos,x
			pgoonvib:
				ldy pnoteok,x		// OOOPSIEE - sieht aus, als nutzen wir immer
				lda pfrqtb.hi,y		// die letzte gespielte Note für Vibrato-Berechnungen
				sta pvibaddlo,x
				and #0
				sta pvibaddhi,x
				sta pvibhi,x
				sta pviblo,x

				ldy pcalcvibn:#0
				beq pcalcnovib
			!:	asl pvibaddlo,x		// OOOPSIEE - ist doch falsch herum gewesen!
				rol pvibaddhi,x
				dey
				bne !-
			pcalcnovib:
				lda phr,x
				cmp #2
				beq pdmc4hr2
				ldy pinum,x
				bcs pkb16hr2
			patchr2:
			//	---ADSR-INIT!
				lda psrtuned,x
				bne pgetnewad
				lda srtb,y
	pgetnewad:	sta ssr,x
				lda padtuned,x
				bne !+
				lda adtb,y
			!:	sta sad,x
				jmp pvoiceplay
	pdmc4hr2:	dec presvoice,x
				lda #$0f
				sta ssr,x
				bpl !-
	pkb16hr2:	dec presvoice,x
				lda #$ff
				bmi pgetnewad
	//
	//	Fertig mit dem Hard Restart - jetzt kommt der Anschlag!
	//
	pnewframe:	lda #$ff
				sta panschlag,x
	.if ( new_ticking ) {
				lda pdur,x
				sta pc,x
	} else {
				ldy  pdur,x
				lda  pdurlo,y
				sta  pc1,x
				lda  pdurhi,y
				sta  pc1h,x
	}
				lda pnxtcom,x
				bpl pnoteglide
				ldy pinum,x
				cmp #$fd
				beq pskip
				bcs pwait
				lda pgatepos,x
				eor #%11111110
				sta presvoice,x
				bne patchr2			// St0fF-Befehl übernimmt ggf. gesetzte ADSR!
		pskip:	lda pgatepos,x
				eor #1
				sta pgatepos,x
		pwait:	and #0
				sta presvoice,x
				beq patchr2			// ebenso die anderen Warte-Befehle
		pnoteglide:
				cmp #$60
				bcs pglide
				adc ptrp,x
				sta pnoteok,x
				jmp pdranhaun		// ein Branch Always wäre sparsamer ...
		pglide:	sta pgldon,x
				and #$1f
				sta pgldeff,x
				and #0
				sta pglddir,x
		// diese Geschichte ist eigentlich Mist ...
				ldy psnum,x
				lda secs_lo,y
				sta padr
				lda secs_hi,y
				sta padr+1
				ldy pspos,x
				iny
				iny
				tya
				sta pspos,x
				dey
				lda (padr),y
				clc
				adc ptrp,x
				sta pnoteok,x
				dey
				lda (padr),y
				clc
				adc ptrp,x
				cmp pnoteok,x
				bcc !+
				dec pglddir,x
			!:	tay
				lda pfrqtb.lo,y
				sta pgldlo,x
				lda pfrqtb.hi,y
				sta pgldhi,x
	pdranhaun:	ldy pinum,x
			//	===ADSR-INIT!
				lda padtuned,x
				bne !+
				lda adtb,y
			!:	sta sad,x
				lda psrtuned,x
				bne !+
				lda srtb,y
			!:	sta ssr,x

				lda ptie,x
				bne pvoiceplay
			//	===RES OFF, GATE ON!
				sta presvoice,x
				ora #%11110111
				sta pgatepos,x
			//	===FILTER-INIT!
				lda fptb,y
				bne penblflt
				lda pfltvoi
				and pfltoff,x
				sta pfltvoi
				jmp pvoiceplay
	penblflt:	tay
				lda pflton,x
				ora pfltvoi
				sta pfltvoi
				lda pfltpnttuned:#0
				beq !+
				tay
			!:	cpy #$ff
				beq pvoiceplay			// Filter-Continue!
				lda pfltfrqtuned:#0
				bne !+
				lda ftb1,y
			!:	sta pfltfrqh
				sta sffh
				and #0
				sta pfltfrql
				sta sffl
				lda ftb2,y
				and #$f0
				sta pfltres
				lda ftb2,y
				asl
				asl
				asl
				asl
				sta pflttyp
				iny
				sty pfltpnt
				lda ftb1+1,y
				clc
				adc #1
				sta pfltcnt
//	============================================================
//	VoicePlay - play the instruments on the different voices ...
//	============================================================
pvoiceplay:		lda presvoice,x
				bne to_workdone
			//	==================
			//	Pulse-Bearbeitung:
			//	==================
				ldy ppupos,x
				beq pnopulse
				lda ptb2,y
				clc
				adc ppulo,x
				sta ppulo,x
				sta spl,x
				lda ptb1,y
				adc ppuhi,x
				sta ppuhi,x
				sta sph,x

				dec ppucnt,x
				bne pnopulse
				lda ptb2+1,y
				sta ppupos,x
				tay
				lda ptb1+1,y
				sta ppucnt,x
	pnopulse:
			//	===================
			//	Wavetable-Handling:
			//	===================
				ldy parpp,x
				lda wave,y
				cmp #$90
				bcc pjustok
				beq parploop
				lda #8
				sta presvoice,x
	to_workdone:sta sct,x
	twd_no_sct:	jmp work_done
	parploop:	lda trans,y
				sta parpp,x
				tay
				lda wave,y
	pjustok:	and pgatepos,x
				sta sct,x
				dec ptablecnt,x
				bpl pnonextarp
				lda ptablespeed,x
				sta ptablecnt,x
				inc parpp,x
	pnonextarp:	lda wave,y
				and #8
				bne pdrummode
				lda pgldon,x
				bne pgliding
		// Nichts an - einfach Arpeggio
				clc
				lda pnoteok,x
				adc trans,y
				tay
				lda pfrqtb.lo,y
				clc
				adc pviblo,x
				sta sfl,x
				lda pfrqtb.hi,y
				adc pvibhi,x
				sta sfh,x
				jmp pvibwork
	pdrummode:	lda trans,y
				sta sfh,x
				lda #$ff
				sta sfl,x
				bmi twd_no_sct
	pgliding:	clc
				lda pgldlo,x
				adc pviblo,x
				sta sfl,x
				lda pgldhi,x
				adc pvibhi,x
				sta sfh,x
				ldy pgldeff,x
				lda pglddir,x
				bmi pglddown

				clc
				lda pgldlo,x
				adc gldtbl,y
				sta pgldlo,x
				lda pgldhi,x
				adc gldtbh,y
				sta pgldhi,x

				ldy pnoteok,x
				cmp pfrqtb.hi,y
				bcc pvibwork
				bne preached
				lda pgldlo,x
				cmp pfrqtb.lo,y
				bcs preached
				bcc pvibwork
	pglddown:	sec
				lda pgldlo,x
				sbc gldtbl,y
				sta pgldlo,x
				lda pgldhi,x
				sbc gldtbh,y
				sta pgldhi,x

				ldy pnoteok,x
				cmp pfrqtb.hi,y
				bcc preached
				bne pvibwork
				lda pfrqtb.lo,y
				cmp pgldlo,x
				bcc pvibwork
	preached:	and #0
				sta pgldon,x
				sta pgldlo,x
				sta pgldhi,x
	pvibwork:	ldy pvibw8,x
				beq work_done
				lda pvibval,x
				bne pw84vib
	pdovib:		lda pvibdir,x
				bmi pvibdown
	pdovibup:	clc
				lda pviblo,x
				adc pvibaddlo,x
				sta pviblo,x
				lda pvibhi,x
				adc pvibaddhi,x
				sta pvibhi,x
				dec pvibcnt,x
				bne work_done
	pvibtrn:	lda pvibdir,x
				lsr
				bcc pnowidevib
				asl pvibaddlo,x
				rol pvibaddhi,x
	pnowidevib:	asl
				eor #$80
				sta pvibdir,x
				tya
				sta pvibcnt,x
				bpl work_done
	pw84vib:	dec pvibval,x
				bne work_done
				lda pvibdir,x
				ora #1
				sta pvibdir,x
				bpl pdovibup
	pvibdown:	sec
				lda pviblo,x
				sbc pvibaddlo,x
				sta pviblo,x
				lda pvibhi,x
				sbc pvibaddhi,x
				sta pvibhi,x
				dec pvibcnt,x
				beq pvibtrn
	work_done:
				txa
				sbx #7
				bmi !+
				jmp pwork_loop
			!:
				dec mp_ratio_cnt
				bpl !+
				lda mp_ratio: #0
				sta mp_ratio_cnt
	.if ( new_ticking ) {
				dec p_tick
				bpl !+
				lda @p_ticks: #0
				sta p_tick
		.watch @p_ticks
	} else {
				ldy  @newdur: #0
				bmi  !+
				lda  pc1h+14	//trk3 bestimmt
				bne  !+
				lda  pc1+14		//zeitpunkt der
				cmp  pdurlo		//geschwindigkeits-
				bcs  !+			//aenderung
				jsr  pchngdur
	}
			!:
				lda pfltvoi:#0
				ora pfltres: #0
				sta srf
				lda pflttyp:#0
				ora pvolume:#0
				sta smv
				lda padrback0:#0
				sta padr
				lda padrback1:#0
				sta padr+1
				rts
//	=========================================================
//	init function
//	-> gets song number in A
//	-> returns speed (play-calls per frame - 1) in A
//	=========================================================
@pinit:			and #max_song
				tay
				lda speed_table,y
				pha						// ich möchte playback-speed zurückgeben
				lsr ; lsr ; lsr ; lsr
				sta mp_ratio
				tya
				asl
				asl
				asl
	.if ( new_ticking ) {
				tay
		// Clear all variables and SID:
		.var ini_cnt = ( last_variable - first_variable ) - 1
		.errorif ( ini_cnt > 132 ), "too many variables (" + ini_cnt + ") - init loop needs a change!"
				ldx #ini_cnt
				and #0
			!:	sta sfl,x
				sta presvoice,x
				dex
				bpl !-
	} else {
				tax
				and #0
				ldy #$69
			!:	sta  sfl,y
				sta  presvoice,y
				dey
				bpl  !-
	}
		// selfmodded variables need special care ...
				sta pfltcnt
				sta pfltfrqtuned
				sta pfltpnttuned
				sta pfltpnt
				ora #8
				sta pfltvoi
		// init variables that need special values to start with ...
	.if ( new_ticking ) {
				ldx #14
			!:	lda #8
				sta presvoice,x
				sta sct,x
				sta pc,x
				lda #$f0
				sta panschlag,x
				txa
				sbx #7
				bpl !-
				jsr pgetdata
				sta p_ticks
	} else {
				ldy #14
			!:	lda #8
				sta  presvoice,y
				sta  sct,y
				sta  pc1,y
				lda #$f0
				sta  panschlag,y
				and #0
				sta  pc1h,y
				tya
				sec
				sbc #7
				tay
				bpl  !-
				jsr pgetdata
				jsr pchngdur
	}
				pla
				and #$0f		// Geschwindigkeit zurückgeben: 0=singleSpeed,
				rts				// 1=double,2=triple,etc.
	.if ( !new_ticking ) {
	@pchngdur:	iny
				sty  paddvalue
	// Um das neu auf zu dröseln, was habe ich hier genau gemacht?
	// Ein "mul $40" sollte recht einfach sein - was genau hab ich hier eigentlich verkehrt gemacht?
	//	$40 = %01000000 = 2^6, also 6x schieben oder schlauer sein ...
	//	... und ich war damals also schlauer.  Leider konnte ich aus den 6 shifts zwar 5 machen,
	//		aber es nicht weiter reduzieren.
	// 	-> ((Speed + 1) >> 2) 			-> hi ( === >($40*(Speed+1)) )
	//	-> ((Speed + 1) >>> 3) & #$c0	-> lo
	// ... Speed = 3->4: $100 %0100 -> (%01)=$01,(%00000100->$00)
	// ... Speed = 5->6: $180 %0110 -> (%01)=$01,(%00000110->%00000011.0->%00000001.1->%10000000.1->$80)
				tya		// calculate
				lsr		// higher
				lsr		// maximum
				tax

				tya		// calculate
				lsr		// lower
				ror		// maximum
				ror
				and #$c0
				ldy #$3f
			!:	sta  pdurlo,y
				sta  padr
				txa
				sta  pdurhi,y
				lda  padr
				sec
				sbc paddvalue: #0
				bcs  *+3
				dex
				dey
				bpl  !-
				sty  newdur
				rts
	}
	pgetdata:
	.if ( !new_ticking ) {
				lda  tunedats,x
				sta  ptk1+1
				sta  ptk11+1
				lda  tunedats+1,x
				sta  ptk1+2
				sta  ptk11+2
				lda  tunedats+2,x
				sta  ptk2+1
				sta  ptk21+1
				lda  tunedats+3,x
				sta  ptk2+2
				sta  ptk21+2
				lda  tunedats+4,x
				sta  ptk3+1
				sta  ptk31+1
				lda  tunedats+5,x
				sta  ptk3+2
				sta  ptk31+2
				lda  tunedats+7,x
				sta  pvolume
				ldy  tunedats+6,x
				rts
	@pdurlo:	.fill $40, 0
	@pdurhi:	.fill $40, 0
	} else {
				lda  tunedats,y
				sta  ptk1+1
				sta  ptk11+1
				lda  tunedats+1,y
				sta  ptk1+2
				sta  ptk11+2
				lda  tunedats+2,y
				sta  ptk2+1
				sta  ptk21+1
				lda  tunedats+3,y
				sta  ptk2+2
				sta  ptk21+2
				lda  tunedats+4,y
				sta  ptk3+1
				sta  ptk31+1
				lda  tunedats+5,y
				sta  ptk3+2
				sta  ptk31+2
				lda  tunedats+7,y
				sta  pvolume
				lda  tunedats+6,y
				rts
	}
	@code_end:

	// Memory-Struktur: Editor-Layout.  Musik-Struktur: soll gepackt werden.
#if HAS_DATA
	.var addr = data.getStructName() == "ATMDS_mem" ? $403c : code_end
	.var chunks = ATMDS3_pack( data, addr )
	.print "Player done, collected data ... start addr = $" + toHexString( addr ) + ", Chunks: " + chunks.size()
	// Das Packen liefert die Chunks, die jetzt der Reihe nach ausgegeben werden.
	.for ( var i=0; i<chunks.size(); i++ ) {
		.var c = chunks.get( i )
		.if ( c.Type == ST ) {
			.if ( addr != code_end ) {			// Dies ist ein Hack!
				* = addr "ATMDS3 Speed Table"	// ... die Adresse muss iwie gesetzt werden ...
			} else .memblock "ATMDS3 Speed Table"
			.label @speed_table = addr
		} else .if ( c.Type == TD ) {		@tunedats:	.memblock "ATMDS3 Tune Datas"
		} else .if ( c.Type == Glide ) {
			.if ( c.Id == 0 ) {				@gldtbl:	.memblock "ATMDS3 Glide Table low"
			} else {						@gldtbh:	.memblock "ATMDS3 Glide Table high"
			}
		} else .if ( c.Type == AD ) {		@adtb:		.memblock "ATMDS3 Instruments - AD Table"
		} else .if ( c.Type == SR ) {		@srtb:		.memblock "ATMDS3 Instruments - SR Table"
		} else .if ( c.Type == WP ) {		@aptb:		.memblock "ATMDS3 Instruments - WaveProgram Table"
		} else .if ( c.Type == PP ) {		@pptb:		.memblock "ATMDS3 Instruments - PulseProgram Table"
		} else .if ( c.Type == FP ) {		@fptb:		.memblock "ATMDS3 Instruments - FilterProgram Table"
		} else .if ( c.Type == V1 ) {		@v1tb:		.memblock "ATMDS3 Instruments - Vibrator Parameter Table #1"
		} else .if ( c.Type == V2 ) {		@v2tb:		.memblock "ATMDS3 Instruments - Vibrator Parameter Table #2"
		} else .if ( c.Type == V3 ) {		@v3tb:		.memblock "ATMDS3 Instruments - Vibrator Parameter Table #3"
		} else .if ( c.Type == HR ) {		@hrtb:		.memblock "ATMDS3 Instruments - Hard Restart Type Table"
		} else .if ( c.Type == H1 ) {		@hf1tb:		.memblock "ATMDS3 Instruments - Hard Restart Frame Table #1"
		} else .if ( c.Type == H2 ) {		@hf2tb:		.memblock "ATMDS3 Instruments - Hard Restart Frame Table #2"
		} else .if ( c.Type == Wave ) {		@wave:		.memblock "ATMDS3 Wave Table"
		} else .if ( c.Type == Trans ) {	@trans:		.memblock "ATMDS3 Arpeggio Table"
		} else .if ( c.Type == P1 ) {		@ptb1:		.memblock "ATMDS3 Pulse Table #1"
		} else .if ( c.Type == P2 ) {		@ptb2:		.memblock "ATMDS3 Pulse Table #2"
		} else .if ( c.Type == F1 ) {		@ftb1:		.memblock "ATMDS3 Filter Table #1"
		} else .if ( c.Type == F2 ) {		@ftb2:		.memblock "ATMDS3 Filter Table #2"
		} else .if ( c.Type == Track ) {	.memblock "ATMDS3 Track"
			.if ( c.Id == 0 ) 		{	@trk10:	}
			else .if ( c.Id == 1 )	{	@trk20:	}
			else .if ( c.Id == 2 )	{	@trk30:	}
		} else .if ( c.Type == Sector ) {	.memblock "ATMDS3 Sector"
			.if ( c.Id == 0 )		{	@sec00: }
		} else .if ( c.Type == SecL ) {		@secs_lo:	.memblock "ATMDS3 Sector Address Table low"
		} else .if ( c.Type == SecH ) {		@secs_hi:	.memblock "ATMDS3 Sector Address Table high"
		}
		// .printnow "CHUNK:" + i + ", " + chunkStr( c )
		.fill c.Bytes.size(), c.Bytes.get( i )
		.eval addr += c.Bytes.size()
	}
	.if ( data.getStructName() == "ATMDS_mem" )
		.print "Final addr = $" + toHexString( * ) + " => seems to fit: " + ( $b4c0 == * )
	else
		.print "Final addr = $" + toHexString( * ) + " => Music-Size = $" + toHexString( * - player_base ) + " Bytes."
#endif
}
