#include "communications.hpp"
#include "../uart/uart_0.hpp"
#include "../uart/uart_1.hpp"
#include "../uart/uart_2.hpp"
#include "../uart/uart_3.hpp"
#include "robot_addresses.hpp"
#include "../memory/memory_variables.hpp"
#include "request.hpp"

#define LANTRONIX_BAUD 115200UL
#define CLOCK_RATE 16000000UL
#define UART_ERROR(x) ((x) & 0xFF00)
const uint8_t FIRST_BYTE = 0x06;
const uint8_t SECOND_BYTE = 0x85;
const uint8_t ROBOT_ADDRESS = 0x04;
const uint8_t CONTROL_BOX_ADDRESS = 0x01;

#define when break;case
#define otherwise break;default

void Communications::initialize (UART_MODULE uart, Memory * memory)
{
  this->uart = uart;
  this->memory = memory;
  initialize_uart();
}

void Communications::receive ()
{
  read();
  memory->write(CONNECTED, get_num_messages());
  while (get_num_messages() > 0)
  {
    Message msg = get_next_message();
    uint16_t data = ((uint16_t)msg.first << 8) + msg.second;
    if (memory->valid_address(msg.address))
      memory->write(msg.address, data);
  }
}

void Communications::transmit ()
{
	const unsigned MAX_NUM_MESSAGES = 20;
	Message msgs [MAX_NUM_MESSAGES];
	unsigned num_messages = 0;
	Request latest_request = (Request)memory->read(REQUEST);
	if (latest_request == EMERGENCY_STOP)
	{
	  estop_msg(msgs, num_messages);
	  timeout.reset();
	  timeout.set_duration(1000); // reject all requests for 1 second
	  timeout.start();
	  memory->write(MACRO_IN_PROGRESS, 0);
	  memory->write(TIMEOUT_IN_PROGRESS, 1);
	}
	else if (memory->read(TRANSMIT_MACRO))
	{
		macro_msg(msgs, num_messages, (Request)memory->read(TRANSMIT_MACRO));
	}
	else if (timeout.is_finished() && !memory->read(MACRO_TYPE))
	{
		memory->write(TIMEOUT_IN_PROGRESS, 0);
		switch (latest_request)
		{
			case BUCKET_UP:      bucket_up_msg(msgs, num_messages);
			when BUCKET_DOWN:    bucket_down_msg(msgs, num_messages);
			when ARM_UP:         arm_up_msg(msgs, num_messages);
			when ARM_DOWN:       arm_down_msg(msgs, num_messages);
			when PLOW_UP:        plow_up_msg(msgs, num_messages);
			when PLOW_DOWN:      plow_down_msg(msgs, num_messages);

			// Macros
			when MACRO_0:        macro_msg(msgs, num_messages, MACRO_0);
			when MACRO_1:        macro_msg(msgs, num_messages, MACRO_1);
			when MACRO_2:        macro_msg(msgs, num_messages, MACRO_2);
			when MACRO_3:        macro_msg(msgs, num_messages, MACRO_3);

			// Joystick
			when JOYSTICK:       joystick_msg(msgs, num_messages);
			otherwise:           stop_all_msg(msgs, num_messages);
		}
		if (get_push_button_index(latest_request) >= 0)
		{
			// reject other requests for one second
			timeout.reset();
			timeout.set_duration(1000);
			timeout.start();
			memory->write(MACRO_IN_PROGRESS, 1);
			memory->write(TIMEOUT_IN_PROGRESS, 1);
		}
		else
		{
			memory->write(MACRO_IN_PROGRESS, 0);
			memory->write(TIMEOUT_IN_PROGRESS, 0);
		}
	}
  /*
	for (unsigned i = 0; i < num_messages; ++i)
	  send(msgs[i]);
  */
  send (msgs, num_messages);
}

void Communications::read ()
{
  uint16_t byte = read_byte();
  while (!UART_ERROR(byte))
  {
    parse(byte);
    byte = read_byte();
  }
}

void Communications::send (const Message & msg)
{
  send_byte(FIRST_BYTE);
  send_byte(SECOND_BYTE);
  send_byte(ROBOT_ADDRESS);
  send_byte(CONTROL_BOX_ADDRESS);
  send_byte(Message::LENGTH);
  send_byte(msg.address);
  send_byte(msg.first);    // switched the order so that least significant byte is send first
  send_byte(msg.second);
  send_byte(crc(msg));
}

