// LVECT.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2007, 2008, 2010, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
//
//
//
LMatrix::LMatrix() : P_Name(0), NumRows(0), NumCols(0), Flags(0), P_Vals(0)
{
}

LMatrix::~LMatrix()
{
	delete P_Vals;
	delete P_Name;
}

int LMatrix::init(uint numRows, uint numCols)
{
	NumRows = numRows;
	NumCols = numCols;
	Flags = 0;
	P_Vals = new double[dim()];
	if(!P_Vals)
		return (SLibError = SLERR_NOMEM, 0);
	else {
		zero(-1, -1);
		return 1;
	}
}

int FASTCALL LMatrix::copy(const LMatrix & s)
{
	if(!init(s.NumRows, s.NumCols))
		return 0;
	else {
		memcpy(P_Vals, s.P_Vals, dim() * sizeof(double));
		return 1;
	}
}

void FASTCALL LMatrix::setname(const char * pName)
{
	delete P_Name;
	P_Name = newStr(pName);
}

int LMatrix::checktarget(uint row, uint col) const
{
	assert(row < NumRows);
	assert(col < NumCols);
	return (checkirange(row, 0U, NumRows-1) && checkirange(col, 0U, NumCols-1)) ? 1 : 0;
}

double * LMatrix::sget(uint row, uint col) const
{
	return &P_Vals[(size_t)(col * NumRows + row)];
}

double LMatrix::get(uint row, uint col) const
{
	return P_Vals && checktarget(row, col) ? *sget(row, col) : 0L;
}

LVect * LMatrix::getrow(uint r) const
{
	LVect * p_x = 0;
	if(checktarget(r, 0) && (p_x = new LVect) != 0 && p_x->init(NumCols))
		for(uint i = 0; i < NumCols; i++)
			p_x->set(i, *sget(r, i));
	else
		ZDELETE(p_x);
	return p_x;
}

LVect * LMatrix::getcol(uint c) const
{
	LVect * p_x = 0;
	if(!checktarget(0, c) || !(p_x = new LVect) || !p_x->init(NumRows, P_Vals+(size_t)(c*NumRows)))
		ZDELETE(p_x);
	return p_x;
}

int LMatrix::set(uint row, uint col, double val)
{
	if(checktarget(row, col)) {
		*sget(row, col) = val;
		return 1;
	}
	else
		return 0;
}

int LMatrix::setrow(uint row, const LVect & rVect)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint  i;
	THROW(checktarget(row, 0));
	THROW_V(rVect.size() <= NumCols, SLERR_MTX_INCOMPATDIM_VIMX);
	for(i = 0; i < rVect.size(); i++)
		*sget(row, i) = rVect.get(i);
	CATCHZOK
	return ok;
}

int LMatrix::setcol(uint col, const LVect & rVect)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	THROW(checktarget(0, col));
	THROW_V(rVect.size() <= NumRows, SLERR_MTX_INCOMPATDIM_VIMX);
	memcpy(P_Vals+(size_t)(col*NumRows), rVect.P_Vals, sizeof(double) * (size_t)rVect.size());
	CATCHZOK
	return ok;
}

void LMatrix::zero(uint row, uint col)
{
	if(P_Vals)
		if(row < 0)
			if(col < 0)
				memzero(P_Vals, dim() * sizeof(double));
			else {
				for(uint i = 0; i < NumRows; i++)
					set(i, col, 0L);
			}
		else
			if(col < 0) {
				for(uint i = 0; i < NumCols; i++)
					set(row, i, 0L);
			}
			else
				set(row, col, 0L);
}

void LMatrix::swaprows(uint r1, uint r2)
{
	if(r1 != r2)
		for(uint i = 0; i < NumCols; i++) {
			double temp = get(r1, i);
			set(r1, i, get(r2, i));
			set(r2, i, temp);
		}
}

int LMatrix::add(const LMatrix & s, int minus)
{
	if(cols() != s.cols() || rows() != s.rows())
		return (SLibError = SLERR_MTX_INCOMPATDIM_MMADD, 0);
	else {
		for(uint j = 0; j < NumCols; j++)
			for(uint i = 0; i < NumRows; i++) {
				double a = minus ? -s.get(i, j) : s.get(i, j);
				set(i, j, get(i, j) + a);
			}
		return 1;
	}
}

int FASTCALL LMatrix::operator += (const LMatrix & s)
{
	return add(s, 0);
}

int FASTCALL LMatrix::operator -= (const LMatrix & s)
{
	return add(s, 1);
}
//
//
//
LVect::LVect() : Dim(0), P_Vals(0)/*, P_Name(0)*/
{
}

