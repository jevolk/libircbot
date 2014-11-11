/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"

using namespace irc::bot;


// irc::bot:: library extern base
decltype(irc::bot::locale) irc::bot::locale;                              // util.h
thread_local decltype(irc::bot::Stream::sbuf) irc::bot::Stream::sbuf;     // stream.h


Bot::Bot(const Opts &opts)
try:
opts(opts),
adb([&]
{
	if(!this->opts.get<bool>("database"))
		return std::string();

	mkdir(this->opts["dbdir"].c_str(),0777);
	return this->opts["dbdir"] + "/ircbot";
}()),
sess(static_cast<std::mutex &>(*this),this->opts),
users(adb,sess),
chans(adb,sess),
ns(adb,sess,users,chans),
cs(adb,sess,chans)
{
	namespace ph = std::placeholders;
	using flag_t = handler::flag_t;

	users.set_service(ns);
	chans.set_service(cs);

	if(this->opts.has("proxy"))
	{
		events.msg.add("HTTP/1.0",std::bind(&Bot::handle_http,this,ph::_1),flag_t(0),handler::Prio::LIB);
		events.msg.add("HTTP/1.1",std::bind(&Bot::handle_http,this,ph::_1),flag_t(0),handler::Prio::LIB);
	}

	#define EVENT(name,func)                                    \
		events.msg.add(name,std::bind(&Bot::func,this,ph::_1),  \
		               handler::RECURRING,                      \
		               handler::Prio::LIB);

	EVENT( handler::MISSING, handle_unhandled)

	EVENT( "ERROR", handle_error)
	EVENT( "QUIT", handle_quit)
	EVENT( "CAP", handle_cap)
	EVENT( "ACCOUNT", handle_account)
	EVENT( "PING", handle_ping)
	EVENT( "MODE", handle_mode)
	EVENT( "NICK", handle_nick)
	EVENT( "JOIN", handle_join)
	EVENT( "PART", handle_part)
	EVENT( "KICK", handle_kick)
	EVENT( "TOPIC", handle_topic)
	EVENT( "INVITE", handle_invite)
	EVENT( "NOTICE", handle_notice)
	EVENT( "ACTION", handle_action)
	EVENT( "PRIVMSG", handle_privmsg)
	EVENT( "AUTHENTICATE", handle_authenticate)
	EVENT( "CTCP", handle_ctcp)

	EVENT( LIBIRC_RFC_RPL_WELCOME, handle_welcome)
	EVENT( LIBIRC_RFC_RPL_YOURHOST, handle_yourhost)
	EVENT( LIBIRC_RFC_RPL_CREATED, handle_created)
	EVENT( LIBIRC_RFC_RPL_MYINFO, handle_myinfo)
	EVENT( LIBIRC_RFC_RPL_BOUNCE, handle_isupport)
	EVENT( LIBIRC_RFC_RPL_LIST, handle_list)
	EVENT( LIBIRC_RFC_RPL_LISTEND, handle_listend)
	EVENT( LIBIRC_RFC_RPL_NAMREPLY, handle_namreply)
	EVENT( LIBIRC_RFC_RPL_ENDOFNAMES, handle_endofnames)
	EVENT( LIBIRC_RFC_RPL_UMODEIS, handle_umodeis)
	EVENT( LIBIRC_RFC_RPL_ISON, handle_ison)
	EVENT( LIBIRC_RFC_RPL_AWAY, handle_away)
	EVENT( LIBIRC_RFC_RPL_WHOREPLY, handle_whoreply)
	EVENT( 354     /* RPL_WHOSPCRPL */, handle_whospecial)
	EVENT( LIBIRC_RFC_RPL_WHOISUSER, handle_whoisuser)
	EVENT( LIBIRC_RFC_RPL_WHOISIDLE, handle_whoisidle)
	EVENT( LIBIRC_RFC_RPL_WHOISSERVER, handle_whoisserver)
	EVENT( 671     /* RPL_WHOISSECURE */, handle_whoissecure)
	EVENT( 330     /* RPL_WHOISACCOUNT */, handle_whoisaccount)
	EVENT( LIBIRC_RFC_RPL_ENDOFWHOIS, handle_endofwhois)
	EVENT( LIBIRC_RFC_RPL_WHOWASUSER, handle_whowasuser)
	EVENT( LIBIRC_RFC_RPL_CHANNELMODEIS, handle_channelmodeis)
	EVENT( LIBIRC_RFC_RPL_TOPIC, handle_rpltopic)
	EVENT( LIBIRC_RFC_RPL_NOTOPIC, handle_notopic)
	EVENT( 333     /* RPL_TOPICWHOTIME */, handle_topicwhotime)
	EVENT( 329     /* RPL_CREATIONTIME */, handle_creationtime)
	EVENT( LIBIRC_RFC_RPL_BANLIST, handle_banlist)
	EVENT( LIBIRC_RFC_RPL_INVITELIST, handle_invitelist)
	EVENT( LIBIRC_RFC_RPL_EXCEPTLIST, handle_exceptlist)
	EVENT( 728     /* RPL_QUIETLIST */, handle_quietlist)
	EVENT( 730     /* RPL_MONONLINE */, handle_mononline)
	EVENT( 731     /* RPL_MONOFFLINE */, handle_monoffline)
	EVENT( 732     /* RPL_MONLIST */, handle_monlist)
	EVENT( 733     /* RPL_ENDOFMONLIST */, handle_endofmonlist)
	EVENT( 281     /* RPL_ACCEPTLIST */, handle_acceptlist)
	EVENT( 282     /* RPL_ENDOFACCEPT */, handle_endofaccept)
	EVENT( 710     /* RPL_KNOCK */, handle_knock)

	EVENT( 742     /* ERR_MODEISLOCKED */, handle_modeislocked)
	EVENT( 734     /* ERR_MONLISTFULL */, handle_monlistfull)
	EVENT( 456     /* ERR_ACCEPTFULL */, handle_acceptfull)
	EVENT( 457     /* ERR_ACCEPTEXIST */, handle_acceptexist)
	EVENT( 458     /* ERR_ACCEPTNOT */, handle_acceptnot)
	EVENT( LIBIRC_RFC_ERR_NOSUCHNICK, handle_nosuchnick)
	EVENT( 714     /* ERR_ALREADYONCHAN */, handle_alreadyonchan)
	EVENT( LIBIRC_RFC_ERR_USERONCHANNEL, handle_useronchannel)
	EVENT( LIBIRC_RFC_ERR_USERNOTINCHANNEL, handle_usernotinchannel)
	EVENT( LIBIRC_RFC_ERR_NICKNAMEINUSE, handle_nicknameinuse)
	EVENT( LIBIRC_RFC_ERR_UNKNOWNMODE, handle_unknownmode)
	EVENT( LIBIRC_RFC_ERR_CHANOPRIVSNEEDED, handle_chanoprivsneeded)
	EVENT( LIBIRC_RFC_ERR_ERRONEUSNICKNAME, handle_erroneusnickname)
	EVENT( LIBIRC_RFC_ERR_BANNEDFROMCHAN, handle_bannedfromchan)
	EVENT( LIBIRC_RFC_ERR_CANNOTSENDTOCHAN, handle_cannotsendtochan)

	#undef EVENT

	if(this->opts.get<bool>("connect"))
		connect();
}
catch(const Internal &e)
{
	std::cerr << "Bot::Bot(): " << e << std::endl;
}


