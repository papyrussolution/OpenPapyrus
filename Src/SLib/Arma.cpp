// ARMA.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2007, 2010, 2011, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

static const double ARMA_INVERTMATRIX_INIT_VAL = 1.e6;

SLAPI ARMA::ARMA()
{
	P = 0;
	Q = 0;
	IterCount = 0;
}

SLAPI ARMA::~ARMA()
{
}

int SLAPI ARMA::GetModel(LVect * pVect) const
{
	return pVect ? pVect->copy(Model) : 0;
}

int SLAPI ARMA::GetR(LMatrix * pR) const
{
	return pR ? pR->copy(R) : 0;
}

int SLAPI ARMA::GetPhi(LVect * pVect) const
{
	return pVect ? pVect->copy(Phi) : 0;
}

void SLAPI ARMA::GetPQ(int * pP, int * pQ) const
{
	ASSIGN_PTR(pP, P);
	ASSIGN_PTR(pQ, Q);
}
//
// In pInitTSeries[P] most recent value has bigger index
//
int SLAPI ARMA::Init(int p, int q, const double * pInitTSeries)
{
	LMIDX i;

	P = p;
	Q = q;
	IterCount = 0;
	Model.init(P+Q+1);
	Phi.init(P+Q+1);
	R.init(P+Q+1, P+Q+1);
	for(i = 0; i < P+Q+1; i++)
		R.set(i, i, ARMA_INVERTMATRIX_INIT_VAL);
	Phi.set(0, 1L);
	for(i = P; i >= 1; i--)
		Phi.set(i, pInitTSeries[(size_t)(P-i)]);
	return 1;
}

int SLAPI ARMA::Init(int p, int q, const LVect * pModel, const LVect * pPhi, const LMatrix * pR)
{
	P = p;
	Q = q;
	IterCount = 0;
	Model.copy(*pModel);
	Phi.copy(*pPhi);
	if(pR)
		R.copy(*pR);
	else {
		R.init(P+Q+1, P+Q+1);
		for(int i = 0; i < P+Q+1; i++)
			R.set(i, i, ARMA_INVERTMATRIX_INIT_VAL);
	}
	return 1;
}
//
// GainFactor = (R * Phi) / (1 + Phi(Transp) * R * Phi))
//
int SLAPI ARMA::ComputeGainFactor(LVect * pGF) const
{
	LVect * p_dividend = R * Phi;
	LVect * p_temp     = Phi * R;
	p_dividend->div(1. + Phi.dot(*p_temp));
	delete p_temp;
	pGF->copy(*p_dividend);
	delete p_dividend;
	return 1;
}

double SLAPI ARMA::ComputePredictError(double val) const
{
	LVect temp;
	temp.copy(Phi);
	return (val - temp.dot(Model));
}

double SLAPI ARMA::Predict() const
{
	return -ComputePredictError(0);
}

int SLAPI ARMA::Step(double val)
{
	IterCount++;

	int ok = 1;
	LMIDX i;
	LVect gain_factor, vec_temp;
	double ape = ComputePredictError(val);
	gain_factor.init(P+Q+1);
	ComputeGainFactor(&gain_factor);

	vec_temp.copy(gain_factor);
	vec_temp.saxpy(ape, Model);
	Model.copy(vec_temp);

	double pe = ComputePredictError(val);
	//
	// Update reverse matrix
	// R = R - gain_factor * (Phi * R)
	//
	LVect   * p_phi_r    = Phi * R;
	LMatrix * p_gf_phi_r = gain_factor * *p_phi_r;
	ZDELETE(p_phi_r);
	R -= *p_gf_phi_r;
	ZDELETE(p_gf_phi_r);
	//
	// Update Phi vect
	//
	Phi.set(0, 1L);
	for(i = P; i > 1; i--)
		Phi.set(i, Phi.get(i-1));
	Phi.set(1, val);
	for(i = P+Q; i > P+1; i--)
		Phi.set(i, Phi.get(i-1));
	Phi.set(P+1, pe);
	return ok;
}
//
//
//
SLAPI RDI::RDI()
{
	P_Queue = 0;
}