LVect::LVect(uint dim) : Dim(0), P_Vals(0)
{
	init(dim, 0);
}

LVect::~LVect()
{
	delete [] P_Vals;
}

int LVect::init(uint dim, const double * pVals)
{
	ZDELETE(P_Vals);
	Dim = dim;
	if(Dim > 0) {
		P_Vals = new double[Dim];
		if(pVals)
			memcpy(P_Vals, pVals, Dim * sizeof(double));
		else
			zero(-1);
	}
	return 1;
}

int FASTCALL LVect::copy(const LVect & s)
{
	return init(s.Dim, s.P_Vals);
}

/*void FASTCALL LVect::setname(const char * pName)
{
	delete P_Name;
	P_Name = newStr(pName);
}*/

double FASTCALL LVect::get(uint p) const
{
	return checkupper((uint)p, (uint)Dim) ? P_Vals[p] : 0;
}

int LVect::set(uint p, double v)
{
	return checkupper((uint)p, (uint)Dim) ? ((P_Vals[p] = v), 1) : 0;
}

void LVect::FillWithSequence(double startVal, double incr)
{
	double value = startVal;
	for(uint i = 0; i < Dim; i++) {
        set(i, value);
        value += incr;
	}
}

int LVect::zero(uint p)
{
	if(p < 0) {
		memzero(P_Vals, Dim * sizeof(double));
		return 1;
	}
	else
		return set(p, 0);
}

void LVect::mult(double v)
{
	if(P_Vals)
		for(uint i = 0; i < Dim; i++)
			P_Vals[i] *= v;
}

void LVect::div(double v)
{
	if(P_Vals)
		for(uint i = 0; i < Dim; i++)
			P_Vals[i] /= v;
}

int LVect::add(const LVect & v)
{
	if(size() != v.size())
		return (SLibError = SLERR_MTX_INCOMPATDIM_VADD, 0);
	else {
		if(P_Vals && v.P_Vals)
			for(uint i = 0; i < Dim; i++)
				P_Vals[i] += v.P_Vals[i];
		return 1;
	}
}

double LVect::dot(const LVect & s) const // return this * s (scalar)
{
	double r = 0;
	if(P_Vals && s.P_Vals) {
		uint d = MIN(Dim, s.Dim);
		for(uint i = 0; i < d; i++)
			r += P_Vals[i] * s.P_Vals[i];
	}
	return r;
}