Bot::~Bot(void)
noexcept try
{


}
catch(const Internal &e)
{
	std::cerr << "Bot::~Bot(): " << e << std::endl;
	return;
}


void Bot::operator()(const Loop &loop)
try
{
	switch(loop)
	{
		case FOREGROUND:
			recvq::worker();
			break;

		case BACKGROUND:
			recvq::min_threads(opts.get<size_t>("threads"));
			break;
	}
}
catch(const Internal &e)
{
	std::cerr << "\033[1;31mBot::run(): unhandled:\033[0m " << e << std::endl;
	throw;
}
catch(const Interrupted &e)
{
	return;
}


void Bot::connect(const milliseconds &to)
{
	namespace ph = std::placeholders;

	auto &sock = sess.get_socket();
	auto &sd = sock.get_sd();
	auto &ep = sock.get_ep();

	if(to > 0ms)
		set_timer(to);

	sd.async_connect(ep,std::bind(&Bot::handle_conn,this,ph::_1));
}


void Bot::quit()
{
	const Opts &opts = sess.get_opts();
	if(opts.get<bool>("quit"))
	{
		Quote(sess,"QUIT") << " :" << opts["quit-msg"];
		return;
	}

	Socket &sock = sess.get_socket();
	const bool hard = opts["quit"] == "hard";
	sock.disconnect(!hard);
}


void Bot::new_handle()
{
	set_handle(std::make_shared<boost::asio::streambuf>());
}


void Bot::set_handle(std::shared_ptr<boost::asio::streambuf> buf)
{
	namespace ph = std::placeholders;

	Socket &sock = sess.get_socket();
	const auto hf = std::bind(&Bot::handle_pck,this,ph::_1,ph::_2,buf);
	boost::asio::async_read_until(sock.get_sd(),*buf,"\r\n",hf);
}


void Bot::set_timeout()
{
	const auto timeout = opts.get<int64_t>("timeout");
	set_timer(milliseconds(timeout));
}


void Bot::set_timer(const milliseconds &ms)
{
	namespace ph = std::placeholders;

	if(ms < 0ms)
		return;

	Socket &sock = sess.get_socket();
	auto &timer = sock.get_timer();
	timer.expires_from_now(ms);
	timer.async_wait(std::bind(&Bot::handle_timeout,this,ph::_1));
}


