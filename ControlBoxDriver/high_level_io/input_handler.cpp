#include "input_handler.hpp"
#include "../memory/memory_variables.hpp"

/*------------------------------ Initialization ------------------------------*/

void InputHandler::initialize (Memory * memory)
{
  this->memory = memory;
  initialize_expanders();
  initialize_push_buttons();
  initialize_arcade_buttons();
  initialize_joysticks();
  initialize_potentiometers();
}

void InputHandler::initialize_expanders ()
{
  expander_left.set_address(0x20);
  expander_right.set_address(0x27);
}

void InputHandler::initialize_push_buttons ()
{
  // Left push-buttons
  push_buttons[0].initialize(&expander_left, A, ZERO, INPUT);
  push_buttons[1].initialize(&expander_left, A, TWO,  INPUT);
  push_buttons[2].initialize(&expander_left, A, FOUR, INPUT);
  push_buttons[3].initialize(&expander_left, A, SIX,  INPUT);
  push_buttons[4].initialize(&expander_left, B, ZERO, INPUT);
  push_buttons[5].initialize(&expander_left, B, TWO,  INPUT);
  push_buttons[6].initialize(&expander_left, B, FOUR, INPUT);
  push_buttons[7].initialize(&expander_left, B, SIX,  INPUT);

  // Right push-buttons
  push_buttons[8].initialize(&expander_right, A, ZERO, INPUT);
  push_buttons[9].initialize(&expander_right, A, TWO,  INPUT);
  push_buttons[10].initialize(&expander_right, A, FOUR, INPUT);
  push_buttons[11].initialize(&expander_right, A, SIX,  INPUT);
  push_buttons[12].initialize(&expander_right, B, ZERO, INPUT);
  push_buttons[13].initialize(&expander_right, B, TWO,  INPUT);
  push_buttons[14].initialize(&expander_right, B, FOUR, INPUT);
  push_buttons[15].initialize(&expander_right, B, SIX,  INPUT);
}

void InputHandler::initialize_arcade_buttons ()
{
  arcade_buttons[0].initialize(K, TWO, INPUT_PULLUP);   // emergency stop
  arcade_buttons[1].initialize(B, SIX, INPUT_PULLUP);   // n/a
  arcade_buttons[2].initialize(B, FOUR, INPUT_PULLUP);  // n/a
  arcade_buttons[3].initialize(B, ZERO, INPUT_PULLUP);  // arm up
  arcade_buttons[4].initialize(E, FOUR, INPUT_PULLUP);  // lower bucket
  arcade_buttons[5].initialize(B, FIVE, INPUT_PULLUP);  // raise bucket
  arcade_buttons[6].initialize(E, SIX, INPUT_PULLUP);   // arm down
  arcade_buttons[7].initialize(E, SEVEN, INPUT_PULLUP); // plow up
  arcade_buttons[8].initialize(E, FIVE, INPUT_PULLUP);  // plow down
}

void InputHandler::initialize_joysticks ()
{
  joystick_left_y.initialize(CHANNEL_ZERO);
  joystick_left_x.initialize(CHANNEL_ONE);
  joystick_right_x.initialize(CHANNEL_SIX);
  joystick_right_y.initialize(CHANNEL_SEVEN);
}

void InputHandler::initialize_potentiometers ()
{
  potentiometer_left.initialize(CHANNEL_TWO);
  potentiometer_right.initialize(CHANNEL_FIVE);
}

/*-------------------- Reading Input and Setting Request ---------------------*/

void InputHandler::poll ()
{
  request = NO_REQUEST;
  detect_arcade_buttons();
  detect_push_buttons();
  detect_joystick();
  memory->write(REQUEST, (uint16_t)request);
}

void InputHandler::detect_push_buttons ()
{
  if (valid_request())
    return;
  int index = 0;
  int count = 0;
  for (int i = 0; i < NUM_PUSH_BUTTONS; ++i)
  {
    if (!push_buttons[i].read())
    {
      ++count;
      index = i;
    }
  }
  if (count == 1)
    request = PUSH_BUTTON_REQUESTS[index];
}

void InputHandler::detect_arcade_buttons ()
{
  if (valid_request())
    return;
  int index = 0;
  int count = 0;
  for (int i = 0; i < NUM_ARCADE_BUTTONS; ++i)
  {
    bool button_pressed = arcade_buttons[i].is_low();
    if (button_pressed)
    {
      ++count;
      index = i;
    }
  }
  if (count == 1)
  {
    request = ARCADE_BUTTON_REQUESTS [index];
  }
}

void InputHandler::detect_joystick ()
{
  uint16_t left_x = joystick_left_x.read();
  uint16_t left_y = joystick_left_y.read();
  uint16_t right_x = joystick_right_x.read();
  uint16_t right_y = joystick_right_y.read();
  
  memory->write(JOYSTICK_LEFT_X,  left_x);
  memory->write(JOYSTICK_LEFT_Y, left_y);
  memory->write(JOYSTICK_RIGHT_X, right_x);
  memory->write(JOYSTICK_RIGHT_Y, right_y);
  memory->write(SLIDER_LEFT,    potentiometer_left.read());
  memory->write(SLIDER_RIGHT,   potentiometer_right.read());
  if (valid_request())
    return;
  const int offset = 50;
  bool x_is_neutral = (left_x > 512 - offset) && (left_x < 512 + offset);
  bool y_is_neutral = (left_y > 512 - offset) && (left_y < 512 + offset);
  if (x_is_neutral && y_is_neutral)
    return;
  request = JOYSTICK;
}

/*----------------------------- Querying Request -----------------------------*/

bool InputHandler::valid_request () const
{
  return request != NO_REQUEST;
}