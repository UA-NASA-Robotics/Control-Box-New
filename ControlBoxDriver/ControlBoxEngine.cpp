#include "ControlBoxEngine.hpp"
#include <avr/interrupt.h>

#define F_CPU 16000000UL
#include <util/delay.h>

void ControlBoxEngine::initialize ()
{
  sei();
  memory.initialize();
  inputs.initialize(&memory);
  outputs.initialize(&memory);
  communications.initialize(UART_1, &memory);
}

void ControlBoxEngine::loop ()
{
  outputs.startup();
  while (1)
  {
    communications.receive();
    inputs.poll();
    communications.transmit();
    outputs.refresh();
  }
}
