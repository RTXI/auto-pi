
#include <NIDAQmx.h>
#include <rtxi/widgets.hpp>

// This is an generated header file. You may change the namespace, but
// make sure to do the same in implementation (.cpp) file
namespace auto_pi
{

constexpr std::string_view MODULE_NAME = "auto-pi";

enum PARAMETER : Widgets::Variable::Id
{
  // set parameter ids here
  KP = 0,
  TI,
  TD,
  TARGET_ISI,
  CONSTANT_CURRENT,
  INCREASE_PERCENT,
  AUTOTUNE,
  HOLD,
  P,
  I,
  D,
  CURRENT_STATE
};

inline std::vector<Widgets::Variable::Info> get_default_vars()
{
  return {
      // PARAMETERS
      {PARAMETER::KP,
       "Kp",
       "Proportional Gain",
       Widgets::Variable::DOUBLE_PARAMETER,
       2e-10},
      {PARAMETER::TI,
       "Ti",
       "Integral time (Gain/Ti)",
       Widgets::Variable::DOUBLE_PARAMETER,
       1e-8},
      {PARAMETER::TD,
       "Td",
       "Derivitive time (Gain*Td)",
       Widgets::Variable::DOUBLE_PARAMETER,
       0.0},
      {PARAMETER::TARGET_ISI,
       "Target ISI",
       "Target inter-spike interval (s)",
       Widgets::Variable::DOUBLE_PARAMETER,
       0.1},
      {PARAMETER::CONSTANT_CURRENT,
       "ConstantCurrent",
       "Constant current (pA)",
       Widgets::Variable::DOUBLE_PARAMETER,
       50e-12},
      {PARAMETER::INCREASE_PERCENT,
       "Increase (%)",
       "% Current increase per step",
       Widgets::Variable::DOUBLE_PARAMETER,
       20.0},
      {PARAMETER::AUTOTUNE,
       "Autotune",
       "Autotune? (1=yes)",
       Widgets::Variable::INT_PARAMETER,
       int64_t {0}},
      {PARAMETER::HOLD,
       "Hold",
       "Hold",
       Widgets::Variable::INT_PARAMETER,
       int64_t {0}},

      // STATES
      {PARAMETER::P,
       "P",
       "Proportional component (A)",
       Widgets::Variable::STATE,
       uint64_t {0}},
      {PARAMETER::I,
       "I",
       "Integrative component (A)",
       Widgets::Variable::STATE,
       uint64_t {0}},
      {PARAMETER::D,
       "D",
       "Derivative component (A)",
       Widgets::Variable::STATE,
       uint64_t {0}},
      {PARAMETER::CURRENT_STATE,
       "CurrentState",
       "Current output (A)",
       Widgets::Variable::STATE,
       uint64_t {0}},
  };
}

inline std::vector<IO::channel_t> get_default_channels()
{
  return {
      // INPUTS
      {
          "State",
          "Spike state variable (0 - 4)",
          IO::INPUT,
      },

      // OUTPUTS
      {
          "Iout (A)",
          "Current output (A)",
          IO::OUTPUT,
      },
      {
          "Target ISI (s)",
          "Target Inter-spike interval (s)",
          IO::OUTPUT,
      },
      {
          "ISI (s)",
          "Inter-spike interval (s)",
          IO::OUTPUT,
      },

  };
}

class Panel : public Widgets::Panel
{
  Q_OBJECT
public:
  Panel(QMainWindow* main_window, Event::Manager* ev_manager);

private:
  // Any functions and data related to the GUI are to be placed here
};

class Component : public Widgets::Component
{
public:
  explicit Component(Widgets::Plugin* hplugin);
  void execute() override;

private:
  // internal variables
  double kp;
  double ti;
  double td;
  double e = 0;  // e is the error
  double pe = 0;  // pe is the previous error
  double target;
  double P = 0;
  double I = 0;
  double D = 0;
  double dt;
  double t = 0;
  double tt = 0;  // time threshold
  double CurrentState = 0;
  double Last_t_spike = 0;
  double Current_t_spike = 0;
  double OldConstantCurrent = 0;
  double ConstantCurrent;
  double t_spike;
  double dI;
  double increase;  // porcentage to increase (default 20%)
  std::array<double, 20> ISIs;
  double mean_ISI = 0.0;
  double last_ISI = 0.0;
  double min_ISI = 0.0;
  double delta_y = 0.0;
  double delta_x = 0.0;
  double K = 0;
  double tau = 0;
  double a = 0;
  int64_t count;
  int state;
  int counter;
  int64_t autotune;
  int stage;
  int first;
  int64_t HoldOn;
};

class Plugin : public Widgets::Plugin
{
public:
  explicit Plugin(Event::Manager* ev_manager);
};

}  // namespace auto_pi
