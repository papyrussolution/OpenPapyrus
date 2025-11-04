// TEST-MYSQL.CPP
// Copytight (c) A.Sobolev 2025
// @codepage UTF-8
// Тестирование интерфейса с MySQL/MariaSQL
//
#include <pp.h>
#pragma hdrstop

SLTEST_R(MySQL) // @v12.4.7
{
	DbLoginBlock dblb;
	SMySqlDbProvider dbp;
	// @doto Setup the dpb
	THROW(SLCHECK_NZ(dbp.DbLogin(&dblb, DbProvider::openfMainThread)));
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}