SLAPI RDI::~RDI()
{
	delete P_Queue;
}

int SLAPI RDI::Init(const DblQueue * pInitQueue)
{
	ZDELETE(P_Queue);
	P_Queue = new DblQueue(*pInitQueue);
	return P_Queue ? 1 : (SLibError = SLERR_NOMEM, 0);
}

int SLAPI RDI::Init(const double * pInitQueue, uint interval)
{
	ZDELETE(P_Queue);
	P_Queue = new DblQueue(interval);
	for(uint i = 0; i < interval; i++)
		P_Queue->push(pInitQueue[i]);
	return 1;
}

double SLAPI RDI::StepDiff(double nextVal) const
{
	return (nextVal - P_Queue->get(0));
}

double SLAPI RDI::StepIntg(double diff) const
{
	return (P_Queue->get(0) + diff);
}

int SLAPI RDI::Step(double newVal)
{
	return P_Queue->push(newVal);
}

int SLAPI RDI::GetQueue(DblQueue * pQueue) const
{
	return (pQueue && P_Queue) ? pQueue->copy(*P_Queue) : 0;
}
//
//
//
SLAPI ARIMA::ARIMA() : ARMA()
{
}

int SLAPI ARIMA::Init(int p, int q, const double * pInitSeries, const DblQueue * pRDIInitQueue)
{
	int i;
	double * p_init_series = new double[p];
	Rdi.Init(pRDIInitQueue);
	if(Rdi.GetSeason() > 0) {
		for(i = 0; i < p; i++) {
			p_init_series[i] = Rdi.StepDiff(pInitSeries[i]);
			Rdi.Step(pInitSeries[i]);
		}
	}
	else
		for(i = 0; i < p; i++)
			p_init_series[i] = pInitSeries[i];
	ARMA::Init(p, q, p_init_series);
	delete p_init_series;
	return 1;
}

int SLAPI ARIMA::Init(int p, int q, const LVect * pModel, const LVect * pPhi, const LMatrix * pR, const DblQueue * pRDIInitQueue)
{
	Rdi.Init(pRDIInitQueue);
	ARMA::Init(p, q, pModel, pPhi, pR);
	return 1;
}

int SLAPI ARIMA::Step(double val)
{
	if(Rdi.GetSeason() > 0) {
		ARMA::Step(Rdi.StepDiff(val));
		Rdi.Step(val);
	}
	else
		ARMA::Step(val);
	return 1;
}

double SLAPI ARIMA::Predict() const
{
	double p = ARMA::Predict();
	return (Rdi.GetSeason() > 0) ? Rdi.StepIntg(p) : p;
}
//
//
#ifdef TEST_ARMA // {

struct DataEntry {
	LDATE  Dt;
	double Val;
};

static int SLAPI GetNextDataEntry(FILE * pInF, DataEntry * pEntry)
{
	char buf[128];
	if(fgets(buf, sizeof(buf), pInF)) {
		chomp(buf);
		char * p = strchr(buf, ';');
		if(p) {
			*p = 0;
			pEntry->Val = atof(p+1);
		}
		else
			pEntry->Val = 0L;
		strtodate(buf, DATF_DMY, &pEntry->Dt);
		return 1;
	}
	else {
		pEntry->Dt = ZERODATE;
		pEntry->Val = 0L;
		return 0;
	}
}

static void PrintOutRow(FILE * pOutF, LDATE dt, double val, double predictVal)
{
	char dt_buf[32];
	datefmt(&dt, DATF_DMY, dt_buf);
	double err = val - predictVal;
	double pct_err = val ? (100.0 * err / val) : 0.0;
	fprintf(pOutF, "%s, %12.2lf, %12.2lf, %12.2lf, %12.4lf\n", dt_buf, val, predictVal, err, pct_err);
}