bool Bot::cancel_timer()
{
	boost::system::error_code ec;
	Socket &sock = sess.get_socket();
	sock.get_timer().cancel_one(ec);
	return !ec;
}


void Bot::handle_timeout(const boost::system::error_code &e)
{
	if(e == boost::asio::error::operation_aborted)
		return;

	const std::lock_guard<Bot> lock(*this);
	if(events.timeout)
		events.timeout();

	Socket &sock = sess.get_socket();
	boost::system::error_code ec;
	sock.get_sd().cancel(ec);
}


void Bot::handle_conn(const boost::system::error_code &e)
{
	cancel_timer();

	if(e)
		throw Internal(e.value(),e.message());

	const std::lock_guard<Bot> lock(*this);
	if(events.connected)
		events.connected();

	if(opts.has("proxy"))
		sess.proxy();

	const FlushHold hold(sess);
	if(opts.get<bool>("caps"))
		sess.cap();

	if(opts.get<bool>("registration"))
		sess.reg();

	new_handle();
}


void Bot::handle_pck(const boost::system::error_code &e,
                     size_t size,
                     std::shared_ptr<boost::asio::streambuf> buf)
{
	if(e)
	{
		const std::lock_guard<Bot> lock(*this);
		if(events.disconnected)
			events.disconnected();

		throw Interrupted(e.value(),e.message());
	}

	std::istream istr(buf.get());
	const Msg msg(istr);

	const std::lock_guard<Bot> lock(*this);
	events.msg(msg);
	set_handle(buf);
}


void Bot::handle_http(const Msg &msg)
{
	using namespace fmt::HTTP;

	log_handle(msg,"HTTP");


}


void Bot::handle_ping(const Msg &msg)
{
	using namespace fmt::PING;

	log_handle(msg,"PING");

	Quote(sess,"PONG") << msg[SOURCE];
}


void Bot::handle_welcome(const Msg &msg)
{
	log_handle(msg,"WELCOME");

	const Opts &opts = sess.get_opts();

	if(opts.has("umode"))
		Quote(sess,"MODE") << sess.get_nick() << " " << opts["umode"];

	if(!opts["ns-acct"].empty() && !opts["ns-pass"].empty())
	{
		ns.identify(opts["ns-acct"],opts["ns-pass"]);
		return;
	}

	chans.autojoin();
}


void Bot::handle_yourhost(const Msg &msg)
{
	log_handle(msg,"YOURHOST");

}


void Bot::handle_created(const Msg &msg)
{
	log_handle(msg,"CREATED");

}


void Bot::handle_myinfo(const Msg &msg)
{
	using namespace fmt::MYINFO;

	log_handle(msg,"MYINFO");

	Server &server = sess.server;
	server.name = msg[SERVNAME];
	server.vers = msg[VERSION];
	server.user_modes = msg[USERMODS];
	server.chan_modes = msg[CHANMODS];
	server.chan_pmodes = msg[CHANPARM];
}


void Bot::handle_isupport(const Msg &msg)
{
	log_handle(msg,"ISUPPORT");

	Server &server = sess.server;
	const std::string &self = msg[0];
	for(size_t i = 1; i < msg.num_params(); i++)
	{
		const std::string &opt = msg[i];
		const size_t sep = opt.find('=');
		const std::string key = opt.substr(0,sep);
		const std::string val = sep == std::string::npos? "" : opt.substr(sep+1);

		if(std::all_of(key.begin(),key.end(),::isupper))
			server.isupport.emplace(key,val);
	}
}


void Bot::handle_authenticate(const Msg &msg)
{
	log_handle(msg,"AUTHENTICATE");


}


void Bot::handle_cap(const Msg &msg)
{
	using namespace fmt::CAP;

	log_handle(msg,"CAP");

	Server &server = sess.server;
	const auto caps = tokens(msg[CAPLIST]);
	switch(hash(msg[COMMAND]))
	{
		case hash("LS"):
			server.caps.insert(caps.begin(),caps.end());
			break;

		case hash("ACK"):
			sess.caps.insert(caps.begin(),caps.end());
			break;

		case hash("LIST"):
			sess.caps.clear();
			sess.caps.insert(caps.begin(),caps.end());
			break;

		case hash("NAK"):
			std::cerr << "UNSUPPORTED CAPABILITIES: " << msg[CAPLIST] << std::endl;
			break;

		default:
			throw Exception("Unhandled CAP response type");
	}
}


void Bot::handle_quit(const Msg &msg)
{
	using namespace fmt::QUIT;

	log_handle(msg,"QUIT");

	// We have quit
	if(msg.get_nick() == sess.get_nick())
		return;

	User &user = users.get(msg.get_nick());
	chans.for_each([&](Chan &chan)
	{
		events.chan_user(msg,chan,user);

		if(chan.users.del(user))
			user.dec_chans();
	});

	events.user(msg,user);

	if(user.num_chans() == 0)
		users.del(user);
}


