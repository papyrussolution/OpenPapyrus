package ru.petroglif.styloq;

import org.junit.Assert;
import org.junit.Test;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.UUID;

public class StyloQ_UnitTest {
	@Test public void satoi()
	{
		Assert.assertTrue(SLib.satoi(null) == 0);
		Assert.assertTrue(SLib.satoi("") == 0);
		Assert.assertTrue(SLib.satoi("0") == 0);
		Assert.assertTrue(SLib.satoi("-") == 0);
		Assert.assertTrue(SLib.satoi("+") == 0);
		Assert.assertTrue(SLib.satoi("0x") == 0);
		Assert.assertTrue(SLib.satoi("0x0") == 0);
		Assert.assertTrue(SLib.satoi("-0x0") == 0);
		Assert.assertTrue(SLib.satoi("1") == 1);
		Assert.assertTrue(SLib.satoi("0x1") == 1);
		Assert.assertTrue(SLib.satoi("-1") == -1);
		Assert.assertTrue(SLib.satoi("+1") == 1);
		Assert.assertTrue(SLib.satoi("0x0ff") == 255);
		Assert.assertTrue(SLib.satoi(" -0x0ff") == -255);
		Assert.assertTrue(SLib.satoi("0xb") == 11);
		Assert.assertTrue(SLib.satoi("  1000") == 1000);
		Assert.assertTrue(SLib.satoi("  1000  ") == 1000);
		Assert.assertTrue(SLib.satoi("  1000nothing  ") == 1000);
		Assert.assertTrue(SLib.satoi(" \t 90001011  ") == 90001011);
		Assert.assertTrue(SLib.satoi(" abracadabra100  ") == 0);
		Assert.assertTrue(SLib.satoi("\t 0xabcdef0") == 0xabcdef0);
		Assert.assertTrue(SLib.satoi("\t\t \t 1234567890") == 1234567890);
	}
	@Test public void DateTime()
	{
		SLib.LDATE d = SLib.GetCurDate();
		Assert.assertTrue(SLib.CheckDate(d.day(), d.month(), d.year()));
		String ds = SLib.datefmt(d.day(), d.month(), d.year(), SLib.DATF_DMY|SLib.DATF_CENTURY);
		SLib.LDATE d2 = SLib.strtodate(ds, SLib.DATF_DMY);
		Assert.assertTrue(d2.v == d.v);
		ds = SLib.datefmt(d.day(), d.month(), d.year(), SLib.DATF_GERMAN);
		d2 = SLib.strtodate(ds, SLib.DATF_DMY);
		Assert.assertTrue(d2.v == d.v);
		ds = SLib.datefmt(d.day(), d.month(), d.year(), SLib.DATF_ISO8601);
		d2 = SLib.strtodate(ds, SLib.DATF_ISO8601);
		Assert.assertTrue(d2.v == d.v);
		ds = SLib.datefmt(d.day(), d.month(), d.year(), SLib.DATF_JAPAN|SLib.DATF_CENTURY);
		d2 = SLib.strtodate(ds, SLib.DATF_JAPAN);
		Assert.assertTrue(d2.v == d.v);
		//
		d = new SLib.LDATE(1, 1, 2001);
		Assert.assertTrue(d.DayOfWeek(true) == 1);
		for(int i = 1; i < 365; i++) {
			d = SLib.LDATE.Plus(new SLib.LDATE(1, 1, 2001), i);
			int expected_dow = (i+1) % 7;
			if(expected_dow == 0)
				expected_dow = 7;
			int dow = d.DayOfWeek(true);
			Assert.assertTrue(dow == expected_dow);
		}
		//
		{
			SLib.LTIME t = new SLib.LTIME(7, 21, 57, 900);
			int h = t.hour();
			int m = t.minut();
			int s = t.sec();
			int hs = t.hs();
			Assert.assertTrue(h == 7);
			Assert.assertTrue(m == 21);
			Assert.assertTrue(s == 57);
			Assert.assertTrue(hs == 90);
			t.settotalsec(50000);
			s = t.totalsec();
			Assert.assertTrue(s == 50000);
			int nd = t.settotalsec((24 * 3600) * 2 + 50000);
			s = t.totalsec();
			Assert.assertTrue(nd == 2);
			Assert.assertTrue(s == (24 * 3600) * 2 + 50000);
			//
			//
			t = new SLib.LTIME(7, 21, 57, 900);
			String ts = SLib.timefmt(t, SLib.TIMF_HMS);
			Assert.assertTrue(ts.equals("07:21:57"));
			ts = SLib.timefmt(t, SLib.TIMF_HM);
			Assert.assertTrue(ts.equals("07:21"));
			ts = SLib.timefmt(t, SLib.TIMF_HMS|SLib.TIMF_NODIV);
			Assert.assertTrue(ts.equals("072157"));
			ts = SLib.timefmt(t, SLib.TIMF_HMS|SLib.TIMF_MSEC);
			Assert.assertTrue(ts.equals("07:21:57.900"));
			//ts = SLib.timefmt(t, SLib.TIMF_HMS|SLib.TIMF_TIMEZONE);
			//Assert.assertTrue(ts.equals("07:21:57"));
			{
				t = SLib.strtotime("07:21:57", SLib.TIMF_HMS);
				Assert.assertTrue(t.hour() == 7);
				Assert.assertTrue(t.minut() == 21);
				Assert.assertTrue(t.sec() == 57);
				Assert.assertTrue(t.hs() == 0);

				t = SLib.strtotime("07:21:57.910", SLib.TIMF_HMS);
				h = t.hour();
				m = t.minut();
				s = t.sec();
				hs = t.hs();
				Assert.assertTrue(t.hour() == 7);
				Assert.assertTrue(t.minut() == 21);
				Assert.assertTrue(t.sec() == 57);
				Assert.assertTrue(t.hs() == 91);
			}
			/*{
				for(h = 0; h < 24; h++) {
					for(m = 0; m < 60; m++) {
						for(s = 0; s < 60; s++) {
							for(hs = 0; hs < 100; hs++) {
								t = new SLib.LTIME(h, m, s, hs*10);
								ts = SLib.timefmt(t, SLib.TIMF_HMS|SLib.TIMF_MSEC);
								SLib.LTIME t2 = SLib.strtotime(ts, 0);
								Assert.assertTrue(t.v == t2.v);
							}
						}
					}
				}
			}*/
		}
	}
	@Test public void CompressionSignature()
	{
		byte [] long_buf = SLib.LongToBytes(SLib.Ssc_CompressionSignature);
		long lv = SLib.BytesToLong(long_buf, 0);
		Assert.assertTrue(lv == SLib.Ssc_CompressionSignature);
	}
	@Test public void STokenRecognizer()
	{
		STokenRecognizer tr = new STokenRecognizer();
		STokenRecognizer.TokenArray ta = null;
		ta = tr.Run("0123");
		Assert.assertTrue(ta != null && ta.S != null && ta.S.Len == 4 && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) > 0.0f);
		ta = tr.Run("4610017121115");
		Assert.assertTrue(ta != null && ta.S != null && ta.S.Len == 13 && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_EAN13) > 0.0f);
		ta = tr.Run("4610017121116"); // Инвалидная контрольная цифра
		Assert.assertTrue(ta != null && ta.S != null && ta.S.Len == 13 && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_EAN13) == 0.0f);
		ta = tr.Run("20352165");
		Assert.assertTrue(ta != null && ta.S != null && ta.S.Len == 8 && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_EAN8) > 0.0f);
		ta = tr.Run("100100802804");
		Assert.assertTrue(ta != null && ta.S != null && ta.S.Len == 12 && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_RU_INN) > 0.0f);
		ta = tr.Run("47296611");
		Assert.assertTrue(ta != null && ta.S != null && ta.S.Len == 8 && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_RU_OKPO) > 0.0f);
		ta = tr.Run("99057850");
		Assert.assertTrue(ta != null && ta.S != null && ta.S.Len == 8 && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_RU_OKPO) > 0.0f);
		ta = tr.Run("6z3417681");
		Assert.assertTrue(ta != null && ta.S != null && ta.S.Len == 9 && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) == 0.0f && ta.Has(STokenRecognizer.SNTOK_RU_OKPO) == 0.0f);
		ta = tr.Run("6993417681");
		Assert.assertTrue(ta != null && ta.S != null && ta.S.Len == 10 && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_RU_OKPO) == 0.0f);
		ta = tr.Run("0034012000001472206");
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_DIGITCODE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_EGAISWARECODE) > 0.0f);
		ta = tr.Run("a98P8s00W");
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_DIGLAT) > 0.0f);
		ta = tr.Run("4FC737F1-C7A5-4376-A066-2A32D752A2FF");
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_GUID) > 0.0f);
		ta = tr.Run("mail.123@gogo-fi.com");
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_EMAIL) > 0.0f);
		ta = tr.Run("123-15-67");
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_PHONE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_EMAIL) == 0.0f);
		ta = tr.Run("+7(911)123-15-67");
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_PHONE) > 0.0f && ta.Has(STokenRecognizer.SNTOK_DIGLAT) == 0.0f);

		ta = tr.Run("00000046209443j+Q\'?P5ACZAC8bG");
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_CHZN_CIGITEM) > 0.0f);
		ta = tr.Run("00000046209443x-8xfgOACZAYGfv");
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_CHZN_CIGITEM) > 0.0f);
		ta = tr.Run("04606203098187o&zWeIyABr8l/nT");
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_CHZN_CIGITEM) > 0.0f);
		//ta = tr.Run("04606203098187o&zWe\u0081yABr8l/nT"); // !SNTOK_CHZN_CIGITEM
		//Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_CHZN_CIGITEM) == 0.0f);
		ta = tr.Run("00000%46209443x-8xfgOACZAYGfv"); // !SNTOK_CHZN_CIGITEM
		Assert.assertTrue(ta != null && ta.S != null && ta.Has(STokenRecognizer.SNTOK_CHZN_CIGITEM) == 0.0f);
		/*
			tr.Run((const uchar *)"354190023896443", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_IMEI));
			tr.Run((const uchar *)"192.168.0.1", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_IP4));
			tr.Run((const uchar *)"1/12/2018", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DATE));
			tr.Run((const uchar *)"20180531", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DATE));
			// @notimplemented tr.Run((const uchar *)"11:30", -1, nta.Z(), 0);
			// @notimplemented SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_TIME));
			// @notimplemented tr.Run((const uchar *)"11:30:46", -1, nta.Z(), 0);
			// @notimplemented SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_TIME));
			tr.Run((const uchar *)"10.2.5", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_SOFTWAREVER));
			tr.Run((const uchar *)"#000000", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_COLORHEX));
			tr.Run((const uchar *)"#f82aB7", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_COLORHEX));

			tr.Run((const uchar *)"+100,000.00", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_DOT));
			tr.Run((const uchar *)"+100 000,00", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_COM));
			tr.Run((const uchar *)"-100'000,00", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_COM));
			tr.Run((const uchar *)"9", -1, nta.Z(), 0);
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_DOT));
		 */
	}
	@Test public void ChZnMarkParsing()
	{
		String test_code_list[] = {
			"0104810319016458212nFgzLex93u0bp",
			"01048103190149422123ogfX8793f1KQ",
			"0104810108007438212nKRgov893My3u",
			"0104810065006079212n2nWLbt93JDxc",
			"010464009192045821,RH>nog\u001D8005120000\u001D93hH2L",
			//"010475023200591017240630101210620112006302102003Z2NXSH2191EE0692FmNVSg3koMVPdHEUDNyx/Z9n5XLEWrU1OBTUTIpNjvs=",
			"(01)04640091920533(21)gSHwSiZ(8005)112000",
			"(01)04640091920533(21):QPIhfa(8005)112000",
			"(01)04640091920533(21)+k'YamU(8005)112000",
			"(01)04640091920533(21)nhB%LT1(8005)112000",
			"(01)04640091920533(21)j6SGMRa(8005)112000",
			"(01)04640091920533(21)feiZ&Wb(8005)112000",
			"(01)04640091920533(21)7>/ojpT(8005)112000",
			"(01)04640091920533(21)y0brn<:(8005)112000",
			"(01)04640091920533(21)m1s/FXH(8005)112000",
			"(01)04640091920533(21)7oGTzU>(8005)112000",
			"(01)04640091920533(21)aPCkl)9(8005)112000",
			"(01)04640091920533(21)zS0pl2f(8005)112000",
			"(01)04640091920533(21)nbDBtrM(8005)112000",
			"(01)04640091920533(21)XIKD,LJ(8005)112000",
			"(01)04640091920533(21)h7Ik<bB(8005)112000",
			"(01)04640091920533(21)gs.rM(2(8005)112000",
			"(01)04640091920533(21))2YJ\"yO(8005)112000",
			"(01)04640091920533(21)U(RDMSD(8005)112000",
			"(01)04640091920533(21)Su'NGii(8005)112000",
			"(01)04640091920533(21)mm2_rJg(8005)112000",
			"(01)04640091920533(21)RS2kp=0(8005)112000",
			"(01)04640091920533(21)sp:2o9r(8005)112000",
			"(01)04640091920533(21)hEDQ!aG(8005)112000",
			"(01)04640091920533(21)WPuQ;lK(8005)112000",
			"(01)04640091920533(21)T!d23a&(8005)112000",
			"(01)04640091920533(21)uOurmb<(8005)112000",
			"(01)04640091920533(21)kY(emyx(8005)112000",
			"(01)04640091920533(21)*mU*LgV(8005)112000",
			"(01)04640091920533(21)ksE6p/2(8005)112000",
			"(01)04640091920533(21)TQQSSqw(8005)112000",
			"(01)04640091920533(21)=sZr8//(8005)112000",
			"(01)04640091920533(21)V7CqmLc(8005)112000",
			"(01)04640091920533(21)xm&?28g(8005)112000",
			"(01)04640091920533(21)UrXncs6(8005)112000",
			"(01)04640091920533(21)haDbOUt(8005)112000",
			"(01)04640091920656(21)B)Z%tgY(8005)112000",
			"(01)04640091920656(21).%FoWf5(8005)112000",
			"(01)04640091920656(21)%R(,TP:(8005)112000",
			"(01)04640091920656(21)VjX-Q=l(8005)112000",
			"(01)04640091920656(21)FJCjlfR(8005)112000",
			"(01)04640091920656(21)KxNVMr?(8005)112000",
			"(01)04640091920656(21)T5Y-CrK(8005)112000",
			"(01)04640091920656(21)f9n_-TR(8005)112000",
			"(01)04640091920656(21)MGw/nYH(8005)112000",
			"(01)04640091920656(21)JZAiWJF(8005)112000",
			"(01)04640091920656(21)gVI)bae(8005)112000",
			"(01)04640091920656(21)zmPk>l!(8005)112000",
			"(01)04640091920656(21)9*jCdlR(8005)112000",
			"(01)04640091920656(21)eA;jphi(8005)112000",
			"(01)04640091920656(21);f0rUWn(8005)112000",
			"(01)04640091920656(21)b-qAkp)(8005)112000",
			"(01)04640091920656(21)JsVsSgN(8005)112000",
			"(01)04640091920656(21)m)5I)CQ(8005)112000",
			"(01)04640091920656(21)jdzDuZW(8005)112000",
			"(01)04640091920656(21)Vn_nfmm(8005)112000",
			"(01)04640091920380(21)CK&PDJ=(8005)130000",
			"(01)04640091920380(21)m\"lryt3(8005)130000",
			"(01)04640091920380(21)kXFXCQ-(8005)130000",
			"(01)04640091920380(21).vEPY%W(8005)130000",
			"(01)04640091920380(21)bNXaIBm(8005)130000",
			"(01)04640091920380(21)sPimXLM(8005)130000",
			"(01)04640091920380(21)iUX*aHF(8005)130000",
			"(01)04640091920380(21)BS2.bfL(8005)130000",
			"(01)04640091920380(21)?pQF?7j(8005)130000",
			"(01)04640091920380(21)ekQpMmg(8005)130000",
			"(01)04640091920380(21)6-VUV'2(8005)130000",
			"(01)04640091920380(21)PdYCnNn(8005)130000",
			"(01)04640091920380(21)pLMe!SH(8005)130000",
			"(01)04640091920380(21)TbOXZ(o(8005)130000",
			"(01)04640091920380(21)=WiDIBG(8005)130000",
			"(01)04640091920380(21)xJErkBS(8005)130000",
			"(01)04640091920380(21);LJl+jB(8005)130000",
			"(01)04640091920380(21)HTLs9qD(8005)130000",
			"(01)04640091920380(21)/tcjReu(8005)130000",
			"(01)04640091920380(21)G3lXIDY(8005)130000",
			"(01)04640091920380(21)iVq_W2M(8005)130000",
			"(01)04640091920380(21)wnbNWDH(8005)130000",
			"(01)04640091920380(21)\"jQJLkD(8005)130000",
			"(01)04640091920380(21)REXFhp;(8005)130000",
			"(01)04640091920380(21)GqoB-hu(8005)130000",
			"(01)04640091920618(21);UCCFMj(8005)135000",
			"(01)04640091920618(21)?5V_Ap-(8005)135000",
			"(01)04640091920618(21)N>ni(oO(8005)135000",
			"(01)04640091920618(21)0fXVIUX(8005)135000",
			"(01)04640091920618(21)uBfKWtY(8005)135000",
			"(01)04640091920618(21)F+VU8YK(8005)135000",
			"(01)04640091920618(21)Fg6qJbf(8005)135000",
			"(01)04640091920618(21)mAsF%u=(8005)135000",
			"(01)04640091920618(21)8ODd4wN(8005)135000",
			"(01)04640091920618(21)dx!9Q3C(8005)135000",
			"(01)04640091920618(21)DcRA&T*(8005)135000",
			"(01)04640091920618(21)lPpGLI&(8005)135000",
			"(01)04640091920618(21)Ex)Crr!(8005)135000",
			"(01)04640091920618(21)'-(%a.i(8005)135000",
			"(01)04640091920618(21)&PukVoL(8005)135000",
			"(01)04640091920519(21)mUstj4o(8005)135000",
			"(01)04640091920519(21)pVuLuTy(8005)135000",
			"(01)04640091920519(21)3ScKbLm(8005)135000",
			"(01)04640091920519(21)f3>g)lt(8005)135000",
			"(01)04640091920519(21)fGiCf8p(8005)135000",
			"010460166901031521unXxjJoRse3rl91EE0692ZIMgcIl2oboNlVDMuI9DYM3bLWSLQ20IZ0FWTTYug6M=",
			"0104660077310158210000A!)",
			"01046501187202132100000bW800509400093+Czf",
			"010460081800787921t\"XzgHU8005095000930p2J24014518552",
			"010460081800729921VM\"<kFP8005100000931Urt24014501206",
			"010460081800729921G9?ZJe3800510000093kzlS24014501206",
			"010460043993538421.UIaWH'8005153000",
			"0104600439935384211:kvaLp8005153000",
			"0104600439935384215QNVY028005153000",
			"010460043993538421=4y6_Kl8005153000",
			"010460043993538421AK,z+qf8005153000",
			"010460043993853821+oD&p6=8005102000",
			"010460043993853821EyF;_dg8005102000",
			"010460043993853821d'2IEE58005102000",
			"010460043993853821f92lGmP8005102000",
			"010460043993853821tHfkgND8005102000",
			"010460043993112621::R*MUW8005108000",
			"010460043993112621MZpy,h<8005108000",
			"010460043993112621OvUyBRL8005108000",
			"010460043993112621UZZ=NB88005108000",
			"010460043993112621dITTF?Y8005108000",
			"010460043993112621v%2J%-B8005108000",
			"01189011480058361722030010V900310240300421B6CL03VI4NAUH",
			"01189011480058361722030010V900310240300421B6NDXH5CF48SB",
			"01189011480058361722030010V900310240300421B6PTKC0ZYQIUQ",
			"01189011480058361722030010V900310240300421B6N9XNABHVW3W",
			"01189011480058361722030010V900310240300421B6G6TLLM3EOC5",
			"011500015806802221UTPS42CPFR699240300410HE67317220630",
			"011500015806802221EVXNWFPHVYAXM240300410HE67317220630",
			"011500015806802221E4VJVWUE59325240300410HE67317220630",
			"011500015806802221D7MNC5F3CGY68240300410HE67317220630",
			"011500015806802221Y97FG6MDRZ2BE240300410HE67317220630",
			"011500015806802221D7JMZME3UTHTT240300410HE67317220630",
			"011500015806802221UTPS42CPFR699240300410HE67317220630",
			"011500015806802221EVXNWFPHVYAXM240300410HE67317220630",
			"011500015806802221E4VJVWUE59325240300410HE67317220630",
			"011500015806802221D7MNC5F3CGY68240300410HE67317220630",
			"011500015806802221Y97FG6MDRZ2BE240300410HE67317220630",
			"011500015806802221D7JMZME3UTHTT240300410HE67317220630",
			//"01046070551312751724090110265509192100533K4CUEUTX",
			//"0104607055131275172409011026550919210052Z7B2BYSVK",
			//"01046070551312751724090110265509192100533JKUR9FA8",
			//"01046070551312751724090110265509192100533HYNJ8YIE",
			//"0104607055131275172409011026550919210052Z3KX1VFBY",
			"01189011482005211724040010A90004621H8KV450B60G",
			"01189011482005211724040010A90004621TQXJ2L4400U",
			"01189011482005211724040010A90004621KEGAPQ73B0Q",
			"01189011482005211724040010A900046213ATNSG300UT",
			"01189011482005211724040010A900046212U07SBCE80L",
			"01189011482005211724040010A90004621ILQOX018FUY",
			"01189011482005211724040010A90004621OB9HVL23F02",
			"01189011482005211724040010A900046219KSCVVD0806",
			"01189011482005211724040010A90004621AAI7CGF510E",
			"01189011482005211724040010A9000462174NJ3GB520Z",
			"01189011482005211724040010V90043621B605RCPCFKOV",
			"01189011482005211724040010V90043621B6J85BNHD956",
			"01189011482005211724040010V90043621B6PBN26CV5MR",
			"01189011482005211724040010V90043621B64N7SJ17TMV",
			"01189011482005211724040010V90043621B6XUNFLSS6MR",
			"01189011482005211724040010V90043621B67AYTHIYVGX",
			"01189011482005211724040010V90043621B6TRIY6D3I73",
			"01189011482005211724040010V90043621B63DAJFKF22C",
			"01189011482005211724040010V90043621B6C08A0N0ZNX",
			"01189011482005211724040010V90043621B66M49BP1JL5",
			"01189011483087221721040010V90044421B60AED05ECJ3",
			"01189011483087221721040010V90044421B6TWVRRZ5MLT",
			"01189011483087221721040010V90044421B6ABLC36PO48",
			"01189011483087221721040010V90044421B6XRXZR1FGH5",
			"01189011483087221721040010V90044421B6KSY9LE6TVZ",
			"01189011483087221721040010V90044421B6T58WB4R4DN",
			"01189011483087221721040010V90044421B6ISH3K58T1J",
			"01189011483087221721040010V90044421B68COV45EI6Y",
			"01189011483087221721040010V90044421B6OLU4EAFGH5",
			"01189011483087221721040010V90044421B6R3HIAW6PAA",
			"01189011480056071722010010B90042221B2M7P5ONRRTG",
			"01189011480056071722010010B90042221B2H1FSJ5SDFF",
			"01189011480056071722010010B90042221B2V22DM9DB4J",
			"00000046209443j+Q'?P5ACZAC8bG",
			"00000046209443x-8xfgOACZAYGfv",
			"04606203098187o&zWeIyABr8l/nT",
			"01189011482003231723110010V80123721B6VMX0KMEXY2",
			//"0104600266011725212095134931209513424010067290",
			"010290000149084921+?L:qdrZkIRcE91803992p7SGHmSO2yUGetnld47/qw3DLcULMO9RF61DD2zImlmWW0HKVHJ0FV13TsJwcyj/qzi7qowKFTcOzXMlIBK/Og==",
			"01189011483087221721070010V90064521B6YBCY83BAJG",
			//"01046070551312751724090110275009192100LPENT9H6BF9",
			//"01046070551312751724090110275009192100LPENR5ESPIJ",
			"01046100177003032100UYTZ174KDE891ee0592IO1Lf8MQpT+BsE5UeVgqXoyBArniWaGRTZbIBJ3zsH0=",
			"010460507700507921110009CAJR3C391ee0592goB4PLPPM1e1HkwFtCH6HRqCrvi+0d1747tZW/1wpiY=",
			"010460507700507921110009CAP0KXG91ee0592NXSXy+Hx9p/OMdZhm8efYJz9RenpHlwssU7H5a83j3g=",
			"01046050770050792111000A7YRJF8T91ee0592NlKfI36V5iG4uyol6iIeYad1Mi/hsfmrVtTHbWuz/ZQ=",
			"01046050770050792111000A7X4X9K791ee0592Bg4TZcAp4vxBOokDkw7aFmyiWWtZhOzBsJtuVBnE9+w=",
			"01189011480055911722090010B90206621B2K662Y1M301",
			"01189011480055911722090010B90206621B20HYL4LKPZE",
			"0104600266012432211sq:SKS800510900093Y5Ec24010093277",
			"010460026601231921'21M/N'800513200093cznu24010091473",
			"0104603988001678211J5HJFJJ071HL91ee0592wc1MT5QBSEz4DtnEnV3SqZ+xfMlfAsoiTGLtWYLPDLU=",
			"010357466061912621102817157979991ee0592wq0XNWyjzkJBHOG3GUgeiVpWzVcmPXV/xGV2IznmAyg=",
			"010290000030666021.ooncrmEhk5M*",
			"010290000030666021ImTRaAvmK=Qb9",
			"010290000030666021SPMZ(oo\"dJnom",
			"010290000030666021<a5Dyv*OuRw&o",
			"010290000030666021-i?pQSESXR<BW",
			"010290000030666021lpK!rLT?h6*Sc",
			"010290000030666021OTMm52DGjLZiy",
			"010290000028592721Weic23pmUqUHR91003A92J7oBDFVCI/XncJQM7kXTaXdKU5UajHXZmnXd0J5dJrPa7g4dXPAI+svZFxH+S87tJF8+KOBJxq8C2wHuTrdniA==",
			"01046511149003882100000038005095000",
			"01046511149003882100000048005095000",
			"01046511149003882100000058005095000",
			"01046511149003882100000078005095000",
			"01046511149003882100000088005095000",
			"010465111490038821000000E8005095000",
			"010465111490038821000000F8005095000",
			"010465111490038821000000G8005095000",
			"010465111490038821000000H8005095000",
			"010465111490038821000000I8005095000",
			"010465111490038821000000P8005095000",
			"010465111490038821000000Q8005095000",
			"010465111490038821000000R8005095000",
			"010465111490038821000000S8005095000",
			"010465111490038821000000T8005095000",
			"010465111490038821000000Z8005095000",
			"010465111490038821000000_8005095000",
			"010465111490038821000000a8005095000",
			"010465111490038821000000b8005095000",
			"010465111490038821000000c8005095000",
			"010465111490038821000000i8005095000",
			"010465111490038821000000j8005095000",
			"010465111490038821000000k8005095000",
			"010465111490038821000000l8005095000",
			"010465111490038821000000m8005095000",
			//"012460081800728621069162482170081024014501203",
			//"012460081800728621071357832171185724014501203",
			//"011462006570025221069036092170327824014626540",
			//"011462006570025221069036092170513124014626540",
			//"020465011872021313200329211000000211",
			//"020465011872021313200416211000000233",
			"010466007731023311200622214ME8604H",
			"01046600773102022100006SC",
			"01046600773101962100003Ez",
			"01046600773101412100004f4",
			"010460620309819421pe*Z8Uo80050990009358bw240FA074825.02",
			"010460620309819421<av6j=f8005099000937zpN240FA074825.02",
			"010460620309819421pe*Z8Uo80050990009358bw240FA074825.02",
			"0104606203098194217mD5jRy800509900093FFcT240FA074825.02",
			"010460620309351921nlEgS+o800509900093Yx8P240FA074826.02",
			"010460620309351921Gxb/L9780050990009384+9240FA074826.02",
			"010460620309351921WSnrBzg800509900093LwPq240FA074826.02",
			"010460620309963421\"-i;q:U800511700093KAdg240FA068872.56",
			//"00000046224811%&bF;tSAE3A",
			//"00000046224811%>DToZnAE3A",
			//"00000046224811*dgX<H3AE3A",
			//"00000046224811*pzS3*TAE3A",
			//"00000046224811;eVa=UNAE3A",
			//"00000046224811?OkCr4+AE3A",
			//"00000046224811?wj-a'\"AE3A",
			//"00000046224811+O1_LOjAE3A",
			//"00000046224811<H!!'2MAE3A",
			//"00000046224811>_Z&O.UAE3A",
			//"000000462248110zzd%fpAE3A",
			//"000000462248112n4S0%;AE3A",
			//"0000004622481131G6T2JAE3A",
			//"0000004622481162Tsd\"9AE3A",
			//"00000046224811bYE,ab4AE3A",
			//"00000046224811DRfddmHAE3A",
			//"00000046224811Ec!t=icAE3A",
			//"00000046224811eFdDsnHAE3A",
			//"00000046224811EoSft*fAE3A",
			//"00000046224811f.;5TBRAE3A",
			//"00000046224811f.'kXAtAE3A",
			//"00000046224811gbOdtt/AE3A",
			//"00000046224811hBWDP-dAE3A",
			//"00000046224811IB'Iji_AE3A",
			//"00000046224811j>w%SYVAE3A",
			//"00000046224811J5/+qZ-AE3A",
			//"00000046224811lw1>i8*AE3A",
			//"00000046224811MrRyVMMAE3A",
			//"00000046224811mw_eeR1AE3A",
			//"00000046224811n7'=t4rAE3A",
			//"00000046224811nnnCTapAE3A",
			//"00000046224811NWL'*KkAE3A",
			//"00000046224811OM0C:k,AE3A",
			//"00000046224811S;-V_u;AE3A",
			//"00000046224811tvHa:!UAE3A",
			//"00000046224811-tx.B3aAE3A",
			//"00000046224811VItU-!MAE3A",
			//"00000046224811VWyRmZHAE3A",
			//"00000046224811wbnLjJpAE3A",
			//"00000046224811X.<k*ixAE3A",
			//"00000046224811Y-a!HdSAE3A",
			//"00000046224811YI=9GL>AE3A",
			//"00000046224811YS5eAf9AE3A",
			//"00000046224811zNm5IRFAE3A",
			//"00000046224811Zu0VH;;AE3A",
			"010460166900805321lAXTmWCDzL0rE91EE0692TBlZu2ignWhszVbnHjzHMOGVlbl51GQvePCnY1ol+e8+",
			"010464004242316821;;fkMTGHyS'hl91006A92XUQ6bi/s210HuozYeuOyw92jRGF4g04+mr03n7RsuKflFDLOUjBV/3hoobhEggD2NDFPLUeDeUFjAxZ0xuPR0w==",
			"010290000131344521JXbVb'=BFCzyN91003A92QcaTHSIi6a5eknWhFHy4UA7WDNOAuGcgR0gSh/2lsYfzh9qaJDP1DMeX7kv+XoJLAn19PwYC9rDgzM9ocoESmQ==",
			"010290000131344521)IlumVoUfO0Ue91003A92oOK8CDvdlYCz5Vat+eoZOF1dQ+U9mUxJ4JNFIZ4gsr6Rt+0Cik4WwN05Gd63cOLinMo/ckAf+S+0a8RPd9eLSw++",
			"010290000131344521hHj3R!lxAy'ts91003A92SV5nm7Qld7bs5XkvJBzFFlT+EPtqkepEEJoKOVhj8E9to1qKsov5IcAEpC8ZJ1SuH1EXcVAkCjSCFlLnNYcSuA==",
			"010290000134000721MmODJS7MSkQ>M91803992Cq7FKb6pa27EWbnvQtHvXjx7ZX9p4Z09q8Fi0k71/KIumSlY/Sg7xI3NaRFGjtgUHJDuLq9V6m1LJ/Nk4Vur3A==",
			"010290000131333921mftJfLAwRoas891803992mJFYbqbG1m3SBl/iVHcK0iIHS7j+KHp5tbk6gM2hirU9zEcj2u5mfaocH288TApsS6lTv/XR3GhYBiRBGq3W/A==",
			"0104601669010315217m2CUceKxx41w91EE0692rUIIFK4Vds1rR0GLNvvo43aAM7CwwSh1twdFV8Y0heQ=",
			"00000046209443X?Q<P;?ACfUC2wb",
			"010460256503127521000000037382891EE0692aRagToIfuI20MqJfXZbJxPdksjUMjKA3h2cMAyzc2yI=",
			"011890114820052121B66FKCSCMIGR491EE0692J5f6Vb7CxqoSS3nUal/avfXcpbFAoNhFpOzgX6hyz8A=",
			"011890114800602421B22F1CIQSFI3W91EE06925UYGEvRAS+67WK1jxsQXjleTUCnxM0We2DFQlwJIJxU=",
			"011890114800751921B2FOD5KTE3BPJ91EE0692kDRq0Umt6TFAT8CUDS7SzGRHGBpp9xt0EaoKUnZY7es=",
			"010290000174369321d4M5C&??ZlBVB91802992kgqZGnJOJWKT7CVOFKxzsVVs1ErReaZjVHsRGyzMh71wIa62LqecJ6eBL6O/BtlfN6L8LPc0ebY0Tnf63cWSGg==",
			"010460166900897821KHr5Sz4hFAi0e91EE069220BoM3i3W6xiQUJYbicpX5HKyWb6UaPzGqLRqngX2Tw=",
			"010460318200251821010000785238291EE0692uy1H5DQr89ewuV4W/ssuZKTxmcX7r0A8/1KZU3tMLSY=",
			"010460282402287721CAZ2P3141QRQ191EE0692LH+v84n18+x2WShFKRFMFALhDrcXD+ArPS2qibDc7e8=",
			"00000046209474d,/MURSAC!oXqim",
			"00000046235039gbEJx3\"ACI8pPN4",
			"00000046235039JHL/WsNACI85XES",
			"00000046235039VqmBj+FACI8+wSe",
			"000000462350390CcBjudACI87LYe",
			"0104603460000809215WWSPE93eMLy",
			"0104603460211915215hdM!Y930A+p",
			"0104603460211915215u'8/>93AIJs",
			"00000046209443mCe:DhcAC;AEEdM",
			"0104606779682407215rYurY937bzX", // йогурт
			"01046067796824072157m&ZL93MsfH", // йогурт
			"0104860102580369215VSQ93&b:dYA593HLkd", // вода
			//"0104860102580369215WsZaD(p5nR+s)93xQ6Y", // вода
		};
		boolean debug_mark = false;
		for(int i = 0; i < test_code_list.length; i++) {
			final String code = test_code_list[i];
			GTIN result = GTIN.ParseChZnCode(code, 0);
			boolean is_valid_grade01 = (result != null && (result.GetChZnParseResult() > 0 && result.GetChZnParseResult() != 100000));
			if(!is_valid_grade01) {
				debug_mark = true;
			}
			Assert.assertTrue(is_valid_grade01);
			if(is_valid_grade01) {
				String gtin14 = result.GetToken(GTIN.fldGTIN14);
				Assert.assertTrue(gtin14 != null && gtin14.length() == 14);
				//
				String serial = result.GetToken(GTIN.fldSerial);
				Assert.assertTrue(serial != null && serial.length() > 6);
			}
		}
	}
	@Test public void Document_ActionFlags()
	{
		//
		// Тест преобразования набора флагов Document.actionXXX в строку и обратно
		//
		final int _all_flags[] = { Document.actionDocStatus, Document.actionDocAcceptance,
				Document.actionDocAcceptanceMarks, Document.actionDocSettingMarks, Document.actionDocInventory, Document.actionGoodsItemCorrection };
		{
			// Пустой набор флагов
			String afs = Document.IncomingListActionsToString(0);
			Assert.assertTrue(SLib.GetLen(afs) == 0);
			int af = Document.IncomingListActionsFromString("");
			Assert.assertTrue(af == 0);
		}
		{
			// Одиночные флаги
			{
				String afs = Document.IncomingListActionsToString(0);
				Assert.assertTrue(SLib.GetLen(afs) == 0);
				int af = Document.IncomingListActionsFromString("");
				Assert.assertTrue(af == 0);
			}
			for(int i = 0; i < _all_flags.length; i++) {
				String afs = Document.IncomingListActionsToString(_all_flags[i]);
				Assert.assertTrue(SLib.GetLen(afs) > 0);
				int af = Document.IncomingListActionsFromString(afs);
				Assert.assertTrue(af == _all_flags[i]);
			}
		}
		{
			// Комбинация флагов по 2.
			for(int i = 0; i < _all_flags.length; i++) {
				final int af1 = _all_flags[i];
				for(int j = 0; j < _all_flags.length; j++) {
					final int af2 = _all_flags[j];
					//if(af2 != af1) {
					String afs = Document.IncomingListActionsToString(af1 | af2);
					Assert.assertTrue(SLib.GetLen(afs) > 0);
					int af = Document.IncomingListActionsFromString(afs);
					Assert.assertTrue(af == (af1 | af2));
					//}
				}
			}
		}
		{
			// Комбинация флагов по 3.
			for(int i = 0; i < _all_flags.length; i++) {
				final int af1 = _all_flags[i];
				for(int j = 0; j < _all_flags.length; j++) {
					final int af2 = _all_flags[j];
					for(int k = 0; k < _all_flags.length; k++) {
						final int af3 = _all_flags[j];
						//if(af2 != af1) {
						String afs = Document.IncomingListActionsToString(af1 | af2 | af3);
						Assert.assertTrue(SLib.GetLen(afs) > 0);
						int af = Document.IncomingListActionsFromString(afs);
						Assert.assertTrue(af == (af1 | af2 | af3));
						//}
					}
				}
			}
		}
	}
	@Test public void Document()
	{
		{
			// Empty doc
			Document doc = new Document();
			Assert.assertTrue(doc != null);
			Document doc_alias = doc;
			Assert.assertTrue(doc_alias == doc);
			Assert.assertTrue(doc_alias.IsEq(doc));
			Document doc_copy = Document.Copy(doc);
			Assert.assertTrue(doc_copy != doc);
			Assert.assertTrue(doc_copy.IsEq(doc));
			//
			/*
			JSONObject js_obj = new JSONObject();
			Assert.assertTrue(js_obj != null);

			JSONObject js_doc = doc.ToJsonObj();
			Assert.assertTrue(js_doc != null);
			Document doc_from_json = new Document();
			Assert.assertTrue(doc_from_json.FromJsonObj(js_doc));
			Assert.assertTrue(doc_from_json.IsEq(doc));
			 */
		}
		{
			Document doc = new Document();
			doc.H = new Document.Head();
			doc.H.ID = 101010;
			doc.H.Uuid = UUID.randomUUID();
			doc.H.OrgCmdUuid = null;
			doc.H.BaseCurrencySymb = "USD";
			doc.H.Flags = 0;
			doc.H.Memo = "Произвольное примечание на русском языке";
			{
				BigInteger bn = SLib.GenerateRandomBigNumber(20*8);
				doc.H.SvcIdent = bn.toByteArray();
			}
			doc.H.Code = "QN00741";
			doc.H.ClientID = 1000;
			doc.H.DlvrLocID = 0;
			doc.H.PosNodeID = 0;
			doc.H.AgentID = 2000;
			doc.H.InterchangeOpID = SLib.PPEDIOP_ORDER;
			doc.H.SvcOpID = 1001;
			doc.H.CreationTime = new SLib.LDATETIME(System.currentTimeMillis());
			doc.H.Time = null;
			doc.H.DueTime = SLib.plusdatetimesec(doc.H.CreationTime, 3600*48);
			doc.H.Amount = 2000;
			doc.H.OrgCmdUuid = UUID.randomUUID();
			{
				doc.TiList = new ArrayList<Document.TransferItem>();
				{
					Document.TransferItem ti = new Document.TransferItem();
					ti.RowIdx = 1;
					ti.GoodsID = 120011;
					ti.UnitID = 1002;
					ti.Flags = 0;
					ti.XcL = new ArrayList<Document.LotExtCode>();
					{
						Document.LotExtCode lec = new Document.LotExtCode("40404040");
						ti.XcL.add(lec);
					}
					{
						Document.LotExtCode lec = null;
						ti.XcL.add(lec);
					}
					{
						Document.LotExtCode lec = new Document.LotExtCode("50505050");
						ti.XcL.add(lec);
					}
					doc.TiList.add(ti);
				}
				{
					Document.TransferItem ti = new Document.TransferItem();
					ti.RowIdx = 2;
					ti.GoodsID = 120021;
					ti.UnitID = 1002;
					ti.Flags = 0;
					doc.TiList.add(ti);
				}
			}
			//
			Document doc_alias = doc;
			Assert.assertTrue(doc_alias == doc);
			Assert.assertTrue(doc_alias.IsEq(doc));
			//
			Document doc_copy = Document.Copy(doc);
			Assert.assertTrue(doc_copy != doc);
			boolean is_eq = doc_copy.IsEq(doc);
			Assert.assertTrue(is_eq);
		}
	}
}