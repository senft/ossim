//  
// =============================================================================
// OSSIM : A Generic Simulation Framework for Overlay Streaming
// =============================================================================
//
// (C) Copyright 2012-2013, by Giang Nguyen (P2P, TU Darmstadt) and Contributors
//
// Project Info: http://www.p2p.tu-darmstadt.de/research/ossim
//
// OSSIM is free software: you can redistribute it and/or modify it under the 
// terms of the GNU General Public License as published by the Free Software 
// Foundation, either version 3 of the License, or (at your option) any later 
// version.
//
// OSSIM is distributed in the hope that it will be useful, but WITHOUT ANY 
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with 
// this program. If not, see <http://www.gnu.org/licenses/>.

// -----------------------------------------------------------------------------
// AppSettingMultitree.h
// -----------------------------------------------------------------------------
// (C) Copyright 2012-2013, by Giang Nguyen (P2P, TU Darmstadt) and Contributors
//
// Contributors: Giang;
// Code Reviewers: -;
// -----------------------------------------------------------------------------
//

#ifndef APPSETTINGMULTITREE_H_
#define APPSETTINGMULTITREE_H_

#include "AppSetting.h"

class AppSettingMultitree : public AppSetting {
public:
    AppSettingMultitree();
    virtual ~AppSettingMultitree();

	inline int getNumStripes(){ return param_numStripes; };
	inline int getStripeSize() { return param_stripeSize; };

protected:
    void handleMessage(cMessage* msg);
    virtual void initialize();
    virtual void finish();

    int param_numStripes;
    int param_stripeSize;
};

#endif /* APPSETTINGMULTITREE_H_ */