void Bot::handle_nick(const Msg &msg)
{
	using namespace fmt::NICK;

	log_handle(msg,"NICK");

	const auto &old_nick = msg.get_nick();
	const auto &new_nick = msg[NICKNAME];

	if(old_nick == sess.get_nick())
	{
		const bool regained = !sess.is_desired_nick();
		sess.set_nick(new_nick);

		if(regained)  // Nick was regained on connect; nothing will exist in Users/chans.
			return;
	}

	users.rename(old_nick,new_nick);
	User &user = users.get(new_nick);
	chans.for_each([&](Chan &chan)
	{
		chan.users.rename(user,old_nick);
		events.chan_user(msg,chan,user);
	});

	events.user(msg,user);
}


void Bot::handle_account(const Msg &msg)
{
	using namespace fmt::ACCOUNT;

	log_handle(msg,"ACCOUNT");

	User &user = users.add(msg.get_nick());
	user.set_acct(msg[ACCTNAME]);
	events.user(msg,user);
}


void Bot::handle_join(const Msg &msg)
{
	using namespace fmt::JOIN;

	log_handle(msg,"JOIN");

	const auto &acct = sess.has_cap("extended-join")? msg[ACCTNAME] : std::string();
	User &user = users.add(msg.get_nick(),msg.get_host(),acct);
	Chan &chan = chans.add(msg[CHANNAME]);
	if(chan.users.add(user))
		user.inc_chans();

	if(msg.get_nick() == sess.get_nick())
	{
		// We have joined
		const Opts &opts = sess.get_opts();
		const FloodGuard guard(sess,opts.get<uint>("throttle-join"));
		chan.set_joined(true);

		if(opts.get<bool>("chan-fetch-mode"))
			chan.mode();

		if(opts.get<bool>("chan-fetch-who"))
			chan.who();

		if(opts.get<bool>("chan-fetch-info") && opts.get<bool>("services"))
			chan.csinfo();

		if(opts.get<bool>("chan-fetch-lists"))
		{
			chan.banlist();
			chan.quietlist();

			if(opts.get<bool>("services"))
				chan.accesslist();
		}
	}

	events.chan_user(msg,chan,user);
}


void Bot::handle_part(const Msg &msg)
{
	using namespace fmt::PART;

	log_handle(msg,"PART");

	User &user = users.get(msg.get_nick());
	Chan &chan = chans.get(msg[CHANNAME]);
	events.chan_user(msg,chan,user);

	if(chan.users.del(user))
		user.dec_chans();

	if(user.num_chans() == 0)
		users.del(user);

	if(msg.get_nick() == sess.get_nick())
		chans.del(chan);
}


void Bot::handle_mode(const Msg &msg)
{
	using namespace fmt::MODE;

	log_handle(msg,"MODE");

	// Our UMODE
	if(msg.num_params() == 1)
	{
		handle_umode(msg);
		return;
	}

	// Our UMODE coming as MODE formatted as UMODEIS (lol)
	if(msg.get_nick() == sess.get_nick() && msg[CHANNAME] == sess.get_nick())
	{
		handle_umodeis(msg);
		return;
	}

	Chan &chan = chans.get(msg[CHANNAME]);
	const Server &serv = sess.get_server();
	const Deltas deltas(detok(msg.begin()+1,msg.end()),serv);
	for(const Delta &d : deltas) try
	{
		chan.set_mode(d);

		// Channel's own mode
		if(std::get<Delta::MASK>(d).empty())
		{
			events.chan(msg,chan);
			continue;
		}

		// Target is a straight nickname
		if(std::get<Delta::MASK>(d) == Mask::INVALID)
		{
			User &user = users.get(std::get<Delta::MASK>(d));
			events.chan_user(msg,chan,user);
			continue;
		}
	}
	catch(const std::exception &e)
	{
		std::cerr << "Mode update failed: chan: " << chan.get_name()
		          << " (modestr: " << msg[DELTASTR] << ")"
		          << ": " << e.what()
		          << std::endl;
	}
}


void Bot::handle_umode(const Msg &msg)
{
	using namespace fmt::UMODE;

	log_handle(msg,"UMODE");

	sess.delta_mode(msg[DELTASTR]);
}


void Bot::handle_umodeis(const Msg &msg)
{
	using namespace fmt::UMODEIS;

	log_handle(msg,"UMODEIS");

	sess.delta_mode(msg[DELTASTR]);
}


void Bot::handle_ison(const Msg &msg)
{
	using namespace fmt::ISON;

	log_handle(msg,"ISON");

	const std::vector<std::string> list = tokens(msg[NICKLIST]);
}


void Bot::handle_away(const Msg &msg)
{
	using namespace fmt::AWAY;

	log_handle(msg,"AWAY");

	User &user = users.get(msg[NICKNAME]);
	user.set_away(true);
	events.user(msg,user);
}


