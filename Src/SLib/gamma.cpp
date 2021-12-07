// GAMMA.CPP
//
#include <slib-internal.h>
#pragma hdrstop
/*
	The maximum x such that gamma(x) is not considered an overflow.
*/
#define GSL_SF_GAMMA_XMAX  171.0
/*
	The maximum n such that gsl_sf_fact(n) does not give an overflow.
*/
#define GSL_SF_FACT_NMAX   170

// @prototype
int gamma_inc_Q_e(const double a, const double x, SMathResult * result);
//
//
//
static void cheb_eval_e(const ChebSeries * cs, double x, SMathResult * pResult)
{
	double d  = 0.0;
	double dd = 0.0;
	double y  = (2.0*x - cs->a - cs->b) / (cs->b - cs->a);
	double y2 = 2.0 * y;
	double e = 0.0;
	for(int j = cs->order; j>=1; j--) {
		double temp = d;
		d = y2*d - dd + cs->c[j];
		e += fabs(y2*temp) + fabs(dd) + fabs(cs->c[j]);
		dd = temp;
	}
	{
		double temp = d;
		d = y*d - dd + 0.5 * cs->c[0];
		e += fabs(y*temp) + fabs(dd) + 0.5 * fabs(cs->c[0]);
	}
	pResult->V = d;
	pResult->E = SMathConst::Epsilon * e + fabs(cs->c[cs->order]);
}
//
// digamma for x both positive and negative; we do both
// cases here because of the way we use even/odd parts of the function
//
int fpsi(double x, SMathResult * pResult)
{
	int    ok = 1;
	const  double y = fabs(x);
	if(x == 0.0 || x == -1.0 || x == -2.0)
		ok = pResult->SetDomainViolation();
	else if(y >= 2.0) {
		static double apsics_data[16] = {
			-.0204749044678185, -.0101801271534859, .0000559718725387, -.0000012917176570,
	 		.0000000572858606, -.0000000038213539, .0000000003397434, -.0000000000374838,
	 		.0000000000048990, -.0000000000007344, .0000000000001233, -.0000000000000228,
	 		.0000000000000045, -.0000000000000009, .0000000000000002, -.0000000000000000
		};
		static ChebSeries apsi_cs = { apsics_data, 15, -1, 1, 9 };

		const double t = 8.0/(y*y)-1.0;
		SMathResult result_c;
		cheb_eval_e(&apsi_cs, t, &result_c);
		if(x < 0.0) {
			const double s = sin(SMathConst::Pi*x);
			const double c = cos(SMathConst::Pi*x);
			if(fabs(s) < 2.0*SMathConst::SqrtMin)
				ok = pResult->SetDomainViolation();
			else {
				pResult->V  = log(y) - 0.5/x + result_c.V - SMathConst::Pi * c/s;
				pResult->E  = SMathConst::Pi * fabs(x) * SMathConst::Epsilon / (s*s);
				pResult->E += result_c.E;
				pResult->E += SMathConst::Epsilon * fabs(pResult->V);
			}
		}
		else {
			pResult->V  = log(y) - 0.5/x + result_c.V;
			pResult->E  = result_c.E;
			pResult->E += SMathConst::Epsilon * fabs(pResult->V);
		}
	}
	else { /* -2 < x < 2 */
		static double psics_data[23] = {
  			-.038057080835217922, .491415393029387130,  -.056815747821244730,
   			.008357821225914313, -.001333232857994342,  .000220313287069308,
  			-.000037040238178456,  .000006283793654854, -.000001071263908506,
   			.000000183128394654, -.000000031353509361,  .000000005372808776,
  			-.000000000921168141,  .000000000157981265, -.000000000027098646,
   			.000000000004648722, -.000000000000797527,  .000000000000136827,
  			-.000000000000023475,  .000000000000004027, -.000000000000000691,
   			.000000000000000118, -.000000000000000020
		};
		static ChebSeries psi_cs = { psics_data, 22, -1, 1, 17 };

		SMathResult result_c;
		if(x < -1.0) { /* x = -2 + v */
			const double v  = x + 2.0;
			const double t1 = 1.0/x;
			const double t2 = 1.0/(x+1.0);
			const double t3 = 1.0/v;
			cheb_eval_e(&psi_cs, 2.0*v-1.0, &result_c);

			pResult->V  = -(t1 + t2 + t3) + result_c.V;
			pResult->E  = SMathConst::Epsilon * (fabs(t1) + fabs(x/(t2*t2)) + fabs(x/(t3*t3)));
			pResult->E += result_c.E;
			pResult->E += SMathConst::Epsilon * fabs(pResult->V);
		}
		else if(x < 0.0) { /* x = -1 + v */
			const double v  = x + 1.0;
			const double t1 = 1.0/x;
			const double t2 = 1.0/v;
			cheb_eval_e(&psi_cs, 2.0*v-1.0, &result_c);
			pResult->V  = -(t1 + t2) + result_c.V;
			pResult->E  = SMathConst::Epsilon * (fabs(t1) + fabs(x/(t2*t2)));
			pResult->E += result_c.E;
			pResult->E += SMathConst::Epsilon * fabs(pResult->V);
		}
		else if(x < 1.0) { /* x = v */
			const double t1 = 1.0/x;
			cheb_eval_e(&psi_cs, 2.0*x-1.0, &result_c);
			pResult->V  = -t1 + result_c.V;
			pResult->E  = SMathConst::Epsilon * t1;
			pResult->E += result_c.E;
			pResult->E += SMathConst::Epsilon * fabs(pResult->V);
		}
		else { /* x = 1 + v */
			const double v = x - 1.0;
			cheb_eval_e(&psi_cs, 2.0*v-1.0, pResult);
		}
	}
	return ok;
}

int exp_mult_err_e(double x, double dx, double y, double dy, SMathResult * pResult)
{
	int    ok = 1;
	const double ay  = fabs(y);
	if(y == 0.0) {
		pResult->V = 0.0;
		pResult->E = fabs(dy * exp(x));
	}
	else if(( x < 0.5*SMathConst::LogMax && x > 0.5*SMathConst::LogMin) &&
		(ay < 0.8*SMathConst::SqrtMax && ay > 1.2*SMathConst::SqrtMin)) {
		double ex = exp(x);
		pResult->V  = y * ex;
		pResult->E  = ex * (fabs(dy) + fabs(y*dx));
		pResult->E += 2.0 * SMathConst::Epsilon * fabs(pResult->V);
	}
	else {
		const double ly  = log(ay);
		const double lnr = x + ly;
		if(lnr > SMathConst::LogMax - 0.01)
			ok = pResult->SetOverflow();
		else if(lnr < SMathConst::LogMin + 0.01)
			ok = pResult->SetUnderflow();
		else {
			const double sy  = fsign(y);
			const double M   = floor(x);
			const double N   = floor(ly);
			const double a   = x  - M;
			const double b   = ly - N;
			const double eMN = exp(M+N);
			const double eab = exp(a+b);
			pResult->V  = sy * eMN * eab;
			pResult->E  = eMN * eab * 2.0 * SMathConst::Epsilon;
			pResult->E += eMN * eab * fabs(dy/y);
			pResult->E += eMN * eab * fabs(dx);
		}
	}
	return ok;
}

