#include	"ALL.H"

kiskep::kiskep( char* pic8nev, pic8* ppic ) {
	int y; int x; // Legacy for-loop scope.

	tomb = NULL;
	if( !ppic ) {
		{ ppic = NEW(pic8*) pic8( pic8nev ); }
	}

	lyuk = ppic->gpixel( 0, 0 );

	xsize = ppic->getxsize();
	ysize = ppic->getysize();
	if( xsize > 255 || ysize > 255 )
		hiba( "kiskepben size > 255!" );
	hossz = ysize*256l;
	{ tomb = NEW(unsigned char*) unsigned char[unsigned(hossz)]; }
	if( !tomb )
		uzenet( "memory!" );
	for( y = 0; y < ysize; y++ )
		for( x = 0; x < xsize; x++ )
			tomb[y*256+x] = ppic->gpixel( x, y );

	DELETE ppic; }
}

kiskep::~kiskep( void ) {
	if( tomb )
		DELETE[] tomb; }
}
