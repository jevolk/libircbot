/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Cork
{
	Socket &socket;

  public:
	Cork(Sess &sess):
	     socket(sess.get_socket())
	{
		socket.set_cork();
	}

	~Cork()
	{
		socket.unset_cork();
		if(!socket.has_cork())
			socket << socket.flush;
	}
};
