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

	// Event in a channel apropos a user in that channel.
	using ChanUser = void (const bot::Msg &, bot::Chan &, bot::User &);
	Handlers<Handler<ChanUser>> chan_user;

	// Event for a channel itself.
	using Chan = void (const bot::Msg &, bot::Chan &);
	Handlers<Handler<Chan>> chan;

	// Event for a user itself.
	using User = void (const bot::Msg &, bot::User &);
	Handlers<Handler<User>> user;

	// Raw message received from the server.
	using Msg = void (const bot::Msg &);
	Handlers<Handler<Msg>> msg;

	// Session state change
	using State = void (const Sess::State &);
	Handlers<Handler<State>> state_enter;
	Handlers<Handler<State>> state_leave;
};
