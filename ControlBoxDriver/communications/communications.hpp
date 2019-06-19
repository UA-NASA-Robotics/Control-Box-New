#ifndef COMMUNICATIONS_HPP
#define COMMUNICATIONS_HPP

#include <stdint-gcc.h>
#include "message.hpp"
#include "message_queue.hpp"
#include "parser.hpp"
#include "../memory/memory.hpp"
#include "../uart/uart_module.hpp"
#include "../timer/SimpleTimer1.hpp"
#include "request.hpp"

class Communications
{
public:
  void initialize (UART_MODULE, Memory *);
  void receive ();
  void transmit ();
  void read ();
  void send (const Message &);
  void send (const Message *, uint8_t);
  uint8_t get_num_messages () const;
  Message get_next_message ();
private:
  void initialize_uart ();
  uint16_t read_byte ();
  void send_byte (uint8_t);
  uint8_t crc (const Message &);
  //uint8_t crc (const Message *, uint8_t);
  uint8_t crc (const Message [], uint8_t);
  void parse (uint8_t);

  // methods to construct messages
    void stop_all_msg (Message [], unsigned &);
    void create_msg (Message [], unsigned &, uint8_t, int);
    void estop_msg (Message [], unsigned &);
    void bucket_up_msg (Message [], unsigned &);
    void bucket_down_msg (Message [], unsigned &);
    void arm_up_msg (Message [], unsigned &);
    void arm_down_msg (Message [], unsigned &);
    void plow_up_msg (Message [], unsigned &);
    void plow_down_msg (Message [], unsigned &);
    void joystick_msg (Message [], unsigned &);
    void macro_msg (Message [], unsigned &, const Request &);
  /*
  void stop_all_msg (Message *, unsigned &);
  void create_msg (Message *, unsigned &, uint8_t, int);
  void estop_msg (Message *, unsigned &);
  void bucket_up_msg (Message *, unsigned &);
  void bucket_down_msg (Message *, unsigned &);
  void arm_up_msg (Message *, unsigned &);
  void arm_down_msg (Message *, unsigned &);
  void plow_up_msg (Message *, unsigned &);
  void plow_down_msg (Message *, unsigned &);
  void joystick_msg (Message *, unsigned &);
  void macro_msg (Message *, unsigned &, const Request &);
  */
private:
  UART_MODULE uart;
  Memory * memory;
  MessageQueue unread_messages;
  FastTransferParser parser;
  SimpleTimer1 timeout;
};

#endif // COMMUNICATIONS_HPP