void Communications::send (const Message * messages, uint8_t count)
{
  uint8_t cyclic_redundancy_check = crc(messages, count);
  send_byte(FIRST_BYTE);
  send_byte(SECOND_BYTE);
  send_byte(ROBOT_ADDRESS);
  send_byte(CONTROL_BOX_ADDRESS);
  send_byte(Message::LENGTH * count);
  for (int i = 0; i < count; ++i)
  {
    send_byte(messages->address);
    send_byte(messages->second);
    send_byte(messages->first);
    ++messages;
  }
  send_byte(cyclic_redundancy_check);
}

uint8_t Communications::get_num_messages () const
{
  return unread_messages.count();
}

Message Communications::get_next_message ()
{
  return unread_messages.pop();
}

void Communications::initialize_uart ()
{
  switch (uart)
  {
    case UART_0: uart0_initialize(UART0_BAUD_SELECT(LANTRONIX_BAUD, CLOCK_RATE));
    when UART_1: uart1_initialize(UART1_BAUD_SELECT(LANTRONIX_BAUD, CLOCK_RATE));
    when UART_2: uart2_initialize(UART2_BAUD_SELECT(LANTRONIX_BAUD, CLOCK_RATE));
    otherwise:   uart3_initialize(UART3_BAUD_SELECT(LANTRONIX_BAUD, CLOCK_RATE));
  } 
}

uint16_t Communications::read_byte ()
{
  switch (uart)
  {
    case UART_0: return uart0_read_byte();
    case UART_1: return uart1_read_byte();
    case UART_2: return uart2_read_byte();
    default:     return uart3_read_byte();
  } 
  return 0xFF00;
}

void Communications::send_byte (uint8_t value)
{
  switch (uart)
  {
    case UART_0: uart0_send_byte(value);
    when UART_1: uart1_send_byte(value);
    when UART_2: uart2_send_byte(value);
    otherwise:   uart3_send_byte(value);
  }
}

uint8_t Communications::crc (const Message messages [], uint8_t count)
{
	const uint8_t CRC_POLYNOMIAL = 0x8C;
	uint8_t value = 0x00;
	for (uint8_t i = 0; i < count; ++i)
	{
		uint8_t arr [] = {messages[i].address, messages[i].second, messages[i].first};
		for (uint8_t j = 0; j < 3; ++j)
		{
			uint8_t data = arr[j];
			for (uint8_t k = 0; k < 8; ++k)
			{
				uint8_t sum = (value ^ data) & 0x01;
				value >>= 1;
				if (sum)
					value ^= CRC_POLYNOMIAL;
				data >>= 1;
			}
		}
	}
	return value;
}

void Communications::parse (uint8_t byte)
{
  parser.push(byte);
  if (parser.valid_messages())
  {
    uint8_t max = parser.num_messages();
    for (uint8_t i = 0; i < max; ++i)
      unread_messages.push(parser.get_message(i));
    parser.clear();
  }
  else if (parser.is_full())
    parser.clear();
}

/* New message construction */

void Communications::stop_all_msg(Message messages [], unsigned & size)
{
	unsigned count = 0;
	for (uint8_t i = 1; i < 6; ++i)
	{
		messages[i - 1].address = i;
		messages[i - 1].first = 0;
		messages[i - 1].second = 0;
		++count;
	}
	size = count;
}

void Communications::create_msg (Message messages [], unsigned & size, uint8_t address, int speed)
{
	unsigned count = 0;
	for (uint8_t i = 1; i < 6; ++i)
	{
		if (i == address)
			continue;
		messages[i - 1].address = i;
		messages[i - 1].first = 0;
		messages[i - 1].second = 0;
		++count;
	}
	uint16_t unsigned_speed;
	if (speed < 0)
		unsigned_speed = ~(uint16_t)(speed * -1) + 1;
	else
		unsigned_speed = (uint16_t)speed;
	messages[address - 1].address = address;
	messages[address - 1].first = unsigned_speed;
	messages[address - 1].second = unsigned_speed >> 8;
	size = count + 1;
}

