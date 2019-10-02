static unsigned int fast_seed = 0;

// Compute a pseudorandom integer.
// Output value in range [0, 32767]
static inline int fast_rand( void )
{
	fast_seed = ( 214013 * fast_seed + 2531011 );
	return ( fast_seed >> 16 ) & 0x7FFF;
}
