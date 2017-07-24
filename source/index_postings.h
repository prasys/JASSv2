/*
	INDEX_POSTINGS.H
	----------------
	Copyright (c) 2016 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
/*!
	@file
	@brief Non-thread-Safe object that holds a single postings list during indexing.
	@author Andrew Trotman
	@copyright 2016 Andrew Trotman
*/
#pragma once

#include <tuple>
#include <sstream>
#include <iostream>

#include "dynamic_array.h"
#include "allocator_pool.h"

namespace JASS
	{
	/*
		CLASS INDEX_POSTINGS
		--------------------
	*/
	/*!
		@brief Non-thread-Safe object that accumulates a single postings list during indexing.
	*/
	class index_postings
		{
		private:
			static constexpr size_t initial_size = 4;		///< Initially allocate space for 4 elements
			static constexpr double growth_factor = 1.5;	///< Grow dynamic arrays by a factor of 1.5
			
		private:
			size_t highest_document;							///< The higest document number seen in this postings list (counting from 1)
			size_t highest_position;							///< The higest position seen in this postings list (counting from 1)

			dynamic_array<uint32_t> document_ids;			///< Array holding the document IDs
			dynamic_array<uint16_t> term_frequencies;		///< Array holding the term frequencies
			dynamic_array<uint32_t> positions;				///< Array holding the term positions

		protected:
			/*!
				@typedef posting
				@brief The representation of a single postings as a tuple of docid, term frequency and position.
			*/
			typedef std::tuple<uint32_t, uint32_t, uint32_t> posting;

			/*
				CLASS INDEX_POSTINGS::ITERATOR
				------------------------------
			*/
			/*!
				@brief C++ iterator for iterating over an index_postings object.
				@details See http://www.cprogramming.com/c++11/c++11-ranged-for-loop.html for details on how to write a C++11 iterator.
			*/
			class iterator
				{
				public:
					/*
						ENUM INDEX_POSTINGS::ITERATOR::WHERE
						------------------------------------
					*/
					/*!
						@brief Whether the iterator is at the start of the end of the postings lists
					*/
					enum where
						{
						START = 0,			///< Iterator is from the start of the postings
						END = 1				///< Iterator is from the end of the postings
						};

				private:
					dynamic_array<uint32_t>::iterator document;			///< The iterator for the documents (1 per document).
					dynamic_array<uint16_t>::iterator frequency;			///< The iterator for the frequencies (1 per document).
					dynamic_array<uint32_t>::iterator position;			///< The iterator for the word positions (frequency times per document).
					dynamic_array<uint16_t>::iterator frequency_end;	///< Use to know when we've walked past the end of the pstings.
					uint32_t frequencies_remaining;							///< The numner of word positions that have not yet been returned for this document.

				public:
					/*
						INDEX_POSTINGS::ITERATOR::ITERATOR()
						------------------------------------
					*/
					/*!
						@brief Constructor
						@param parent [in] The object that this iterator is iterating over
						@param start [in] Whether this is an iterator for the START or END of the postings lists
					*/
					iterator(const index_postings &parent, where start):
						document(start == START ? parent.document_ids.begin() : parent.document_ids.end()),
						frequency(start == START ? parent.term_frequencies.begin() : parent.term_frequencies.end()),
						position(start == START ? parent.positions.begin() : parent.positions.end()),
						frequency_end(parent.term_frequencies.end()),
						frequencies_remaining(frequency != parent.term_frequencies.end() ? *frequency : 0)
						{
						/*
							Nothing.
						*/
						}

					/*
						INDEX_POSTINGS::ITERATOR::OPERATOR!=()
						--------------------------------------
					*/
					/*!
						@brief Compare two iterator objects for non-equality.
						@param other [in] The iterator object to compare to.
						@return true if they differ, else false.
					*/

					bool operator!=(const iterator &other) const
						{
						if (other.position != position)
							return true;

						/*
							These cases can't happen because if the positions are the same then the document and term frequency must be the same.
						*/
						#ifdef NEVER
						else if (other.document != document)
							return true;
						else if (other.position != position)
							return true;
						#endif
						
						return false;
						}

					/*
						INDEX_POSTINGS::ITERATOR::OPERATOR*()
						-------------------------------------
					*/
					/*!
						@brief Return a reference to the element pointed to by this iterator.
					*/
					const posting operator*() const
						{
						return std::make_tuple(*document, *frequency, *position);
						}

					/*
						INDEX_POSTINGS::ITERATOR::OPERATOR++()
						--------------------------------------
					*/
					/*!
						@brief Increment this iterator.
					*/
					const iterator &operator++()
						{
						frequencies_remaining--;
						if (frequencies_remaining <= 0)
							{
							++document;
							++frequency;
							frequencies_remaining = frequency != frequency_end ? *frequency : 0;
							}
						++position;
						return *this;
						}
				};
			
		private:
			/*
				INDEX_POSTINGS::INDEX_POSTINGS()
				--------------------------------
			*/
			/*!
				@brief Parameterless construction is forbidden (so private).
			*/
			index_postings() :
				index_postings(*new allocator_pool(1024))
				{
				assert(0);
				}

		public:
			/*
				INDEX_POSTINGS::INDEX_POSTINGS()
				--------------------------------
			*/
			/*!
				@brief Constructor.
				@param memory_pool [in] All allocation is from this allocator.
			*/
			index_postings(allocator &memory_pool) :
				highest_document(0),															// starts at 0, counts from 1
				highest_position(0),															// starts at 0, counts from 1
				document_ids(memory_pool, initial_size, growth_factor),			// give the allocator to the array
				term_frequencies(memory_pool, initial_size, growth_factor),		// give the allocator to the array
				positions(memory_pool, initial_size, growth_factor)				// give the allocator to the array
				{
				/*
					Nothing
				*/
				}

			/*
				INDEX_POSTINGS::BEGIN()
				-----------------------
			*/
			/*!
				@brief Return an iterator pointing to the start of the postings.
				@return Iterator pointing to start of the postings.
			*/
			iterator begin(void) const
				{
				return iterator(*this, iterator::START);
				}

			/*
				INDEX_POSTINGS::END()
				---------------------
			*/
			/*!
				@brief Return an iterator pointing to the end of the postings.
				@return Iterator pointing to end of tne postings.
			*/

			iterator end(void) const
				{
				return iterator(*this, iterator::END);
				}

			/*
				INDEX_POSTINGS::PUSH_BACK()
				---------------------------
			*/
			/*!
				@brief Add to the end of the postings list.
			*/
			virtual void push_back(size_t document_id, size_t position)
				{
				if (document_id == highest_document)
					{
					/*
						If this is the second or subseqent occurrence then just add 1 to the term frequency (and make sure it doesn't overflow).
					*/
					uint16_t &frequency = term_frequencies.back();
					if (frequency <= 0xFFFE)
						frequency++;
					}
				else
					{
					/*
						First time we've seen this term in this document so add a new document ID and set the term frequency to 1.
					*/
					document_ids.push_back(document_id);
					highest_document = document_id;
					term_frequencies.push_back(1);
					}
					
				/*
					Always add a new position
				*/
				positions.push_back(position);
				highest_position = position;
				}
			
			/*
				INDEX_POSTINGS::TEXT_RENDER()
				-----------------------------
			*/
			/*!
				@brief Dump a human-readable version of the postings list down the stream. Format is: <DocID, TF, Pos, Pos, Pos>...
				@param stream [in] The stream to write to.
			*/
			void text_render(std::ostream &stream) const
				{
				uint32_t previous_document_id = std::numeric_limits<uint32_t>::max();
				for (const auto &posting : *this)
					{
					if (std::get<0>(posting) != previous_document_id)
						{
						if (previous_document_id != std::numeric_limits<uint32_t>::max())
							stream << '>';
						stream << '<' << std::get<0>(posting) << ',' << std::get<1>(posting) << ',' << std::get<2>(posting);
						previous_document_id = std::get<0>(posting);
						}
					else
						stream << ',' << std::get<2>(posting);
					}
				stream << '>';
				}
			
			/*
				INDEX_POSTINGS::UNITTEST()
				--------------------------
			*/
			/*!
				@brief Unit test this class.
			*/
			static void unittest(void)
				{
				allocator_pool pool;
				index_postings postings(pool);
				
				postings.push_back(1, 100);
				postings.push_back(1, 101);
				postings.push_back(2, 102);
				postings.push_back(2, 103);
				
				std::ostringstream result;
				
				postings.text_render(result);

				JASS_assert(strcmp(result.str().c_str(), "<1,2,100,101><2,2,102,103>") == 0);

				puts("index_postings::PASSED");
				}
		};
		
	/*
		OPERATOR<<()
		------------
	*/
	/*!
		@brief Dump a human readable version of the postings list down an output stream.
		@param stream [in] The stream to write to.
		@param data [in] The postings list to write.
		@return The stream once the postings list has been written.
	*/
	inline std::ostream &operator<<(std::ostream &stream, const index_postings &data)
		{
		data.text_render(stream);
		return stream;
		}
	}