void Communications::estop_msg (Message messages [], unsigned & size)
{
	unsigned count = 0;
	for (uint8_t i = 1; i < 6; ++i)
	{
		messages[i - 1].address = i;
		messages[i - 1].first = 0;
		messages[i - 1].second = 0;
		++count;
	}
	messages[6].address = MACRO_COMMAND;
	messages[6].first = 0;
	messages[6].second = 0;
	size = count + 1;
	memory->write(TRANSMIT_MACRO, 0);
}

void Communications::joystick_msg (Message messages [], unsigned & size)
{
	static const double center = 512;
	static const double x_offset = 100;
	static const double y_offset = 50;
	static const double convert = 100.0 / 1023.0;
	static const uint16_t min_speed = 30;
	
	double x = (double)memory->read(JOYSTICK_LEFT_X);
	double y = (double)memory->read(JOYSTICK_LEFT_Y);
	uint16_t speed = 50;
	//uint16_t speed = memory->read(SLIDER_LEFT) * convert;
	speed = speed < min_speed ? min_speed : speed;
	
	uint16_t left_motor = 0;
	uint16_t right_motor = 0;
	if (x <= (center - x_offset))
	{
		left_motor = ~speed + 1;
		right_motor = speed;
	}
	else if (x > (center + x_offset))
	{
		left_motor = speed;
		right_motor = ~speed + 1;
	}
	else if (y < (center - y_offset))
	{
		//left_motor = right_motor = ~speed + 1;
		left_motor = right_motor = speed;
	}
	else if (y > (center + y_offset))
	{
		//left_motor = right_motor = speed;
		left_motor = right_motor = ~speed + 1;
	}
	messages[0].address = LEFT_MOTOR_SPEED;
	messages[0].first = left_motor;
	messages[0].second = left_motor >> 8;
	messages[1].address = RIGHT_MOTOR_SPEED;
	messages[1].first = right_motor;
	messages[1].second = right_motor >> 8;
	size = 2;
}

void Communications::bucket_up_msg (Message messages [], unsigned & size)
{
	create_msg(messages, size, BUCKET_ACTUATOR_SPEED, 20);
}

void Communications::bucket_down_msg (Message messages [], unsigned & size)
{
	create_msg(messages, size, BUCKET_ACTUATOR_SPEED, -20);
}

void Communications::arm_up_msg (Message messages [], unsigned & size)
{
	create_msg(messages, size, ARM_MOTOR_SPEED, 20);
}

void Communications::arm_down_msg (Message messages [], unsigned & size)
{
	create_msg(messages, size, ARM_MOTOR_SPEED, -20);
}

void Communications::plow_up_msg (Message messages [], unsigned & size)
{
	create_msg(messages, size, PLOW_SPEED_DIRECTION, 50);
}

void Communications::plow_down_msg (Message messages [], unsigned & size)
{
	create_msg(messages, size, PLOW_SPEED_DIRECTION, -50);
}

void Communications::macro_msg (Message messages [], unsigned & size, const Request & request)
{
	uint16_t command = 0;
	uint16_t argument = 0;
	switch (request)
	{
		case MACRO_0:
		command = 0x0001;
		argument = 0x0000;
		break;
		case MACRO_1:
		command = 0x0002;
		argument = 0x0000;
		break;
		case MACRO_2:
		command = 0x0003;
		argument = 0x0000;
		break;
		case MACRO_3:
		command = 0x0004;
		argument = 0x0000;
		break;
	}
	messages[0].address = MACRO_COMMAND;
	messages[0].first = command;
	messages[0].second = command >> 8;
	messages[1].address = MACRO_ARGUMENT;
	messages[1].first = argument;
	messages[1].second = argument >> 8;
	size = 2;
	memory->write(TRANSMIT_MACRO, (uint16_t)request);
}

