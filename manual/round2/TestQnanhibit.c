#include <stdio.h>

extern void abort();
void reach_error();

extern void* __VERIFIER_nondet_pointer(void);

void __VERIFIER_assert(int cond) { if(!cond) { reach_error(); abort(); } }

#if defined(__BORLANDC__)
# include <math.h>
# include <float.h>
#endif


int main() {
    {
        const char *const me = (const char*)(__VERIFIER_nondet_pointer());
        const float zero = 0.F;
        union {
            float flt32bit;
            int int32bit;
        } qnan;
        if (sizeof(float) != sizeof(int)) {
            fprintf(stderr, "%s: MADNESS:  sizeof(float)=%d != sizeof(int)=%d\n", me, (int)sizeof(float), (int)sizeof(int));
            return -1;
        }
        /// Check that the union vals are the same size if reaches this far
        __VERIFIER_assert(sizeof(qnan.flt32bit) == sizeof(qnan.int32bit));
        qnan.flt32bit = zero / zero;
        printf("-DTEEM_QNANHIBIT=%d\n", (qnan.int32bit >> 22) & 1);

        /// After dividing by zero the union values are NAN
        /// All bits in the Exponent of NAN are 1
        __VERIFIER_assert((int)((qnan.int32bit >> 23) & (unsigned int)(256)));

        /// For a Quiet NaN the Mantissa must start with a 1 bit
        __VERIFIER_assert((int)((qnan.int32bit >> 22) & 1));

        return (int)((qnan.int32bit >> 22) & 1);
    }
}