void Bot::handle_channelmodeis(const Msg &msg)
{
	using namespace fmt::CHANNELMODEIS;

	log_handle(msg,"CHANNELMODEIS");

	const Server &serv = sess.get_server();
	Chan &chan = chans.get(msg[CHANNAME]);
	const Deltas deltas(msg[DELTASTR],serv);
	for(const auto &delta : deltas)
		chan.set_mode(delta);

	events.chan(msg,chan);
}


void Bot::handle_chanoprivsneeded(const Msg &msg)
{
	using namespace fmt::CHANOPRIVSNEEDED;

	log_handle(msg,"CHANOPRIVSNEEDED");

	Chan &chan = chans.get(msg[CHANNAME]);
	events.chan(msg,chan);
}


void Bot::handle_creationtime(const Msg &msg)
{
	using namespace fmt::CREATIONTIME;

	log_handle(msg,"CREATION TIME");

	Chan &chan = chans.get(msg[CHANNAME]);
	chan.set_creation(msg.get<time_t>(TIME));
	events.chan(msg,chan);
}


void Bot::handle_topicwhotime(const Msg &msg)
{
	using namespace fmt::TOPICWHOTIME;

	log_handle(msg,"TOPIC WHO TIME");

	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<Mask>(chan.set_topic()) = msg[MASK];
	std::get<time_t>(chan.set_topic()) = msg.get<time_t>(TIME);
	events.chan(msg,chan);
}


void Bot::handle_banlist(const Msg &msg)
{
	using namespace fmt::BANLIST;

	log_handle(msg,"BAN LIST");

	Chan &chan = chans.get(msg[CHANNAME]);
	chan.lists.bans.emplace(msg[BANMASK],msg[OPERATOR],msg.get<time_t>(TIME));
	events.chan(msg,chan);
}


void Bot::handle_invitelist(const Msg &msg)
{
	using namespace fmt::INVITELIST;

	log_handle(msg,"INVITE LIST");

	Chan &chan = chans.get(msg[CHANNAME]);
	chan.lists.invites.emplace(msg[MASK],msg[OPERATOR],msg.get<time_t>(TIME));
	events.chan(msg,chan);
}


void Bot::handle_exceptlist(const Msg &msg)
{
	using namespace fmt::EXCEPTLIST;

	log_handle(msg,"EXEMPT LIST");

	Chan &chan = chans.get(msg[CHANNAME]);
	chan.lists.excepts.emplace(msg[MASK],msg[OPERATOR],msg.get<time_t>(TIME));
	events.chan(msg,chan);
}


void Bot::handle_quietlist(const Msg &msg)
{
	using namespace fmt::QUIETLIST;

	log_handle(msg,"728 LIST");

	if(msg[MODECODE] != "q")
	{
		std::cout << "Received non 'q' mode for 728" << std::endl;
		return;
	}

	Chan &chan = chans.get(msg[CHANNAME]);
	chan.lists.quiets.emplace(msg[BANMASK],msg[OPERATOR],msg.get<time_t>(TIME));
	events.chan(msg,chan);
}


void Bot::handle_topic(const Msg &msg)
{
	using namespace fmt::TOPIC;

	log_handle(msg,"TOPIC");

	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<std::string>(chan.set_topic()) = msg[TEXT];
	events.chan(msg,chan);
}


void Bot::handle_rpltopic(const Msg &msg)
{
	using namespace fmt::RPLTOPIC;

	log_handle(msg,"RPLTOPIC");

	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<std::string>(chan.set_topic()) = msg[TEXT];
	events.chan(msg,chan);
}


void Bot::handle_notopic(const Msg &msg)
{
	using namespace fmt::NOTOPIC;

	log_handle(msg,"NOTOPIC");

	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<0>(chan.set_topic()) = {};
	std::get<1>(chan.set_topic()) = {};
	std::get<2>(chan.set_topic()) = {};
	events.chan(msg,chan);
}


void Bot::handle_kick(const Msg &msg)
{
	using namespace fmt::KICK;

	log_handle(msg,"KICK");

	const auto kicker = msg.get_nick();
	const auto &kickee = msg.num_params() > 1? msg[KICK::TARGET] : kicker;

	if(sess.get_nick() == kickee)
	{
		//TODO: move down
		//We have been kicked
		chans.del(msg[CHANNAME]);
		chans.join(msg[CHANNAME]);
		return;
	}

	User &user = users.get(kickee);
	Chan &chan = chans.get(msg[CHANNAME]);

	events.chan_user(msg,chan,user);
	events.chan(msg,chan);

	if(chan.users.del(user))
		user.dec_chans();

	if(user.num_chans() == 0)
		users.del(user);
}


void Bot::handle_chanmsg(const Msg &msg)
{
	using namespace fmt::CHANMSG;

	User &user = users.get(msg.get_nick());
	Chan &chan = chans.get(msg[CHANNAME]);
	events.chan_user(msg,chan,user);
}


