/*  NOTICE: Change of Copyright Status
 *
 *  The author of this module, Carsten Grammes, has expressed in
 *  personal email that he has no more interest in this code, and
 *  doesn't claim any copyright. He has agreed to put this module
 *  into the public domain.
 *
 *  Lars Hecking  15-02-1999
 */

/*
 *      Matrix algebra, part of
 *
 *      Nonlinear least squares fit according to the
 *      Marquardt-Levenberg-algorithm
 *
 *      added as Patch to Gnuplot (v3.2 and higher)
 *      by Carsten Grammes
 *      Experimental Physics, University of Saarbruecken, Germany
 *
 *      Previous copyright of this module:   Carsten Grammes, 1993
 *
 */
#include <gnuplot.h>
#pragma hdrstop

/*****************************************************************/

#define Swap(a, b)   {double _temp = (a); (a) = (b); (b) = _temp;}
/* HBB 20010424: unused: */
/* #define WINZIG	      1e-30 */

/*****************************************************************
    internal prototypes
*****************************************************************/

/*****************************************************************
    first straightforward vector and matrix allocation functions
*****************************************************************/

/* allocates a double vector with n elements */
double * vec(int n)
{
	if(n < 1)
		return NULL;
	else {
		double * dp = (double *)SAlloc::M(n * sizeof(double));
		return dp;
	}
}

/* allocates a double matrix */
double ** matr(int rows, int cols)
{
	double ** m = 0;
	if(rows > 0 && cols > 0) {
		m = (double **)SAlloc::M(rows * sizeof(m[0]));
		m[0] = (double *)SAlloc::M(rows * cols * sizeof(m[0][0]));
		for(int i = 1; i < rows; i++)
			m[i] = m[i-1] + cols;
	}
	return m;
}

void free_matr(double ** m)
{
	if(m) {
		SAlloc::F(m[0]);
		SAlloc::F(m);
	}
}

double * redim_vec(double ** v, int n)
{
	*v = (n < 1) ? 0 : (double *)SAlloc::R(*v, n * sizeof((*v)[0]));
	return *v;
}

/*****************************************************************

     Solve least squares Problem C*x+d = r, |r| = min!, by Given rotations
     (QR-decomposition). Direct implementation of the algorithm
     presented in H.R.Schwarz: Numerische Mathematik, Equation 7.33

     If 'd == NULL', d is not accessed: the routine just computes the QR
     decomposition of C and exits.

*****************************************************************/

//void Givens(double ** ppC, double * pD, double * pX, int N, int n)
void GnuPlot::Givens(double ** ppC, double * pD, double * pX, int N, int n)
{
	int i, j, k;
	double w, gamma, sigma, rho, temp;
	double epsilon = DBL_EPSILON;   /* FIXME (?) */
	// 
	// First, construct QR decomposition of C, by 'rotating away'
	// all elements of C below the diagonal. The rotations are
	// stored in place as Givens coefficients rho.
	// Vector d is also rotated in this same turn, if it exists
	// 
	for(j = 0; j < n; j++) {
		for(i = j + 1; i < N; i++) {
			if(ppC[i][j]) {
				if(fabs(ppC[j][j]) < epsilon * fabs(ppC[i][j])) {
					// find the rotation parameters 
					w = -ppC[i][j];
					gamma = 0;
					sigma = 1;
					rho = 1;
				}
				else {
					w = fsignz(ppC[j][j]) * sqrt(ppC[j][j] * ppC[j][j] + ppC[i][j] * ppC[i][j]);
					if(w == 0)
						Eex3("w = 0 in Givens();  Cjj = %g,  Cij = %g", ppC[j][j], ppC[i][j]);
					gamma = ppC[j][j] / w;
					sigma = -ppC[i][j] / w;
					rho = (fabs(sigma) < gamma) ? sigma : fsignz(sigma) / gamma;
				}
				ppC[j][j] = w;
				ppC[i][j] = rho; /* store rho in place, for later use */
				for(k = j + 1; k < n; k++) {
					// rotation on index pair (i,j) 
					temp = gamma * ppC[j][k] - sigma * ppC[i][k];
					ppC[i][k] = sigma * ppC[j][k] + gamma * ppC[i][k];
					ppC[j][k] = temp;
				}
				if(pD) { // if no d vector given, don't use it 
					temp = gamma * pD[j] - sigma * pD[i]; // rotate d 
					pD[i] = sigma * pD[j] + gamma * pD[i];
					pD[j] = temp;
				}
			}
		}
	}
	if(!pD) // stop here if no d was specified 
		return;
	// solve R*x+d = 0, by backsubstitution 
	for(i = n - 1; i >= 0; i--) {
		double s = pD[i];
		for(k = i + 1; k < n; k++)
			s += ppC[i][k] * pX[k];
		if(ppC[i][i] == 0)
			Eex("Singular matrix in Givens()");
		pX[i] = -s / ppC[i][i];
	}
}
// 
// Given a triangular Matrix R, compute (R^T * R)^(-1), by forward then back substitution
// R, I are n x n Matrices, I is for the result. Both must already be allocated.
// 
// Will only calculate the lower triangle of I, as it is symmetric
// 
//void Invert_RtR(double ** ppR, double ** ppI, int n)
void GnuPlot::Invert_RtR(double ** ppR, double ** ppI, int n)
{
	int i, j, k;
	// fill in the I matrix, and check R for regularity : 
	for(i = 0; i < n; i++) {
		for(j = 0; j < i; j++) // upper triangle isn't needed 
			ppI[i][j] = 0;
		ppI[i][i] = 1;
		if(!ppR[i][i])
			Eex("Singular matrix in Invert_RtR");
	}
	// Forward substitution: Solve R^T * B = I, store B in place of I 
	for(k = 0; k < n; k++) {
		for(i = k; i < n; i++) { /* upper half needn't be computed */
			double s = ppI[i][k];
			for(j = k; j < i; j++) /* for j<k, I[j][k] always stays zero! */
				s -= ppR[j][i] * ppI[j][k];
			ppI[i][k] = s / ppR[i][i];
		}
	}
	// Backward substitution: Solve R * A = B, store A in place of B 
	for(k = 0; k < n; k++) {
		for(i = n - 1; i >= k; i--) { /* don't compute upper triangle of A */
			double s = ppI[i][k];
			for(j = i + 1; j < n; j++)
				s -= ppR[i][j] * ppI[j][k];
			ppI[i][k] = s / ppR[i][i];
		}
	}
}

