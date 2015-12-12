/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Cork
{
	Cork()
	{
		auto &sock(get_sock());
		sock.set_cork();
	}

	~Cork()
	{
		auto &sock(get_sock());
		sock.unset_cork();
		if(!sock.has_cork())
			sock << Socket::flush;
	}
};
