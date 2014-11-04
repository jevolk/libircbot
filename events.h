/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Events
{
	template<class Prototype> using Handler = handler::Handler<Prototype>;
	template<class Handler> using Handlers = handler::Handlers<Handler>;

	using ChanUser = void (const bot::Msg &, bot::Chan &, bot::User &);
	using Chan = void (const bot::Msg &, bot::Chan &);
	using User = void (const bot::Msg &, bot::User &);
	using Msg = void (const bot::Msg &);

	Handlers<Handler<ChanUser>> chan_user;
	Handlers<Handler<Chan>> chan;
	Handlers<Handler<User>> user;
	Handlers<Handler<Msg>> msg;

	std::function<void ()> disconnected;
	std::function<void ()> connected;
	std::function<void ()> timeout;
};
