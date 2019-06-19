#include "output_handler.hpp"
#include "../digital_io/digital_io_params.hpp"
#include "../digital_io/digital_pin.hpp"
#include "../communications/request.hpp"
#include "../memory/memory_variables.hpp"

#define F_CPU 16000000UL
#include <util/delay.h>

void OutputHandler::initialize (Memory * memory)
{
  this->memory = memory;
  initialize_expanders();
  initialize_push_button_leds();
  initialize_panel_leds();
  screen.initialize(memory, UART_3);
}

void OutputHandler::startup ()
{
  // flash the panel-mounted LEDs
  for (int i = 0; i < 4; ++i)
  {
	  for (int j = 0; j < 4; ++j)
	  {
		int index = i % 2 ? 3 - j : j;
	    panel_leds[index].write(1);
		panel_leds[index + 4].write(1);
		_delay_ms(50);
		panel_leds[index].write(0);
		panel_leds[index + 4].write(0);
	  }	
  }

  // flash the push-button LEDs
  spiral();
}

void OutputHandler::refresh ()
{
  refresh_push_button_leds();
  refresh_panel_leds();
  screen.refresh();
}

void OutputHandler::initialize_expanders ()
{
  expander_left.set_address(0x20);
  expander_right.set_address(0x27);
}

void OutputHandler::initialize_push_button_leds ()
{
  // Left push-button LEDs
  push_button_leds[0].initialize(&expander_left, A, ONE, OUTPUT);
  push_button_leds[1].initialize(&expander_left, A, THREE, OUTPUT);
  push_button_leds[2].initialize(&expander_left, A, FIVE, OUTPUT);
  push_button_leds[3].initialize(&expander_left, A, SEVEN, OUTPUT);
  push_button_leds[4].initialize(&expander_left, B, ONE, OUTPUT);
  push_button_leds[5].initialize(&expander_left, B, THREE, OUTPUT);
  push_button_leds[6].initialize(&expander_left, B, FIVE, OUTPUT);
  push_button_leds[7].initialize(&expander_left, B, SEVEN, OUTPUT);

  // Right push-button LEDs
  push_button_leds[8].initialize(&expander_right, A, ONE, OUTPUT);
  push_button_leds[9].initialize(&expander_right, A, THREE, OUTPUT);
  push_button_leds[10].initialize(&expander_right, A, FIVE, OUTPUT);
  push_button_leds[11].initialize(&expander_right, A, SEVEN, OUTPUT);
  push_button_leds[12].initialize(&expander_right, B, ONE, OUTPUT);
  push_button_leds[13].initialize(&expander_right, B, THREE, OUTPUT);
  push_button_leds[14].initialize(&expander_right, B, FIVE, OUTPUT);
  push_button_leds[15].initialize(&expander_right, B, SEVEN, OUTPUT);
}

void OutputHandler::initialize_panel_leds ()
{
  panel_leds[0].initialize(C, ZERO, OUTPUT);
  panel_leds[1].initialize(C, ONE, OUTPUT);
  panel_leds[2].initialize(C, TWO, OUTPUT);
  panel_leds[3].initialize(C, THREE, OUTPUT);
  panel_leds[4].initialize(C, FOUR, OUTPUT);
  panel_leds[5].initialize(C, FIVE, OUTPUT);
  panel_leds[6].initialize(C, SIX, OUTPUT);
  panel_leds[7].initialize(C, SEVEN, OUTPUT);
  for (int i = 0; i < 8; ++i)
    panel_leds[i].write(0);
}

/*
void OutputHandler::refresh_push_button_leds ()
{
  if (!memory->read(MACRO_TYPE) && timer.is_finished()) // add condition (minimum time)
    turn_off_push_button_leds();
  Request request = (Request)memory->read(REQUEST);
  if (request == NO_REQUEST)
	return;
  int index = get_push_button_index(request);
  if (index >= 0 && timer.is_finished()) // add condition (minimum time)
  {
    push_button_leds[index].write(1);
	timer.start();
  }
}
*/

void OutputHandler::refresh_push_button_leds ()
{
	static int count = 0;
	if (!memory->read(MACRO_IN_PROGRESS))
	{
		turn_off_push_button_leds();
		count = 0;
	}
	Request request = (Request)memory->read(REQUEST);
	if (request == NO_REQUEST)
	{
		return;
	}
	int index = get_push_button_index(request);
	if (index >= 0 && count == 0)
	{
		++count;
		push_button_leds[index].write(1);
	}
}

void OutputHandler::refresh_panel_leds ()
{
  if (memory->read(CONNECTED))
  {
    panel_leds[0].write(1);
	panel_leds[4].write(0);
  }
  else
  {
    panel_leds[0].write(0);
    panel_leds[4].write(1);
  }
  
  if (memory->read(TIMEOUT_IN_PROGRESS))
  {
	panel_leds[3].write(0);
	panel_leds[7].write(1);
  }
  else
  {
	panel_leds[3].write(1);
	panel_leds[7].write(0);
  }
  
  if (memory->read(MACRO_TYPE))
  {
	  panel_leds[2].write(0);
	  panel_leds[6].write(1);
  }
  else
  {
	  panel_leds[2].write(1);
	  panel_leds[6].write(0);
  }
}

void OutputHandler::turn_off_push_button_leds ()
{
  for (int i = 0; i < NUM_PUSH_BUTTONS; ++i)
    push_button_leds[i].write(0);
}

void OutputHandler::spiral ()
{
  int indices [] = {0, 1, 8, 9, 11, 13, 15, 14, 7, 6, 4, 2, 3, 10, 12, 5};
  const int pause = 50;
  for (int j = 0; j < 16; ++j)
  {
	int index = indices[15 - j];
	push_button_leds[index].write(1);
	_delay_ms(pause);
  }
  for (int j = 0; j < 16; ++j)
  {
	int index = indices[15 - j];
	push_button_leds[index].write(0);
	_delay_ms(pause);
  }
}

void OutputHandler::drop ()
{
  int inner [] = {3, 5, 10, 12};
  int outer [] = {0, 1, 2, 4, 6, 7, 8, 9, 11, 13, 14, 15};
  int pause = 200;
  for (int j = 0; j < 4; ++j)
    push_button_leds[inner[j]].write(1);
  _delay_ms(pause);
  for (int j = 0; j < 12; ++j)
    push_button_leds[outer[j]].write(1);
  _delay_ms(pause);
  for (int j = 0; j < 4; ++j)
    push_button_leds[inner[j]].write(0);
  _delay_ms(pause);
  for (int j = 0; j < 12; ++j)
    push_button_leds[outer[j]].write(0);
  _delay_ms(pause);
}