void LVect::saxpy(double a, const LVect & y) // this = this * a + y
{
	if(P_Vals && y.P_Vals) {
		uint d = MIN(Dim, y.Dim);
		for(uint i = 0; i < d; i++)
			P_Vals[i] = P_Vals[i] * a + y.P_Vals[i];
	}
}
//
//
//
LVect * FASTCALL operator * (const LMatrix & m, const LVect & v)
{
	EXCEPTVAR(SLibError);
	uint i, j;
	LVect * p_result = 0;
	THROW_V(m.cols() == v.size(), SLERR_MTX_INCOMPATDIM_MVMUL);
	THROW_V(p_result = new LVect, SLERR_NOMEM);
	THROW(p_result->init(m.NumRows));
	for(j = 0; j < m.cols(); j++) {
		uint col_idx = j * m.rows();
		double jv = v.P_Vals[j];
		for(i = 0; i < m.rows(); i++)
			p_result->P_Vals[i] += jv * m.P_Vals[col_idx + i];
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}
//
// z = y + a * x
//
LVect * gaxpy(const LMatrix & a, const LVect & x, const LVect & y)
{
	LVect * p_result = a * x;
	CALLPTRMEMB(p_result, add(y));
	return p_result;
}

LMatrix * FASTCALL operator * (const LMatrix & x, const LMatrix & y)
{
	EXCEPTVAR(SLibError);
	LMatrix * p_z = 0;
	uint i, j, k;
	THROW_V(x.cols() == y.rows(), SLERR_MTX_INCOMPATDIM_MMMUL);
	THROW_V(p_z = new LMatrix, SLERR_NOMEM);
	THROW(p_z->init(x.rows(), y.cols()));
	for(j = 0; j < y.cols(); j++) {
		for(k = 0; k < x.cols(); k++) {
			uint col_idx = j * p_z->rows();
		   	for(i = 0; i < x.rows(); i++)
				p_z->P_Vals[col_idx + i] += x.get(i, k) * y.get(k, j);
		}
	}
	CATCH
		ZDELETE(p_z);
	ENDCATCH
	return p_z;
}
//
// Product of row-vect[m] and matrix[m, n] : row-vect[n]
//
LVect * FASTCALL operator * (const LVect & x, const LMatrix & y)
{
	EXCEPTVAR(SLibError);
	LVect * p_z = 0;
	uint j, k;
	THROW_V(x.size() == y.rows(), SLERR_MTX_INCOMPATDIM_MMMUL);
	THROW_V(p_z = new LVect, SLERR_NOMEM);
	THROW(p_z->init(y.cols()));
	for(j = 0; j < y.cols(); j++)
		for(k = 0; k < x.size(); k++)
			p_z->P_Vals[j] += x.get(k) * y.get(k, j);
	CATCH
		ZDELETE(p_z);
	ENDCATCH
	return p_z;
}
//
// Product of column-vect[n] and row-vect[m] : matrix[n, m]
//
LMatrix * FASTCALL operator * (const LVect & x, const LVect & y)
{
	EXCEPTVAR(SLibError);
	LMatrix * p_z = 0;
	uint i, j;
	THROW_V(p_z = new LMatrix, SLERR_NOMEM);
	THROW(p_z->init(x.size(), y.size()));
	for(j = 0; j < y.size(); j++) {
		//uint col_idx = j * p_z->rows();
	   	for(i = 0; i < x.size(); i++) {
			// @v3.11.11 p_z->P_Vals[col_idx + i] += x.get(i) * y.get(j);
			p_z->set(i, j, p_z->get(i, j) + x.get(i) * y.get(j));
		}
	}
	CATCH
		ZDELETE(p_z);
	ENDCATCH
	return p_z;
}

void print(const LVect & vect, FILE * pF, long fmt)
{
	fprintf(pF, "\n[");
	/*if(vect.getname())
		fprintf(pF, vect.getname());*/
	fprintf(pF, ",1,%ld]\n", vect.size());
	for(uint i = 0; i < vect.size(); i++) {
		char buf[64];
		fprintf(pF, realfmt(vect.get(i), fmt, buf));
		if(i < (vect.size()-1))
			fputc(',', pF);
	}
}

void print(const LMatrix & matrix, FILE * pF, long fmt)
{
	fprintf(pF, "\n[");
	if(matrix.getname())
		fprintf(pF, matrix.getname());
	fprintf(pF, ",%ld,%ld]\n", matrix.rows(), matrix.cols());
	for(uint i = 0; i < matrix.rows(); i++) {
		for(uint j = 0; j < matrix.cols(); j++) {
			char buf[64];
			fprintf(pF, realfmt(matrix.get(i, j), fmt, buf));
			if(j < (matrix.cols()-1))
				fputc(',', pF);
		}
		fputc('\n', pF);
	}
}

static int read_header(FILE * pF, uint * pRows, uint * pCols, char * pName, size_t bufSize)
{
	int    ok = 1, c;
	char   buf[256], sub[64];
	uint   p;
	uint   num_rows = 0, num_cols = 0;
	StringSet ss(',', 0);
	while((c = fgetc(pF)) != 0 && c != '[')
		;
	THROW(c == '[');
	p = 0;
	while((c = fgetc(pF)) != 0 && c != ']' && c != '\n')
		buf[p++] = c;
	THROW(c == ']');
	buf[p] = 0;
	while((c = fgetc(pF)) != 0 && c != '\n')
		;
	THROW(c == '\n');
	ss.setBuf(buf, strlen(buf)+1);
	p = 0;
	THROW(ss.get(&p, sub, sizeof(sub)));
	strnzcpy(pName, strip(sub), bufSize);
	THROW(ss.get(&p, sub, sizeof(sub)));
	THROW(num_rows = atol(strip(sub)));
	THROW(ss.get(&p, sub, sizeof(sub)));
	THROW(num_cols = atol(strip(sub)));
	CATCHZOK
	ASSIGN_PTR(pRows, num_rows);
	ASSIGN_PTR(pCols, num_cols);
	return ok;
}

static int read_row(FILE * pF, LVect * pVect)
{
	int  ok = 1, c = 0;
	for(uint i = 0; i < pVect->size(); i++) {
		size_t p = 0;
		char buf[64];
		while((c = fgetc(pF)) != ',' && c != '\n' && c != 0) {
			THROW(isdec(c) || oneof4(c, '.', '-', 'e', 'E'));
			buf[p++] = c;
		}
		buf[p] = 0;
		pVect->set(i, satof(buf)); // @v10.7.9 atof-->satof
	}
	THROW(c == '\n');
	CATCHZOK
	return ok;
}

int read(LVect * pVect, FILE * pF)
{
	int    ok = 1;
	uint   rows = 0, cols = 0;
	char name_buf[64];
	THROW(read_header(pF, &rows, &cols, name_buf, sizeof(name_buf)));
	THROW(rows == 1);
	pVect->init(cols);
	//pVect->setname(name_buf);
	THROW(read_row(pF, pVect));
	CATCHZOK
	return ok;
}

int read(LMatrix * pMtx, FILE * pF)
{
	int    ok = 1;
	uint   rows = 0, cols = 0, i;
	char   name_buf[64];
	THROW(read_header(pF, &rows, &cols, name_buf, sizeof(name_buf)));
	pMtx->init(rows, cols);
	pMtx->setname(name_buf);
	for(i = 0; i < rows; i++) {
		LVect vect;
		vect.init(cols);
		THROW(read_row(pF, &vect));
		THROW(pMtx->setrow(i, vect));
	}
	CATCHZOK
	return ok;
}

int minv(LMatrix & a)
{
	double s, t, tq = 0., zr = 1.e-15;
	//double *pa, *pd, *ps, *p, *q;
	uint  pa, pd;
	uint  i, j, k, /*m, l,*/ lc;
	uint  N = a.rows();
	uint * le = static_cast<uint *>(SAlloc::M(N * sizeof(uint)));
	LVect q0;
	q0.init(N);
	/*
		Цикл по столбцам матрицы a
		pa - начальная позиция j-го столбца
		pd - диагональный элемент матрицы a
		j  - индекс столбца
	*/
	for(j = 0, pa = pd = 0; j < N; j++, pa++, pd++) {
		if(j > 0) {
			/*
				В вектор q0 записывается столбец по позиции pa (j-й)
			*/
			for(i = 0; i < N; i++)
				q0.set(i, a.get(i, j));
			/*
				Цикл по строкам матрицы a
				i - индекс строки
			*/
			for(i = 1; i < N; i++) {
				lc = i < j ? i : j;
				/*
					p - начальный элемент строки i
					t - скалярное произведение первых lc элементов строки i
						на первые lc элементов столбца j
				*/
				t = 0;
				for(k = 0; k < lc; k++)
					t += a.get(i, k) * q0.get(k);
				/*
					Из i-го элемента столбца q0 вычитаем t
				*/
				q0.set(i, q0.get(i) - t);
			}
			a.setcol(j, q0);
		}
		s = fabs(a.get(j, j));
		lc = j;
		for(k = j + 1; k < N; k++) {
			t = fabs(a.get(j, k));
			if(t > s) {
				s = t;
				lc = k;
			}
		}
		tq = tq > s ? tq : s;
		if(s < zr * tq) {
			SAlloc::F(le - j);
			return -1;
		}

		*le++ = lc;
		a.swaprows(lc, j);
		t = 1. / a.get(j, j);
		for(k = j + 1; k < N; k++)
			a.set(k, j, a.get(k, j) * t);
		a.set(j, j, t);
	}
	/*
		Цикл по столбцам матрицы a
	*/
	for(j = 1; j < N; j++)
		for(k = 1; k <= j; k++)
			a.set(k, j, a.get(k, j) * a.get(j, j));
	for(j = 1; j < N; j++) {
		for(i = 0; i < j; i++)
			q0.set(i, a.get(i, j));
		for(k = 0; k < j; k++) {
			t = 0.;
			for(i = k; i < j; i++)
				t -= a.get(k, k) * q0.get(i);
			q0.set(k, t);
		}
		for(i = 0; i < j; i++)
			a.set(i, j, q0.get(i));
	}
	for(j = N - 2; j >= 0; j--) {
		for(i = 0; i < N - j - 1; i++)
			q0.set(i, a.get(j+i+1, j));
		for(k = N - 1; k > j; k--) {
			t = -a.get(k, j-1);
			for(i = j + 1; i < k; i++)
				t -= a.get(k, i-1) * q0.get(i-j-1);
			q0.set(N-j-2, t);
		}
		for(i = 0; i < N - j - 1; i++)
			a.set(j+i+1, j, q0.get(i));
	}
	for(k = 0; k < N - 1; k++, pa++) {
		for(i = 0; i < N; i++)
			q0.set(i, a.get(i, k));
		for(j = 0; j < N; j++) {
			if(j > k) {
				t = 0.;
				i = j;
			}
			else {
				t = q0.get(j);
				i = k + 1;
			}
			for(; i < N; i++)
				t += a.get(j, i) * q0.get(i);
			q0.set(j, t);
		}
		for(i = 0; i < N; i++)
			a.set(i, k, q0.get(i));
	}
	for(j = N - 2, le--; j >= 0; j--) {
		le--;
		for(k = 0; k < N; k++) {
			t = a.get(k, j);
			a.set(k, j, a.get(k, *le));
			a.set(k, *le, t);
		}
	}
	SAlloc::F(le);
	return 0;
}
//
// Replaces the matrix a with its inverse.  The routine returns true
// if the inversion was successful, otherwise it returns false.
//
int inverse(LMatrix & a)
{
	uint  i, j;
	int   ret = 0;
	uint  n = a.rows();
	LMatrix ai;
	LVect col;
	int d;
	ai.init(n, n);
	col.init(n);
	uint * p_indx = new uint[n];
	if(ludcmp(a, p_indx, d)) {
		for(j = 0; j < n; j++) {
			for(i = 0; i < n; i++)
				col.set(i, 0);
			col.set(j, 1);
			lubksb(a, p_indx, col);
			ai.setcol(j, col);
		}
		a = ai;
		ret = 1;
	}
	delete [] p_indx; // @v9.8.4 @fix []
	return ret;
};
//
// Given a matrix a and permutation vector indx returned from ludcmp,
// this routines solves the set of linear equations a.x = b.  On
// input b holds the right-hand side vector.  On output, it holds the
// solution vector x. a and indx are not modified by this routine
// and can be left in place for successive colls with different
// right-hand sides b.
//
void lubksb(LMatrix & a, const uint indx[/*b.size()*/], LVect & b)
{
	uint ii = -1, j;
	double sum;
	uint n = a.rows();
	uint i;
	for(i = 0; i < n; i++) {
		sum = b.get(indx[i]);
		b.set(indx[i], b.get(i));
		if(ii > -1)
			for(j = ii; j <= i - 1; j++)
				sum -= a.get(i, j) * b.get(j);
		else if(sum)
			ii = i;
		b.set(i, sum);
	}
	for(i = n - 1; i >= 0; i--) {
		sum = b.get(i);
		for(j = i + 1; j < n; j++)
			sum -= a.get(i, j) * b.get(j);
		b.set(i, sum / a.get(i, i));
	}
}
//
// See above comment.  d is output as 1 or -1 depending on whether
// the number of row interchanges was even or odd, respectively.
// This routine is used in combination with lubksb to solve linear
// equations or invert a matrix.
//
int ludcmp(LMatrix & a, uint indx[/*a.rows()*/], int & d)
{
	uint   i, imax, j, k;
	double big, dum, sum, temp;
	uint   n = a.rows();
	LVect vv;
	vv.init(n);
	double zero = 0L;
	double one = 1;
	d = 1;
	for(i = 0; i < n; i++) {
		big = zero;
		for(j = 0; j < n; j++)
			if((temp = fabs(a.get(i, j))) > big)
				big = temp;
		if(big == zero) {
			//throw Exception("num::ludcmp", "Singular Matrix");
			return 0;
		}
		vv.set(i, one / big);
	}
	for(j = 0; j < n; j++) {
		for(i = 0; i < j; i++) {
			sum = a.get(i, j);
			for(k = 0; k < i; k++)
				sum -= a.get(i, k) * a.get(k, j);
			a.set(i, j, sum);
		}
		big = zero;
		for(i = j; i < n; i++) {
			sum = a.get(i, j);
			for(k = 0; k < j; k++)
				sum -= a.get(i, k) * a.get(k, j);
			a.set(i, j, sum);
			if((dum = vv.get(i) * fabs(sum)) >= big) {
				big = dum;
				imax = i;
			}
		}
		if(j != imax) {
			for(k = 0; k < n; k++) {
				dum = a.get(imax, k);
				a.set(imax, k, a.get(j, k));
				a.set(j, k, dum);
			}
			d = -d;
			vv.set(imax, vv.get(j));
		}
		indx[j] = imax;
		if(a.get(j, j) == 0)
			a.set(j, j, SMathConst::Min);
		if(j != n - 1) {
			dum = one / a.get(j, j);
			for(i = j + 1; i < n; i++)
				a.set(i, j, a.get(i, j) * dum);
		}
	}
	return 1;
}
//
//
//
LMatrix2D::LMatrix2D()
{
	InitUnit(1.0);
}

LMatrix2D & FASTCALL LMatrix2D::operator = (double s)
{
	return InitUnit(s);
}

LMatrix2D & FASTCALL LMatrix2D::operator = (const LMatrix2D & rS)
{
	memcpy(this, &rS, sizeof(*this));
	return *this;
}

int LMatrix2D::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, xx, rBuf));
	THROW(pCtx->Serialize(dir, yx, rBuf));
	THROW(pCtx->Serialize(dir, xy, rBuf));
	THROW(pCtx->Serialize(dir, yy, rBuf));
	THROW(pCtx->Serialize(dir, x0, rBuf));
	THROW(pCtx->Serialize(dir, y0, rBuf));
	CATCHZOK
	return ok;
}

