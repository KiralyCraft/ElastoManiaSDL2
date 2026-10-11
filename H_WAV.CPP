#include	"ALL.H"

void wav::lefoglal( void ) {
	if( size > 1000000 )
		hiba( "wav::alloc-ban size > 1000000!" );
	{ tomb = NEW(signed short*) signed short[size]; }
	if( !tomb )
		uzenet( "wav::alloc-ban memory!" );
}

wav::wav( unsigned sizepar ) {
	tomb = NULL;
	size = sizepar;
	lefoglal();
}

static void nevwavtest( char* nev ) {
	int i = strlen( nev ) - 1;
	while( i > 0 ) {
		if( nev[i] == '.' ) {
			// Most ponton all:
			if( strcmpi( ".wav", &nev[i] ) == 0 )
				return;
			else
				hiba( "nevrawtest-ben nincs .raw!" );
		}
		i--;
	}
	hiba( "nevwavtest-ben nincs .wav!" );
}


struct wavheader {
	char riff[4];
	int32_t hossz1;
	char wave[4];
	// format:
	char fmt[4];
	int32_t miez1;
	short mod; // 1 -> 8bit uncompressed
	short channel; // 1 -> mono, 2 -> stereo
	unsigned short frekvencia;
	short miez2, miez3, miez4;
	int32_t nemkene;
	// audio data:
	char data[4];
	int32_t hossz;
};

static_assert(sizeof(wavheader) == 44, "Original PCM WAV header layout.");

wav::wav( char* nev, double maxamplitudo, int start, int ucsoutani ) {
	int i; // Legacy for-loop scope.

	tomb = NULL;
	nevwavtest( nev );
	// Lemeri file hosszat:
	FILE* h = qopen( nev, "rb" );
	if( !h )
		hiba( "Nem tudta megnyitni file-t: ", nev );

	wavheader header;
	int negyvennegy = 44;
	if( sizeof( wavheader ) != negyvennegy )
		hiba( "sizeof( wavheader ) != 44!" );

	if( fread( &header, 1, 44, h ) != 44 )
		hiba( "Nem sikerult beolvasni wav-ot!" );

	int32_t hossz = header.hossz;
	int bit16 = 1;

	// Reszlet eldontese:
	int reszlet = 1;
	if( ucsoutani <= 0 )
		reszlet = 0;
	int reszhossz = ucsoutani-start;
	if( !reszlet ) {
		start = 0;
		if( bit16 )
			reszhossz = hossz/2;
		else
			reszhossz = hossz;
	}

	if( header.channel != 1 )
		hiba( "Wav file nem mono!: ", nev );

	if( bit16 ) {
		// 16 bites uncompressed:
		if( hossz%2 )
			hiba( "16 bites wav-nal nem paros byteszam! (abcd)" );
		size = reszhossz;
		if( start+reszhossz > hossz/2 )
			hiba( "Reszlet kilog wav-bol!" );
		lefoglal();
		qseek( h, start*2, SEEK_CUR );
		if( fread( tomb, 1, size*2, h ) != size*2 )
			hiba( "Nem sikerult beolvasni wav-ot!: ", nev );
	}
	else {
	int i; // Legacy for-loop scope.

		// 8 bites uncompressed:
		if( reszlet )
			hiba( "8 bitre meg nincs megirva reszlet!" );
		size = hossz;
		lefoglal();
		if( fread( tomb, 1, hossz, h ) != hossz )
			hiba( "Nem sikerult beolvasni wav-ot!: ", nev );
		unsigned char* bytetomb = (unsigned char*)tomb;
		for( i = size-1; i >= 0; i-- ) {
			tomb[i] = short((((short)(bytetomb[i]))-128)<<8);
		}
	}

	qclose( h );

	// Megkeresi maximalis amplitudot:
	int maxamp = 1;
	for( i = 0; i < size; i++ ) {
		if( tomb[i] > 0 ) {
			if( tomb[i] > maxamp )
				maxamp = tomb[i];
		}
		else {
			if( -tomb[i] > maxamp )
				maxamp = -tomb[i];
		}
	}
	// Kiszamolja szorzot:
	double szorzo = 32000.0*maxamplitudo/double(maxamp);
	for( i = 0; i < size; i++ ) {
		tomb[i] *= szorzo;
	}
}

void wav::save( char* nev ) {
	nevwavtest( nev );
	// Lemeri file hosszat:
	FILE* h = fopen( nev, "wb" );
	if( !h )
		hiba( "Nem tudta megnyitni file-t: ", nev );
	if( fwrite( tomb, 1, size*2, h ) != size*2 )
			hiba( "Nem sikerult irni wav-ot wav::save-ben!" );
	fclose( h );
}


// Elejere tesz egy loopot a vegebol, amit aztan le is vag:
void wav::loopol( int n ) {
	int i; // Legacy for-loop scope.

	if( n >= size )
		hiba( "n >= size!" );
	for( i = 0; i < n; i++ ) {
		double arany = ((double)i)/n;
		tomb[i] = arany*tomb[i] + (1-arany)*tomb[size-n+i];
	}
	size -= n;
}

// pwav elejet rateszi vegere:
void wav::vegereilleszt( wav* pwav, int n ) {
	int i; // Legacy for-loop scope.

	if( n >= size )
		hiba( "n >= size!" );
	if( n >= pwav->size )
		hiba( "n >= pwav->size!" );
	for( i = 0; i < n; i++ ) {
		double arany = ((double)i)/n;
		tomb[size-n+i] = (1-arany)*tomb[i] + arany*pwav->tomb[i];
	}
}

void wav::hangero( double szorzo ) {
	int i; // Legacy for-loop scope.

	for( i = 0; i < size; i++ ) {
		tomb[i] *= szorzo;
	}
}

// WAV2 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//short* tomb, mtomb;
wav2::wav2( wav* pwav ) {
	int i; // Legacy for-loop scope.

	mtomb = NULL;
	tomb = pwav->tomb;
	#ifndef WATCOM
		hiba( "wav2 csak Watcom alatt mukodik!" );
	#endif
	if( pwav->size > 64000 )
		hiba( "wav2-ben size > 64000!" );
	{ mtomb = NEW(short*) short[pwav->size]; }
	if( !mtomb )
		uzenet( "memory" );
	for( i = 0; i < pwav->size-1; i++ )
		mtomb[i] = tomb[i+1]-tomb[i];
	mtomb[pwav->size-1] = tomb[0]-tomb[pwav->size-1];
	ulsize = ((uint32_t)pwav->size)*65536;
	ulfuto = 0;
	plow = (unsigned short*)&ulfuto;
	phigh = plow+1;
}

void wav2::reset( int fazis ) {
	if( fazis < 0 )
		ulfuto = 0;
	else
		ulfuto = fazis*65536;
}

// dt 65536 szoros:
short wav2::getnextsample( int32_t dt ) {
	ulfuto += dt;
	if( ulfuto >= ulsize )
		ulfuto -= ulsize;
	return tomb[*phigh] + ((mtomb[*phigh]*(*plow))>>16);
}

