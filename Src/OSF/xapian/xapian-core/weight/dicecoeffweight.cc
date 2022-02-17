/** @file
 * @brief Xapian::DiceCoeffWeight class
 */
// Copyright (C) 2018 Guruprasad Hegde
// @license GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

namespace Xapian {
DiceCoeffWeight * DiceCoeffWeight::clone() const
{
	return new DiceCoeffWeight();
}

void DiceCoeffWeight::init(double factor_)
{
	if(factor_ == 0.0) {
		// This object is for the term-independent contribution, and that's
		// always zero for this scheme.
		return;
	}

	factor = get_wqf() * factor_;

	// Upper bound computation:
	// dice_coeff(q, d) = 2.0 * (q ∩ d) / (|q| + |d|)
	// To maximize the result minimize the denominator, hence
	// |q|= 1, |d| = lower bound on unique term.
	// FIXME lower bound on unique term is not tracked in database,
	// hence keeping its value as 1, this will not give a tight bound.
	// Plan to fix in future.
	double uniqtermlen_lb = 1;
	upper_bound = factor * (2.0 / (get_query_length() + uniqtermlen_lb));
}

string DiceCoeffWeight::name() const
{
	return "Xapian::DiceCoeffWeight";
}

string DiceCoeffWeight::short_name() const
{
	return "dicecoeff";
}

string DiceCoeffWeight::serialise() const
{
	return string();
}

DiceCoeffWeight * DiceCoeffWeight::unserialise(const string & s) const
{
	if(rare(!s.empty())) {
		throw Xapian::SerialisationError("Extra data in DiceCoeffWeight::unserialise()");
	}
	return new DiceCoeffWeight;
}

double DiceCoeffWeight::get_sumpart(Xapian::termcount wdf, Xapian::termcount, Xapian::termcount uniqterms, Xapian::termcount) const
{
	if(wdf == 0) return 0.0;
	return factor * 2.0 / (get_query_length() + uniqterms);
}

double DiceCoeffWeight::get_maxpart() const
{
	return upper_bound;
}

double DiceCoeffWeight::get_sumextra(Xapian::termcount, Xapian::termcount, Xapian::termcount) const
{
	return 0;
}

double DiceCoeffWeight::get_maxextra() const
{
	return 0;
}

DiceCoeffWeight * DiceCoeffWeight::create_from_parameters(const char * p) const
{
	if(*p != '\0') {
		throw InvalidArgumentError("No parameters are required for DiceCoeffWeight");
	}
	return new Xapian::DiceCoeffWeight;
}
}