LMatrix2D & LMatrix2D::InitUnit(double v)
{
	xx = v;
	yx = 0.0;
	yy = v;
	xy = 0.0;
	x0 = 0.0;
	y0 = 0.0;
	return *this;
}

LMatrix2D & LMatrix2D::InitScale(double x, double y)
{
	InitUnit(x);
	if(y != x)
		yy = y;
	return *this;
}

LMatrix2D & LMatrix2D::Mult(const LMatrix2D & rM1, const LMatrix2D & rM2)
{
	xx = rM1.xx*rM2.xx+rM1.yx*rM2.xy;
	yx = rM1.xx*rM2.yx+rM1.yx*rM2.yy;
	xy = rM1.xy*rM2.xx+rM1.yy*rM2.xy;
	yy = rM1.xy*rM2.yx+rM1.yy*rM2.yy;
	x0 = rM1.x0*rM2.xx+rM1.y0*rM2.xy+rM2.x0;
	y0 = rM1.x0*rM2.yx+rM1.y0*rM2.yy+rM2.y0;
	return *this;
}

LMatrix2D & LMatrix2D::Mult(double m)
{
	xx *= m;
	yx *= m;
	xy *= m;
	yy *= m;
	x0 *= m;
	y0 *= m;
	return *this;
}

LMatrix2D & LMatrix2D::ComputeAdjoint()
{
	//
	// adj (A) = transpose (C:cofactor (A,i,j))
	//
	//double a, b, c, d, tx, ty;
	//_cairo_matrix_get_affine(matrix, &a,  &b, &c,  &d, &tx, &ty);
	//cairo_matrix_init(matrix, d, -b, -c, a, c*ty-d*tx, b*tx-a*ty);

	LMatrix2D temp = *this;
	xx = temp.yy;
	yx = -temp.yx;
	xy = -temp.xy;
	yy = temp.xx;
	x0 = temp.xy * temp.y0 - temp.yy * temp.x0;
	y0 = temp.yx * temp.x0 - temp.xx * temp.y0;
	return *this;
}

