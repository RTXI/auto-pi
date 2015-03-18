/*
=========================================================================
Module Name: AUTO_PI

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
//#include "AUTO_PI_gui_model.h"

class AUTO_PI : public DefaultGUIModel
{

public:

   AUTO_PI(void);
    virtual ~AUTO_PI(void);

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
