#include "widget.hpp"

#include <math.h>

auto_pi::Plugin::Plugin(Event::Manager* ev_manager)
    : Widgets::Plugin(ev_manager, std::string(auto_pi::MODULE_NAME))
{
}

auto_pi::Panel::Panel(QMainWindow* main_window, Event::Manager* ev_manager)
    : Widgets::Panel(std::string(auto_pi::MODULE_NAME), main_window, ev_manager)
{
  setWhatsThis("Automatic Interspike Interval Generator");
  createGUI(auto_pi::get_default_vars(), {P, I, D, CURRENT_STATE});  // this is required to create the GUI
  resizeMe();
  refresh();
}

auto_pi::Component::Component(Widgets::Plugin* hplugin)
    : Widgets::Component(hplugin,
                         std::string(auto_pi::MODULE_NAME),
                         auto_pi::get_default_channels(),
                         auto_pi::get_default_vars())
    , kp(2e-10)
    , ti(1e-8)
    , td(0.0)
    , target(0.1)
    , ConstantCurrent(50e-12)
    , increase(20.0)
    , autotune(0)
    , HoldOn(0)
    , dt(static_cast<double>(RT::OS::getPeriod()) * 1e-9)
{
}

void auto_pi::Component::execute()
{
  dt = static_cast<double>(RT::OS::getPeriod()) * 1e-9;
  // This is the real-time function that will be called
  switch (this->getState()) {
    case RT::State::EXEC:
      if (HoldOn == 1) {
        writeoutput(0, CurrentState);
        return;
      }
      t += dt;
      state = static_cast<int>(readinput(0));
      if (autotune == 0) {
        stage = 0;
      }
      if (autotune == 1) {
        if (first == 1) {
          stage = 5;
          first = 0;
          CurrentState = ConstantCurrent;
          tt = t;
        }
      }

      switch (stage) {
        case (0):
          if (state == 4) {
            writeoutput(0, 0);
            ConstantCurrent = 0;
            setState(RT::State::PAUSE);
            return;
          }

          if (t > Last_t_spike
                  + 10 * target)  // The neuron hasn't fired in a long time,
                                  // let's ramp up the current until it does.
          {
            if (state == 1) {
              Current_t_spike = t;
              Last_t_spike = t;
              P = 0;
              I = 0;
              D = 0;
              return;
            }
            if (ConstantCurrent <= 0) {
              ConstantCurrent = dI;
            } else {
              ConstantCurrent *= exp(.69 * dt / 4);
              if (ConstantCurrent > 1e3) {
                ConstantCurrent = 1e3;
              }
              CurrentState = ConstantCurrent + P + I + D;
            }
            return;
          }

          if (state == 1)  // i.e. Spike detect has just detected the neuron's
                           // voltage has passed threshold on the way up
          {
            Last_t_spike = Current_t_spike;
            Current_t_spike = t;
            t_spike = Current_t_spike - Last_t_spike;
            e = t_spike - target;
            P = kp * e;
            I = I + (ti * e * t_spike);
            D = (td / t_spike) * (P - pe)
                / kp;  // OM, Jul 20, 2009 -Oscar; accurate for small td -Ansel;
            pe = P;  // OM, Jul 20, 2009
            Last_t_spike = Current_t_spike;
          }
          CurrentState = ConstantCurrent + P + I + D;
          break;

        case (1):
          if (state == 1) {
            if (counter < 20) {
              Last_t_spike = Current_t_spike;
              Current_t_spike = t;
              t_spike = Current_t_spike - Last_t_spike;
              ISIs[counter] = t_spike;
              counter = counter + 1;
              if (counter == 20) {
                stage = 2;
              }
            }
          }
          break;

        case (2):
          last_ISI = mean_ISI;
          mean_ISI = 0;
          for (counter = 10; counter < 20; counter++) {
            mean_ISI = mean_ISI + ISIs[counter];
          }
          mean_ISI = mean_ISI / 10.0;
          counter = 0;
          if (mean_ISI > target) {
            stage = 3;
          } else {
            stage = 4;
          }
          break;

        case (3):
          CurrentState = CurrentState * (1 + increase / 100);
          // last_ISI=mean_ISI;
          stage = 1;
          break;

        case (4):
          delta_y = mean_ISI - last_ISI;
          delta_x = CurrentState - CurrentState / (1 + increase / 100);
          K = delta_y / delta_x;
          min_ISI = 20;
          for (counter = 1; counter < 20; counter++) {
            if (ISIs[counter] > mean_ISI) {
              if (counter < min_ISI)
                min_ISI = counter;
            }
          }
          counter = 0;
          tau = (min_ISI + 1) / 5;
          if (tau < 1.22) {
            tau = 1.22;
          }
          a = 1 - 1 / tau;
          kp = -(1 / K) * (201 * a - 1 - 20 * sqrt(101 * a * a - a));
          ti = 100 * kp;  // 100*kp is arbitrary. see paper listed in README
          setValue(KP, kp);
          setValue(TI, ti);
          stage = 0;
          autotune = 0;
          setValue(AUTOTUNE, autotune);
          ConstantCurrent = CurrentState;
          if (kp < 0) {
            CurrentState = CurrentState * (1 - increase / 100 - .1);
            ConstantCurrent = CurrentState;
            first = 1;
            autotune = 1;
            increase = 1.5 * increase;
          }
          break;

        case (5):
          if (CurrentState <= 0) {
            CurrentState = 1e-12;
          }
          if (state == 1) {
            Last_t_spike = Current_t_spike;
            Current_t_spike = t;
            t_spike = Current_t_spike - Last_t_spike;
          }
          if (t > 3 * target + tt)  // The neuron hasn't fired in a long time,
                                    // let's ramp up the current until it does.
          {
            printf("t=%f tt=%f\n", t, tt);
            if (t_spike > target) {
              stage = 1;
            }
            if (t > Last_t_spike + 2.99 * target) {
              CurrentState = CurrentState * 1.1;
              ConstantCurrent = CurrentState;
              first = 1;
            } else {
              if (t_spike < target) {
                CurrentState = CurrentState / 1.1;
                ConstantCurrent = CurrentState;
                first = 1;
              }
            }
          }
          break;
      }

      if (CurrentState > 4e-9) {
        CurrentState = 4e-9;
      }
      writeoutput(0, CurrentState);
      writeoutput(1, target);
      writeoutput(2, t_spike);
    case RT::State::INIT:
      setValue(PARAMETER::KP, kp);
      setValue(PARAMETER::TI, ti);
      setValue(PARAMETER::TD, td);
      setValue(PARAMETER::TARGET_ISI, target);
      setValue(PARAMETER::CONSTANT_CURRENT, ConstantCurrent * 1e12);
      setValue(PARAMETER::INCREASE_PERCENT, increase);
      setValue(PARAMETER::AUTOTUNE, autotune);
      setValue(PARAMETER::HOLD, HoldOn);
      setValue(PARAMETER::P, P);
      setValue(PARAMETER::I, I);
      setValue(PARAMETER::D, D);
      setValue(PARAMETER::CURRENT_STATE, CurrentState);
      break;
    case RT::State::MODIFY:
      kp = getValue<double>(PARAMETER::KP);
      ti = getValue<double>(PARAMETER::TI);
      td = getValue<double>(PARAMETER::TD);
      target = getValue<double>(PARAMETER::TARGET_ISI);
      HoldOn = getValue<int64_t>(PARAMETER::HOLD);
      OldConstantCurrent = ConstantCurrent;
      ConstantCurrent = getValue<double>(PARAMETER::CONSTANT_CURRENT)
          * 1e-12;  // if (flag!=PAUSE) ConstantCurrent=CurrentState;//just this
                    // added avoid return to constant current when you change
                    // ISI
      if (OldConstantCurrent != ConstantCurrent) {
        CurrentState = ConstantCurrent;
      } else {
        ConstantCurrent = CurrentState;
      }
      increase = getValue<double>(PARAMETER::INCREASE_PERCENT);
      autotune = getValue<int64_t>(PARAMETER::AUTOTUNE);
      if (autotune == 1) {
        first = 1;
      }
      P = 0;
      I = 0;
      D = 0;
      pe = 0;  // reset the previous error (used in the derivitive)
      t = 0;
      Last_t_spike = 0;
      Current_t_spike = 0;
      break;
    case RT::State::PERIOD:
      setState(RT::State::EXEC);
      break;
    case RT::State::PAUSE:
      writeoutput(0 , 0);
      setValue(PARAMETER::CONSTANT_CURRENT, (CurrentState) * 1e12);
      ConstantCurrent = CurrentState;
      P = 0;
      I = 0;
      D = 0;
      break;
    case RT::State::UNPAUSE:
      setState(RT::State::EXEC);
    default:
      break;
  }
}

///////// DO NOT MODIFY BELOW //////////
// The exception is if your plugin is not going to need real-time functionality.
// For this case just replace the craeteRTXIComponent return type to nullptr.
// RTXI will automatically handle that case and won't attach a component to the
// real time thread for your plugin.

std::unique_ptr<Widgets::Plugin> createRTXIPlugin(Event::Manager* ev_manager)
{
  return std::make_unique<auto_pi::Plugin>(ev_manager);
}

Widgets::Panel* createRTXIPanel(QMainWindow* main_window,
                                Event::Manager* ev_manager)
{
  return new auto_pi::Panel(main_window, ev_manager);
}

std::unique_ptr<Widgets::Component> createRTXIComponent(
    Widgets::Plugin* host_plugin)
{
  return std::make_unique<auto_pi::Component>(host_plugin);
}

Widgets::FactoryMethods fact;

extern "C"
{
Widgets::FactoryMethods* getFactories()
{
  fact.createPanel = &createRTXIPanel;
  fact.createComponent = &createRTXIComponent;
  fact.createPlugin = &createRTXIPlugin;
  return &fact;
}
};

//////////// END //////////////////////