int SLAPI InitSeries(int P, int Q, const char * pInFileName, const char * pOutFileName)
{
	int    i = 0;
  	char   dt_buf[32];
	double * p_init_series = new double[P];
	LDATE  prev_dt = ZERODATE;
	DataEntry entry;
	FILE *p_in = 0, *p_out = 0;
	if((p_in = fopen(pInFileName, "r")) == NULL || (p_out = fopen(pOutFileName, "w")) == NULL) {
		printf("Error open file");
		return 0;
	}
	for(i = 0; i < P && GetNextDataEntry(p_in, &entry) > 0; i++) {
		p_init_series[i] = entry.Val;
		datefmt(&entry.Dt, DATF_DMY, dt_buf);
	   	fprintf(p_out, "%s;%lf\n", dt_buf, entry.Val);

	}
	ARMA model;
	model.Init(P, Q, p_init_series);
	prev_dt = entry.Dt;
	while(GetNextDataEntry(p_in, &entry) > 0) {
		double predict_val = 0.0;
		for(i = diffdate(&entry.Dt, &prev_dt, 0); i > 1; i--) {
		    plusdate(&prev_dt, 1, 0);
			model.Step((predict_val = roundnev(model.Predict(), 0)));
			datefmt(&prev_dt, DATF_DMY, dt_buf);
			fprintf(p_out, "%s;%lf;ARMA PREDICT\n", dt_buf, predict_val);
		}
		model.Predict();
		model.Step(entry.Val);
		plusdate(&prev_dt, 1, 0);
		datefmt(&entry.Dt, DATF_DMY, dt_buf);
	   	fprintf(p_out, "%s;%lf\n", dt_buf, entry.Val);
	}
	SFile::ZClose(&p_in);
	SFile::ZClose(&p_out);
	return 1;
}

int SLAPI TestARMA(int P, int Q, const char * pInFileName, const char * pOutFileName)
{
	FILE * f_in = fopen(pInFileName, "r");
	FILE * f_out = fopen(pOutFileName, "w");
	DataEntry entry;
	double predict_val = 0.0;
	int    i;
	double * p_init_series = new double[P];
	LDATE  last_dt = ZERODATE;
	ARMA   model;
	if(f_in == 0) {
		printf("Error opening file %s\n", pInFileName);
		return 0;
	}
	if(f_out == 0) {
		printf("Error opening file %s\n", pOutFileName);
		return 0;
	}
	for(i = 0; i < P; i++)
		if(GetNextDataEntry(f_in, &entry)) {
			p_init_series[i] = entry.Val;
			//PrintOutRow(f_out, entry.Dt, entry.Val, 0L);
		}
	model.Init(P, Q, p_init_series);
	ZDELETE(p_init_series);
	while(GetNextDataEntry(f_in, &entry) > 0) {
		/*
		if(fabs(entry.Val) < 0.001) {
			entry.Val = predict_val = model.Predict();
			model.Step(entry.Val);
		}
		else {
		*/
			predict_val = model.Predict();
			model.Step(entry.Val);
		//}
		PrintOutRow(f_out, entry.Dt, entry.Val, predict_val);
		last_dt = entry.Dt;
	}
	//
	//
	//
	for(i = 0; i < 10; i++) {
		plusdate(&last_dt, 1, 0);
		predict_val = model.Predict();
		model.Step(predict_val);
		PrintOutRow(f_out, last_dt, predict_val, predict_val);
	}

	fprintf(f_out, "\n\n");

	LVect m;
	model.GetModel(&m);
	m.setname("Model");
	print(m, f_out, MKSFMTD(0, 8, NMBF_NOTRAILZ));

	fprintf(f_out, "\n\n");
	model.GetPhi(&m);
	m.setname("Phi");
	print(m, f_out, MKSFMTD(0, 8, NMBF_NOTRAILZ));

	/*
	for(i = 0; i < 20; i++) {
		plusdate(&entry.Dt, 1, 0);
		entry.Val = predict_val = model.Predict();
		model.Step(entry.Val);
		PrintOutRow(f_out, entry.Dt, entry.Val, predict_val);
	}
	*/
	SFile::ZClose(&f_in);
	SFile::ZClose(&f_out);
	return 1;
}

