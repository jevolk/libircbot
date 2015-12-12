/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class FloodGuard
{
	milliseconds saved;

  public:
	FloodGuard(const uint64_t &ms):
	           FloodGuard(milliseconds(ms)) {}

	FloodGuard(const milliseconds &inc):
	           saved(get_sock().get_throttle().get_inc())
	{
		get_sock().set_throttle(inc);
	}

	~FloodGuard()
	{
		get_sock().set_throttle(saved);
	}
};
