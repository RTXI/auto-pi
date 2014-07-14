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

#include <math.h>
#include <stdio.h>
#include <auto-pi.h>


extern "C" Plugin::Object *createRTXIPlugin(void) {
    return new AutoPi();
}


static DefaultGUIModel::variable_t vars[] = {
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//INPUTS

   {"State","Spike State Variable (0 - 4)",DefaultGUIModel::INPUT,},

//OUTPUTS

   {"Iout","Current Output, A",DefaultGUIModel::OUTPUT,},  
   {"Target ISI","Target ISI",DefaultGUIModel::OUTPUT,},
   {"ISI","ISI",DefaultGUIModel::OUTPUT,},

//PARAMETERS

   {"Kp","Proportional Gain",DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},   
   {"Ti","Integral Time",DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},
   {"Td","Derivitive Time",DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},
   {"Target Ts","Target Inter Spike Interval (sec)",DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},
   {"ConstantCurrent","Constant Current (pA)",DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},
   {"Increase","% of current increase per step",DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},
   {"Autotune","Autotune?",DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},
   {"Hold","Hold",DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,},

//STATES

   {"P","Scalar",DefaultGUIModel::STATE,},
   {"I","Scalar",DefaultGUIModel::STATE,},
   {"D","Scalar",DefaultGUIModel::STATE,}, 
   {"CurrentState","pA",DefaultGUIModel::STATE,}, 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 };

// some necessary variable
static size_t num_vars = sizeof(vars)/sizeof(DefaultGUIModel::variable_t);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// constructor 
// provides default values for paramters, calls update(INIT)
AutoPi::AutoPi(void) : DefaultGUIModel("Auto PI",::vars,::num_vars)
{ createGUI(vars, num_vars);
  kp=2e-10;
  ti=1e-8;
  td=0;
  ConstantCurrent=50e-12;
  dI=10; //5e-17Rate to ramp current up if cell is not spiking;
  e=0;
  pe=0;
  target=.1;
  P=0;
  I=0;
  D=0;
  CurrentState=0;
  Last_t_spike=0;
  Current_t_spike=0;
  t=0;
  tt=0;
  t_spike=0;
  increase=20;
  mean_ISI=0;
  last_ISI=0;
  counter=0;
  autotune=0;
  stage=0;
  first=1;
  OldConstantCurrent=0;
  HoldOn=0;

    update(INIT);
    refresh();
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
}
 
AutoPi::~AutoPi(void) {}

