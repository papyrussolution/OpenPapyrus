/** @file
 *  @brief Diversification API
 */
// Copyright (C) 2018 Uppinder Chugh
// @license GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop
//#include "diversifyinternal.h"
// 
// Internal class for Diversify
// 
class Xapian::Diversify::Internal : public Xapian::Internal::intrusive_base {
	Internal(const Internal&) = delete; /// Copies are not allowed
	void operator = (const Internal&) = delete; /// Assignment is not allowed
	Xapian::doccount k; /// Top-k documents of given mset are diversified
	Xapian::doccount r; /// Number of relevant documents from each cluster used for building topC
	double lambda, b, sigma_sqr; /// MPT parameters
	std::unordered_map<Xapian::docid, Xapian::Point> points; /// Store each document from given mset as a point
	std::unordered_map<Xapian::docid, double> scores; /// Store the relevance score of each document
	std::map<std::pair<Xapian::docid, Xapian::docid>, double> pairwise_sim; /// Store pairwise cosine similarities of documents of given mset
	std::vector <Xapian::docid> main_dmset; /// Store docids of top k diversified documents
public:
	/// Constructor for initialising diversification parameters
	explicit Internal(Xapian::doccount k_, Xapian::doccount r_, double lambda_, double b_, double sigma_sqr_) : 
		k(k_), r(r_), lambda(lambda_), b(b_), sigma_sqr(sigma_sqr_) 
	{
	}
	/** Initialise diversified document set
	 *
	 *  Convert top-k documents of mset into vector of Points, which
	 *  represents the initial diversified document set.
	 *
	 *  @param source	MSet object containing the documents of which
	 *			top-k are to be diversified
	 */
	void initialise_points(const Xapian::MSet& source);
	/** Return a key for a pair of documents
	 *
	 *  Returns a key as a pair of given documents ids
	 *
	 *  @param doc_id	Document id of the document
	 *  @param centroid_idx	Index of cluster to which the given centroid
	 *                      belongs to in the cluster set
	 */
	std::pair<Xapian::docid, uint>get_key(Xapian::docid doc_id, uint centroid_idx);
	/** Compute pairwise similarities
	 *
	 *  Used for pre-computing pairwise cosine similarities of documents
	 *  of given mset, which is used to speed up evaluate_dmset
	 *
	 *  @param cset	Cluster of given relevant documents
	 */
	void compute_similarities(const Xapian::ClusterSet& cset);
	/** Return difference of 'points' and current dmset
	 *
	 *  Return the difference of 'points' and the current diversified
	 *  document match set
	 *
	 *  @param dmset	Document set representing a diversified document set
	 */
	std::vector <Xapian::docid>compute_diff_dmset(const std::vector <Xapian::docid>& dmset);
	/** Evaluate a diversified mset
	 *
	 *  Evaluate a diversified mset using MPT algorithm
	 *
	 *  @param dmset	Set of points representing candidate diversifed
	 *			set of documents
	 *  @param cset	Set of clusters of given mset
	 */
	double evaluate_dmset(const std::vector <Xapian::docid>& dmset, const Xapian::ClusterSet& cset);
	/// Return diversified document set from given mset
	Xapian::DocumentSet get_dmset(const MSet& mset);
};
//

using namespace Xapian;
using namespace std;

Diversify::Diversify(const Diversify&) = default;
Diversify& Diversify::operator = (const Diversify&) = default;
Diversify::Diversify(Diversify&&) = default;
Diversify& Diversify::operator = (Diversify&&) = default;

Diversify::Diversify(Xapian::doccount k_, Xapian::doccount r_, double lambda_, double b_, double sigma_sqr_) : 
	internal(new Xapian::Diversify::Internal(k_, r_, lambda_, b_, sigma_sqr_))
{
	LOGCALL_CTOR(API, "Diversify", k_ | r_ | lambda_ | b_ | sigma_sqr_);
	if(r_ == 0)
		throw InvalidArgumentError("Value of r should be greater than zero");
}

Diversify::~Diversify()
{
	LOGCALL_DTOR(API, "Diversify");
}

string Diversify::get_description() const
{
	return "Diversify()";
}

DocumentSet Diversify::get_dmset(const MSet& mset)
{
	LOGCALL(API, MSet, "Diversify::get_dmset", mset);
	return internal->get_dmset(mset);
}

void Diversify::Internal::initialise_points(const MSet &source)
{
	uint count = 0;
	TermListGroup tlg(source);
	for(MSetIterator it = source.begin(); it != source.end(); ++it) {
		points.emplace(*it, Xapian::Point(tlg, it.get_document()));
		scores[*it] = it.get_weight();
		// Initial top-k diversified documents
		if(count < k) {
			main_dmset.push_back(*it);
			++count;
		}
	}
}

