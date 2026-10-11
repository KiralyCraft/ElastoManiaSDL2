#include	"ALL.H"

double K_pi = 3.141592;
double K_pip2 = K_pi*0.5;
double K_2pi = K_pi*2.0;

void k_memcpy( void *dest, const void *src, size_t n ) {
	//inkremental();
	memcpy( dest, src, n );
}