void Bot::handle_privmsg(const Msg &msg)
{
	using namespace fmt::PRIVMSG;

	log_handle(msg,"PRIVMSG");

	if(msg[SELFNAME] != sess.get_nick())
	{
		handle_chanmsg(msg);
		return;
	}

	if(msg[TEXT].size() >= 2 && msg[TEXT].front() == 0x01 && msg[TEXT].back() == 0x01)
	{
		const Msg m("CTCP",msg.get_origin(),{between(msg[TEXT],"\0x01")});
		events.msg(m);
		return;
	}

	if(!users.has(msg.get_nick()))
	{
		// User is not in any channel with us.
		User user(adb,sess,ns,msg.get_nick(),msg.get_host());
		events.user(msg,user);
		return;
	}

	User &user = users.get(msg.get_nick());
	events.user(msg,user);
}


void Bot::handle_cnotice(const Msg &msg)
{
	using namespace fmt::CNOTICE;

	if(msg[CHANNAME] == "$$*")
	{
		chans.for_each([&]
		(Chan &chan)
		{
			chan << msg[TEXT] << Chan::flush;
			events.chan(msg,chan);
		});

		return;
	}

	const Server &serv = sess.get_server();
	const bool wch_prefix = serv.has_prefix(msg[CHANNAME].at(0));
	const auto chname = wch_prefix? msg[CHANNAME].substr(1) : msg[CHANNAME];
	Chan &chan = chans.get(chname);

	if(msg.from("chanserv"))
	{
		cs.handle_cnotice(msg,chan);
		return;
	}

	User &user = users.get(msg.get_nick());
	events.chan_user(msg,chan,user);
}


void Bot::handle_notice(const Msg &msg)
{
	using namespace fmt::NOTICE;

	log_handle(msg,"NOTICE");

	if(msg.from_server())
		return;

	if(msg[SELFNAME] != sess.get_nick())
	{
		handle_cnotice(msg);
		return;
	}

	if(msg.from("nickserv"))
	{
		ns.handle(msg);
		return;
	}

	if(msg.from("chanserv"))
	{
		cs.handle(msg);
		return;
	}

	if(!users.has(msg.get_nick()))
	{
		// User is not in any channel with us.
		User user(adb,sess,ns,msg.get_nick(),msg.get_host());
		events.user(msg,user);
		return;
	}

	User &user = users.get(msg.get_nick());
	events.user(msg,user);
}


void Bot::handle_caction(const Msg &msg)
{
	using namespace fmt::CACTION;

	User &user = users.get(msg.get_nick());
	Chan &chan = chans.get(msg[CHANNAME]);
	events.chan_user(msg,chan,user);

	//TODO: Move down
	if(!msg[TEXT].empty() && user.is_owner())
		handle_caction_owner(msg,chan,user);
}


void Bot::handle_action(const Msg &msg)
{
	using namespace fmt::ACTION;

	log_handle(msg,"ACTION");

	if(msg[SELFNAME] != sess.get_nick())
	{
		handle_caction(msg);
		return;
	}

	if(!users.has(msg.get_nick()))
	{
		// User is not in any channel with us.
		User user(adb,sess,ns,msg.get_nick(),msg.get_host());
		events.user(msg,user);
		return;
	}

	User &user = users.get(msg.get_nick());
	events.user(msg,user);
}


void Bot::handle_caction_owner(const Msg &msg,
                               Chan &chan,
                               User &user)
{
	using namespace fmt::CACTION;

	const auto tok = tokens(msg[TEXT]);
	switch(hash(tolower(tok.at(0))))
	{
		case hash("proves"):
			chan << Chan::PRIVMSG << user.get_nick() << ": I recognize you"
			     << " ($a:" << user.get_acct() << ")"
			     << " @ " << user.get_host()
			     << Chan::flush;
			break;
	}
}


void Bot::handle_invite(const Msg &msg)
{
	using namespace fmt::INVITE;

	log_handle(msg,"INVITE");

	const Opts &opts = sess.get_opts();
	if(!opts.get<bool>("invite"))
	{
		std::cerr << "Attempt at INVITE was ignored by configuration." << std::endl;
		return;
	}

	const time_t limit = opts.get<time_t>("invite-throttle");
	static time_t throttle = 0;
	if(time(NULL) - limit < throttle)
	{
		std::cerr << "Attempt at INVITE was throttled by configuration." << std::endl;
		return;
	}

	chans.join(msg[CHANNAME]);
	throttle = time(NULL);
}


void Bot::handle_ctcp(const Msg &msg)
{
	log_handle(msg,"CTCP");

	if(!users.has(msg.get_nick()))
	{
		// User is not in any channel with us.
		User user(adb,sess,ns,msg.get_nick(),msg.get_host());
		events.user(msg,user);
		return;
	}

	User &user = users.get(msg.get_nick());
	events.user(msg,user);
}


