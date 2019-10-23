/*
	EVALUATE_CHEAPEST_PRECISION.CPP
	-------------------------------
	Copyright (c) 2019 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#include <cmath>

#include "maths.h"
#include "asserts.h"
#include "unittest_data.h"
#include "evaluate_cheapest_precision.h"

namespace JASS
	{
	/*
		EVALUATE_CHEAPEST_PRECISION::COMPUTE()
		--------------------------------------
	*/
	double evaluate_cheapest_precision::compute(const std::string &query_id, const std::vector<std::string> &results_list, size_t depth)
		{
		size_t which = 0;
		size_t found_and_relevant = 0;
		std::vector<judgement> query_prices;

		/*
			Get the lowest k priced item's price though a linear seach for the assessments for this query
			since this is only going to happen once per run, it doesn't seem worthwhile trying to optimise this.
		*/
		for (auto assessment = assessments.find_first(query_id); assessment != assessments.assessments.end(); assessment++)
			if ((*assessment).query_id == query_id)
				{
				if ((*assessment).score != 0)
					{
					auto price = prices.find("PRICE", (*assessment).document_id);
					query_prices.push_back(*assessment);
					}
				}
			else
				break;

		/*
			If there are no relevant items then we get a perfect score.
		*/
		if (query_prices.size() == 0)
			return 1;

		sort(query_prices.begin(), query_prices.end());
		size_t query_depth = maths::minimum(query_prices.size(), depth);		// if there are fewer then top_k relevant items then reduce k

		/*
			Compute the precision
		*/
		for (const auto &result : results_list)
			{
			/*
				Have we got a relevant item?
			*/
			judgement looking_for(query_id, result, 0);
			auto found = std::lower_bound(query_prices.begin(), query_prices.end(), looking_for);
			if (found->document_id == result && found->query_id == query_id)
				found_and_relevant++;

			/*
				Have we exceeded the search depth?
			*/
			which++;
			if (which >= depth)
				break;
			}

		return static_cast<double>(found_and_relevant) / static_cast<double>(query_depth);
		}

	/*
		EVALUATE_CHEAPEST_PRECISION::UNITTEST()
		---------------------------------------
	*/
	void evaluate_cheapest_precision::unittest(void)
		{
		/*
			Example results list with one relevant document
		*/
		std::vector<std::string> results_list_one =
			{
			"one",
			"two",  		// lowest priced relevant item
			"three",
			"four",
			"five",
			};

		/*
			Example results list with three relevant documents
		*/
		std::vector<std::string> results_list_two =
			{
			"six",
			"seven",		// lowest priced relevant item
			"eight",		// relevant
			"ten",
			"eleven",
			};

		/*
			Load the sample price list
		*/
		evaluate prices;
		std::string copy = unittest_data::ten_price_assessments_prices;
		prices.decode_assessments_trec_qrels(copy);

		/*
			Load the sample assessments
		*/
		evaluate container;
		copy = unittest_data::ten_price_assessments;
		container.decode_assessments_trec_qrels(copy);

		/*
			Evaluate the first results list
		*/
		evaluate_cheapest_precision calculator(prices, container);
		double calculated_precision = calculator.compute("1", results_list_one, 5);

		/*
			Compare to 5 decimal places
		*/
		double true_precision_one = 1.0;		// 0 because there is 1 relecant item so the effective depth is 1 and result[1] is not relevant
		JASS_assert(std::round(calculated_precision * 10000) == std::round(true_precision_one * 10000));

		/*
			Evaluate the second results list and check the result to 5 decimal places
		*/
		calculated_precision = calculator.compute("2", results_list_two, 5);
		double true_precision_two = 2.0 / 3.0;
		JASS_assert(std::round(calculated_precision * 10000) == std::round(true_precision_two * 10000));

		puts("evaluate_cheapest_precision::PASSED");
		}
	}