int LMatrix2D::Invert()
{
	int    ok = 1;
	//
	// Simple scaling|translation matrices are quite common...
	//
	if(xy == 0.0 && yx == 0.0) {
		x0 = -x0;
		y0 = -y0;
		if(xx != 1.0) {
			if(xx == 0.0)
				ok = SLS.SetError(SLERR_MTX_INVERSE);
			else {
				xx = 1.0/xx;
				x0 *= xx;
			}
		}
		if(yy != 1.0) {
			if(yy == 0.0)
				ok = SLS.SetError(SLERR_MTX_INVERSE);
			else {
				yy = 1.0/yy;
				y0 *= yy;
			}
		}
	}
	else {
		//
		// inv (A) = 1/det (A) * adj (A)
		//
		double det = GetDeterminant();
		if(fisinf(det) || det == 0.0)
			ok = SLS.SetError(SLERR_MTX_INVERSE);
		else
			ComputeAdjoint().Mult(1.0/det);
	}
	return ok;
}

LMatrix2D & LMatrix2D::InitTranslate(double dx, double dy)
{
	xx = 1.0;
	yx = 0.0;
	yy = 1.0;
	xy = 0.0;
	x0 = dx;
	y0 = dy;
	return *this;
}

LMatrix2D & LMatrix2D::InitTranslate(SPoint2F p)
{
	xx = 1.0;
	yx = 0.0;
	yy = 1.0;
	xy = 0.0;
	x0 = p.x;
	y0 = p.y;
	return *this;
}

