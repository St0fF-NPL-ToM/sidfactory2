/******************************************************************************
/*	Data-Helfer ... Unterfunktionen, die sonst die Übersichtlichkeit versauen.
/*****************************************************************************/
#importonce		// erst noch ein paar Helferlein ...
.function fread_w( file, pos ) { .return file.uget( pos ) + ( file.uget( pos + 1 ) << 8 ) }
.function read_w( bytelist, pos ) { .return bytelist.get( pos ) + ( bytelist.get( pos + 1 ) << 8 ) }
.function fread_wo( f, pos, hi_offset ) { .return f.uget( pos ) + ( f.uget( pos + hi_offset ) << 8 ) }
.function read_wo( b, pos, hi_offset ) { .return b.get( pos ) + ( b.get( pos + hi_offset ) << 8 ) }
.function fread_20( f, pos ) {
	.var l = List()
	.for ( var i = 0; i < $20; i++ ) .eval l.add( f.uget( pos + i ) )
	.return l }
.function read_20( b, pos ) {
	.var l = List()
	.for ( var i = 0; i < $20; i++ ) .eval l.add( b.get( pos + i ) )
	.return l }
.function fread_page_full( f, pos ) {
	.var l = List()
	.errorif ( f.getSize() - pos < 256 ), "Datafile '" + f.filename() + "' too small, was expecting a whole page!"
	.for ( var i = 0; i < $100; i++ ) .eval l.add( f.uget( pos + i ) )
	.return l }
.function read_page_full( b, pos ) {
	.var l = List()
	.for ( var i = 0; i < $100; i++ ) .eval l.add( b.get( pos + i ) )
	.return l }
.function fread_page( f, pos ) {
	.var l = List()
	.var left = f.getSize() - pos
	.for ( var i = 0; i < min( $100, left ); i++ ) .eval l.add( f.uget( pos + i ) )
	.return l }

.function e20() { .var l=List(32) ; .for(var i=0;i<32;i++) .eval l.set( i, 0 ) ; .return l }
.function e100() { .var l=List(256) ; .for(var i=0;i<256;i++) .eval l.set( i, 0 ) ; .return l }
.function e20(x) { .var l=List(32) ; .for(var i=0;i<32;i++) .eval l.set( i, x ) ; .return l }
.function e100(x) { .var l=List(256) ; .for(var i=0;i<256;i++) .eval l.set( i, x ) ; .return l }

.function fill_page( pl, fillByte ) {
	.var r = List().addAll( pl )
	.while ( r.size() < 256 ) .eval r.add( fillByte )
	.return r  }

// find_len sucht lediglich rückwärts nach dem ersten nicht-Null-Byte.
.function find_len( p ) {
	.var i=255
	.while ( i >= 0 && p.get(i) == 0 ) .eval i--
	.return i+1
}
.function find_len( t1, t2 ) {
	.var i=255
	.while ( i >= 0 && t1.get(i) == 0 && t2.get(i) == 0 ) .eval i--
	.return i+1
}