// execute is run every time step
void AutoPi::execute(void)
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
{

  if(HoldOn==1)
    {
      output(0)=CurrentState;
      return;
    }
  t += dt;    
  state=(int)(input(0));
  if (autotune==0) stage=0;
  if (autotune==1) {
    if (first==1){
      stage=5;
      first=0;
      CurrentState=ConstantCurrent;
      tt=t;
    }
  }
 
  switch (stage) {
  case (0):
    if(state == 4)
    {
      output(0) = 0;
      ConstantCurrent = 0;
      DefaultGUIModel::pause(true);
      return;
    }
  
    if (t > Last_t_spike + 10*target) //The neuron hasn't fired in a long time, let's ramp up the current until it does.
    {
      if(state == 1) 
	{
	  Current_t_spike = t;
	  Last_t_spike = t;
	  P=0;
	  I=0;
	  D=0;
	  return;
	}
      // printf("Current=%f\n",CurrentState*1e12);
      // ConstantCurrent = dI;//Ramp up constant current if cell is not spiking;
      if(ConstantCurrent <= 0)
	{
	 ConstantCurrent = dI;
	}
      else
	{
	  ConstantCurrent *= exp(.69*dt/4);
	  if(ConstantCurrent>1e3) ConstantCurrent=1e3;
	  CurrentState=ConstantCurrent+P+I+D;
	}
      return;
    }
    
    if(state == 1) //i.e. Spike detect has just detected the neuron's voltage has passed threshold on the way up
    {
      Last_t_spike = Current_t_spike;
      Current_t_spike = t;
      t_spike = Current_t_spike - Last_t_spike;  
      e = t_spike - target;
      P = kp*e;
      I = I +(ti*e*t_spike);
      D = (td/t_spike)*(P-pe)/kp; //OM, Jul 20, 2009
      pe = P;//OM, Jul 20, 2009
      Last_t_spike = Current_t_spike;
      // printf("error=%f, P=%f, I=%f, D=%f, ThisSpike=%f, LastSpike=%f\n",e,P,I,D,t_spike,Last_t_spike);
    } 
    CurrentState=ConstantCurrent+P+I+D;
    break;

  case (1):
    if(state == 1){
      if (counter<20){
      Last_t_spike = Current_t_spike;
      Current_t_spike = t;
      t_spike = Current_t_spike - Last_t_spike;  
      ISIs[counter]=t_spike;
      counter=counter+1;
      if (counter==20) stage =2;
      }
    }
    break;

  case (2):
    last_ISI=mean_ISI;
    mean_ISI=0;
    for (counter = 10; counter < 20; counter++)
       {
	 mean_ISI=mean_ISI+ISIs[counter];
       }
    mean_ISI=mean_ISI/10.0;
    counter=0;
    if (mean_ISI>target)  stage=3;
    else stage=4;
    break;
  
case (3):
  CurrentState=CurrentState*(1+increase/100);
  //last_ISI=mean_ISI;
  stage=1;
    break;

case (4):
   delta_y=mean_ISI-last_ISI;
   delta_x=CurrentState-CurrentState/(1+increase/100);
   K=delta_y/delta_x;
   min_ISI=20;
   for (counter = 1; counter < 20; counter++)
       {
	 if (ISIs[counter]>mean_ISI) if (counter<min_ISI) min_ISI=counter;
       }
   counter=0;
   tau=(min_ISI+1)/5;
   if (tau<1.22) tau=1.22;
   a=1-1/tau;
   /*
printf("Current=%f\n",CurrentState);
printf("mean_ISI=%f\n",mean_ISI);
printf("last_ISI=%f\n",last_ISI);
printf("increase=%f\n",increase);
printf("tau=%f\n",tau);
printf("ISIs=%f\n",ISIs[0]);
printf("ISIs=%f\n",ISIs[1]);
printf("ISIs=%f\n",ISIs[2]);
printf("ISIs=%f\n",ISIs[3]);
printf("ISIs=%f\n",ISIs[4]);
printf("ISIs=%f\n",ISIs[5]);
printf("ISIs=%f\n",ISIs[6]);
printf("ISIs=%f\n",ISIs[7]);
printf("ISIs=%f\n",ISIs[8]);
printf("ISIs=%f\n",ISIs[9]);
printf("ISIs=%f\n",ISIs[10]);
printf("ISIs=%f\n",ISIs[11]);
printf("ISIs=%f\n",ISIs[12]);
printf("ISIs=%f\n",ISIs[13]);
printf("ISIs=%f\n",ISIs[14]);
printf("ISIs=%f\n",ISIs[15]);
printf("ISIs=%f\n",ISIs[16]);
printf("ISIs=%f\n",ISIs[17]);
printf("ISIs=%f\n",ISIs[18]);
printf("ISIs=%f\n",ISIs[19]);
   */
   kp=-(1/K)*(201*a-1-20*sqrt(101*a*a-a));
   ti=100*kp;
   setParameter("Kp",kp);
   setParameter("Ti",ti);
   stage=0;
   autotune=0;
   setParameter("Autotune",autotune);
   ConstantCurrent=CurrentState;
   if (kp<0)
     {
       CurrentState=CurrentState*(1-increase/100-.1);
       ConstantCurrent=CurrentState;
       first=1;
       autotune=1;
       increase=1.5*increase;
      //% setParameter("ConstantCurrent",(CurrentState)*1e12);
      //% setParameter("Increase",increase);
     }
   break;
  case (5):
    if(CurrentState <= 0) CurrentState = 1e-12;
    //printf("Entering Stage 5");
    //printf("Stage=%i\n",stage);
    if (state==1)
      {
	Last_t_spike = Current_t_spike;
	Current_t_spike = t;
	t_spike = Current_t_spike - Last_t_spike;
      }
    //printf("t=%f tt=%f\n",t,tt);
    if (t > 3*target+tt) //The neuron hasn't fired in a long time, let's ramp up the current until it does.
    {
      printf("t=%f tt=%f\n",t,tt);
      if (t_spike>target) stage=1;
       if (t>Last_t_spike+2.99*target)
	{
	  CurrentState=CurrentState*1.1;
	  ConstantCurrent=CurrentState;
	  first=1;
	  //%setParameter("ConstantCurrent",(CurrentState)*1e12);
	  }
       else
	 {
	   if (t_spike<target)
	     {
	       CurrentState=CurrentState/1.1;
	       ConstantCurrent=CurrentState;
	       first=1;
	     //%  setParameter("ConstantCurrent",(CurrentState)*1e12);

	     }
	 }
    }
      /*
      else
	{
	  if (t > Last_t_spike + 3*target)
	    {
	      CurrentState=CurrentState*(1+increase/100);
	    }
	}
     

      //CurrentState=CurrentState*(1+increase/100);

      if(CurrentState>1e-8) CurrentState=1e-8;
	  printf("Here");
	  //CurrentState=ConstantCurrent+P+I+D;
    }
      //      return;
 
    else
      {
	//	stage=1;
	stage=5;
      }
      */
    break;

  }
 output(1)=target;  
 if(CurrentState>4e-9)
   {
     CurrentState=4e-9;
   }
 output(0)=CurrentState;
 output(2)= t_spike;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
}


