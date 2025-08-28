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

#include <math.h>

extern void abort();
void reach_error();

extern int __VERIFIER_nondet_int(void);
extern float __VERIFIER_nondet_float(void);

void __VERIFIER_assert(int cond) { if(!cond) { reach_error(); abort(); } }

/* Local prototypes. */
static int l_u_decomp4(float M[4][4], int indx[4]);
static void l_u_backsub4(float M[4][4], int indx[4], float v[4]);

/**************************************************************************
 *
 * Calculate the inversion of a n by m matrix via lower-upper decomposition
 * and backsubstitution.  Efficient and effective.
 *
 **************************************************************************/

/**************************************************************************
 * Function:   l_u_decomp4()
 *
 * Programmer: Jeremy Meredith
 *             March 18, 1998
 *
 * Purpose:
 *  Perform L-U decomposition on the 4x4 matrix M, return result in M
 *  Return the permutation vector in indx[]
 *
 * Modifications:
 *
 **************************************************************************/
static int
l_u_decomp4(float M[4][4], int indx[4])
{
    int i,j,k;
    float vv[4];

    for (i=0; i<4; i++)
    {
        float aamax=0;
        for (j=0; j<4; j++)
        {
            if (fabs(M[i][j]) > aamax)
                aamax = fabs(M[i][j]);
        }
        if (aamax == 0)
            return -1; /* singular */
        vv[i] = 1./aamax;
    }

    for (j=0; j<4; j++)
    {
         float aamax;
         int   imax;

         if (j>0)
         {
             for (i=0; i<j; i++)
             {
                 float sum = M[i][j];
                 if (i>0)
                 {
                     for (k=0; k<i; k++)
                     {
                         sum -= M[i][k]*M[k][j];
                     }
                     M[i][j] = sum;
                 }
             }
         }

         aamax = 0.;
         imax  = -1;
         for (i=j; i<4; i++)
         {
             float sum = M[i][j];
             float t;
             if (j>0)
             {
                 for (k=0; k<j; k++)
                 {
                     sum -= M[i][k]*M[k][j];
                 }
                 M[i][j] = sum;
             }
             t = vv[i]*fabs(sum);
             if (t >= aamax)
             {
                 imax  = i;
                 aamax = t;
             }
         }

         if (j != imax)
         {
             for (k=0; k<4; k++)
             {
                 float t    = M[imax][k];
                 M[imax][k] = M[j][k];
                 M[j][k]    = t;
             }
             vv[imax] = vv[j];
         }

         indx[j] = imax;

         if (j < 3)
         {
             float t;

             if (M[j][j]==0)
                 return -1; /* singular */

             t = 1. / M[j][j];
             for (i=j+1; i<4; i++)
             {
                 M[i][j] *= t;
             }
         }
    }
    if (M[3][3] == 0)
        return -1; /* singular */

    return 0;
}

/**************************************************************************
 * Function:   l_u_backsub4()
 *
 * Programmer: Jeremy Meredith
 *             March 18, 1998
 *
 * Purpose:
 *  Perform back-substitution on matrix M[][] and vector v[] with
 *  permutation vector indx[].
 *  Return the result in v[]
 *
 * Modifications:
 *   Lisa J. Roberts, Fri Nov 19 10:04:49 PST 1999
 *   Removed k, which was unused.
 *
 **************************************************************************/
static void
l_u_backsub4(float M[4][4], int indx[4], float v[4])
{
    int ii = -1;
    int i,j;

    for (i=0; i<4; i++)
    {
        int ll = indx[i];
        float sum = v[ll];
        v[ll] = v[i];

        if (ii >= 0)
        {
            for (j=ii; j<i; j++)
            {
                sum -= M[i][j]*v[j];
            }
        }
        else if (sum != 0)
        {
            ii = i;
        }
        v[i] = sum;
    }

    for (i=3; i>=0; i--)
    {
        float sum = v[i];
        if (i < 3)
        {
            for (j=i+1; j<4; j++)
            {
                sum -= M[i][j]*v[j];
            }
        }
        v[i] = sum / M[i][i];
    }
}

/**************************************************************************
 * Function:   matrix_invert()
 *
 * Programmer: Jeremy Meredith
 *             March 18, 1998
 *
 * Purpose:
 *   Invert matrix M.
 *   If M is invertible:  return a 0, and return M^(-1) in I.
 *   If M is singular:    return nonzero, return identity matrix in I.
 *
 * Notes:
 *   This makes a copy of M and transposes it.  This makes the back-
 *   substitution straightforward since we are working on rows instead
 *   of columns.  It also prevents modification of M.
 *   Also, 'I' need NOT be the identity matrix before calling this function.
 *
 * Modifications:
 *
 **************************************************************************/
static int
matrix_invert(float M[4][4], float I[4][4])
{
    int i,j;
    int indx[4];
    float T[4][4];
    int err;

    for (i=0; i<4; i++)
    {
        for (j=0; j<4; j++)
        {
            T[i][j] = M[j][i];        /* copy and transpose M */
            I[i][j] = (i==j ? 1 : 0); /* set I to identity    */
        }
    }

    err = l_u_decomp4(T, indx);

    if (! err)
    {
        for (i=0; i<4; i++)
        {
            l_u_backsub4(T, indx, I[i]);
        }
    }

    /* No need to transpose I since we transposed M */

    return err;
}

static void matrix_mul_point(float out[3], float in[3], float M[4][4])
{
    float tmp[3];
    tmp[0] = in[0]*M[0][0] + in[1]*M[1][0] + in[2]*M[2][0] + M[3][0];
    tmp[1] = in[0]*M[0][1] + in[1]*M[1][1] + in[2]*M[2][1] + M[3][1];
    tmp[2] = in[0]*M[0][2] + in[1]*M[1][2] + in[2]*M[2][2] + M[3][2];
    out[0] = tmp[0];
    out[1] = tmp[1];
    out[2] = tmp[2];
}

int main(void) {
    float M[4][4];
    float O[4][4];
    float I[4][4];
    float temp;
    for (int i=0; i<4; i++)
    {
        for (int j=0; j<4; j++)
        {
            temp = __VERIFIER_nondet_float();
            M[i][j] = temp;
            O[i][j] = temp;
        }
    }

    int result = matrix_invert(M, I);

    __VERIFIER_assert(result ||
                      M[0][0] * I[0][0] +
                      M[1][0] * I[0][1] +
                      M[2][0] * I[0][2] +
                      M[3][0] * I[0][3] == 1);

    __VERIFIER_assert(result ||
                      M[0][1] * I[0][0] +
                      M[1][1] * I[0][1] +
                      M[2][1] * I[0][2] +
                      M[3][1] * I[0][3] == 0);
    return 0;
}