LMatrix2D & LMatrix2D::InitRotate(double teta)
{
	double s = sin(teta);
	double c = cos(teta);
	xx = c;
	yx = s;
	xy = -s;
	yy = c;
	x0 = 0.0;
	y0 = 0.0;
	return *this;
}

LMatrix2D & LMatrix2D::InitRotateDeg(double tetaDeg)
{
	return InitRotate(SMathConst::PiDiv180 * tetaDeg);
}

LMatrix2D & LMatrix2D::InitSkew(double angleX, double angleY)
{
	InitUnit(1.0);
	yx = tan(angleY);
	xy = tan(angleX);
	return *this;
}

LMatrix2D & LMatrix2D::InitSkewDeg(double angleXDeg, double angleYDeg)
	{ return InitSkew(SMathConst::PiDiv180 * angleXDeg, SMathConst::PiDiv180 * angleYDeg); }
int LMatrix2D::IsIdentical() const
	{ return (xx == yy && xx == 1.0 && x0 == 0.0 && y0 == 0.0 && yx == 0.0 && xy == 0.0); }
double LMatrix2D::GetDeterminant() const
	{ return xx * yy - yx * xy; }
SPoint2R & FASTCALL LMatrix2D::TransformDistance(SPoint2R & rP) const
	{ return rP.Set(xx * rP.x + xy * rP.y, yx * rP.x + yy * rP.y); }
