// LSS_LIN.CPP
// Copyright (c) A.Sobolev 2004, 2007, 2010, 2016, 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
// Fit the data (x_i, y_i) to the linear relationship
//
//	 Y = c0 + c1 x
//
// returning,
//
//   c0, c1  --  coefficients
//   cov00, cov01, cov11  --  variance-covariance matrix of c0 and c1,
//   sumsq   --   sum of squares of residuals
//
//   This fit can be used in the case where the errors for the data are
//   uknown, but assumed equal for all points. The resulting
//   variance-covariance matrix estimates the error in the coefficients
//   from the observed variance of the points around the best fit line.
//
SLAPI LssLin::LssLin()
{
	THISZERO();
}

int SLAPI LssLin::Solve(const LVect & x, const LVect & y)
{
	if(x.size() != y.size())
		return 0;
	else {
		double m_x = 0.0;
		double m_y = 0.0;
		double m_dx2 = 0.0;
		double m_dxdy = 0.0;
		LMIDX  i;
		THISZERO();
		const  LMIDX n = x.size();
		for(i = 0; i < n; i++) {
			m_x += (x.get(i) - m_x) / (i + 1);
			m_y += (y.get(i) - m_y) / (i + 1);
		}
		for(i = 0; i < n; i++) {
			const double dx = x.get(i) - m_x;
			const double dy = y.get(i) - m_y;
			m_dx2  += (dx * dx - m_dx2)  / (i + 1);
			m_dxdy += (dx * dy - m_dxdy) / (i + 1);
		}
		//
		// In terms of y = a + b x
		//
		{
			double s2 = 0, d2 = 0;
			B = m_dxdy / m_dx2;
			A = m_y - m_x * B;
			//
			// Compute chi^2 = \sum (y_i - (a + b * x_i))^2
			//
			for(i = 0; i < n; i++) {
				const double dx = x.get(i) - m_x;
				const double dy = y.get(i) - m_y;
				const double d = dy - B * dx;
				d2 += d * d;
			}
			s2 = d2 / (n - 2); // chisq per degree of freedom
			Cov00 = s2 * (1 / n) * (1 + m_x * m_x / m_dx2);
			Cov11 = s2 * 1 / (n * m_dx2);
			Cov01 = s2 * (-m_x) / (n * m_dx2);
			SumSq = d2;
		}
		return 1;
	}
}

int SLAPI LssLin::Solve(uint count, const double * pX, const double * pY)
{
	if(count > 2) {
		double m_x = 0.0;
		double m_y = 0.0;
		double m_dx2 = 0.0;
		double m_dxdy = 0.0;
		uint   i;
		THISZERO();
		for(i = 0; i < count; i++) {
			m_x += (pX[i] - m_x) / (i + 1);
			m_y += (pY[i] - m_y) / (i + 1);
		}
		for(i = 0; i < count; i++) {
			const double dx = pX[i] - m_x;
			const double dy = pY[i] - m_y;
			m_dx2  += (dx * dx - m_dx2)  / (i + 1);
			m_dxdy += (dx * dy - m_dxdy) / (i + 1);
		}
		//
		// In terms of y = a + b x
		//
		{
			double s2 = 0.0;
			double d2 = 0.0;
			B = m_dxdy / m_dx2;
			A = m_y - m_x * B;
			//
			// Compute chi^2 = \sum (y_i - (a + b * x_i))^2
			//
			for(i = 0; i < count; i++) {
				//const double dx = pX[i] - m_x;
				//const double dy = pY[i] - m_y;
				const double d = (pY[i] - m_y) - B * (pX[i] - m_x);
				d2 += d * d;
			}
			s2 = d2 / (count - 2.0); // chisq per degree of freedom
			Cov00 = s2 * (1.0 / count) * (1.0 + m_x * m_x / m_dx2);
			Cov11 = s2 * 1.0 / (count * m_dx2);
			Cov01 = s2 * (-m_x) / (count * m_dx2);
			SumSq = d2;
		}
	}
	return 1;
}

double SLAPI LssLin::Estimation(double x, double * pYErr) const
{
	double y = A + B * x;
	if(pYErr)
		*pYErr = sqrt(Cov00 + x * (2 * Cov01 + Cov11 * x));
	return y;
}
//
//
//
//#define LSS_LIN_TEST

#ifdef LSS_LIN_TEST // {

int SLAPI LssTest(const char * pFileName)
{
	const LDATE start_date = encodedate(1, 1, 2000);
	FILE * f_in = fopen(pFileName, "r");
	FILE * f_out = fopen("lss_test.out", "w");
	if(f_in == 0) {
		printf("Error: Unable open file %s\n", pFileName);
		return 0;
	}
	LMIDX count = 0;
	char input_buf[256];
	while(fgets(input_buf, sizeof(input_buf), f_in))
		if(*strip(input_buf))
			count++;
	LVect vect_x, vect_y;
	vect_x.init(count);
	vect_y.init(count);
	rewind(f_in);
	for(count = 0; fgets(input_buf, sizeof(input_buf), f_in); count++) {
		input_buf[sizeof(input_buf)-1] = 0;
		StringSet ss(';', input_buf);
		char field[64];
		uint pos = 0;
		if(ss.get(&pos, field, sizeof(field))) {
			LDATE dt;
			strtodate(strip(field), DATF_DMY, &dt);
			long x = diffdate(dt, start_date);
			vect_x.set(count, x);
		}
		else {
			printf("Error: Unable accept x-value\n");
			return 0;
		}
		if(ss.get(&pos, field, sizeof(field))) {
			long y = atol(strip(field));
			vect_y.set(count, y);
		}
		else {
			printf("Error: Unable accept y-value\n");
			return 0;
		}
	}
	LssLin lss;
	lss.Solve(vect_x, vect_y);
	fprintf(f_out, "A = %lf\nB = %lf\n", lss.A, lss.B);
	fclose(f_in);
	fclose(f_out);
	return 1;
}

#endif // } LSS_LIN_TEST
