/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class FloodGuard
{
	Socket &socket;
	const milliseconds saved;

  public:
	FloodGuard(Sess &sess, const uint64_t &ms):
	           FloodGuard(sess,milliseconds(ms)) {}

	FloodGuard(Sess &sess, const milliseconds &inc):
	           socket(sess.get_socket()), saved(socket.get_throttle().get_inc())
	{
		socket.set_throttle(inc);
	}

	~FloodGuard()
	{
		socket.set_throttle(saved);
	}
};


class FlushHold
{
	Socket &socket;

  public:
	FlushHold(Sess &sess):
	          socket(sess.get_socket())
	{
		socket.set_hold();
	}

	~FlushHold()
	{
		socket.unset_hold();

		if(!socket.has_hold())
			socket << socket.flush;
	}
};