static void PrintQueue(const SQueue * pQueue)
{
	uint c = pQueue->getNumItems();
	for(uint i = 0; i < c; i++) {
		double v = *(double *)pQueue->get(i);
		printf("%5.0lf ", v);
	}
	printf("\n");
}

static int SLAPI TestQueue()
{
	printf("\n");
	SQueue q(sizeof(double), 10);
	for(double v = 0.0; v < 10000.0; v += 1.0) {
		if(((long)v) % 5 == 0) {
			q.pop();
			q.pop();
		}
		else
			q.push(&v);
		PrintQueue(&q);
	}
	return 1;
}

int SLAPI TestTimSerStat(const char * pInFileName, const char * pOutFileName)
{
	FILE * f_in = fopen(pInFileName, "r");
	FILE * f_out = fopen(pOutFileName, "w");
	DataEntry entry;

	uint i;
	if(f_in == 0) {
		printf("Error opening file %s\n", pInFileName);
		return 0;
	}
	if(f_out == 0) {
		printf("Error opening file %s\n", pOutFileName);
		return 0;
	}
	TimSerStat tss;
	tss.Init(400);
	while(GetNextDataEntry(f_in, &entry) > 0) {
		if(entry.Val > 0)
			tss.Step(entry.Val);
	}
	tss.Finish();

	double var = sqrt(tss.GetVar());
	double exp = tss.GetExp();

	rewind(f_in);
	tss.Init(400);
	while(GetNextDataEntry(f_in, &entry) > 0) {
		if(fabs(entry.Val - exp) < var)
			tss.Step(entry.Val);
		else {
			fabs(-1);
		}
	}
	tss.Finish();

	fprintf(f_out, "\n");
	fprintf(f_out, "Count    = %ld\n", tss.GetCount());
	fprintf(f_out, "NumLags  = %u\n", tss.GetNumLags());
	fprintf(f_out, "Sum      = %.6lf\n", tss.GetSum());
	fprintf(f_out, "Exp      = %.6lf\n", tss.GetExp());
	fprintf(f_out, "Variance = %.6lf\n", sqrt(tss.GetVar()));
	fprintf(f_out, "Autocorrelation\n");
	for(i = 0; i < tss.GetNumLags(); i++)
		fprintf(f_out, "%.10lf\n", tss.GetAutocorrel(i));

	TimSerSpikes tssp, tssp2;
	tssp.Init(1);
	for(i = 0; i < tss.GetNumLags(); i++)
		tssp.Step(i, tss.GetAutocorrel(i));
	fprintf(f_out, "\nSpikes\n");
	for(i = 0; i < tssp.GetNumSpikes(); i++) {
		long n;
		double s = tssp.GetSpike(i, &n);
		fprintf(f_out, "%5ld   %.10lf\n", n, s);
	}
	fprintf(f_out, "%\n\nMost Common Distance: 5ld\n\n", tssp.GetMostCommonDistance());
	tssp2.Init(1);
	for(i = 0; i < tssp.GetNumSpikes(); i++) {
		long n;
		double s = tssp.GetSpike(i, &n);
		tssp2.Step(n, s);
	}
	fprintf(f_out, "\nSpikes 2\n");
	for(i = 0; i < tssp2.GetNumSpikes(); i++) {
		long n;
		double s = tssp2.GetSpike(i, &n);
		fprintf(f_out, "%5ld   %.10lf\n", n, s);
	}
	SFile::ZClose(&f_in);
	SFile::ZClose(&f_out);
	return 1;
}

