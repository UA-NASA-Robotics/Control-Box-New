#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include "../digital_io/expander.hpp"
#include "../digital_io/expander_pin.hpp"
#include "../digital_io/digital_pin.hpp"
#include "../communications/request.hpp"
#include "../analog_input/analog_input.hpp"
#include "../memory/memory.hpp"

class InputHandler
{
public:
  void initialize (Memory *);
  void poll ();
  bool valid_request () const;
private:
  // Initialize Devices
  void initialize_expanders ();
  void initialize_push_buttons ();
  void initialize_arcade_buttons ();
  void initialize_joysticks ();
  void initialize_potentiometers ();

  // Detect Input Signals
  void detect_push_buttons ();
  void detect_arcade_buttons ();
  void detect_joystick ();
private:
  Request request;
  Memory * memory;
  Expander expander_left;
  Expander expander_right;
  ExpanderPin push_buttons [NUM_PUSH_BUTTONS];
  DigitalPin arcade_buttons [NUM_ARCADE_BUTTONS];
  AnalogInput joystick_left_x;
  AnalogInput joystick_left_y;
  AnalogInput joystick_right_x;
  AnalogInput joystick_right_y;
  AnalogInput potentiometer_left;
  AnalogInput potentiometer_right;
};

#endif // INPUT_HANDLER_HPP