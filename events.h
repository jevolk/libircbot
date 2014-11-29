/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Events
{
	template<class Prototype> using Handler = handler::Handler<Prototype>;
	template<class Prototype> using Handlers = handler::Handlers<Handler<Prototype>>;

	// Event in a channel apropos a user in that channel.
	using ChanUser = void (const bot::Msg &, bot::Chan &, bot::User &);
	Handlers<ChanUser> chan_user;

	// Event for a channel itself.
	using Chan = void (const bot::Msg &, bot::Chan &);
	Handlers<Chan> chan;

	// Event for a user itself.
	using User = void (const bot::Msg &, bot::User &);
	Handlers<User> user;

	// Raw message received from the server.
	using Msg = void (const bot::Msg &);
	Handlers<Msg> msg;

	// Session state change
	using State = void (const bot::State &);
	Handlers<State> state_leave;
	Handlers<State> state;
};