void TestArima(int P, int Q, const char * pInFileName, const char * pInFileName1, const char * pOutFileName)
{
	FILE *p_in = 0, *p_out = 0;
	int i = 0;
	double * p_init_series = new double[P];
	DataEntry entry;
	TimSerStat tss;
	TimSerSpikes tssp;

	//InitSeries(P, Q, pInFileName, pInFileName1);
	if(!pInFileName || (p_in = fopen(pInFileName, "r")) == NULL &&
		!pOutFileName || (p_out = fopen(pOutFileName, "w")) == NULL
	) {
		printf("Error");
		return;
	}
	tss.Init(366);
	while(GetNextDataEntry(p_in, &entry) > 0) {
		tss.Step(entry.Val);
	}
	tss.Finish();
	rewind(p_in);

	tssp.Init(1);
	for(i = 0; i < tss.GetNumLags(); i++)
		tssp.Step(i, tss.GetAutocorrel(i));
	long interval = tssp.GetMostCommonDistance();
	SQueue init_queue(sizeof(double), interval);
	RDI rdi;
	for(i = 0; i < interval && GetNextDataEntry(p_in, &entry) > 0; i++)
		init_queue.push(&entry.Val);
	rdi.Init(&init_queue);
	for(i = 0; i < P && GetNextDataEntry(p_in, &entry) > 0; i++) {
		p_init_series[i] = rdi.StepDiff(entry.Val);
		rdi.Step(entry.Val);
	}
	if(i >= P) {
		// ARIMA prediction
		rdi.GetQueue(&init_queue);
		double predict_val = 0.0;
		LDATE  last_dt = ZERODATE;
		ARIMA model;
		// prediction
		model.Init(P, Q, p_init_series, &init_queue);
		i = 0;
		while(GetNextDataEntry(p_in, &entry) > 0) {
			predict_val = roundnev(model.Predict(), 0);
			model.Step(entry.Val);
			last_dt = entry.Dt;
		}
		/*
		for(i = 0; i < 7; i++) {
			plusdate(&last_dt, 1, 0);
			predict_val = model.Predict();
			model.Step(predict_val);
			PrintOutRow(p_out, last_dt, predict_val, predict_val);
		}
		*/
		fprintf(p_out, "\n\n");
		LVect m;
		model.GetModel(&m);
		m.setname("Model");
		print(m, p_out, MKSFMTD(0, 8, NMBF_NOTRAILZ));

		fprintf(p_out, "\n\n");
		model.GetPhi(&m);
		m.setname("Phi");
		print(m, p_out, MKSFMTD(0, 8, NMBF_NOTRAILZ));
	}
	else
		printf("Error: not enought init data");
	ZDELETE(p_init_series);
	SFile::ZClose(&p_in);
	SFile::ZClose(&p_out);
}

void TestRdi(const char * pOutFileName)
{
	uint i = 0;
	SQueue queue(sizeof(double), 7);
	RDI rdi;
	FILE *p_out = 0;
	if((p_out = fopen(pOutFileName, "w")) == NULL) {
		printf("Cant open file %s", pOutFileName);
		return;
	}
	for(i = 1; i < 8; i++) {
		double j = i;
		queue.push(&j);
	}
	fprintf(p_out, "Queue\n");
	for(i = 0; i < 7; i++)
		fprintf(p_out, "%f ", *(double*) queue.get(i));
	rdi.Init(&queue);
	fprintf(p_out, "\n\n Diff. Vals.\n");
	for(i = 8; i < 11; i++) {
		fprintf(p_out, "%f ", rdi.StepDiff(i));
		double j = i;
   		rdi.Step(j);
	}
	rdi.GetQueue(&queue);
	fprintf(p_out, "\n\nChanged Queue\n");
	for(i = 0; i < 7; i++)
		fprintf(p_out, "%f ", *(double*) queue.get(i));
	SFile::ZClose(&p_out);
}

void main()
{
	//TestRdi("rdi.txt");
	//TestQueue();
	TestArima(3, 3, "baltika3", "armanorm", "baltika3.txt");
	//TestARMA(3, 3, "armanorm", "pilsner.txt");
	//TestTimSerStat("sales", "autocorr");
}

#endif // } TEST_ARMA