SPoint2R & FASTCALL LMatrix2D::Transform(SPoint2R & rP) const
	{ return rP.Set(xx * rP.x + xy * rP.y + x0, yx * rP.x + yy * rP.y + y0); }
static LMatrix2D & FASTCALL Push_LMatrix2D(LMatrix2D & rMtx)
	{ return PushRecycledObject <LMatrix2D, 64> (rMtx); }

const LMatrix2D & FASTCALL operator * (const LMatrix2D & rLeft, const LMatrix2D & rRight)
{
	LMatrix2D temp;
	return Push_LMatrix2D(temp.Mult(rLeft, rRight));
}

int LMatrix2D::FromStr(const char * pStr, int fmt)
{
	int    ok = 1;
	int    err = 0;
	SStrScan scan(pStr);
	SString temp_buf;
	InitUnit();
	while(ok && scan.Skip().GetIdent(temp_buf)) {
		uint   min_arg = 0, max_arg = 0;
		enum {
			opMatrix = 1,
			opTranslate,
			opScale,
			opRotate,
			opSkewX,
			opSkewY
		};
		int    op = 0;
		if(temp_buf.IsEqiAscii("matrix")) {
			min_arg = max_arg = 6;
			op = opMatrix;
		}
		else if(temp_buf.IsEqiAscii("translate")) {
			min_arg = 1; max_arg = 2;
			op = opTranslate;
		}
		else if(temp_buf.IsEqiAscii("scale")) {
			min_arg = 1; max_arg = 2;
			op = opScale;
		}
		else if(temp_buf.IsEqiAscii("rotate")) {
			min_arg = 1; max_arg = 3;
			op = opRotate;
		}
		else if(temp_buf.IsEqiAscii("skewx")) {
			min_arg = 1; max_arg = 1;
			op = opSkewX;
		}
		else if(temp_buf.IsEqiAscii("skewy")) {
			min_arg = 1; max_arg = 1;
			op = opSkewY;
		}
		else
			break;
		uint   arg_count = 0;
		double arg_list[32];
		if(scan.Skip()[0] == '(') {
			scan.Incr();
			while(ok && scan.Skip()[0] != ')') {
				if(scan.GetNumber(temp_buf)) {
					if(arg_count < SIZEOFARRAY(arg_list)-1) {
						arg_list[arg_count++] = temp_buf.ToReal();
						if(scan.Skip()[0] == ',')
							scan.Incr();
					}
					else
						ok = 0;
				}
				else
					ok = 0;
			}
			if(arg_count < min_arg)
				ok = 0;
		}
		if(ok) {
			LMatrix2D temp;
			if(op == opMatrix) {
				temp.xx = arg_list[0];
				temp.yx = arg_list[1];
				temp.xy = arg_list[2];
				temp.yy = arg_list[3];
				temp.x0 = arg_list[4];
				temp.y0 = arg_list[5];
			}
			else if(op == opTranslate)
				temp.InitTranslate(arg_list[0], (arg_count > 1) ? arg_list[1] : 0.0);
			else if(op == opScale)
				temp.InitScale(arg_list[0], (arg_count > 1) ? arg_list[1] : arg_list[0]);
			else if(op == opRotate) {
				temp.InitRotate(arg_list[0]);
				if(arg_count >= 3) {
					double dx = arg_list[1];
					double dy = arg_list[2];
					LMatrix2D temp2;
					temp = temp2.InitTranslate(dx, dy) * temp * temp2.InitTranslate(-dx, -dy);
				}
			}
			else if(op == opSkewX)
				temp.InitSkew(arg_list[0], 0.0);
			else if(op == opSkewY)
				temp.InitSkew(0.0, arg_list[0]);
			else
				ok = 0;
			if(ok)
				*this = *this * temp;
		}
	}
	return ok;
}
//
//
//
LMatrix3D::LMatrix3D()
{
	InitUnit(1.0);
}

LMatrix3D & LMatrix3D::InitUnit(double mult)
{
	memzero(M, sizeof(M));
	M[0][0] = mult;
	M[1][1] = mult;
	M[2][2] = mult;
	M[3][3] = mult;
	return *this;
}