void Bot::handle_namreply(const Msg &msg)
{
	using namespace fmt::NAMREPLY;

	log_handle(msg,"NAM REPLY");

	const Server &serv = sess.get_server();
	Chan &chan = chans.add(msg[CHANNAME]);
	for(const auto &nick : tokens(msg[NAMELIST]))
	{
		auto modes = chan::nick_prefix(serv,nick);
		std::transform(modes.begin(),modes.end(),modes.begin(),[&]
		(const char &prefix)
		{
			return serv.prefix_to_mode(prefix);
		});

		const auto &raw_nick = nick.substr(modes.size());
		User &user = users.add(raw_nick);
		if(chan.users.add(user,modes))
			user.inc_chans();
	}

	events.chan(msg,chan);
}


void Bot::handle_endofnames(const Msg &msg)
{
	log_handle(msg,"END OF NAMES");
}


void Bot::handle_list(const Msg &msg)
{
	using namespace fmt::LIST;

	log_handle(msg,"LIST");
}


void Bot::handle_listend(const Msg &msg)
{
	log_handle(msg,"LIST END");
}


void Bot::handle_unknownmode(const Msg &msg)
{
	log_handle(msg,"UNKNOWN MODE");
}


void Bot::handle_bannedfromchan(const Msg &msg)
{
	log_handle(msg,"BANNED FROM CHAN");
}


void Bot::handle_alreadyonchan(const Msg &msg)
{
	using namespace fmt::ALREADYONCHAN;

	log_handle(msg,"ALREADY ON CHAN");
}


void Bot::handle_useronchannel(const Msg &msg)
{
	using namespace fmt::USERONCHANNEL;

	log_handle(msg,"USER ON CHAN");

	Chan &chan = chans.get(msg[CHANNAME]);
	User &user = users.get(msg[NICKNAME]);
	events.chan_user(msg,chan,user);
}


void Bot::handle_usernotinchannel(const Msg &msg)
{
	using namespace fmt::USERNOTINCHANNEL;

	log_handle(msg,"USERNOTINCHAN");

}


void Bot::handle_nicknameinuse(const Msg &msg)
{
	log_handle(msg,"NICK IN USE");

	const Opts &opts = sess.get_opts();
	if(!opts["ns-acct"].empty() && !opts["ns-pass"].empty())
	{
		const std::string randy(randstr(14));

		Quote nick(sess,"NICK");
		nick(randy);
		sess.set_nick(randy);
		ns.regain(opts["ns-acct"],opts["ns-pass"]);
		return;
	}
}


void Bot::handle_erroneusnickname(const Msg &msg)
{
	log_handle(msg,"ERRONEOUS NICKNAME");
	quit();
}


void Bot::handle_whoreply(const Msg &msg)
{
	log_handle(msg,"WHO REPLY");

	const std::string &selfserv = msg.get_nick();
	const std::string &self = msg[WHOREPLY::SELFNAME];
	const std::string &chan = msg[WHOREPLY::CHANNAME];
	const std::string &user = msg[WHOREPLY::USERNAME];
	const std::string &host = msg[WHOREPLY::HOSTNAME];
	const std::string &serv = msg[WHOREPLY::SERVNAME];
	const std::string &nick = msg[WHOREPLY::NICKNAME];
	const std::string &flag = msg[WHOREPLY::FLAGS];
	const std::string &addl = msg[WHOREPLY::ADDL];

}


void Bot::handle_whospecial(const Msg &msg)
{
	log_handle(msg,"WHO SPECIAL");

	const std::string &server = msg.get_nick();
	const std::string &self = msg[0];
	const int recipe = msg.get<int>(1);

	switch(recipe)
	{
		case User::WHO_RECIPE:
		{
			const std::string &host = msg[2];
			const std::string &nick = msg[3];
			const std::string &acct = msg[4];
			//const time_t idle = msg.get<time_t>(4);

			User &user = users.get(nick);
			user.set_host(host);
			user.set_acct(acct);
			//user.set_idle(idle);

			if(user.is_logged_in() && opts.get<bool>("database") && !user.Acct::exists())
				user.set_val("first_seen",time(NULL));

			events.user(msg,user);
			break;
		}

		default:
			throw Exception("WHO SPECIAL recipe unrecognized");
	}
}


void Bot::handle_whoisuser(const Msg &msg)
try
{
	using namespace fmt::WHOISUSER;

	log_handle(msg,"WHOIS USER");

	User &user = users.get(msg[NICKNAME]);
	user.set_host(msg[HOSTNAME]);
	events.user(msg,user);
}
catch(const Exception &e)
{
	std::cerr << "handle_whoisidle(): " << e << std::endl;
	return;
}