void AutoPi::update(DefaultGUIModel::update_flags_t flag)
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
{
  switch(flag) 
 {
  case INIT:
    setParameter("Kp",kp);
    setParameter("Ti",ti);
    setParameter("Td",td);
    setParameter("Target Ts",target);
    setParameter("ConstantCurrent",ConstantCurrent*1e12);
    setParameter("Increase",increase);
    setParameter("Autotune",autotune);
    setParameter("Hold",HoldOn);
    setState("P", P);
    setState("I", I);
    setState("D", D);
    setState("CurrentState",CurrentState);
    break;
  case MODIFY:
    kp = getParameter("Kp").toDouble(); 
    ti = getParameter("Ti").toDouble();
    td = getParameter("Td").toDouble();
    target = getParameter("Target Ts").toDouble();
    HoldOn = getParameter("Hold").toDouble();
    OldConstantCurrent=ConstantCurrent;
    ConstantCurrent = getParameter("ConstantCurrent").toDouble()*1e-12;// if (flag!=PAUSE) ConstantCurrent=CurrentState;//just this added avoid return to constant current when you change ISI
    if (OldConstantCurrent!=ConstantCurrent)
      {  
      	CurrentState=ConstantCurrent;   
      }
    
    else
      {
	ConstantCurrent=CurrentState;
      }  
    //OldConstantCurrent=ConstantCurrent;
    increase = getParameter("Increase").toDouble();
    autotune = getParameter("Autotune").toInt();if (autotune==1) first=1;
    P=0;
    I=0;
    D=0;
    pe=0;            //reset the previous error (used in the derivitive)
    t=0;   
    Last_t_spike=0;  
    Current_t_spike=0; 
    break;
  case PAUSE:
    output(0)=0;
    setParameter("ConstantCurrent",(ConstantCurrent + P+I+D)*1e12);
    setParameter("ConstantCurrent",(CurrentState)*1e12);
    ConstantCurrent=CurrentState;
    P=0;
    I=0;
    D=0;
    break;
  case UNPAUSE:
    break;
  case PERIOD: 
    break;
    /*
 case HOLD:
   printf("holding PI controller");
   HoldOn=1;
   break;
 case UNHOLD:
   HoldOn=0;
   break;
    */
  default:
    break;
 }
  //printf("Target ISI = %f\n",target);
  dt = RT::System::getInstance()->getPeriod()*1e-9;
  //printf("dT=%f\n",dt);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
}
       

