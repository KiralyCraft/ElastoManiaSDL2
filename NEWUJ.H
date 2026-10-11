
// Dosextenderes forditasnal mindig ugyanott elszall, ha az itteni WATCOMOS
// ket define-t hasznalom akkor is!

/*#ifdef WATCOM
	#define NEW( a )    newnovel2() + new
	#define DELETE      { Newszam--; delete
#else
	#define NEW( a )    (a) *newnovel() = new
	#define DELETE      { Newszam--; delete
#endif*/


#define NEW( a )    new
#define DELETE      { delete

extern int32_t Newszam;

void** newnovel( void );
int newnovel2( void );