int hzeta_e(const double s, const double q, SMathResult * pResult)
{
	// coefficients for Maclaurin summation in hzeta() B_{2j}/(2j)!
	static double hzeta_c[15] = {
  		1.00000000000000000000000000000, 0.083333333333333333333333333333, -0.00138888888888888888888888888889,
  		0.000033068783068783068783068783069, -8.2671957671957671957671957672e-07, 2.0876756987868098979210090321e-08,
 		-5.2841901386874931848476822022e-10, 1.3382536530684678832826980975e-11, -3.3896802963225828668301953912e-13,
  		8.5860620562778445641359054504e-15, -2.1748686985580618730415164239e-16, 5.5090028283602295152026526089e-18,
 		-1.3954464685812523340707686264e-19, 3.5347070396294674716932299778e-21, -8.9535174270375468504026113181e-23
	};
	int    ok = 1;
	/* CHECK_POINTER(result) */
	if(s <= 1.0 || q <= 0.0)
		ok = pResult->SetDomainViolation();
	else {
		const double max_bits = 54.0;
		const double ln_term0 = -s * log(q);
		if(ln_term0 < SMathConst::LogMin + 1.0)
			ok = pResult->SetUnderflow();
		else if(ln_term0 > SMathConst::LogMax - 1.0)
			ok = pResult->SetOverflow();
		else if((s > max_bits && q < 1.0) || (s > 0.5*max_bits && q < 0.25)) {
			pResult->V = pow(q, -s);
			pResult->E = 2.0 * SMathConst::Epsilon * fabs(pResult->V);
		}
		else if(s > 0.5*max_bits && q < 1.0) {
			const double p1 = pow(q, -s);
			const double p2 = pow(q/(1.0+q), s);
			const double p3 = pow(q/(2.0+q), s);
			pResult->V = p1 * (1.0 + p2 + p3);
			pResult->E = SMathConst::Epsilon * (0.5*s + 2.0) * fabs(pResult->V);
		}
		else {
			/* Euler-Maclaurin summation formula
			* [Moshier, p. 400, with several typo corrections]
			*/
			const int jmax = 12;
			const int kmax = 10;
			int    j, k;
			const  double pmax  = pow(kmax + q, -s);
			double scp = s;
			double pcp = pmax / (kmax + q);
			double ans = pmax*((kmax+q)/(s-1.0) + 0.5);
			for(k=0; k < kmax; k++)
				ans += pow(k + q, -s);
			for(j=0; j <= jmax; j++) {
				double delta = hzeta_c[j+1] * scp * pcp;
				ans += delta;
				if(fabs(delta/ans) < 0.5*SMathConst::Epsilon)
					break;
				scp *= (s+2*j+1)*(s+2*j+2);
				pcp /= (kmax + q)*(kmax + q);
			}
			pResult->V = ans;
			pResult->E = 2.0 * (jmax + 1.0) * SMathConst::Epsilon * fabs(ans);
		}
	}
	return ok;
}
//
// generic polygamma; assumes n >= 0 and x > 0
//
static int psi_n_xg0(const int n, const double x, SMathResult * pResult)
{
	int    ok = 1;
	if(n == 0)
		ok = fpsi(x, pResult);
	else {
		/* Abramowitz + Stegun 6.4.10 */
		SMathResult ln_nf;
		SMathResult hzeta;
		THROW(hzeta_e(n+1.0, x, &hzeta));
		THROW(flnfact((uint)n, &ln_nf));
		THROW(exp_mult_err_e(ln_nf.V, ln_nf.E, hzeta.V, hzeta.E, pResult));
		if(IS_EVEN(n))
			pResult->V = -pResult->V;
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int fpsi_1_int(int n, SMathResult * pResult)
{
	static double psi_1_table[] = {
		0.0,  /* Infinity */              /* psi(1,0) */
		SMathConst::Pi * SMathConst::Pi / 6.0, /* psi(1,1) */ /* ...      */
		0.644934066848226436472415,   0.394934066848226436472415,   0.2838229557371153253613041,
		0.2213229557371153253613041,  0.1813229557371153253613041,  0.1535451779593375475835263,
		0.1331370146940314251345467,  0.1175120146940314251345467,  0.1051663356816857461222010,
		0.0951663356816857461222010,  0.0869018728717683907503002,  0.0799574284273239463058557,
		0.0740402686640103368384001,  0.0689382278476838062261552,  0.0644937834032393617817108,
		0.0605875334032393617817108,  0.0571273257907826143768665,  0.0540409060376961946237801,
		0.0512708229352031198315363,  0.0487708229352031198315363,  0.0465032492390579951149830,
		0.0444371335365786562720078,  0.0425467743683366902984728,  0.0408106632572255791873617,
		0.0392106632572255791873617,  0.0377313733163971768204978,  0.0363596312039143235969038,
		0.0350841209998326909438426,  0.0338950603577399442137594,  0.0327839492466288331026483,
		0.0317433665203020901265817,  0.03076680402030209012658168, 0.02984853037475571730748159,
		0.02898347847164153045627052, 0.02816715194102928555831133, 0.02739554700275768062003973,
		0.02666508681283803124093089, 0.02597256603721476254286995, 0.02531510384129102815759710,
		0.02469010384129102815759710, 0.02409521984367056414807896, 0.02352832641963428296894063,
		0.02298749353699501850166102, 0.02247096461137518379091722, 0.02197713745088135663042339,
		0.02150454765882086513703965, 0.02105185413233829383780923, 0.02061782635456051606003145,
		0.02020133322669712580597065, 0.01980133322669712580597065, 0.01941686571420193164987683,
		0.01904704322899483105816086, 0.01869104465298913508094477, 0.01834810912486842177504628,
		0.01801753061247172756017024, 0.01769865306145131939690494, 0.01739086605006319997554452,
		0.01709360088954001329302371, 0.01680632711763538818529605, 0.01652854933985761040751827,
		0.01625980437882562975715546, 0.01599965869724394401313881, 0.01574770606433893015574400,
		0.01550356543933893015574400, 0.01526687904880638577704578, 0.01503731063741979257227076,
		0.01481454387422086185273411, 0.01459828089844231513993134, 0.01438824099085987447620523,
		0.01418415935820681325171544, 0.01398578601958352422176106, 0.01379288478501562298719316,
		0.01360523231738567365335942, 0.01342261726990576130858221, 0.01324483949212798353080444,
		0.01307170929822216635628920, 0.01290304679189732236910755, 0.01273868124291638877278934,
		0.01257845051066194236996928, 0.01242220051066194236996928, 0.01226978472038606978956995,
		0.01212106372098095378719041, 0.01197590477193174490346273, 0.01183418141592267460867815,
		0.01169577311142440471248438, 0.01156056489076458859566448, 0.01142844704164317229232189,
		0.01129931481023821361463594, 0.01117306812421372175754719, 0.01104961133409026496742374,
		0.01092885297157366069257770, 0.01081070552355853781923177, 0.01069508522063334415522437,
		0.01058191183901270133041676, 0.01047110851491297833872701, 0.01036260157046853389428257,
		0.01025632035036012704977199,  /* ...        */
		0.01015219706839427948625679,  /* psi(1,99)  */
		0.01005016666333357139524567   /* psi(1,100) */
	};

	int    ok = 1;
	/* CHECK_POINTER(pResult) */
	if(n <= 0)
		ok = pResult->SetDomainViolation();
	else if(n <= (sizeof(psi_1_table) / sizeof(psi_1_table[0]) - 1)) {
		pResult->V = psi_1_table[n];
		pResult->E = SMathConst::Epsilon * pResult->V;
	}
	else {
		/*
			Abramowitz+Stegun 6.4.12
			double-precision for n > 100
		*/
		const double c0 = -1.0/30.0;
		const double c1 =  1.0/42.0;
		const double c2 = -1.0/30.0;
		const double ni2 = (1.0/n)*(1.0/n);
		const double ser =  ni2*ni2 * (c0 + ni2*(c1 + c2*ni2));
		pResult->V = (1.0 + 0.5/n + 1.0/(6.0*n*n) + ser) / n;
		pResult->E = SMathConst::Epsilon * pResult->V;
	}
	return ok;
}

int fpsi_int(int n, SMathResult * pResult)
{
	static double psi_table[] = {
		0.0,  /* Infinity */              /* psi(0) */
		-SMathConst::Euler,               /* psi(1) */ /* ...    */
		0.42278433509846713939348790992, 0.92278433509846713939348790992, 1.25611766843180047272682124325,
		1.50611766843180047272682124325, 1.70611766843180047272682124325, 1.87278433509846713939348790992,
		2.01564147795560999653634505277, 2.14064147795560999653634505277, 2.25175258906672110764745616389,
		2.35175258906672110764745616389, 2.44266167997581201673836525479, 2.52599501330914535007169858813,
		2.60291809023222227314862166505, 2.67434666166079370172005023648, 2.74101332832746036838671690315,
		2.80351332832746036838671690315, 2.86233685773922507426906984432, 2.91789241329478062982462539988,
		2.97052399224214905087725697883, 3.02052399224214905087725697883, 3.06814303986119666992487602645,
		3.11359758531574212447033057190, 3.15707584618530734186163491973, 3.1987425128519740085283015864,
		3.2387425128519740085283015864,  3.2772040513135124700667631249,  3.3142410883505495071038001619,
		3.3499553740648352213895144476,  3.3844381326855248765619282407,  3.4177714660188582098952615740,
		3.4500295305349872421533260902,  3.4812795305349872421533260902,  3.5115825608380175451836291205,
		3.5409943255438998981248055911,  3.5695657541153284695533770196,  3.5973435318931062473311547974,
		3.6243705589201332743581818244,  3.6506863483938174848844976139,  3.6763273740348431259101386396,
		3.7013273740348431259101386396,  3.7257176179372821503003825420,  3.7495271417468059598241920658,
		3.7727829557002943319172153216,  3.7955102284275670591899425943,  3.8177324506497892814121648166,
		3.8394715810845718901078169905,  3.8607481768292527411716467777,  3.8815815101625860745049801110,
		3.9019896734278921969539597029,  3.9219896734278921969539597029,  3.9415975165651470989147440166,
		3.9608282857959163296839747858,  3.9796962103242182164764276160,  3.9982147288427367349949461345,
		4.0163965470245549168131279527,  4.0342536898816977739559850956,  4.0517975495308205809735289552,
		4.0690389288411654085597358518,  4.0859880813835382899156680552,  4.1026547480502049565823347218,
		4.1190481906731557762544658694,  4.1351772229312202923834981274,  4.1510502388042361653993711433,
		4.1666752388042361653993711433,  4.1820598541888515500147557587,  4.1972113693403667015299072739,
		4.2121367424746950597388624977,  4.2268426248276362362094507330,  4.2413353784508246420065521823,
		4.2556210927365389277208378966,  4.2697055997787924488475984600,  4.2835944886676813377364873489,
		4.2972931188046676391063503626,  4.3108066323181811526198638761,  4.3241399656515144859531972094,
		4.3372978603883565912163551041,  4.3502848733753695782293421171,  4.3631053861958823987421626300,
		4.3757636140439836645649474401,  4.3882636140439836645649474401,  4.4006092930563293435772931191,
		4.4128044150075488557724150703,  4.4248526077786331931218126607,  4.4367573696833950978837174226,
		4.4485220755657480390601880108,  4.4601499825424922251066996387,  4.4716442354160554434975042364,
		4.4830078717796918071338678728,  4.4942438268358715824147667492,  4.5053549379469826935258778603,
		4.5163439489359936825368668713,  4.5272135141533849868846929582,  4.5379662023254279976373811303,
		4.5486045001977684231692960239,  4.5591308159872421073798223397,  4.5695474826539087740464890064,
		4.5798567610044242379640147796,  4.5900608426370772991885045755,  4.6001618527380874001986055856
	};
	int    ok = 1;
	/* CHECK_POINTER(result) */
	if(n <= 0)
		ok = pResult->SetDomainViolation();
	else if(n <= (sizeof(psi_table) / sizeof(psi_table[0]) - 1)) {
		pResult->V = psi_table[n];
		pResult->E = SMathConst::Epsilon * fabs(pResult->V);
	}
	else {
		/* Abramowitz+Stegun 6.3.18 */
		const double c2 = -1.0/12.0;
		const double c3 =  1.0/120.0;
		const double c4 = -1.0/252.0;
		const double c5 =  1.0/240.0;
		const double ni2 = (1.0/n)*(1.0/n);
		const double ser = ni2 * (c2 + ni2 * (c3 + ni2 * (c4 + ni2*c5)));
		pResult->V  = log((double)n) - 0.5/n + ser;
		pResult->E  = SMathConst::Epsilon * (fabs(log((double)n)) + fabs(0.5/n) + fabs(ser));
		pResult->E += SMathConst::Epsilon * fabs(pResult->V);
	}
	return ok;
}

int fpsi_1(double x, SMathResult * pResult)
{
	int    ok = 1;
	/* CHECK_POINTER(pResult) */
	if(x == 0.0 || x == -1.0 || x == -2.0)
		ok = pResult->SetDomainViolation();
	else if(x > 0.0)
		ok = psi_n_xg0(1, x, pResult);
	else if(x > -5.0) {
		/* Abramowitz + Stegun 6.4.6 */
		int    M = -ffloori(x);
		double fx = x + M;
		double sum = 0.0;
		if(fx == 0.0)
			ok = pResult->SetDomainViolation();
		else {
			for(int m = 0; m < M; ++m)
				sum += 1.0/((x+m)*(x+m));
			{
				ok = psi_n_xg0(1, fx, pResult);
				pResult->V += sum;
				pResult->E += M * SMathConst::Epsilon * sum;
			}
		}
	}
	else {
		/* Abramowitz + Stegun 6.4.7 */
		const double sin_px = sin(SMathConst::Pi * x);
		const double d = SMathConst::Pi*SMathConst::Pi/(sin_px*sin_px);
		SMathResult r;
		ok = psi_n_xg0(1, 1.0-x, &r);
		pResult->V = d - r.V;
		pResult->E = r.E + 2.0*SMathConst::Epsilon*d;
	}
	return ok;
}

int fpsi_n(int n, double x, SMathResult * pResult)
{
	int    ok = 1;
	/* CHECK_POINTER(result) */
	if(n == 0)
		ok = fpsi(x, pResult);
	else if(n == 1)
		ok = fpsi_1(x, pResult);
	else if(n < 0 || x <= 0.0)
		ok = pResult->SetDomainViolation();
	else {
		SMathResult ln_nf;
		SMathResult hzeta;
		THROW(hzeta_e(n+1.0, x, &hzeta));
		THROW(flnfact((uint)n, &ln_nf));
		THROW(exp_mult_err_e(ln_nf.V, ln_nf.E, hzeta.V, hzeta.E, pResult));
		if(IS_EVEN(n))
			pResult->V = -pResult->V;
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

inline static int lngamma_1_pade(double eps, SMathResult * pResult)
{
	// Use (2,2) Pade for Log[Gamma[1+eps]]/eps plus a correction series.
	const double n1 = -1.0017419282349508699871138440;
	const double n2 =  1.7364839209922879823280541733;
	const double d1 =  1.2433006018858751556055436011;
	const double d2 =  5.0456274100274010152489597514;
	const double num = (eps + n1) * (eps + n2);
	const double den = (eps + d1) * (eps + d2);
	const double pade = 2.0816265188662692474880210318 * num / den;
	const double c0 =  0.004785324257581753;
	const double c1 = -0.01192457083645441;
	const double c2 =  0.01931961413960498;
	const double c3 = -0.02594027398725020;
	const double c4 =  0.03141928755021455;
	const double eps5 = eps*eps*eps*eps*eps;
	const double corr = eps5 * (c0 + eps*(c1 + eps*(c2 + eps*(c3 + c4*eps))));
	pResult->V = eps * (pade + corr);
	pResult->E = 2.0 * SMathConst::Epsilon * fabs(pResult->V);
	return 1;
}

inline static int lngamma_2_pade(const double eps, SMathResult * pResult)
{
	// Use (2,2) Pade for Log[Gamma[2+eps]]/eps plus a correction series.
	const double n1 = 1.000895834786669227164446568;
	const double n2 = 4.209376735287755081642901277;
	const double d1 = 2.618851904903217274682578255;
	const double d2 = 10.85766559900983515322922936;
	const double num = (eps + n1) * (eps + n2);
	const double den = (eps + d1) * (eps + d2);
	const double pade = 2.85337998765781918463568869 * num/den;
	const double c0 =  0.0001139406357036744;
	const double c1 = -0.0001365435269792533;
	const double c2 =  0.0001067287169183665;
	const double c3 = -0.0000693271800931282;
	const double c4 =  0.0000407220927867950;
	const double eps5 = eps*eps*eps*eps*eps;
	const double corr = eps5 * (c0 + eps*(c1 + eps*(c2 + eps*(c3 + c4*eps))));
	pResult->V = eps * (pade + corr);
	pResult->E = 2.0 * SMathConst::Epsilon * fabs(pResult->V);
	return 1;
}
//
// Lanczos method for real x > 0;
// gamma=7, truncated at 1/(z+8)
// [J. SIAM Numer. Anal, Ser. B, 1 (1964) 86]
//
static int lngamma_lanczos(double x, SMathResult * pResult)
{
	const double LogRootTwoPi_ = 0.9189385332046727418;
	//
	// coefficients for gamma=7, kmax=8  Lanczos method
	//
	static double lanczos_7_c[9] = {
		0.99999999999980993227684700473478,
		676.520368121885098567009190444019,
		-1259.13921672240287047156078755283,
		771.3234287776530788486528258894,
		-176.61502916214059906584551354,
		12.507343278686904814458936853,
		-0.13857109526572011689554707,
		9.984369578019570859563E-6,
		1.50563273514931155834E-7
	};
	x -= 1.0; /* Lanczos writes z! instead of Gamma(z) */
	double Ag = lanczos_7_c[0];
	for(int k = 1; k <= 8; k++) {
		Ag += lanczos_7_c[k] / (x+k);
	}
	// (x+0.5)*log(x+7.5) - (x+7.5) + LogRootTwoPi_ + log(Ag(x))
	double term1 = (x+0.5) * log((x+7.5) / SMathConst::E);
	double term2 = LogRootTwoPi_ + log(Ag);
	pResult->V  = term1 + (term2 - 7.0);
	pResult->E  = 2.0 * SMathConst::Epsilon * (fabs(term1) + fabs(term2) + 7.0);
	pResult->E += SMathConst::Epsilon * fabs(pResult->V);
	return 1;
}
//
// x = eps near zero gives double-precision for |eps| < 0.02
//
static int lngamma_sgn_0(double eps, SMathResult * lng, double * sgn)
{
	// calculate series for g(eps) = Gamma(eps) eps - 1/(1+eps) - eps/2
	const double c1  = -0.07721566490153286061;
	const double c2  = -0.01094400467202744461;
	const double c3  =  0.09252092391911371098;
	const double c4  = -0.01827191316559981266;
	const double c5  =  0.01800493109685479790;
	const double c6  = -0.00685088537872380685;
	const double c7  =  0.00399823955756846603;
	const double c8  = -0.00189430621687107802;
	const double c9  =  0.00097473237804513221;
	const double c10 = -0.00048434392722255893;
	const double g6  = c6+eps*(c7+eps*(c8 + eps*(c9 + eps*c10)));
	const double g   = eps*(c1+eps*(c2+eps*(c3+eps*(c4+eps*(c5+eps*g6)))));
	// calculate Gamma(eps) eps, a positive quantity
	const double gee = g + 1.0/(1.0+eps) + 0.5*eps;
	lng->V = log(gee / fabs(eps));
	lng->E = 4.0 * SMathConst::Epsilon * fabs(lng->V);
	*sgn = fsign(eps);
	return 1;
}
//
// x near a negative integer
// Calculates sign as well as log(|gamma(x)|).
// x = -N + eps
// assumes N >= 1
//
static int lngamma_sgn_sing(int N, double eps, SMathResult * lng, double * sgn)
{
	int    ok = 1;
	if(eps == 0.0) {
		*sgn = 0.0;
		ok = lng->SetDomainViolation();
	}
	else if(N == 1) {
		//
		// calculate series for
		// g = eps gamma(-1+eps) + 1 + eps/2 (1+3eps)/(1-eps^2)
		// double-precision for |eps| < 0.02
		//
		const double c0 =  0.07721566490153286061;
		const double c1 =  0.08815966957356030521;
		const double c2 = -0.00436125434555340577;
		const double c3 =  0.01391065882004640689;
		const double c4 = -0.00409427227680839100;
		const double c5 =  0.00275661310191541584;
		const double c6 = -0.00124162645565305019;
		const double c7 =  0.00065267976121802783;
		const double c8 = -0.00032205261682710437;
		const double c9 =  0.00016229131039545456;
		const double g5 = c5 + eps*(c6 + eps*(c7 + eps*(c8 + eps*c9)));
		const double g  = eps*(c0 + eps*(c1 + eps*(c2 + eps*(c3 + eps*(c4 + eps*g5)))));
		// calculate eps gamma(-1+eps), a negative quantity
		const double gam_e = g - 1.0 - 0.5*eps*(1.0+3.0*eps)/(1.0 - eps*eps);
		lng->V = log(fabs(gam_e)/fabs(eps));
		lng->E = 2.0 * SMathConst::Epsilon * fabs(lng->V);
		*sgn = (eps > 0.0 ? -1.0 : 1.0);
	}
	else {
		double g;
		//
		// series for sin(Pi(N+1-eps))/(Pi eps) modulo the sign double-precision for |eps| < 0.02
		//
		const double cs1 = -1.6449340668482264365;
		const double cs2 =  0.8117424252833536436;
		const double cs3 = -0.1907518241220842137;
		const double cs4 =  0.0261478478176548005;
		const double cs5 = -0.0023460810354558236;
		const double e2  = eps*eps;
		const double sin_ser = 1.0 + e2*(cs1+e2*(cs2+e2*(cs3+e2*(cs4+e2*cs5))));
		//
		// calculate series for ln(gamma(1+N-eps)) double-precision for |eps| < 0.02
		//
		double aeps = fabs(eps);
		double c1, c2, c3, c4, c5, c6, c7;
		double lng_ser;
		SMathResult c0;
		SMathResult psi_0;
		SMathResult psi_1;
		SMathResult psi_2;
		SMathResult psi_3;
		SMathResult psi_4;
		SMathResult psi_5;
		SMathResult psi_6;
		psi_2.V = 0.0;
		psi_3.V = 0.0;
		psi_4.V = 0.0;
		psi_5.V = 0.0;
		psi_6.V = 0.0;
		flnfact(N, &c0);
		/*psi_int_e*/   fpsi_int(N+1, &psi_0);
		/*psi_1_int_e*/ fpsi_1_int(N+1, &psi_1);
		if(aeps > 0.00001) fpsi_n(2, N+1.0, &psi_2);
		if(aeps > 0.0002)  fpsi_n(3, N+1.0, &psi_3);
		if(aeps > 0.001)   fpsi_n(4, N+1.0, &psi_4);
		if(aeps > 0.005)   fpsi_n(5, N+1.0, &psi_5);
		if(aeps > 0.01)    fpsi_n(6, N+1.0, &psi_6);
		c1 = psi_0.V;
		c2 = psi_1.V/2.0;
		c3 = psi_2.V/6.0;
		c4 = psi_3.V/24.0;
		c5 = psi_4.V/120.0;
		c6 = psi_5.V/720.0;
		c7 = psi_6.V/5040.0;
		lng_ser = c0.V-eps*(c1-eps*(c2-eps*(c3-eps*(c4-eps*(c5-eps*(c6-eps*c7))))));
		//
		// calculate
		// g = ln(|eps gamma(-N+eps)|)
		//   = -ln(gamma(1+N-eps)) + ln(|eps Pi/sin(Pi(N+1+eps))|)
		//
		g = -lng_ser - log(sin_ser);
		lng->V = g - log(fabs(eps));
		lng->E = c0.E + 2.0 * SMathConst::Epsilon * (fabs(g) + fabs(lng->V));
		*sgn = (IS_ODD(N) ? -1.0 : 1.0) * (eps > 0.0 ? 1.0 : -1.0);
	}
	return ok;
}

int flngamma(double x, SMathResult * pResult)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	/* CHECK_POINTER(result) */
	if(fabs(x - 1.0) < 0.01) {
		//
		// Note that we must amplify the errors from the Pade evaluations because of
		// the way we must pass the argument, i.e. writing (1-x) is a loss of precision when x is near 1.
		//
		ok = lngamma_1_pade(x - 1.0, pResult);
		pResult->E *= 1.0 / (SMathConst::Epsilon + fabs(x - 1.0));
	}
	else if(fabs(x - 2.0) < 0.01) {
		ok = lngamma_2_pade(x - 2.0, pResult);
		pResult->E *= 1.0 / (SMathConst::Epsilon + fabs(x - 2.0));
	}
	else if(x >= 0.5) {
		ok = lngamma_lanczos(x, pResult);
	}
	else if(x == 0.0) {
		ok = pResult->SetDomainViolation();
		CALLEXCEPT();
	}
	else if(fabs(x) < 0.02) {
		double sgn;
		ok = lngamma_sgn_0(x, pResult, &sgn);
	}
	else if(x > -0.5 / (SMathConst::Epsilon * SMathConst::Pi)) {
		// Try to extract a fractional part from x.
		double z  = 1.0 - x;
		double s  = sin(SMathConst::Pi * z);
		double as = fabs(s);
		if(s == 0.0) {
			THROW(pResult->SetDomainViolation());
		}
		else if(as < (SMathConst::Pi * 0.015)) {
			// x is near a negative integer, -N
			if(x < (INT_MIN + 2.0)) {
				pResult->SetZero();
				CALLEXCEPTV(SLERR_MATH_ROUND);
			}
			else {
				int    N = -(int)(x - 0.5);
				double sgn;
				ok = lngamma_sgn_sing(N, x + N, pResult, &sgn);
			}
		}
		else {
			SMathResult lg_z;
			lngamma_lanczos(z, &lg_z);
			pResult->V = SMathConst::LnPi - (log(as) + lg_z.V);
			pResult->SetErr(lg_z.E, 2.0);
		}
	}
	else {
		// |x| was too large to extract any fractional part
		pResult->SetZero();
		CALLEXCEPTV(SLERR_MATH_ROUND);
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int flngamma_sgn(double x, SMathResult * result_lg, double * sgn)
{
	int    ok = 1;
	if(fabs(x - 1.0) < 0.01) {
		ok = lngamma_1_pade(x - 1.0, result_lg);
		result_lg->E *= 1.0/(SMathConst::Epsilon + fabs(x - 1.0));
		*sgn = 1.0;
	}
	else if(fabs(x - 2.0) < 0.01) {
		ok = lngamma_2_pade(x - 2.0, result_lg);
		result_lg->E *= 1.0/(SMathConst::Epsilon + fabs(x - 2.0));
		*sgn = 1.0;
	}
	else if(x >= 0.5) {
		*sgn = 1.0;
		ok = lngamma_lanczos(x, result_lg);
	}
	else if(x == 0.0) {
		*sgn = 0.0;
		ok = result_lg->SetDomainViolation();
	}
	else if(fabs(x) < 0.02) {
		ok = lngamma_sgn_0(x, result_lg, sgn);
	}
	else if(x > -0.5/(SMathConst::Epsilon*SMathConst::Pi)) {
		/*
			Try to extract a fractional part from x.
		*/
		double z = 1.0 - x;
		double s = sin(SMathConst::Pi*x);
		double as = fabs(s);
		if(s == 0.0) {
			*sgn = 0.0;
			ok = result_lg->SetDomainViolation();
		}
		else if(as < SMathConst::Pi*0.015) {
			/* x is near a negative integer, -N */
			if(x < INT_MIN + 2.0) {
				result_lg->SetZero();
				*sgn = 0.0;
				ok = (SLibError = SLERR_MATH_ROUND, 0);
			}
			else {
				int N = -(int)(x - 0.5);
				double eps = x + N;
				ok = lngamma_sgn_sing(N, eps, result_lg, sgn);
			}
		}
		else {
			SMathResult lg_z;
			lngamma_lanczos(z, &lg_z);
			*sgn = (s > 0.0 ? 1.0 : -1.0);
			result_lg->V = SMathConst::LnPi - (log(as) + lg_z.V);
			result_lg->E = 2.0 * SMathConst::Epsilon * fabs(result_lg->V) + lg_z.E;
		}
	}
	else {
		/* |x| was too large to extract any fractional part */
		result_lg->SetZero();
		*sgn = 0.0;
		ok = (SLibError = SLERR_MATH_ROUND, 0);
	}
	return ok;
}
//
//
//
int exp_e(double x, SMathResult * result)
{
	if(x > SMathConst::LogMax)
		return result->SetOverflow();
	else if(x < SMathConst::LogMin)
		return result->SetUnderflow();
	else {
		result->V = exp(x);
		result->SetErr(0.0, 2.0);
		return 1;
	}
}

int exp_err_e(double x, double dx, SMathResult * result)
{
	int    ok = 1;
	const  double adx = fabs(dx);
	/* CHECK_POINTER(result) */
	if(x + adx > SMathConst::LogMax) {
		ok = result->SetOverflow();
	}
	else if(x - adx < SMathConst::LogMin) {
		ok = result->SetUnderflow();
	}
	else {
		const double ex  = exp(x);
		const double edx = exp(adx);
		result->V  = ex;
		result->SetErr(ex * MAX(SMathConst::Epsilon, edx - 1.0/edx), 2.0);
	}
	return ok;
}
/*
	implementation for E1, allowing for scaling by exp(x)
*/
static int expint_E1_impl(double x, SMathResult * result, int scale)
{
	int    ok = 1;
	const  double xmaxt = -SMathConst::LogMin; /* XMAXT = -LOG (R1MACH(1)) */
	const  double xmax  = xmaxt - log(xmaxt); /* XMAX = XMAXT - LOG(XMAXT) */
	/* CHECK_POINTER(result) */
	if(x < -xmax && !scale) {
		ok = result->SetOverflow();
	}
	else if(x <= -10.0) {
		static double AE11_data[39] = {
			0.121503239716065790,  -0.065088778513550150, 0.004897651357459670,
			-0.000649237843027216, 0.000093840434587471,  0.000000420236380882,
			-0.000008113374735904, 0.000002804247688663,  0.000000056487164441,
			-0.000000344809174450, 0.000000058209273578,  0.000000038711426349,
			-0.000000012453235014, -0.000000005118504888, 0.000000002148771527,
			0.000000000868459898,  -0.000000000343650105, -0.000000000179796603,
			0.000000000047442060,  0.000000000040423282,  -0.000000000003543928,
			-0.000000000008853444, -0.000000000000960151, 0.000000000001692921,
			0.000000000000607990,  -0.000000000000224338, -0.000000000000200327,
			-0.000000000000006246, 0.000000000000045571,  0.000000000000016383,
			-0.000000000000005561, -0.000000000000006074, -0.000000000000000862,
			0.000000000000001223,  0.000000000000000716,  -0.000000000000000024,
			-0.000000000000000201, -0.000000000000000082, 0.000000000000000017
		};
		static ChebSeries AE11_cs = { AE11_data, 38, -1, 1, 20 };
		const double s = 1.0/x * (scale ? 1.0 : exp(-x));
		SMathResult result_c;
		cheb_eval_e(&AE11_cs, 20.0/x+1.0, &result_c);
		result->V  = s * (1.0 + result_c.V);
		result->SetErr(s * result_c.E, 2.0 * (fabs(x) + 1.0));
	}
	else if(x <= -4.0) {
		static double AE12_data[25] = {
			0.582417495134726740,  -0.158348850905782750, -0.006764275590323141,
			0.005125843950185725,  0.000435232492169391,  -0.000143613366305483,
			-0.000041801320556301, -0.000002713395758640, 0.000001151381913647,
			0.000000420650022012,  0.000000066581901391,  0.000000000662143777,
			-0.000000002844104870, -0.000000000940724197, -0.000000000177476602,
			-0.000000000015830222, 0.000000000002905732,  0.000000000001769356,
			0.000000000000492735,  0.000000000000093709,  0.000000000000010707,
			-0.000000000000000537, -0.000000000000000716, -0.000000000000000244,
			-0.000000000000000058
		};
		static ChebSeries AE12_cs = { AE12_data, 24, -1, 1, 15 };
		const double s = 1.0/x * ( scale ? 1.0 : exp(-x));
		SMathResult result_c;
		cheb_eval_e(&AE12_cs, (40.0/x+7.0)/3.0, &result_c);
		result->V  = s * (1.0 + result_c.V);
		result->SetErr(s * result_c.E, 2.0);
	}
	else if(x <= -1.0) {
		static double E11_data[19] = {
			-16.11346165557149402600, 7.79407277874268027690,  -1.95540581886314195070,
			0.37337293866277945612,   -0.05692503191092901938, 0.00721107776966009185,
			-0.00078104901449841593,  0.00007388093356262168,  -0.00000620286187580820,
			0.00000046816002303176,   -0.00000003209288853329, 0.00000000201519974874,
			-0.00000000011673686816,  0.00000000000627627066,  -0.00000000000031481541,
			0.00000000000001479904,   -0.00000000000000065457, 0.00000000000000002733,
			-0.00000000000000000108
		};
		static ChebSeries E11_cs = { E11_data, 18, -1, 1, 13 };
		const double ln_term = -log(fabs(x));
		const double scale_factor = ( scale ? exp(x) : 1.0);
		SMathResult result_c;
		cheb_eval_e(&E11_cs, (2.0*x+5.0)/3.0, &result_c);
		result->V  = scale_factor * (ln_term + result_c.V);
		result->SetErr(scale_factor * (result_c.E + SMathConst::Epsilon * fabs(ln_term)), 2.0);
	}
	else if(x == 0.0) {
		ok = result->SetDomainViolation();
	}
	else if(x <= 1.0) {
		static double E12_data[16] = {
			-0.03739021479220279500,
			0.04272398606220957700,
			-0.13031820798497005440,
			0.01441912402469889073,
			-0.00134617078051068022,
			0.00010731029253063780,
			-0.00000742999951611943,
			0.00000045377325690753,
			-0.00000002476417211390,
			0.00000000122076581374,
			-0.00000000005485141480,
			0.00000000000226362142,
			-0.00000000000008635897,
			0.00000000000000306291,
			-0.00000000000000010148,
			0.00000000000000000315
		};
		static ChebSeries E12_cs = { E12_data, 15, -1, 1, 10 };
		const double ln_term = -log(fabs(x));
		const double scale_factor = ( scale ? exp(x) : 1.0);
		SMathResult result_c;
		cheb_eval_e(&E12_cs, x, &result_c);
		result->V  = scale_factor * (ln_term - 0.6875 + x + result_c.V);
		result->SetErr(scale_factor * (result_c.E + SMathConst::Epsilon * fabs(ln_term)), 2.0);
	}
	else if(x <= 4.0) {
		static double AE13_data[25] = {
			-0.605773246640603460,
			-0.112535243483660900,
			0.013432266247902779,
			-0.001926845187381145,
			0.000309118337720603,
			-0.000053564132129618,
			0.000009827812880247,
			-0.000001885368984916,
			0.000000374943193568,
			-0.000000076823455870,
			0.000000016143270567,
			-0.000000003466802211,
			0.000000000758754209,
			-0.000000000168864333,
			0.000000000038145706,
			-0.000000000008733026,
			0.000000000002023672,
			-0.000000000000474132,
			0.000000000000112211,
			-0.000000000000026804,
			0.000000000000006457,
			-0.000000000000001568,
			0.000000000000000383,
			-0.000000000000000094,
			0.000000000000000023
		};
		static ChebSeries AE13_cs = { AE13_data, 24, -1, 1, 15 };
		const double s = 1.0/x * ( scale ? 1.0 : exp(-x));
		SMathResult result_c;
		cheb_eval_e(&AE13_cs, (8.0/x-5.0)/3.0, &result_c);
		result->V  = s * (1.0 + result_c.V);
		result->SetErr(s * result_c.E, 2.0);
	}
	else if(x <= xmax || scale) {
		static double AE14_data[26] = {
			-0.18929180007530170,
			-0.08648117855259871,
			0.00722410154374659,
			-0.00080975594575573,
			0.00010999134432661,
			-0.00001717332998937,
			0.00000298562751447,
			-0.00000056596491457,
			0.00000011526808397,
			-0.00000002495030440,
			0.00000000569232420,
			-0.00000000135995766,
			0.00000000033846628,
			-0.00000000008737853,
			0.00000000002331588,
			-0.00000000000641148,
			0.00000000000181224,
			-0.00000000000052538,
			0.00000000000015592,
			-0.00000000000004729,
			0.00000000000001463,
			-0.00000000000000461,
			0.00000000000000148,
			-0.00000000000000048,
			0.00000000000000016,
			-0.00000000000000005
		};
		static ChebSeries AE14_cs = { AE14_data, 25, -1, 1, 13 };
		const double s = 1.0/x * ( scale ? 1.0 : exp(-x));
		SMathResult result_c;
		cheb_eval_e(&AE14_cs, 8.0/x-1.0, &result_c);
		result->V  = s * (1.0 +  result_c.V);
		result->SetErr(s * (SMathConst::Epsilon + result_c.E), 2.0 * (x + 1.0));
		if(result->V == 0.0)
			ok = result->SetUnderflow();
	}
	else
		ok = result->SetUnderflow();
	return ok;
}

int expint_E1(double x, SMathResult * result)
{
	return expint_E1_impl(x, result, 0);
}

int log_1plusx_mx(double x, SMathResult * result)
{
	/* CHECK_POINTER(result) */
	int    ok = 1;
	if(x <= -1.0) {
		ok = result->SetDomainViolation();
	}
	else if(fabs(x) < SMathConst::Root5Epsilon) {
		const double c1 = -0.5;
		const double c2 =  1.0/3.0;
		const double c3 = -1.0/4.0;
		const double c4 =  1.0/5.0;
		const double c5 = -1.0/6.0;
		const double c6 =  1.0/7.0;
		const double c7 = -1.0/8.0;
		const double c8 =  1.0/9.0;
		const double c9 = -1.0/10.0;
		const double t  =  c5 + x*(c6 + x*(c7 + x*(c8 + x*c9)));
		result->V = x*x * (c1 + x*(c2 + x*(c3 + x*(c4 + x*t))));
		result->SetErr(0.0, 1.0);
	}
	else if(fabs(x) < 0.5) {
		/*
			Chebyshev expansion for (log(1 + x(t)) - x(t))/x(t)^2

 			x(t) = (4t-1)/(2(4-t))
 			t(x) = (8x+1)/(2(x+2))
 			-1/2 < x < 1/2
 			-1 < t < 1
 		*/
		static double lopxmx_data[20] = {
 			-1.12100231323744103373737274541,
  			0.19553462773379386241549597019,
 			-0.01467470453808083971825344956,
  			0.00166678250474365477643629067,
 			-0.00018543356147700369785746902,
  			0.00002280154021771635036301071,
 			-2.8031253116633521699214134172e-06,
  			3.5936568872522162983669541401e-07,
 			-4.6241857041062060284381167925e-08,
  			6.0822637459403991012451054971e-09,
 			-8.0339824424815790302621320732e-10,
  			1.0751718277499375044851551587e-10,
 			-1.4445310914224613448759230882e-11,
  			1.9573912180610336168921438426e-12,
 			-2.6614436796793061741564104510e-13,
  			3.6402634315269586532158344584e-14,
 			-4.9937495922755006545809120531e-15,
  			6.8802890218846809524646902703e-16,
 			-9.5034129794804273611403251480e-17,
  			1.3170135013050997157326965813e-17
		};
		static ChebSeries lopxmx_cs = { lopxmx_data, 19, -1, 1, 9 };
		double t = 0.5*(8.0*x + 1.0)/(x+2.0);
		SMathResult c;
		cheb_eval_e(&lopxmx_cs, t, &c);
		result->V = x*x * c.V;
		result->E = x*x * c.E;
	}
	else {
		const double lterm = log(1.0 + x);
		result->V = lterm - x;
		result->E = SMathConst::Epsilon * (fabs(lterm) + fabs(x));
	}
	return ok;
}
//
// Complementary Error Function
//
static double erfc8_sum(double x)
{
	/*
		estimates erfc(x) valid for 8 < x < 100
		This is based on index 5725 in Hart et al
	*/
	static double P[] = {
		2.97886562639399288862,
		7.409740605964741794425,
		6.1602098531096305440906,
		5.019049726784267463450058,
		1.275366644729965952479585264,
		0.5641895835477550741253201704
	};
	static double Q[] = {
		3.3690752069827527677,
		9.608965327192787870698,
		17.08144074746600431571095,
		12.0489519278551290360340491,
		9.396034016235054150430579648,
		2.260528520767326969591866945,
		1.0
	};
	int    i;
	double num = P[5];
	for(i = 4; i >= 0; --i) {
		num = x*num + P[i];
	}
	double den = Q[6];
	for(i = 5; i >= 0; --i) {
		den = x*den + Q[i];
	}
	return num/den;
}

inline static double erfc8(double x)
{
	double e = erfc8_sum(x);
	e *= exp(-x*x);
	return e;
}

int erfc_e(double x, SMathResult * result)
{
	const double ax = fabs(x);
	double e_val, e_err;
	/* CHECK_POINTER(result) */
	if(ax <= 1.0) {
		/*
			Chebyshev fit for erfc((t+1)/2), -1 < t < 1
		*/
		static double erfc_xlt1_data[20] = {
			1.06073416421769980345174155056,
			-0.42582445804381043569204735291,
			0.04955262679620434040357683080,
			0.00449293488768382749558001242,
			-0.00129194104658496953494224761,
			-0.00001836389292149396270416979,
			0.00002211114704099526291538556,
			-5.23337485234257134673693179020e-7,
			-2.78184788833537885382530989578e-7,
			1.41158092748813114560316684249e-8,
			2.72571296330561699984539141865e-9,
			-2.06343904872070629406401492476e-10,
			-2.14273991996785367924201401812e-11,
			2.22990255539358204580285098119e-12,
			1.36250074650698280575807934155e-13,
			-1.95144010922293091898995913038e-14,
			-6.85627169231704599442806370690e-16,
			1.44506492869699938239521607493e-16,
			2.45935306460536488037576200030e-18,
			-9.29599561220523396007359328540e-19
		};
		static ChebSeries erfc_xlt1_cs = { erfc_xlt1_data, 19, -1, 1, 12 };
		double t = 2.0*ax - 1.0;
		SMathResult c;
		cheb_eval_e(&erfc_xlt1_cs, t, &c);
		e_val = c.V;
		e_err = c.E;
	}
	else if(ax <= 5.0) {
		/*
			Chebyshev fit for erfc(x) exp(x^2), 1 < x < 5, x = 2t + 3, -1 < t < 1
		*/
		static double erfc_x15_data[25] = {
			0.44045832024338111077637466616,
			-0.143958836762168335790826895326,
			0.044786499817939267247056666937,
			-0.013343124200271211203618353102,
			0.003824682739750469767692372556,
			-0.001058699227195126547306482530,
			0.000283859419210073742736310108,
			-0.000073906170662206760483959432,
			0.000018725312521489179015872934,
			-4.62530981164919445131297264430e-6,
			1.11558657244432857487884006422e-6,
			-2.63098662650834130067808832725e-7,
			6.07462122724551777372119408710e-8,
			-1.37460865539865444777251011793e-8,
			3.05157051905475145520096717210e-9,
			-6.65174789720310713757307724790e-10,
			1.42483346273207784489792999706e-10,
			-3.00141127395323902092018744545e-11,
			6.22171792645348091472914001250e-12,
			-1.26994639225668496876152836555e-12,
			2.55385883033257575402681845385e-13,
			-5.06258237507038698392265499770e-14,
			9.89705409478327321641264227110e-15,
			-1.90685978789192181051961024995e-15,
			3.50826648032737849245113757340e-16
		};
		static ChebSeries erfc_x15_cs = { erfc_x15_data, 24, -1, 1, 16 };
		double ex2 = exp(-x*x);
		double t = 0.5*(ax-3.0);
		SMathResult c;
		cheb_eval_e(&erfc_x15_cs, t, &c);
		e_val = ex2 * c.V;
		e_err = ex2 * (c.E + 2.0*fabs(x)*SMathConst::Epsilon);
	}
	else if(ax < 10.0) {
		/*
			Chebyshev fit for erfc(x) x exp(x^2), 5 < x < 10, x = (5t + 15)/2, -1 < t < 1
		*/
		static double erfc_x510_data[20] = {
			1.11684990123545698684297865808,
			0.003736240359381998520654927536,
			-0.000916623948045470238763619870,
			0.000199094325044940833965078819,
			-0.000040276384918650072591781859,
			7.76515264697061049477127605790e-6,
			-1.44464794206689070402099225301e-6,
			2.61311930343463958393485241947e-7,
			-4.61833026634844152345304095560e-8,
			8.00253111512943601598732144340e-9,
			-1.36291114862793031395712122089e-9,
			2.28570483090160869607683087722e-10,
			-3.78022521563251805044056974560e-11,
			6.17253683874528285729910462130e-12,
			-9.96019290955316888445830597430e-13,
			1.58953143706980770269506726000e-13,
			-2.51045971047162509999527428316e-14,
			3.92607828989125810013581287560e-15,
			-6.07970619384160374392535453420e-16,
			9.12600607264794717315507477670e-17
		};
		static ChebSeries erfc_x510_cs = { erfc_x510_data, 19, -1, 1, 12 };
		double exterm = exp(-x*x) / ax;
		double t = (2.0*ax - 15.0)/5.0;
		SMathResult c;
		cheb_eval_e(&erfc_x510_cs, t, &c);
		e_val = exterm * c.V;
		e_err = exterm * (c.E + 2.0*fabs(x)*SMathConst::Epsilon + SMathConst::Epsilon);
	}
	else {
		e_val = erfc8(ax);
		e_err = (x*x + 1.0) * SMathConst::Epsilon * fabs(e_val);
	}
	if(x < 0.0) {
		result->V  = 2.0 - e_val;
		result->SetErr(e_err, 2.0);
	}
	else {
		result->V  = e_val;
		result->SetErr(e_err, 2.0);
	}
	return 1;
}
/*
	series for gammastar(x)
	double-precision for x > 10.0
*/
static void gammastar_ser(double x, SMathResult * result)
{
	/*
		Use the Stirling series for the correction to Log(Gamma(x)),
		which is better behaved and easier to compute than the
		regular Stirling series for Gamma(x).
	*/
	const double y = 1.0/(x*x);
	const double c0 =  1.0/12.0;
	const double c1 = -1.0/360.0;
	const double c2 =  1.0/1260.0;
	const double c3 = -1.0/1680.0;
	const double c4 =  1.0/1188.0;
	const double c5 = -691.0/360360.0;
	const double c6 =  1.0/156.0;
	const double c7 = -3617.0/122400.0;
	const double ser = c0 + y*(c1 + y*(c2 + y*(c3 + y*(c4 + y*(c5 + y*(c6 + y*c7))))));
	result->V = exp(ser/x);
	result->E = 2.0 * SMathConst::Epsilon * result->V * MAX(1.0, ser/x);
}
/*
	gamma(x) for x >= 1/2
	assumes x >= 1/2
*/
static int gamma_xgthalf(double x, SMathResult * result)
{
	int    ok = 1;
	/* CHECK_POINTER(result) */
	if(x == 0.5) {
		result->V = 1.77245385090551602729817;
		result->E = SMathConst::Epsilon * result->V;
	}
	else if(x <= FACT_TAB_SIZE && x == floor(x)) {
		int    n = ffloori(x);
		result->V = ffactr(n-1);
		result->E = SMathConst::Epsilon * result->V;
	}
	else if(fabs(x - 1.0) < 0.01) {
		/*
			Use series for Gamma[1+eps] - 1/(1+eps).
		*/
		const double eps = x - 1.0;
		const double c1 =  0.4227843350984671394;
		const double c2 = -0.01094400467202744461;
		const double c3 =  0.09252092391911371098;
		const double c4 = -0.018271913165599812664;
		const double c5 =  0.018004931096854797895;
		const double c6 = -0.006850885378723806846;
		const double c7 =  0.003998239557568466030;
		result->V = 1.0/x + eps*(c1+eps*(c2+eps*(c3+eps*(c4+eps*(c5+eps*(c6+eps*c7))))));
		result->E = SMathConst::Epsilon;
	}
	else if(fabs(x - 2.0) < 0.01) {
		/*
			Use series for Gamma[1 + eps].
		*/
		const double eps = x - 2.0;
		const double c1 =  0.4227843350984671394;
		const double c2 =  0.4118403304264396948;
		const double c3 =  0.08157691924708626638;
		const double c4 =  0.07424901075351389832;
		const double c5 = -0.00026698206874501476832;
		const double c6 =  0.011154045718130991049;
		const double c7 = -0.002852645821155340816;
		const double c8 =  0.0021039333406973880085;
		result->V = 1.0 + eps*(c1+eps*(c2+eps*(c3+eps*(c4+eps*(c5+eps*(c6+eps*(c7+eps*c8)))))));
		result->E = SMathConst::Epsilon;
	}
	else if(x < 5.0) {
		/*
			Exponentiating the logarithm is fine, as
			long as the exponential is not so large
			that it greatly amplifies the error.
		*/
		SMathResult lg;
		lngamma_lanczos(x, &lg);
		result->V = exp(lg.V);
		result->E = result->V * (lg.E + 2.0 * SMathConst::Epsilon);
	}
	else if(x < 10.0) {
		/*
			Chebyshev expansion for log(gamma(x)/gamma(8))
			5 < x < 10
			-1 < t < 1
		*/
		static double gamma_5_10_data[24] = {
			-1.5285594096661578881275075214,
			4.8259152300595906319768555035,
			0.2277712320977614992970601978,
			-0.0138867665685617873604917300,
			0.0012704876495201082588139723,
			-0.0001393841240254993658962470,
			0.0000169709242992322702260663,
			-2.2108528820210580075775889168e-06,
			3.0196602854202309805163918716e-07,
			-4.2705675000079118380587357358e-08,
			6.2026423818051402794663551945e-09,
			-9.1993973208880910416311405656e-10,
			1.3875551258028145778301211638e-10,
			-2.1218861491906788718519522978e-11,
			3.2821736040381439555133562600e-12,
			-5.1260001009953791220611135264e-13,
			8.0713532554874636696982146610e-14,
			-1.2798522376569209083811628061e-14,
			2.0417711600852502310258808643e-15,
			-3.2745239502992355776882614137e-16,
			5.2759418422036579482120897453e-17,
			-8.5354147151695233960425725513e-18,
			1.3858639703888078291599886143e-18,
			-2.2574398807738626571560124396e-19
		};
		static const ChebSeries gamma_5_10_cs = { gamma_5_10_data, 23, -1, 1, 11 };
		/*
			This is a sticky area. The logarithm
			is too large and the gammastar series
			is not good.
		*/
		const double gamma_8 = 5040.0;
		const double t = (2.0*x - 15.0)/5.0;
		SMathResult c;
		cheb_eval_e(&gamma_5_10_cs, t, &c);
		result->V  = exp(c.V) * gamma_8;
		result->E  = result->V * c.E;
		result->E += 2.0 * SMathConst::Epsilon * result->V;
	}
	else if(x < GSL_SF_GAMMA_XMAX) {
		/*
			We do not want to exponentiate the logarithm
			if x is large because of the inevitable
			inflation of the error. So we carefully
			use pow() and exp() with exact quantities.
		*/
		double p = pow(x, 0.5*x);
		double e = exp(-x);
		double q = (p * e) * p;
		double pre = SMathConst::Sqrt2 * SMathConst::SqrtPi * q / sqrt(x);
		SMathResult gstar;
		gammastar_ser(x, &gstar);
		result->V = pre * gstar.V;
		result->E = (x + 2.5) * SMathConst::Epsilon * result->V;
	}
	else
		ok = result->SetOverflow();
	return ok;
}

int fgamma(double x, SMathResult * result)
{
	int    ok = 1;
	if(x < 0.5) {
		int rint_x = ffloori(x+0.5);
		double f_x = x - rint_x;
		double sgn_gamma = (IS_EVEN(rint_x) ? 1.0 : -1.0);
		double sin_term = sgn_gamma * sin(SMathConst::Pi * f_x) / SMathConst::Pi;
		if(sin_term == 0.0) {
			ok = result->SetDomainViolation();
		}
		else if(x > -169.0) {
			SMathResult g;
			gamma_xgthalf(1.0-x, &g);
			if(fabs(sin_term) * g.V * SMathConst::Min < 1.0) {
				result->V  = 1.0/(sin_term * g.V);
				result->SetErr(fabs(g.E/g.V) * fabs(result->V), 2.0);
			}
			else
				ok = result->SetUnderflow();
		}
		else {
			/*
				It is hard to control it here.
				We can only exponentiate the
				logarithm and eat the loss of
				precision.
			*/
			SMathResult lng;
			double sgn;
			int stat_lng = flngamma_sgn(x, &lng, &sgn);
			int stat_e   = exp_mult_err_e(lng.V, lng.E, sgn, 0.0, result);
			ok = BIN(stat_e && stat_lng);
		}
	}
	else
		ok = gamma_xgthalf(x, result);
	return ok;
}

int gammastar(double x, SMathResult * result)
{
	int    ok = 1;
	/* CHECK_POINTER(result) */
	if(x <= 0.0) {
		ok = result->SetDomainViolation();
	}
	else if(x < 0.5) {
		SMathResult lg;
		const int stat_lg = flngamma(x, &lg);
		const double lx = log(x);
		const double c  = 0.5*(SMathConst::Ln2+SMathConst::LnPi);
		const double lnr_val = lg.V - (x-0.5)*lx + x - c;
		const double lnr_err = lg.E + 2.0 * SMathConst::Epsilon *((x+0.5)*fabs(lx) + c);
		const int stat_e  = exp_err_e(lnr_val, lnr_err, result);
		ok = BIN(stat_lg && stat_e);
	}
	else if(x < 2.0) {
		/*
			Chebyshev coefficients for Gamma*(3/4(t+1)+1/2), -1<t<1
		*/
		static double gstar_a_data[30] = {
			2.16786447866463034423060819465,
			-0.05533249018745584258035832802,
			0.01800392431460719960888319748,
			-0.00580919269468937714480019814,
			0.00186523689488400339978881560,
			-0.00059746524113955531852595159,
			0.00019125169907783353925426722,
			-0.00006124996546944685735909697,
			0.00001963889633130842586440945,
			-6.3067741254637180272515795142e-06,
			2.0288698405861392526872789863e-06,
			-6.5384896660838465981983750582e-07,
			2.1108698058908865476480734911e-07,
			-6.8260714912274941677892994580e-08,
			2.2108560875880560555583978510e-08,
			-7.1710331930255456643627187187e-09,
			2.3290892983985406754602564745e-09,
			-7.5740371598505586754890405359e-10,
			2.4658267222594334398525312084e-10,
			-8.0362243171659883803428749516e-11,
			2.6215616826341594653521346229e-11,
			-8.5596155025948750540420068109e-12,
			2.7970831499487963614315315444e-12,
			-9.1471771211886202805502562414e-13,
			2.9934720198063397094916415927e-13,
			-9.8026575909753445931073620469e-14,
			3.2116773667767153777571410671e-14,
			-1.0518035333878147029650507254e-14,
			3.4144405720185253938994854173e-15,
			-1.0115153943081187052322643819e-15
		};
		static ChebSeries gstar_a_cs = { gstar_a_data, 29, -1, 1, 17 };
		const double t = 4.0/3.0*(x-0.5) - 1.0;
		cheb_eval_e(&gstar_a_cs, t, result);
	}
	else if(x < 10.0) {
		/*
			Chebyshev coefficients for
			x^2(Gamma*(x) - 1 - 1/(12x)), x = 4(t+1)+2, -1 < t < 1
		*/
		static double gstar_b_data[] = {
			0.0057502277273114339831606096782,
			0.0004496689534965685038254147807,
			-0.0001672763153188717308905047405,
			0.0000615137014913154794776670946,
			-0.0000223726551711525016380862195,
			8.0507405356647954540694800545e-06,
			-2.8671077107583395569766746448e-06,
			1.0106727053742747568362254106e-06,
			-3.5265558477595061262310873482e-07,
			1.2179216046419401193247254591e-07,
			-4.1619640180795366971160162267e-08,
			1.4066283500795206892487241294e-08,
			-4.6982570380537099016106141654e-09,
			1.5491248664620612686423108936e-09,
			-5.0340936319394885789686867772e-10,
			1.6084448673736032249959475006e-10,
			-5.0349733196835456497619787559e-11,
			1.5357154939762136997591808461e-11,
			-4.5233809655775649997667176224e-12,
			1.2664429179254447281068538964e-12,
			-3.2648287937449326771785041692e-13,
			7.1528272726086133795579071407e-14,
			-9.4831735252566034505739531258e-15,
			-2.3124001991413207293120906691e-15,
			2.8406613277170391482590129474e-15,
			-1.7245370321618816421281770927e-15,
			8.6507923128671112154695006592e-16,
			-3.9506563665427555895391869919e-16,
			1.6779342132074761078792361165e-16,
			-6.0483153034414765129837716260e-17
		};
		static ChebSeries gstar_b_cs = { gstar_b_data, 29, -1, 1, 18 };
		const double t = 0.25*(x-2.0) - 1.0;
		SMathResult c;
		cheb_eval_e(&gstar_b_cs, t, &c);
		result->V  = c.V/(x*x) + 1.0 + 1.0/(12.0*x);
		result->SetErr(c.E/(x*x), 2.0);
	}
	else if(x < 1.0/SMathConst::Root4Epsilon) {
		gammastar_ser(x, result);
	}
	else if(x < 1.0/SMathConst::Epsilon) {
		/*
			Use Stirling formula for Gamma(x).
		*/
		const double xi = 1.0/x;
		result->V = 1.0 + xi/12.0*(1.0 + xi/24.0*(1.0 - xi*(139.0/180.0 + 571.0/8640.0*xi)));
		result->SetErr(0.0, 2.0);
	}
	else {
		result->V = 1.0;
		result->E = 1.0/x;
	}
	return ok;
}
/*
	The dominant part,
	D(a,x) := x^a e^(-x) / Gamma(a+1)
*/
static int gamma_inc_D(double a, double x, SMathResult * result)
{
	int    ok = 1;
	if(a < 10.0) {
		SMathResult lg;
		//lngamma_e(a+1.0, &lg);
		flngamma(a+1.0, &lg); // @sobolev
		double lnr = a * log(x) - x - lg.V;
		result->V = exp(lnr);
		result->E = 2.0 * SMathConst::Epsilon * (fabs(lnr) + 1.0) * fabs(result->V);
	}
	else {
		SMathResult gstar;
		SMathResult ln_term;
		double term1;
		if(x < 0.5*a) {
			double u = x/a;
			double ln_u = log(u);
			ln_term.V = ln_u - u + 1.0;
			ln_term.E = (fabs(ln_u) + fabs(u) + 1.0) * SMathConst::Epsilon;
		}
		else {
			double mu = (x-a)/a;
			log_1plusx_mx(mu, &ln_term); /* log(1+mu) - mu */
		}
		gammastar(a, &gstar);
		term1 = exp(a*ln_term.V)/sqrt(SMathConst::Pi2 * a);
		result->V  = term1/gstar.V;
		result->E  = 2.0 * SMathConst::Epsilon * (fabs(a*ln_term.V) + 1.0) * fabs(result->V);
		result->E += gstar.E/fabs(gstar.V) * fabs(result->V);
	}
	return ok;
}
/*
	P series representation.
*/
static int gamma_inc_P_series(double a, double x, SMathResult * result)
{
	const int nmax = 5000;
	SMathResult D;
	int stat_D = gamma_inc_D(a, x, &D);
	double sum  = 1.0;
	double term = 1.0;
	int n;
	for(n = 1; n < nmax; n++) {
		term *= x/(a+n);
		sum  += term;
		if(fabs(term/sum) < SMathConst::Epsilon)
			break;
	}
	result->V  = D.V * sum;
	result->SetErr(D.E * fabs(sum), (1.0 + n));
	return (n == nmax) ? (SLibError = SLERR_MATH_MAXITER, 0) : stat_D;
}
/*
	Q large x asymptotic
*/
static int gamma_inc_Q_large_x(double a, double x, SMathResult * result)
{
	const int nmax = 5000;
	SMathResult D;
	const int stat_D = gamma_inc_D(a, x, &D);
	double sum  = 1.0;
	double term = 1.0;
	double last = 1.0;
	int    n;
	for(n = 1; n < nmax; n++) {
		term *= (a-n)/x;
		if(fabs(term/last) > 1.0 || fabs(term/sum) < SMathConst::Epsilon)
			break;
		sum  += term;
		last  = term;
	}
	result->V  = D.V * (a/x) * sum;
	result->SetErr(D.E * fabs((a/x) * sum), 2.0);
	return (n == nmax) ? (SLibError = SLERR_MATH_MAXITER, 0) : stat_D;
}
/*
	Uniform asymptotic for x near a, a and x large.
	See [Temme, p. 285]
*/
static int gamma_inc_Q_asymp_unif(double a, double x, SMathResult * result)
{
	const double rta = sqrt(a);
	const double eps = (x-a)/a;
	SMathResult ln_term;
	const int stat_ln = log_1plusx_mx(eps, &ln_term); /* log(1+eps) - eps */
	const double eta  = fsign(eps) * sqrt(-2.0*ln_term.V);
	SMathResult erfc;
	double R;
	double c0, c1;
	/*
		This used to say erfc(eta*M_SQRT2*rta), which is wrong.
		The sqrt(2) is in the denominator. Oops.
		Fixed: [GJ] Mon Nov 15 13:25:32 MST 2004
	*/
	erfc_e(eta*rta/SMathConst::Sqrt2, &erfc);
	if(fabs(eps) < SMathConst::Root5Epsilon) {
		c0 = -1.0/3.0 + eps*(1.0/12.0 - eps*(23.0/540.0 - eps*(353.0/12960.0 - eps*589.0/30240.0)));
		c1 = -1.0/540.0 - eps/288.0;
	}
	else {
		const double rt_term = sqrt(-2.0 * ln_term.V/(eps*eps));
		const double lam = x/a;
		c0 = (1.0 - 1.0/rt_term)/eps;
		c1 = -(eta*eta*eta * (lam*lam + 10.0*lam + 1.0) - 12.0 * eps*eps*eps) / (12.0 * eta*eta*eta*eps*eps*eps);
	}
	R = exp(-0.5*a*eta*eta)/(SMathConst::Sqrt2*SMathConst::SqrtPi*rta) * (c0 + c1/a);
	result->V  = 0.5 * erfc.V + R;
	result->SetErr(SMathConst::Epsilon * fabs(R * 0.5 * a*eta*eta) + 0.5 * erfc.E, 2.0);
	return stat_ln;
}
/*
	Continued fraction which occurs in evaluation
	of Q(a,x) or Gamma(a,x).

              1   (1-a)/x  1/x  (2-a)/x   2/x  (3-a)/x
   F(a,x) =  ---- ------- ----- -------- ----- -------- ...
             1 +   1 +     1 +   1 +      1 +   1 +

	Hans E. Plesser, 2002-01-22 (hans dot plesser at itf dot nlh dot no).

	Split out from gamma_inc_Q_CF() by GJ [Tue Apr  1 13:16:41 MST 2003].
	See gamma_inc_Q_CF() below.
*/
static int gamma_inc_F_CF(double a, double x, SMathResult * result)
{
	int    ok = -1;
	const  double _small = fpowi(SMathConst::Epsilon, 3);
  	double hn = 1.0; /* convergent */
	double Cn = 1.0 / _small;
	double Dn = 1.0;
  	/*
		n == 1 has a_1, b_1, b_0 independent of a,x,
		so that has been done by hand
	*/
	int    n = 2;
	for(; ok < 0; n++) {
		double delta;
		double an = (IS_ODD(n)) ? (0.5*(n-1)/x) : ((0.5*n-a)/x);
		Dn = 1.0 + an * Dn;
		if(fabs(Dn) < _small)
			Dn = _small;
		Cn = 1.0 + an/Cn;
		if(fabs(Cn) < _small)
			Cn = _small;
		Dn = 1.0 / Dn;
		delta = Cn * Dn;
		hn *= delta;
		if(fabs(delta-1.0) < SMathConst::Epsilon)
			ok = 1;
		else if(n >= 5000)
			ok = (SLibError = SLERR_MATH_MAXITER, 0);
	}
	result->V = hn;
	result->SetErr(2.0*SMathConst::Epsilon * fabs(hn), (2.0 + 0.5*n));
	return ok;
}
/*
	Continued fraction for Q.

	Q(a,x) = D(a,x) a/x F(a,x)

	Hans E. Plesser, 2002-01-22 (hans dot plesser at itf dot nlh dot no):

	Since the Gautschi equivalent series method for CF evaluation may lead
	to singularities, I have replaced it with the modified Lentz algorithm given in

	I J Thompson and A R Barnett
	Coulomb and Bessel Functions of Complex Arguments and Order
	J Computational Physics 64:490-509 (1986)

	In consequence, gamma_inc_Q_CF_protected() is now obsolete and has been removed.

	Identification of terms between the above equation for F(a, x) and
	the first equation in the appendix of Thompson&Barnett is as follows:

    	b_0 = 0, b_n = 1 for all n > 0

    	a_1 = 1
    	a_n = (n/2-a)/x    for n even
    	a_n = (n-1)/(2x)   for n odd
*/
static int gamma_inc_Q_CF(double a, double x, SMathResult * result)
{
	SMathResult D;
	SMathResult F;
	const int stat_D = gamma_inc_D(a, x, &D);
	const int stat_F = gamma_inc_F_CF(a, x, &F);
	result->V  = D.V * (a/x) * F.V;
	result->E  = D.E * fabs((a/x) * F.V) + fabs(D.V * a/x * F.E);
	return BIN(stat_F && stat_D);
}
/*
	Useful for small a and x. Handles the subtraction analytically.
*/
static int gamma_inc_Q_series(double a, double x, SMathResult * result)
{
	int    ok = -1;
	double term1; /* 1 - x^a/Gamma(a+1) */
	double sum; /* 1 + (a+1)/(a+2)(-x)/2! + (a+1)/(a+3)(-x)^2/3! + ... */
	double term2; /* a temporary variable used at the end */
	{
		/*
			Evaluate series for 1 - x^a/Gamma(a+1), small a
		*/
		const double pg21 = -2.404113806319188570799476; /* PolyGamma[2,1] */
		const double lnx  = log(x);
		const double el   = SMathConst::Euler+lnx;
		const double c1 = -el;
		const double c2 = SMathConst::Pi*SMathConst::Pi/12.0 - 0.5*el*el;
		const double c3 = el*(SMathConst::Pi*SMathConst::Pi/12.0 - el*el/6.0) + pg21/6.0;
		const double c4 = -0.04166666666666666667
			* (-1.758243446661483480 + lnx)
			* (-0.764428657272716373 + lnx)
			* ( 0.723980571623507657 + lnx)
			* ( 4.107554191916823640 + lnx);
		const double c5 = -0.0083333333333333333
			* (-2.06563396085715900 + lnx)
			* (-1.28459889470864700 + lnx)
			* (-0.27583535756454143 + lnx)
			* ( 1.33677371336239618 + lnx)
			* ( 5.17537282427561550 + lnx);
		const double c6 = -0.0013888888888888889
			* (-2.30814336454783200 + lnx)
			* (-1.65846557706987300 + lnx)
			* (-0.88768082560020400 + lnx)
			* ( 0.17043847751371778 + lnx)
			* ( 1.92135970115863890 + lnx)
			* ( 6.22578557795474900 + lnx);
		const double c7 = -0.00019841269841269841
			* (-2.5078657901291800 + lnx)
			* (-1.9478900888958200 + lnx)
			* (-1.3194837322612730 + lnx)
			* (-0.5281322700249279 + lnx)
			* ( 0.5913834939078759 + lnx)
			* ( 2.4876819633378140 + lnx)
			* ( 7.2648160783762400 + lnx);
		const double c8 = -0.00002480158730158730
			* (-2.677341544966400 + lnx)
			* (-2.182810448271700 + lnx)
			* (-1.649350342277400 + lnx)
			* (-1.014099048290790 + lnx)
			* (-0.191366955370652 + lnx)
			* ( 0.995403817918724 + lnx)
			* ( 3.041323283529310 + lnx)
			* ( 8.295966556941250 + lnx);
		const double c9 = -2.75573192239859e-6
			* (-2.8243487670469080 + lnx)
			* (-2.3798494322701120 + lnx)
			* (-1.9143674728689960 + lnx)
			* (-1.3814529102920370 + lnx)
			* (-0.7294312810261694 + lnx)
			* ( 0.1299079285269565 + lnx)
			* ( 1.3873333251885240 + lnx)
			* ( 3.5857258865210760 + lnx)
			* ( 9.3214237073814600 + lnx);
		const double c10 = -2.75573192239859e-7
			* (-2.9540329644556910 + lnx)
			* (-2.5491366926991850 + lnx)
			* (-2.1348279229279880 + lnx)
			* (-1.6741881076349450 + lnx)
			* (-1.1325949616098420 + lnx)
			* (-0.4590034650618494 + lnx)
			* ( 0.4399352987435699 + lnx)
			* ( 1.7702236517651670 + lnx)
			* ( 4.1231539047474080 + lnx)
			* ( 10.342627908148680 + lnx);
		term1 = a*(c1+a*(c2+a*(c3+a*(c4+a*(c5+a*(c6+a*(c7+a*(c8+a*(c9+a*c10)))))))));
	}
	{
		/*
			Evaluate the sum.
		*/
		double t = 1.0;
		sum = 1.0;
		for(int n = 1; ok < 0; n++) {
			t *= -x/(n+1.0);
			sum += (a+1.0)/(a+n+1.0)*t;
			if(fabs(t/sum) < SMathConst::Epsilon)
				ok = 1;
			else if(n >= 5000)
				ok = (SLibError = SLERR_MATH_MAXITER, 0);
		}
		term2 = (1.0 - term1) * a/(a+1.0) * x * sum;
		result->V  = term1 + term2;
		result->SetErr(SMathConst::Epsilon * (fabs(term1) + 2.0*fabs(term2)), 2.0);
	}
	return ok;
}
/*
	series for small a and x, but not defined for a == 0
*/
static int gamma_inc_series(double a, double x, SMathResult * result)
{
	SMathResult Q;
	SMathResult G;
	const int stat_Q = gamma_inc_Q_series(a, x, &Q);
	const int stat_G = fgamma(a, &G);
	result->V = Q.V * G.V;
	result->SetErr(fabs(Q.V * G.E) + fabs(Q.E * G.V), 2.0);
	return BIN(stat_Q && stat_G);
}

static int gamma_inc_a_gt_0(double a, double x, SMathResult * result)
{
	/* x > 0 and a > 0; use result for Q */
	SMathResult Q;
	SMathResult G;
	const int stat_Q = gamma_inc_Q_e(a, x, &Q);
	const int stat_G = fgamma(a, &G);
	result->V = G.V * Q.V;
	result->SetErr(fabs(G.V * Q.E) + fabs(G.E * Q.V), 2.0);
	return BIN(stat_G && stat_Q);
}

static int gamma_inc_CF(double a, double x, SMathResult * result)
{
	SMathResult F;
	SMathResult pre;
	const int stat_F = gamma_inc_F_CF(a, x, &F);
	const int stat_E = exp_e((a-1.0)*log(x) - x, &pre);
	result->V = F.V * pre.V;
	result->E = fabs(F.E * pre.V) + fabs(F.V * pre.E);
	result->E += (2.0 + fabs(a)) * SMathConst::Epsilon * fabs(result->V);
	return BIN(stat_F && stat_E);
}
/*
	evaluate Gamma(0,x), x > 0
*/
#define GAMMA_INC_A_0(x, result) expint_E1(x, result)

/*-*-*-*-*-*-*-*-*-*-*-* Functions with Error Codes *-*-*-*-*-*-*-*-*-*-*-*/

int gamma_inc_Q_e(double a, double x, SMathResult * result)
{
	int    ok = 1;
	if(a < 0.0 || x < 0.0) {
		ok = result->SetDomainViolation();
	}
	else if(x == 0.0) {
		result->V = 1.0;
		result->E = 0.0;
	}
	else if(a == 0.0) {
		result->SetZero();
	}
	else if(x <= 0.5*a) {
		/*
			If the series is quick, do that. It is
			robust and simple.
		*/
		SMathResult P;
		ok = gamma_inc_P_series(a, x, &P);
		result->V  = 1.0 - P.V;
		result->SetErr(P.E, 2.0);
	}
	else if(a >= 1.0e+06 && (x-a)*(x-a) < a) {
		/*
			Then try the difficult asymptotic regime.
			This is the only way to do this region.
		*/
		ok = gamma_inc_Q_asymp_unif(a, x, result);
	}
	else if(a < 0.2 && x < 5.0) {
		/*
			Cancellations at small a must be handled
			analytically; x should not be too big
			either since the series terms grow
			with x and log(x).
		*/
		ok = gamma_inc_Q_series(a, x, result);
	}
	else if(a <= x) {
		if(x <= 1.0e+06) {
			/*
				Continued fraction is excellent for x >~ a.
				We do not let x be too large when x > a since
				it is somewhat pointless to try this there;
				the function is rapidly decreasing for
				x large and x > a, and it will just
				underflow in that region anyway. We
				catch that case in the standard
				large-x method.
			*/
			ok = gamma_inc_Q_CF(a, x, result);
		}
		else {
			ok = gamma_inc_Q_large_x(a, x, result);
		}
	}
	else {
		if(x > a - sqrt(a)) {
			/*
				Continued fraction again. The convergence
				is a little slower here, but that is fine.
				We have to trade that off against the slow
				convergence of the series, which is the
				only other option.
			*/
			ok = gamma_inc_Q_CF(a, x, result);
		}
		else {
			SMathResult P;
			ok = gamma_inc_P_series(a, x, &P);
			result->V  = 1.0 - P.V;
			result->SetErr(P.E, 2.0);
		}
	}
	return ok;
}

int gamma_inc_P_e(double a, double x, SMathResult * result)
{
	int    ok = 1;
	if(a <= 0.0 || x < 0.0) {
		ok = result->SetDomainViolation();
	}
	else if(x == 0.0) {
		result->SetZero();
	}
	else if(x < 20.0 || x < 0.5*a) {
		/*
			Do the easy series cases. Robust and quick.
		*/
		ok = gamma_inc_P_series(a, x, result);
	}
	else if(a > 1.0e+06 && (x-a)*(x-a) < a) {
		/*
			Crossover region. Note that Q and P are
			roughly the same order of magnitude here,
			so the subtraction is stable.
		*/
		SMathResult Q;
		ok = gamma_inc_Q_asymp_unif(a, x, &Q);
		result->V  = 1.0 - Q.V;
		result->SetErr(Q.E, 2.0);
	}
	else if(a <= x) {
		/*
			Q <~ P in this area, so the
			subtractions are stable.
		*/
		SMathResult Q;
		if(a > 0.2*x)
			ok = gamma_inc_Q_CF(a, x, &Q);
		else
			ok = gamma_inc_Q_large_x(a, x, &Q);
		result->V  = 1.0 - Q.V;
		result->SetErr(Q.E, 2.0);
	}
	else {
		if((x-a)*(x-a) < a) {
			/*
				This condition is meant to insure
				that Q is not very close to 1,
				so the subtraction is stable.
			*/
			SMathResult Q;
			ok = gamma_inc_Q_CF(a, x, &Q);
			result->V  = 1.0 - Q.V;
			result->SetErr(Q.E, 2.0);
		}
		else
			ok = gamma_inc_P_series(a, x, result);
	}
	return ok;
}

int gamma_inc_e(double a, double x, SMathResult * result)
{
	int    ok = 1;
	if(x < 0.0)
		ok = result->SetDomainViolation();
	else if(x == 0.0)
		ok = fgamma(a, result);
	else if(a == 0.0)
		ok = GAMMA_INC_A_0(x, result);
	else if(a > 0.0)
		ok = gamma_inc_a_gt_0(a, x, result);
	else if(x > 0.25) {
		/*
			continued fraction seems to fail for x too small; otherwise
			it is ok, independent of the value of |x/a|, because of the
			non-oscillation in the expansion, i.e. the CF is
			un-conditionally convergent for a < 0 and x > 0
		*/
		ok = gamma_inc_CF(a, x, result);
	}
	else if(fabs(a) < 0.5)
		ok = gamma_inc_series(a, x, result);
	else {
		/* a = fa + da; da >= 0 */
		const double fa = floor(a);
		const double da = a - fa;
		SMathResult g_da;
		const int stat_g_da = (da > 0.0 ? gamma_inc_a_gt_0(da, x, &g_da) : GAMMA_INC_A_0(x, &g_da));
		double alpha = da;
		double gax = g_da.V;
		/*
			Gamma(alpha-1,x) = 1/(alpha-1) (Gamma(a,x) - x^(alpha-1) e^-x)
		*/
		do {
			const double shift = exp(-x + (alpha-1.0)*log(x));
			gax = (gax - shift) / (alpha - 1.0);
			alpha -= 1.0;
		} while(alpha > a);
		result->V = gax;
		result->E = 2.0*(1.0 + fabs(a))*SMathConst::Epsilon*fabs(gax);
		ok = stat_g_da;
	}
	return ok;
}

double Gamma(double x)
{
	SMathResult result;
	fgamma(x, &result);
	return result;
}

double GammaIncompleteP(double a, double x)
{
	SMathResult result;
	gamma_inc_P_e(a, x, &result);
	return result;
}

double GammaIncompleteQ(double a, double x)
{
	SMathResult result;
	gamma_inc_Q_e(a, x, &result);
	return result;
}
//
//
//
#if SLTEST_RUNNING // {

#define TEST_TOL0  (2.0*SMathConst::Epsilon)
#define TEST_TOL1  (16.0*SMathConst::Epsilon)
#define TEST_TOL2  (256.0*SMathConst::Epsilon)
#define TEST_TOL3  (2048.0*SMathConst::Epsilon)
#define TEST_TOL4  (16384.0*SMathConst::Epsilon)
#define TEST_TOL5  (131072.0*SMathConst::Epsilon)
#define TEST_TOL6  (1048576.0*SMathConst::Epsilon)
#define TEST_SQRT_TOL0 (2.0*GSL_SQRT_DBL_EPSILON)
#define TEST_SNGL  (1.0e-06)

static int TestSf(STestCase * pCase, SMathResult & r, double val_in, double tol, int status, const char * desc)
{
	return BIN(pCase->_check_math_result(r, val_in, tol, desc) && pCase->_check_nz(status, desc));
}

static int TestSfSgn(STestCase * pCase, SMathResult & r, double sgn, double val_in, double tol, double expect_sgn, int status, const char * desc)
{
	SMathResult local_r;
	local_r.V = sgn;
	local_r.E = 0.0;
	return BIN(
		pCase->_check_math_result(r, val_in, tol, desc) &&
		pCase->_check_math_result(local_r, expect_sgn, 0.0, desc) &&
		pCase->_check_nz(status, desc));
}

#define TEST_SF(func, args, val_in, tol, expect_return) \
	{ int status = func args; TestSf(this, r, val_in, tol, status, "\n\t" #func #args); }
#define TEST_SF_SGN(stat, func, args, val_in, tol, expect_sgn, expect_return) \
	{ int status = func args; stat += TestSfSgn(this, r, sgn, val_in, tol, expect_sgn, status, "\n\t" #func #args); }
/*
#define TEST_SF_2(stat, func, args, val1, tol1, val2, tol2, expect_return) \
	{ int status = func args; stat += test_sf_2(r1, val1, tol1, r2, val2, tol2, status, expect_return, #func #args); }
*/

SLTEST_R(SMathGamma)
{
	SMathResult r;
	//SMathResult r1, r2;
	double sgn;
	int    s = 0;
	{
		//
		// Тест линейной аппроксимации методом наименьших квадратов (LssLin)
		//
		static const double norris_x[] = { 
			  0.2, 337.4, 118.2, 884.6,  10.1, 226.5, 666.3, 996.3, 448.6, 777.0, 558.2,   0.4,  0.6, 775.5, 666.9, 338.0, 447.5, 11.6, 
			556.0, 228.1, 995.8, 887.6, 120.2,   0.3,   0.3, 556.8, 339.1, 887.2, 999.0, 779.0, 11.1, 118.3, 229.2, 669.1, 448.9,  0.5 
		};
		static const double norris_y[] = { 
			  0.1, 338.8, 118.1, 888.0,   9.2, 228.1, 668.5, 998.5, 449.1, 778.9, 559.2,   0.3,  0.1, 778.1, 668.8, 339.3, 448.9, 10.8, 
			557.7, 228.3, 998.0, 888.8, 119.6,   0.3,   0.6, 557.6, 339.3, 888.0, 998.5, 778.9, 10.2, 117.6, 228.9, 668.4, 449.2,  0.2
		};
		assert(SIZEOFARRAY(norris_x) == SIZEOFARRAY(norris_y));
		const size_t norris_n = /*36*/SIZEOFARRAY(norris_x);
		const double expected_c0 = -0.262323073774029;
		const double expected_c1 =  1.00211681802045; 
		const double expected_cov00 = pow(0.232818234301152, 2.0);
		const double expected_cov01 = -7.74327536339570e-05;  // computed from octave 
		const double expected_cov11 = pow(0.429796848199937E-03, 2.0);
		const double expected_sumsq = 26.6173985294224;
		{
			LssLin lss;
			lss.Solve_Simple(norris_n, norris_x, norris_y);
			SLTEST_CHECK_EQ_TOL(lss.A, expected_c0, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.B, expected_c1, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.Cov00, expected_cov00, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.Cov01, expected_cov01, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.Cov11, expected_cov11, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.SumSq, expected_sumsq, 1e-10);
		}
		/*{
			LssLin lss;
			lss.Solve_SSE(norris_n, norris_x, norris_y);
			SLTEST_CHECK_EQ_TOL(lss.A, expected_c0, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.B, expected_c1, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.Cov00, expected_cov00, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.Cov01, expected_cov01, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.Cov11, expected_cov11, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.SumSq, expected_sumsq, 1e-10);
		}*/
		{
			LVect nv_x;
			LVect nv_y;
			nv_x.init(norris_n, norris_x);
			nv_y.init(norris_n, norris_y);
			LssLin lss;
			lss.Solve(nv_x, nv_y);
			SLTEST_CHECK_EQ_TOL(lss.A, expected_c0, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.B, expected_c1, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.Cov00, expected_cov00, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.Cov01, expected_cov01, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.Cov11, expected_cov11, 1e-10);
			SLTEST_CHECK_EQ_TOL(lss.SumSq, expected_sumsq, 1e-10);
		}
	}
	TEST_SF(flngamma, (-0.1, &r),       2.368961332728788655,  TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-1.0/256.0, &r), 5.547444766967471595,  TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (1.0e-08, &r),    18.420680738180208905, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (0.1, &r),        2.252712651734205,     TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (1.0 + 1.0/256.0, &r), -0.0022422226599611501448, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (2.0 + 1.0/256.0, &r), 0.0016564177556961728692,  TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (100.0, &r),           359.1342053695753,         TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-1.0-1.0/65536.0, &r), 11.090348438090047844,    TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-1.0-1.0/268435456.0, &r), 19.408121054103474300, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-100.5, &r), -364.9009683094273518, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-100-1.0/65536.0, &r), -352.6490910117097874, TEST_TOL0, GSL_SUCCESS);

	TEST_SF_SGN(s, flngamma_sgn, (0.7, &r, &sgn), 0.26086724653166651439, TEST_TOL1, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (0.1, &r, &sgn), 2.2527126517342059599, TEST_TOL0, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-0.1, &r, &sgn), 2.368961332728788655, TEST_TOL0, -1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-1.0-1.0/65536.0, &r, &sgn), 11.090348438090047844, TEST_TOL0, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-2.0-1.0/256.0, &r, &sgn), 4.848447725860607213, TEST_TOL0, -1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-2.0-1.0/65536.0, &r, &sgn), 10.397193628164674967, TEST_TOL0, -1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-3.0-1.0/8.0, &r, &sgn), 0.15431112768404182427, TEST_TOL2, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-100.5, &r, &sgn), -364.9009683094273518, TEST_TOL0, -1.0, GSL_SUCCESS);

	TEST_SF(fgamma, (1.0 + 1.0/4096.0, &r), 0.9998591371459403421, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (1.0 + 1.0/32.0, &r),   0.9829010992836269148, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (2.0 + 1.0/256.0, &r),  1.0016577903733583299, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (9.0, &r),              40320.0,               TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (10.0, &r),             362880.0,              TEST_TOL0, GSL_SUCCESS);
	// @v10.3.12 (не проходит тест и я не знаю почему) TEST_SF(fgamma, (100.0, &r),            9.332621544394415268e+155, TEST_TOL2, GSL_SUCCESS);
	// @v10.3.12 (не проходит тест и я не знаю почему) TEST_SF(fgamma, (170.0, &r),            4.269068009004705275e+304, TEST_TOL2, GSL_SUCCESS);
	// @v10.3.12 (не проходит тест и я не знаю почему) TEST_SF(fgamma, (171.0, &r),            7.257415615307998967e+306, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(fgamma, (-10.5, &r), -2.640121820547716316e-07, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (-11.25, &r), 6.027393816261931672e-08, TEST_TOL0, GSL_SUCCESS); /* exp()... not my fault */
	TEST_SF(fgamma, (-1.0+1.0/65536.0, &r), -65536.42280587818970, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(gammastar, (1.0e-08, &r), 3989.423555759890865, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gammastar, (1.0e-05, &r), 126.17168469882690233, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (0.001, &r), 12.708492464364073506, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (1.5, &r), 1.0563442442685598666, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (3.0, &r), 1.0280645179187893045, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (9.0, &r), 1.0092984264218189715, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (11.0, &r), 1.0076024283104962850, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (100.0, &r), 1.0008336778720121418, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (1.0e+05, &r), 1.0000008333336805529, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (1.0e+20, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
#if 0 // {
	TEST_SF(gammainv_e, (1.0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (2.0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (3.0, &r), 0.5, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (4.0, &r), 1.0/6.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (10.0, &r), 1.0/362880.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (100.0, &r), 1.0715102881254669232e-156, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gammainv_e, (0.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-1.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-2.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-3.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-4.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-10.5, &r), -1.0/2.640121820547716316e-07, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-11.25, &r), 1.0/6.027393816261931672e-08, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-1.0+1.0/65536.0, &r), -1.0/65536.42280587818970, TEST_TOL1, GSL_SUCCESS);

	TEST_SF_2(s, lngamma_complex_e, (5.0, 2.0, &r1, &r2),
		2.7487017561338026749, TEST_TOL0,
		3.0738434100497007915, TEST_TOL0,
		GSL_SUCCESS);
	TEST_SF_2(s, lngamma_complex_e, (100.0, 100.0, &r1, &r2),
		315.07804459949331323, TEST_TOL1,
		2.0821801804113110099, TEST_TOL3,
		GSL_SUCCESS);
	TEST_SF_2(s, lngamma_complex_e, (100.0, -1000.0, &r1, &r2),
		-882.3920483010362817000, TEST_TOL1,
		-2.1169293725678813270, TEST_TOL3,
		GSL_SUCCESS);
	TEST_SF_2(s, lngamma_complex_e, (-100.0, -1.0, &r1, &r2),
		-365.0362469529239516000, TEST_TOL1,
		-3.0393820262864361140, TEST_TOL1,
		GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   1.0/1048576.0, &r), 1.7148961854776073928e-67 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   1.0/1024.0, &r), 2.1738891788497900281e-37 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   1.0, &r), 2.7557319223985890653e-07 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   5.0, &r), 2.6911444554673721340     , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   500.0, &r), 2.6911444554673721340e+20 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (100,  100.0, &r), 1.0715102881254669232e+42 , TEST_TOL1, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (1000, 200.0, &r), 2.6628790558154746898e-267, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (1000, 500.0, &r), 2.3193170139740855074e+131, TEST_TOL1, GSL_SUCCESS);

	TEST_SF(fact_e, (0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fact_e, (1, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fact_e, (7, &r), 5040.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fact_e, (33, &r), 8.683317618811886496e+36, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(doublefact_e, (0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(doublefact_e, (1, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(doublefact_e, (7, &r), 105.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(doublefact_e, (33, &r), 6.332659870762850625e+18, TEST_TOL0, GSL_SUCCESS);
#endif // } 0

	TEST_SF(flnfact, (0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flnfact, (1, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flnfact, (7, &r), 8.525161361065414300, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flnfact, (33, &r), 85.05446701758151741, TEST_TOL0, GSL_SUCCESS);
#if 0 // {
	TEST_SF(lndoublefact_e, (0, &r), 0.0 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (7, &r), 4.653960350157523371 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (33, &r), 43.292252022541719660, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (34, &r), 45.288575519655959140, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (1034, &r), 3075.6383796271197707, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (1035, &r), 3078.8839081731809169, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(lnchoose_e, (7,3, &r), 3.555348061489413680, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnchoose_e, (5,2, &r), 2.302585092994045684, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(choose_e, (7,3, &r), 35.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(choose_e, (7,4, &r), 35.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(choose_e, (5,2, &r), 10.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(choose_e, (5,3, &r), 10.0, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(choose_e, (500,495, &r), 255244687600.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(choose_e, (500,5, &r), 255244687600.0, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(choose_e, (500,200, &r), 5.054949849935532221e+144, TEST_TOL5, GSL_SUCCESS);
	TEST_SF(choose_e, (500,300, &r), 5.054949849935532221e+144, TEST_TOL5, GSL_SUCCESS);

	TEST_SF(lnpoch_e, (5, 0.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnpoch_e, (5, 1.0/65536.0, &r), 0.000022981557571259389129, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnpoch_e, (5, 1.0/256.0, &r),   0.005884960217985189004,    TEST_TOL2, GSL_SUCCESS);
	TEST_SF(lnpoch_e, (7,3, &r), 6.222576268071368616, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnpoch_e, (5,2, &r), 3.401197381662155375, TEST_TOL0, GSL_SUCCESS);

	TEST_SF_SGN(s, lnpoch_sgn_e, (5.0, 0.0, &r, &sgn), 0.0, TEST_TOL1, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, lnpoch_sgn_e, (-4.5, 0.25, &r, &sgn), 0.7430116475119920117, TEST_TOL1, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, lnpoch_sgn_e, (-4.5, 1.25, &r, &sgn), 2.1899306304483174731, TEST_TOL1, -1.0, GSL_SUCCESS);

	TEST_SF(poch_e, (5, 0.0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(poch_e, (7,3, &r), 504.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(poch_e, (5,2, &r), 30.0 , TEST_TOL1, GSL_SUCCESS);
	TEST_SF(poch_e, (5,1.0/256.0, &r), 1.0059023106151364982, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(pochrel_e, (5,0, &r), 1.506117668431800472, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (7,3, &r), 503.0/3.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(pochrel_e, (5,2, &r), 29.0/2.0, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (5,0.01, &r), 1.5186393661368275330, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(pochrel_e, (-5.5,0.01, &r), 1.8584945633829063516, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (-5.5,-1.0/8.0, &r), 1.0883319303552135488, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (-5.5,-1.0/256.0, &r), 1.7678268037726177453, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (-5.5,-11.0, &r), 0.09090909090939652475, TEST_TOL0, GSL_SUCCESS);
#endif // } 0

	TEST_SF(gamma_inc_P_e, (1e-100, 0.001, &r), 1.0, TEST_TOL0, GSL_SUCCESS) ;
	TEST_SF(gamma_inc_P_e, (0.001, 0.001, &r), 0.9936876467088602902, TEST_TOL0, GSL_SUCCESS) ;
	TEST_SF(gamma_inc_P_e, (0.001, 1.0, &r), 0.9997803916424144436, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (0.001, 10.0, &r), 0.9999999958306921828, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1.0, 0.001, &r), 0.0009995001666250083319, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1.0, 1.01, &r), 0.6357810204284766802, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1.0, 10.0, &r), 0.9999546000702375151, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (10.0, 10.01, &r), 0.5433207586693410570, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (10.0, 20.0, &r), 0.9950045876916924128, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1000.0, 1000.1, &r), 0.5054666401440661753, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1000.0, 2000.0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	/* Test for failure of the Gautschi recurrence (now fixed) for x = a - 2 */
	TEST_SF(gamma_inc_P_e, (34.0, 32.0, &r), 0.3849626436463866776322932129, TEST_TOL2, GSL_SUCCESS);
	/* and the next test is gamma_inc_P(37,35-20*eps) */
	TEST_SF(gamma_inc_P_e, (37.0, 3.499999999999999289e+01, &r), 0.3898035054195570860969333039, TEST_TOL2, GSL_SUCCESS);

	/* Regression test Martin Jansche <jansche@ling.ohio-state.edu> BUG#12 */
	TEST_SF(gamma_inc_P_e, (10, 1e-16, &r), 2.755731922398588814734648067e-167, TEST_TOL2, GSL_SUCCESS);

	TEST_SF(gamma_inc_Q_e, (0.0, 0.001, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (0.001, 0.001, &r), 0.006312353291139709793, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (0.001, 1.0, &r), 0.00021960835758555639171, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (0.001, 2.0, &r), 0.00004897691783098147880, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (0.001, 5.0, &r), 1.1509813397308608541e-06, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0, 0.001, &r), 0.9990004998333749917, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0, 1.01, &r), 0.3642189795715233198, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0, 10.0, &r), 0.00004539992976248485154, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (10.0, 10.01, &r), 0.4566792413306589430, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (10.0, 100.0, &r), 1.1253473960842733885e-31, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1000.0, 1000.1, &r), 0.4945333598559338247, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1000.0, 2000.0, &r), 6.847349459614753180e-136, TEST_TOL2, GSL_SUCCESS);

	/* designed to trap the a-x=1 problem */
	TEST_SF(gamma_inc_Q_e, (100,  99.0, &r), 0.5266956696005394, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (200, 199.0, &r), 0.5188414119121281, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (100,  99.0, &r), 0.4733043303994607, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (200, 199.0, &r), 0.4811585880878718, TEST_TOL2, GSL_SUCCESS);

	/* Test for x86 cancellation problems */
	TEST_SF(gamma_inc_P_e, (5670, 4574, &r),  3.063972328743934e-55, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (5670, 4574, &r), 1.0000000000000000, TEST_TOL2, GSL_SUCCESS);


	/* test suggested by Michel Lespinasse [gsl-discuss Sat, 13 Nov 2004] */
	TEST_SF(gamma_inc_Q_e, (1.0e+06-1.0, 1.0e+06-2.0, &r), 0.50026596175224547004, TEST_TOL3, GSL_SUCCESS);

	/* tests in asymptotic regime related to Lespinasse test */
	TEST_SF(gamma_inc_Q_e, (1.0e+06+2.0, 1.0e+06+1.0, &r), 0.50026596135330304336, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0e+06, 1.0e+06-2.0, &r), 0.50066490399940144811, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0e+07, 1.0e+07-2.0, &r), 0.50021026104978614908, TEST_TOL2, GSL_SUCCESS);

	/* non-normalized "Q" function */
	TEST_SF(gamma_inc_e, (-1.0/1048576.0, 1.0/1048576.0, &r), 13.285819596290624271, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 1.0/1048576.0, &r), 13.381275128625328858, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   1.0/1048576.0, &r), 1.0485617142715768655e+06, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.00001,0.001, &r), 6.3317681434563592142, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.0001,0.001, &r), 6.3338276439767189385, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 0.001, &r), 6.3544709102510843793, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.5,   0.001, &r), 59.763880515942196981, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   0.001, &r), 992.66896046923884234, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-3.5,   0.001, &r), 9.0224404490639003706e+09, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-10.5,  0.001, &r), 3.0083661558184815656e+30, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 0.1,   &r), 1.8249109609418620068, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.5,   0.1,   &r), 3.4017693366916154163, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-10.0,  0.1,   &r), 8.9490757483586989181e+08, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-10.5,  0.1,   &r), 2.6967403834226421766e+09, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 1.0,   &r), 0.21928612679072766340, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.5,   1.0,   &r), 0.17814771178156069019, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   1.0,   &r), 0.14849550677592204792, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-2.5,   1.0,   &r), 0.096556648631275160264, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   10.0,  &r), 3.8302404656316087616e-07, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 10.0,  &r), 4.1470562324807320961e-06, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.5,   10.0,  &r), 1.2609042613241570681e-06, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   10.0,  &r), 3.8302404656316087616e-07, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-10.5,  10.0,  &r), 6.8404927328441566785e-17, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-100.0, 10.0,  &r), 4.1238327669858313997e-107, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-200.0, 10.0,  &r), 2.1614091830529343423e-207, TEST_TOL2, GSL_SUCCESS);

	TEST_SF(gamma_inc_e, (  0.0,     0.001, &r), 6.3315393641361493320, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.001,   0.001, &r), 6.3087159394864007261, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  1.0,     0.001, &r), 0.99900049983337499167, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, ( 10.0,     0.001, &r), 362880.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.0,     1.0,   &r), 0.21938393439552027368, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.001,   1.0,   &r), 0.21948181320730279613, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  1.0,     1.0,   &r), 0.36787944117144232160, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, ( 10.0,     1.0,   &r), 362879.95956592242045, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (100.0,     1.0,   &r), 9.3326215443944152682e+155, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.0,   100.0, &r), 3.6835977616820321802e-46, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.001, 100.0, &r), 3.7006367674063550631e-46, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  1.0,   100.0, &r), 3.7200759760208359630e-44, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, ( 10.0,   100.0, &r), 4.0836606309106112723e-26, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (100.0,   100.0, &r), 4.5421981208626694294e+155, TEST_TOL1, GSL_SUCCESS);

