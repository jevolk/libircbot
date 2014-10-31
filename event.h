/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Handler
{
	enum Flag : uint8_t
	{
		RECUR     = 0x01,

	};

	Flag flags;


	Handler();
};


inline
Handler::Handler():
flags(0)
{


}
