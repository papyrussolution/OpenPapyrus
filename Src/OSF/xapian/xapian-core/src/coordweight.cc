/** @file
 * @brief Xapian::CoordWeight class - coordinate matching
 */
// Copyright (C) 2004,2009,2011,2016 Olly Betts
// @license GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

namespace Xapian {
	CoordWeight * CoordWeight::clone() const { return new CoordWeight; }
	void CoordWeight::init(double factor_) { factor = factor_; }
	string CoordWeight::name() const { return "Xapian::CoordWeight"; }
	string CoordWeight::short_name() const { return "coord"; }
	string CoordWeight::serialise() const { return string(); } // No parameters to serialise.

	CoordWeight * CoordWeight::unserialise(const string & s) const
	{
		if(UNLIKELY(!s.empty()))
			throw Xapian::SerialisationError("Extra data in CoordWeight::unserialise()");
		return new CoordWeight;
	}

	double CoordWeight::get_sumpart(Xapian::termcount, Xapian::termcount, Xapian::termcount, Xapian::termcount) const { return factor; }
	double CoordWeight::get_maxpart() const { return factor; }
	double CoordWeight::get_sumextra(Xapian::termcount, Xapian::termcount, Xapian::termcount) const { return 0; }
	double CoordWeight::get_maxextra() const { return 0; }

	CoordWeight * CoordWeight::create_from_parameters(const char * p) const
	{
		if(*p != '\0')
			throw InvalidArgumentError("No parameters are required for CoordWeight");
		return new Xapian::CoordWeight();
	}
}
