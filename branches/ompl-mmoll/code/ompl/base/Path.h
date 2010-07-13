/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* \author Ioan Sucan */

#ifndef OMPL_BASE_PATH_
#define OMPL_BASE_PATH_

#include "ompl/base/General.h"

namespace ompl
{
    namespace base
    {
	class SpaceInformation;
	
	/// Abstract definition of a path
	class Path
	{
	public:
		/// Constructor. A path must always know the space information it is part of
		Path(const StateSpace& statespace) : m_statespace(statespace)
		{
		}

		/// Returns the state space this path is part of
		const StateSpace& getStateSpace(void) const
		{
			return m_si;
		}

 		/// Return the length of a path
		virtual double length(void) const = 0;
	    
	protected:
	    const StateSpace& m_statespace;
	};
	
	class DefaultPath : public Path
	{
		DefaultPath(const StateSpace& statespace) : Path(statespace)
		{
		}
		
		~DefaultPath()
		{
			std::vector<AbstractState*>::iterator i;
			for (i = m_states.begin(); i != m_states.end(); i++)
				delete m_states[i];
		}
		
		virtual double length(void)
		{
			return static_cast<double>(m_states.size());
		}
		
		std::vector<AbstractState*> m_states;
	};
	
    }
}

#endif