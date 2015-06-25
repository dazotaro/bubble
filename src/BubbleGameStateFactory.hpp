/*
 * BubbleGameStateFactory.hpp
 *
 *  Created on: Jun 25, 2015
 *      Author: jusabiaga
 */

#ifndef BUBBLEGAMESTATEFACTORY_HPP_
#define BUBBLEGAMESTATEFACTORY_HPP_

#include <core/GameStateFactory.hpp>


class BubbleGameStateFactory : public JU::GameStateFactory
{
	public:
		BubbleGameStateFactory();
		virtual ~BubbleGameStateFactory();

	public:
		JU::GameStateInterface* create(const std::string& state_name) const;
};

#endif /* BUBBLEGAMESTATEFACTORY_HPP_ */