pair<Xapian::docid, uint> Diversify::Internal::get_key(Xapian::docid doc_id, uint centroid_id)
{
	return make_pair(doc_id, centroid_id);
}

void Diversify::Internal::compute_similarities(const Xapian::ClusterSet& cset)
{
	Xapian::CosineDistance d;
	for(auto p : points) {
		Xapian::docid point_id = p.first;
		Xapian::Point point = p.second;
		for(uint c = 0; c < cset.size(); ++c) {
			double dist = d.similarity(point, cset[c].get_centroid());
			auto key = get_key(point_id, c);
			pairwise_sim[key] = dist;
		}
	}
}

vector <Xapian::docid> Diversify::Internal::compute_diff_dmset(const vector <Xapian::docid>&dmset)
{
	vector <Xapian::docid> diff_dmset;
	for(auto point : points) {
		Xapian::docid point_id = point.first;
		bool found_point = false;
		for(auto doc_id : dmset) {
			if(point_id == doc_id) {
				found_point = true;
				break;
			}
		}
		if(!found_point) {
			diff_dmset.push_back(point_id);
		}
	}
	return diff_dmset;
}

double Diversify::Internal::evaluate_dmset(const vector <Xapian::docid>&dmset, const Xapian::ClusterSet& cset)
{
	double score_1 = 0, score_2 = 0;
	for(auto doc_id : dmset)
		score_1 += scores[doc_id];
	for(uint c = 0; c < cset.size(); ++c) {
		double min_dist = numeric_limits<double>::max();
		uint pos = 1;
		for(auto doc_id : dmset) {
			auto key = get_key(doc_id, c);
			double sim = pairwise_sim[key];
			double weight = 2 * b * sigma_sqr / log(1 + pos) * (1 - sim);
			min_dist = min(min_dist, weight);
			++pos;
		}
		score_2 += min_dist;
	}
	return -lambda * score_1 + (1 - lambda) * score_2;
}

DocumentSet Diversify::Internal::get_dmset(const MSet &mset)
{
	// Return original mset if no need to diversify
	if(k == 0 || mset.size() <= 2) {
		DocumentSet dmset;
		for(MSetIterator it = mset.begin(); it != mset.end(); ++it)
			dmset.add_document(it.get_document());
		return dmset;
	}
	uint k_ = k;
	SETMIN(k_, mset.size());
	initialise_points(mset);
	// Cluster the given mset into k clusters
	Xapian::LCDClusterer lc(k_);
	Xapian::ClusterSet cset = lc.cluster(mset);
	compute_similarities(cset);

	// topC contains union of top-r relevant documents of each cluster
	vector <Xapian::docid> topc;

	// Build topC
	for(uint c = 0; c < cset.size(); ++c) {
		auto documents = cset[c].get_documents();
		for(uint d = 0; d < r && d < documents.size(); ++d) {
			auto doc_id = documents[d].get_docid();
			topc.push_back(doc_id);
		}
	}
	vector <Xapian::docid> curr_dmset = main_dmset;
	while(true) {
		bool found_better_dmset = false;
		for(uint i = 0; i < main_dmset.size(); ++i) {
			auto curr_doc = main_dmset[i];
			double best_score = evaluate_dmset(curr_dmset, cset);
			bool found_better_doc = false;
			for(uint j = 0; j < topc.size(); ++j) {
				// Continue if candidate document from topC already
				// exists in curr_dmset
				auto candidate_doc = find(curr_dmset.begin(), curr_dmset.end(), topc[j]);
				if(candidate_doc != curr_dmset.end()) {
					continue;
				}
				auto temp_doc = curr_dmset[i];
				curr_dmset[i] = topc[j];
				double score = evaluate_dmset(curr_dmset, cset);
				if(score < best_score) {
					curr_doc = curr_dmset[i];
					best_score = score;
					found_better_doc = true;
				}
				curr_dmset[i] = temp_doc;
			}
			if(found_better_doc) {
				curr_dmset[i] = curr_doc;
				found_better_dmset = true;
			}
		}
		// Terminate algorithm when there's no change in current
		// document matchset
		if(!found_better_dmset)
			break;
		main_dmset = curr_dmset;
	}
	// Merge main_dmset and diff_dmset into final dmset
	DocumentSet dmset;
	for(auto doc_id : main_dmset)
		dmset.add_document(points.at(doc_id).get_document());
	vector <Xapian::docid> diff_dmset = compute_diff_dmset(main_dmset);
	for(auto doc_id : diff_dmset)
		dmset.add_document(points.at(doc_id).get_document());
	return dmset;
}