void Bot::handle_whoisidle(const Msg &msg)
try
{
	using namespace fmt::WHOISIDLE;

	log_handle(msg,"WHOIS IDLE");

	User &user = users.get(msg[NICKNAME]);
	user.set_idle(msg.get<time_t>(SECONDS));
	user.set_signon(msg.get<time_t>(SIGNON));
	events.user(msg,user);
}
catch(const Exception &e)
{
	std::cerr << "handle_whoisidle(): " << e << std::endl;
	return;
}


void Bot::handle_whoisserver(const Msg &msg)
{
	log_handle(msg,"WHOIS SERVER");

}


void Bot::handle_whoissecure(const Msg &msg)
try
{
	using namespace fmt::WHOISSECURE;

	log_handle(msg,"WHOIS SECURE");

	User &user = users.get(msg[NICKNAME]);
	user.set_secure(true);
	events.user(msg,user);
}
catch(const Exception &e)
{
	std::cerr << "handle_whoissecure(): " << e << std::endl;
	return;
}


void Bot::handle_whoisaccount(const Msg &msg)
try
{
	using namespace fmt::WHOISACCOUNT;

	log_handle(msg,"WHOIS ACCOUNT");

	User &user = users.get(msg[NICKNAME]);
	user.set_acct(msg[ACCTNAME]);
	events.user(msg,user);
}
catch(const Exception &e)
{
	std::cerr << "handle_whoisaccount(): " << e << std::endl;
	return;
}


void Bot::handle_whoischannels(const Msg &msg)
{
	log_handle(msg,"WHOIS CHANNELS");

}


void Bot::handle_endofwhois(const Msg &msg)
{
	log_handle(msg,"END OF WHOIS");

}


void Bot::handle_whowasuser(const Msg &msg)
{
	log_handle(msg,"WHOWAS USER");

}


void Bot::handle_monlist(const Msg &msg)
{
	using namespace fmt::MONLIST;

	log_handle(msg,"MONLIST");

	const auto nicks = tokens(msg[NICKLIST],",");
}


void Bot::handle_mononline(const Msg &msg)
{
	using namespace fmt::MONONLINE;

	log_handle(msg,"MONONLINE");

	const auto masks = tokens(msg[MASKLIST],",");
}


void Bot::handle_monoffline(const Msg &msg)
{
	using namespace fmt::MONOFFLINE;

	log_handle(msg,"MONOFFLINE");

	const auto nicks = tokens(msg[NICKLIST],",");
}


void Bot::handle_monlistfull(const Msg &msg)
{
	log_handle(msg,"MONLISTFULL");
}


void Bot::handle_endofmonlist(const Msg &msg)
{
	log_handle(msg,"ENDOFMONLIST");
}


void Bot::handle_acceptlist(const Msg &msg)
{
	using namespace fmt::ACCEPTLIST;

	log_handle(msg,"ACCEPTLIST");
}


void Bot::handle_acceptfull(const Msg &msg)
{
	using namespace fmt::ACCEPTFULL;

	log_handle(msg,"ACCEPTFULL");
}


void Bot::handle_acceptexist(const Msg &msg)
{
	using namespace fmt::ACCEPTEXIST;

	log_handle(msg,"ACCEPTEXIST");
}


void Bot::handle_acceptnot(const Msg &msg)
{
	using namespace fmt::ACCEPTNOT;

	log_handle(msg,"ACCEPTNOT");
}


void Bot::handle_endofaccept(const Msg &msg)
{
	log_handle(msg,"ENDOFACCEPT");
}


void Bot::handle_knock(const Msg &msg)
{
	using namespace fmt::KNOCK;

	log_handle(msg,"KNOCK");

	Chan &chan = chans.get(msg[CHANNAME]);
	events.chan(msg,chan);
}


void Bot::handle_nosuchnick(const Msg &msg)
{
	log_handle(msg,"NO SUCH NICK");

}


void Bot::handle_cannotsendtochan(const Msg &msg)
{
	log_handle(msg,"CANNOTSENDTOCHAN");

}


void Bot::handle_modeislocked(const Msg &msg)
{
	using namespace fmt::MODEISLOCKED;

	log_handle(msg,"MODE IS LOCKED");

	Chan &chan = chans.get(msg[CHANNAME]);
	events.chan(msg,chan);
}


void Bot::handle_error(const Msg &msg)
{
	throw Interrupted(msg[0]);
}


void Bot::handle_unhandled(const Msg &msg)
{
	log_handle(msg);

}


void Bot::log_handle(const Msg &msg,
                     const std::string &name)
const
{
	const std::string &n = name.empty()? msg.get_name() : name;
	std::cout << "<< " << std::setw(24) << std::setfill(' ') << std::left << n;
	std::cout << msg;
	std::cout << std::endl;
}


std::ostream &irc::bot::operator<<(std::ostream &s,
                                   const Bot &b)
{
	s << "Session: " << std::endl;
	s << b.sess << std::endl;
	s << b.chans << std::endl;
	s << b.users << std::endl;
	return s;
}
