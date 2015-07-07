/*
	Copyright (C) 2011 Georgia Institute of Technology, University of Utah, Weill Cornell Medical College

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
=========================================================================
Module Name: AutoPi

Module Version: v1.0

Date: 2011-10-07

Created by: Oscar Miranda-Dominguez

Description: This module uses a PI process to inject current to a cell, it also does auto calculation of the best parameters per cell.

Analysis File: none

References: none 

Notes:

Changelog:
	-v1.0-Initial version of the module.

=========================================================================
*/

 
#include <default_gui_model.h>
//#include "AutoPi_gui_model.h"

class AutoPi : public DefaultGUIModel
{

public:

   AutoPi(void);
    virtual ~AutoPi(void);

    // the main function run every time step, contains model logic
    virtual void execute(void);

protected:

    // run each time model parameters are updated
    virtual void update(DefaultGUIModel::update_flags_t);

private:

        // internal variables
    double kp;
    double ti;
    double td;
    double e;         //e is the error
    double pe;        //pe is the previous error
    double target;
    double P;
    double I;
    double D;
    double dt;
    double t;
    double tt;//time threshold
    double CurrentState;
    double Last_t_spike;
    double Current_t_spike;
    double OldConstantCurrent;
    double ConstantCurrent;
    double t_spike;
    double dI;
    double increase;//porcentage to increase (default 20%)
    double ISIs [20];
    double mean_ISI;
    double last_ISI;
    double min_ISI;
    double delta_y;
    double delta_x;
    double K;
    double tau;
    double a;
    long long count;
    int state;
    int counter;
    int autotune;
    int stage;
    int first;
    int HoldOn;
};