/* Old message construction */
/*
void Communications::stop_all_msg (Message * messages, unsigned & size)
{
	unsigned count = 0;
	for (uint8_t i = 1; i <= 5; ++i)
	{
		messages->address = i;
		messages->first = 0;
		messages->second = 0;
		++messages;
		++count;
	}
	size = count;
}

void Communications::create_msg (Message * messages, unsigned & size, uint8_t address, int speed)
{
	unsigned count = 0;
	for (uint8_t i = 1; i <= 5; ++i)
	{
	  if (i == address)
	    continue;
	  messages->address = i;
	  messages->first = 0;
	  messages->second = 0;
	  ++messages;
	  ++count;
	}
	uint16_t unsigned_speed;
	if (speed < 0)
		unsigned_speed = ~(uint16_t)(speed * -1) + 1;
	else
		unsigned_speed = (uint16_t)speed;
	messages->address = address;
	messages->first = unsigned_speed >> 8;
	messages->second = unsigned_speed;
	size = count + 1;
}

void Communications::estop_msg (Message * messages, unsigned & size)
{
  	unsigned count = 0;
  	for (uint8_t i = 1; i <= 5; ++i)
  	{
	  	messages->address = i;
	  	messages->first = 0;
	  	messages->second = 0;
	  	++messages;
	  	++count;
  	}
	messages->address = MACRO_COMMAND;
	messages->first = 0;
	messages->second = 0;
	++count;
  	size = count;
	memory->write(TRANSMIT_MACRO, 0);
}

void Communications::bucket_up_msg (Message * messages, unsigned & size)
{
  create_msg(messages, size, BUCKET_ACTUATOR_SPEED, 20);
}

void Communications::bucket_down_msg (Message * messages, unsigned & size)
{
  create_msg(messages, size, BUCKET_ACTUATOR_SPEED, -20);
}

void Communications::arm_up_msg (Message * messages, unsigned & size)
{
  create_msg(messages, size, ARM_MOTOR_SPEED, 20);
}

void Communications::arm_down_msg (Message * messages, unsigned & size)
{
  create_msg(messages, size, ARM_MOTOR_SPEED, -20);
}

void Communications::plow_up_msg (Message * messages, unsigned & size)
{
  create_msg(messages, size, PLOW_SPEED_DIRECTION, 25);
}

void Communications::plow_down_msg (Message * messages, unsigned & size)
{
  create_msg(messages, size, PLOW_SPEED_DIRECTION, -25);
}

void Communications::joystick_msg (Message * messages, unsigned & size)
{
  static const double center = 512;
  static const double x_offset = 20;
  static const double y_offset = 20;
  static const double convert = 100.0 / 1023.0;
  static const uint16_t min_speed = 20;
  
  double x = (double)memory->read(JOYSTICK_LEFT_X);
  double y = (double)memory->read(JOYSTICK_LEFT_Y);
  uint16_t speed = memory->read(SLIDER_LEFT) * convert;
  speed = speed < min_speed ? min_speed : speed;
  
  uint16_t left_motor = 0;
  uint16_t right_motor = 0;
  if (x <= (center - x_offset))
  {
    right_motor = ~speed + 1;
	left_motor = speed;
  }
  else if (x > (center + x_offset))
  {
    right_motor = speed;
	left_motor = ~speed + 1;
  }
  else if (y < (center - y_offset))
  {
    left_motor = right_motor = ~speed + 1;
  }
  else if (y > (center + y_offset))
  {
    left_motor = right_motor = speed;
  }
  messages->address = LEFT_MOTOR_SPEED;
  messages->first = left_motor;
  messages->second = left_motor >> 8;
  ++messages;
  messages->address = RIGHT_MOTOR_SPEED;
  messages->first = right_motor;
  messages->second = right_motor >> 8;
  size = 2;
}

void Communications::macro_msg (Message * messages, unsigned & size, const Request & request)
{
  uint16_t command = 0;
  uint16_t argument = 0;
  switch (request)
  {
	  case MACRO_0:
		command = 0x0001;
		argument = 0x0000;
		break;
	  case MACRO_1:
	    command = 0x0002;
		argument = 0x0000;
		break;
	  case MACRO_2:
	    command = 0x0003;
		argument = 0x0000;
		break;
	  case MACRO_3:
	    command = 0x0004;
		argument = 0x0000;
		break;
  }
  messages->address = MACRO_COMMAND;
  messages->first = command >> 8;
  messages->second = command;
  ++messages;
  messages->address = MACRO_ARGUMENT;
  messages->first = argument >> 8;
  messages->second = argument;
  size = 2;
  memory->write(TRANSMIT_MACRO, (uint16_t)request);
}
*/