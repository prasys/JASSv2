/*
	EVALUATE_SELLING_POWER.H
	------------------------
	Copyright (c) 2019 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
/*!
	@file
	@brief Compute the sellng power of the results list
	@author Andrew Trotman
	@copyright 2019 Andrew Trotman
*/
#pragma once

#include "evaluate.h"

namespace JASS
	{
	/*
		CLASS EVALUATE_SELLING_POWER
		----------------------------
	*/
	/*!
		@brief Compute the selling power for the results list.
		@details The selling power of the top k items is computed by generating the ideal gain vector (price low to high)
		for the top k items (cost_K) then computing cost_K/actual_k for each k in the results list.  That is, 0 for non-relevance
		and for relevance it is the rtatio of idealprice to chargedprice.  This is then summed and divided by k.
	*/
	class evaluate_selling_power : evaluate
		{
		private:
			evaluate &prices;						///< Each item has a price regardless of the query being processed
			evaluate &assessments;				///< The assessments, which items are relevant to which queries (and at what price)

		public:
			/*
				EVALUATE_SELLING_POWER::EVALUATE_SELLING_POWER()
				------------------------------------------------
			*/
			/*!
				@brief Constructor.
				@details  As all possible prices are valid prices (0 == "free", -1 == "I'll pay for you to take it away), the
				assessments are split into two seperate parts. Ther prices of the items and the relevance of the items.  Each
				of these two are stored in trec_eval format:

			 	1 0 AP880212-0161 1

			 	where the first column is the query id, the second is ignored, the third is the document ID, and the fourth is the
			 	relevance.  The prices use a query id of "PRICE" and the relevance coulmn is the price of the item.  The assessments
			 	are the usual trec_eval format where a relevance of 1 means releance, but a relefvance of 0 is not-relevant.

				@param prices [in] An assessments object which holds the price of each item
				@param assessments [in] A pre-constructed assessments object.
			*/
			evaluate_selling_power(evaluate &prices, evaluate &assessments) :
				prices(prices),
				assessments(assessments)
				{
				}

			/*
				EVALUATE_SELLING_POWER::COMPUTE()
				---------------------------------
			*/
			/*!
				@brief Compute the selling power metric over the results list.

				@details The selling power of the top k items is computed by generating the ideal gain vector (price low to high)
				for the top k items (cost_K) then computing cost_K/actual_k for each k in the results list.  That is, 0 for non-relevance
				and for relevance it is the rtatio of idealprice to chargedprice.  This is then summed and divided by k.  We assume
				a shop-front (or set-based) model in which we only increase K if we find a relevant document at k.  This guarantees
				that the metric cannot fall outside the range [0..1].

				Although prices are assumed to be in dollars and (2 is $2.00, 2.2 = $2.20), the implementation is unit agnostic
				and therefore prices might be in cents (or pence, etc.) or even units other than financial (such as time).

				@pram query_id [in] The ID of the query being evaluated.
				@param results_list [in] The results list to measure.
				@param depth [in] How far down the results list to look.
				@return The selling power of this results list for this query.
			*/
			virtual double compute(const std::string &query_id, const std::vector<std::string> &results_list, size_t depth = 1000);

			/*
				EVALUATE_SELLING_POWER::UNITTEST()
				----------------------------------
			*/
			/*!
				@brief Unit test this class
			*/
			static void unittest(void);

		};
	}