//void lu_decomp(double ** ppA, int n, int * pIndx, double * pD)
void GnuPlot::LuDecomp(double ** ppA, int n, int * pIndx, double * pD)
{
	int i, imax = -1, j, k; /* HBB: added initial value, to shut up gcc -Wall */
	double large, dummy, temp, ** ar, ** lim, * limc, * ac, * dp, * vscal;
	dp = vscal = vec(n);
	*pD = 1.0;
	for(ar = ppA, lim = &(ppA[n]); ar < lim; ar++) {
		large = 0.0;
		for(ac = *ar, limc = &(ac[n]); ac < limc;)
			if((temp = fabs(*ac++)) > large)
				large = temp;
		if(large == 0.0)
			IntError(NO_CARET, "Singular matrix in LU-DECOMP");
		*dp++ = 1 / large;
	}
	ar = ppA;
	for(j = 0; j < n; j++, ar++) {
		for(i = 0; i < j; i++) {
			ac = &(ppA[i][j]);
			for(k = 0; k < i; k++)
				*ac -= ppA[i][k] * ppA[k][j];
		}
		large = 0.0;
		dp = &(vscal[j]);
		for(i = j; i < n; i++) {
			ac = &(ppA[i][j]);
			for(k = 0; k < j; k++)
				*ac -= ppA[i][k] * ppA[k][j];
			if((dummy = *dp++ *fabs(*ac)) >= large) {
				large = dummy;
				imax = i;
			}
		}
		if(j != imax) {
			ac = ppA[imax];
			dp = *ar;
			for(k = 0; k < n; k++, ac++, dp++)
				Swap(*ac, *dp);
			*pD = -(*pD);
			vscal[imax] = vscal[j];
		}
		pIndx[j] = imax;
		if(*(dp = &(*ar)[j]) == 0)
			*dp = 1e-30;

		if(j != n - 1) {
			dummy = 1 / (*ar)[j];
			for(i = j + 1; i < n; i++)
				ppA[i][j] *= dummy;
		}
	}
	SAlloc::F(vscal);
}

void lu_backsubst(double ** a, int n, int * indx, double * b)
{
	int i, memi = -1, ip, j;
	double sum, * bp, * bip, * ac;
	double ** ar = a;
	for(i = 0; i < n; i++, ar++) {
		ip = indx[i];
		sum = b[ip];
		b[ip] = b[i];
		if(memi >= 0) {
			ac = &((*ar)[memi]);
			bp = &(b[memi]);
			for(j = memi; j <= i - 1; j++)
				sum -= *ac++ **bp++;
		}
		else if(sum)
			memi = i;
		b[i] = sum;
	}
	ar--;
	for(i = n - 1; i >= 0; i--) {
		ac = &(*ar)[i + 1];
		bp = &(b[i + 1]);
		bip = &(b[i]);
		for(j = i + 1; j < n; j++)
			*bip -= *ac++ **bp++;
		*bip /= (*ar--)[i];
	}
}

/*****************************************************************

    Sum up squared components of a vector
    In order to reduce rounding errors in summing up the entries
    of a vector, we employ the Neumaier variant of the Kahan and
    Babuska algorithm:
    A. Neumaier, Rundungsfehleranalyse einiger Verfahren zur
    Summation endlicher Summen, Z. angew. Math. Mechanik, 54:39-51, 1974

*****************************************************************/
double sumsq_vec(int n, const double * x)
{
	int i;
	double s;
	double c = 0.0;
	if(!x || n == 0)
		return 0;
	s =  x[0] * x[0];
	for(i = 1; i < n; i++) {
		double xi = x[i] * x[i];
		double t  = s + xi;
		if(fabs(s) >= fabs(xi))
			c += ((s - t) + xi);
		else
			c += ((xi - t) + s);
		s = t;
	}
	s += c;
	return s;
}
//
// Euclidean norm of a vector
//
double enorm_vec(int n, const double * x)
{
	return sqrt(sumsq_vec(n, x));
}
