/** @file
 * @brief Xapian::BoolWeight class - boolean weighting
 */
// Copyright (C) 2009,2011,2016 Olly Betts
// @license GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

namespace Xapian {
BoolWeight * BoolWeight::clone() const
{
	return new BoolWeight;
}

void BoolWeight::init(double)
{
	// Nothing to do here.
}

string BoolWeight::name() const { return "Xapian::BoolWeight"; }
string BoolWeight::short_name() const { return "bool"; }
string BoolWeight::serialise() const { return string(); } // No parameters to serialise.

BoolWeight * BoolWeight::unserialise(const string & s) const
{
	if(UNLIKELY(!s.empty()))
		throw Xapian::SerialisationError("Extra data in BoolWeight::unserialise()");
	return new BoolWeight;
}

double BoolWeight::get_sumpart(Xapian::termcount, Xapian::termcount, Xapian::termcount, Xapian::termcount) const
{
	return 0;
}

double BoolWeight::get_maxpart() const
{
	return 0;
}

double BoolWeight::get_sumextra(Xapian::termcount, Xapian::termcount, Xapian::termcount) const
{
	return 0;
}

double BoolWeight::get_maxextra() const
{
	return 0;
}

BoolWeight * BoolWeight::create_from_parameters(const char * p) const
{
	if(*p != '\0')
		throw InvalidArgumentError("No parameters are required for BoolWeight");
	return new Xapian::BoolWeight();
}
}
