/* 
 * Routines to convert a double	to engineering notation
 * and the reverse.
 *
 * J.M.	Belleman - May 2004
 */
#include "numbers.h"

#define	MICRO "\316\274"

/* Smallest power of then for which there is a prefix defined.
   If the set of prefixes will be extended, change this	constant
   and update the table	"prefix". */
#define	PREFIX_START (-24)

static char *prefix[] = {
	"y", "z", "a", "f", "p", "n", MICRO, "m", "",
	"k", "M", "G", "T", "P", "E", "Z", "Y"
};

static double factor[] = {
	1.e-24, 1.e-21, 1.e-18, 1.e-15, 1.e-12, 1.e-9, 1.e-6, 1.e-3,
	1., 1.e3, 1.e6, 1.e9, 1.e12, 1.e15, 1.e18, 1.e21, 1.e24
};

#define	PREFIX_END (PREFIX_START+\
(int)((sizeof(prefix)/sizeof(char *)-1)*3))

void nb_eng(double value, int digits, int numeric, char *out)
{
	int expof10;
	unsigned char *res = out;

	if (value < 0.) {
		*res++ = '-';
		value = -value;
	}

	expof10 = (int) log10(value);
	if (expof10 > 0)
		expof10 = (expof10 / 3) * 3;
	else
		expof10 = (-expof10 + 3) / 3 * (-3);

	value *= pow(10, -expof10);

	if (value >= 1000.) {
		value /= 1000.0;
		expof10 += 3;
	} else if (value >= 100.0)
		digits -= 2;
	else if (value >= 10.0)
		digits -= 1;

	if (numeric || (expof10 < PREFIX_START) || (expof10 > PREFIX_END))
		sprintf(res, "%.*fe%d", digits - 1, value, expof10);
	else
		sprintf(res, "%.*f %s", digits - 1, value,
				prefix[(expof10 - PREFIX_START) / 3]);
}

/* 
 * Convert a numerical string followed by a single SI multiplier
 * to a	double.	In fact, it will even accept oddities like 
 * 1.0e3m (Wow!	One thousand milli, like a real	saleman).
 * A single space separating the number	from the multiplier
 * is accepted.	Unrecognised multipliers are ignored.
 *
 * -- Jerome, 20081019: handles mu character thanks to a simple
 * extra check (*end == '\316'). But it's a smelly fix, I'll give
 * you that.
 */
double nb_engtod(const char *s)
{
	int i;
	char *end;
	double d, m;

	d = strtod(s, &end);
	if (end == NULL)
		return d;
	if (*end == ' ')
		end++;
	if (*end == '\0')
		return d;
	if (isalpha(*end) || *end == '\316') {
		for (i = 0; i < 17; i++) {
			if (*end == *prefix[i]) {
				d *= factor[i];
				break;
			}
		}
	}
	return d;
}