LMatrix3D & FASTCALL LMatrix3D::operator = (double s)
{
	return InitUnit(s);
}

LMatrix3D & FASTCALL LMatrix3D::operator = (const LMatrix3D & rS)
{
	memcpy(M, rS.M, sizeof(M));
	return *this;
}

LMatrix3D & LMatrix3D::InitScale(double x, double y, double z)
{
	InitUnit(1.0);
    M[0][0] = x;
    M[1][1] = y;
    M[2][2] = z;
	return *this;
}

LMatrix3D & LMatrix3D::InitRotateX(double teta)
{
	teta = degtorad(teta);
	double cos_teta = cos(teta);
	double sin_teta = sin(teta);
	InitUnit(1.0);
	M[1][1] = cos_teta;
	M[1][2] = -sin_teta;
	M[2][1] = sin_teta;
	M[2][2] = cos_teta;
	return *this;
}

LMatrix3D & LMatrix3D::InitRotateY(double teta)
{
	teta = degtorad(teta);
	double cos_teta = cos(teta);
	double sin_teta = sin(teta);
	InitUnit(1.0);
	M[0][0] = cos_teta;
	M[0][2] = -sin_teta;
	M[2][0] = sin_teta;
	M[2][2] = cos_teta;
	return *this;
}

LMatrix3D & LMatrix3D::InitRotateZ(double teta)
{
	teta = degtorad(teta);
	double cos_teta = cos(teta);
	double sin_teta = sin(teta);
	InitUnit(1.0);
	M[0][0] = cos_teta;
	M[0][1] = -sin_teta;
	M[1][0] = sin_teta;
	M[1][1] = cos_teta;
	return *this;
}

LMatrix3D & LMatrix3D::Mult(const LMatrix3D & rM1, const LMatrix3D & rM2)
{
	for(uint i = 0; i < 4; i++) {
		M[i][0] = rM1.M[i][0]*rM2.M[0][0] + rM1.M[i][1]*rM2.M[1][0] + rM1.M[i][2]*rM2.M[2][0] + rM1.M[i][3]*rM2.M[3][0];
		M[i][1] = rM1.M[i][0]*rM2.M[0][1] + rM1.M[i][1]*rM2.M[1][1] + rM1.M[i][2]*rM2.M[2][1] + rM1.M[i][3]*rM2.M[3][1];
		M[i][2] = rM1.M[i][0]*rM2.M[0][2] + rM1.M[i][1]*rM2.M[1][2] + rM1.M[i][2]*rM2.M[2][2] + rM1.M[i][3]*rM2.M[3][2];
		M[i][3] = rM1.M[i][0]*rM2.M[0][3] + rM1.M[i][1]*rM2.M[1][3] + rM1.M[i][2]*rM2.M[2][3] + rM1.M[i][3]*rM2.M[3][3];
	}
	return *this;
}

//#define TEST_LMATRIX
// @todo Заменить на регулярный SLTEST
#ifdef TEST_LMATRIX 

void main()
{
	const char * p_infname = "matrix.";
	FILE * f = fopen(p_infname, "r");
	if(f == 0) {
		printf("Error opening file %s\n", p_infname);
		return;
	}
	LMatrix A, m1, m2, * p_rm = 0;
	if(read(&A, f) == 0) {
		printf("Error reading matrix A\n");
		fclose(f);
		return;
	}
	if(read(&m1, f) == 0) {
		printf("Error reading matrix m1\n");
		fclose(f);
		return;
	}
	if(read(&m2, f) == 0) {
		printf("Error reading matrix m2\n");
		fclose(f);
		return;
	}

	p_rm = m1 * m2;
	if(p_rm == 0) {
		printf("Error mult m1 * m2\n");
		fclose(f);
		return;
	}
	fclose(f);
	f = 0;

	f = fopen("mtx_res.", "w");
	if(f == 0) {
		printf("Error opening file mtx_res for writing\n");
		return;
	}

	inverse(A);
	print(A, f, MKSFMTD(0, 8, NMBF_NOTRAILZ));
	printf("\n");

	print(m1, f, MKSFMTD(0, 8, NMBF_NOTRAILZ));
	printf("\n");
	print(m2, f, MKSFMTD(0, 8, NMBF_NOTRAILZ));
	printf("\n");
	p_rm->setname("m1 * m2");
	print(*p_rm, f, MKSFMTD(0, 8, NMBF_NOTRAILZ));
	printf("\n");
	fclose(f);
	delete p_rm;
}

#endif