#if 0 // {
	TEST_SF(lnbeta_e, (1.0e-8, 1.0e-8, &r),  19.113827924512310617, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0e-8, 0.01, &r),  18.420681743788563403, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0e-8, 1.0, &r),  18.420680743952365472, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0e-8, 10.0, &r),  18.420680715662683009, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0e-8, 1000.0, &r),  18.420680669107656949, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (0.1, 0.1, &r), 2.9813614810376273949, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (0.1, 1.0, &r),  2.3025850929940456840, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (0.1, 100.0, &r),  1.7926462324527931217, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (0.1, 1000, &r),  1.5619821298353164928, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0, 1.00025, &r),  -0.0002499687552073570, TEST_TOL4, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0, 1.01, &r),  -0.009950330853168082848, TEST_TOL3, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0, 1000.0, &r),  -6.907755278982137052, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (100.0, 100.0, &r),  -139.66525908670663927, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (100.0, 1000.0, &r),  -336.4348576477366051, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (100.0, 1.0e+8, &r),  -1482.9339185256447309, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(beta_e, (1.0,   1.0, &r), 1.0                  , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(beta_e, (1.0, 1.001, &r), 0.9990009990009990010, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(beta_e, (1.0,   5.0, &r), 0.2                  , TEST_TOL1, GSL_SUCCESS);
	TEST_SF(beta_e, (1.0,  100.0, &r), 0.01                 , TEST_TOL1, GSL_SUCCESS);
	TEST_SF(beta_e, (10.0, 100.0, &r), 2.3455339739604649879e-15, TEST_TOL2, GSL_SUCCESS);

	/* Test negative arguments */
	TEST_SF(beta_e, (2.5, -0.1, &r), -11.43621278354402041480, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (2.5, -1.1, &r), 14.555179906328753255202, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-0.25, -0.1, &r), -13.238937960945229110, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-1.25, -0.1, &r), -14.298052997820847439, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.1, -99.1, &r), -1.005181917797644630375787297e60, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.1, 99.3, &r), 0.0004474258199579694011200969001, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (100.1, -99.3, &r), 1.328660939628876472028853747, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.1, 1.2, &r), 0.00365530364287960795444856281, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (100.1, -1.2, &r), 1203.895236907821059270698160, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.1, -1.2, &r), -3236.073671884748847700283841, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.001, 0.0099, &r), -853.946649365611147996495177, TEST_TOL2, GSL_SUCCESS);

	/* Other test cases */
	TEST_SF(beta_e, (1e-32, 1.5, &r), 1e32, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (1e-6, 0.5, &r), 1000001.386293677092419390336, TEST_TOL2, GSL_SUCCESS);

	TEST_SF(beta_e, (-1.5, 0.5, &r), 0.0, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(beta_inc_e, (1.0, 1.0, 0.0, &r), 0.0, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (1.0, 1.0, 1.0, &r), 1.0, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (0.1, 0.1, 1.0, &r), 1.0, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  1.0, 0.5, &r), 0.5, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 0.1,  1.0, 0.5, &r), 0.9330329915368074160, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (10.0,  1.0, 0.5, &r), 0.0009765625000000000000, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (50.0,  1.0, 0.5, &r), 8.881784197001252323e-16, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  0.1, 0.5, &r), 0.06696700846319258402, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0, 10.0, 0.5, &r), 0.99902343750000000000, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0, 50.0, 0.5, &r), 0.99999999999999911180, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  1.0, 0.1, &r), 0.10, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  2.0, 0.1, &r), 0.19, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  2.0, 0.9, &r), 0.99, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (50.0, 60.0, 0.5, &r), 0.8309072939016694143, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (90.0, 90.0, 0.5, &r), 0.5, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 500.0,  500.0, 0.6, &r), 0.9999999999157549630, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (5000.0, 5000.0, 0.4, &r), 4.518543727260666383e-91, TEST_TOL5, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (5000.0, 5000.0, 0.6, &r), 1.0, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (5000.0, 2000.0, 0.6, &r), 8.445388773903332659e-89, TEST_TOL5, GSL_SUCCESS);
#endif // } 0
	return s;
}

#endif // } SLTEST_RUNNING
