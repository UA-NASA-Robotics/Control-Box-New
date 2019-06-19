#include <stdint-gcc.h>

class SimpleTimer1
{
public:
	SimpleTimer1 ();
	void start ();
	void restart ();
	void set_duration (uint32_t);
	bool is_finished ();
	void reset ();
};