/*
*                            Copyright (c) 2000 - 2015
*                    Lawrence Livermore National Security, LLC.
*              Produced at the Lawrence Livermore National Laboratory
*                                LLNL-CODE-442911
*                              All rights reserved.
* 
* This file is part of VisIt. For details, see https://visit.llnl.gov/.
* 
* Redistribution  and  use   in  source  and  binary  forms,   with  or  without
* modification, are permitted provided that the following conditions are met:
* 
*  - Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the disclaimer below.
*  - Redistributions in  binary form must reproduce the above  copyright notice,
*    this  list of  conditions  and  the  disclaimer (as  noted  below)  in  the
*    documentation and/or other materials provided with the distribution.
*  - Neither the name of the LLNS/LLNL nor  the names of its contributors may be
*    used to  endorse or  promote products  derived from  this software  without
*    specific prior written permission.
* 
* THIS SOFTWARE IS  PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS  "AS IS"
* AND ANY  EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED  TO, THE
* IMPLIED WARRANTIES  OF MERCHANTABILITY AND  FITNESS FOR  A PARTICULAR  PURPOSE
* ARE  DISCLAIMED. IN   NO  EVENT SHALL  LAWRENCE  LIVERMORE  NATIONAL SECURITY,
* LLC, THE U.S.  DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE  FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER  CAUSED AND ON ANY THEORY
* OF LIABILITY,  WHETHER  IN  CONTRACT,  STRICT  LIABILITY,  OR  TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT OF THE  USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Additional BSD Notice
* 
* 1. This notice  is required to be  provided under our  contract with  the U.S.
*    Department of Energy  (DOE). This work was  produced at Lawrence  Livermore
*    National Laboratory under Contract No. DE-AC52-07NA27344 with the DOE.
* 
* 2. Neither the  United  States  Government  nor  Lawrence  Livermore  National
*    Security, LLC  nor any of their  employees, makes any  warranty, express or
*    implied, or  assumes  any  liability or  responsibility  for the  accuracy,
*    completeness, or  usefulness of  any information,  apparatus,  product,  or
*    process  disclosed,  or   represents  that  its  use   would  not  infringe
*    privately-owned rights.
* 
* 3. Also, reference  herein to any  specific commercial  products,  process, or
*    services by  trade name,  trademark,  manufacturer  or otherwise  does  not
*    necessarily  constitute  or  imply   its  endorsement,  recommendation,  or
*    favoring by the  United States  Government  or Lawrence  Livermore National
*    Security, LLC.  The views and opinions of  authors expressed  herein do not
*    necessarily  state or  reflect those  of the  United  States  Government or
*    Lawrence  Livermore  National  Security, LLC,  and shall  not be  used  for
*    advertising or product endorsement purposes./
*/

/*
 * Aug 27, 2025
 * Modified by PACLab Arg-C Transformer v0.0.0 and development team for use as
 * a benchmark for Static Verification tools
*/

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

