/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"

using namespace irc::bot;


// bot.h globals
std::locale irc::bot::locale;


Bot::Bot(const Opts &opts)
try:
adb([&]
{
	mkdir(opts["dbdir"].c_str(),0777);
	return opts["dbdir"] + "/ircbot";
}()),
sess(*this,opts,callbacks),
users(adb,sess),
chans(adb,sess),
ns(adb,sess,users),
cs(adb,sess,chans),
logs(chans,users,*this),
dispatch_thread(&Bot::dispatch_worker,this)
{
	users.set_service(ns);
	chans.set_service(cs);

	irc_set_ctx(sess,this);
}
catch(const Internal &e)
{
	std::cerr << "Bot::Bot(): " << e << std::endl;
}


Bot::~Bot(void)
noexcept try
{
	sess.disconn();
	dispatch_thread.join();
}
catch(const Internal &e)
{
	std::cerr << "Bot::~Bot(): " << e << std::endl;
	return;
}


void Bot::quit()
{
	Sess &sess = get_sess();
	sess.quit << ":Alea iacta est" << Stream::flush;
}


void Bot::run()
try
{
	Sess &sess = get_sess();
	irc_call(sess,irc_run);                            // Loops forever here
}
catch(const Internal &e)
{
	switch(e.code())
	{
		case 15:
			std::cout << "libircclient worker exiting: " << e << std::endl;
			return;

		default:
			std::cerr << "\033[1;31mBot::run(): unhandled:\033[0m " << e << std::endl;
			throw;
	}
}


void Bot::dispatch_worker()
try
{
	while(1)
	{
		const Msg msg = dispatch_next();
		const std::unique_lock<Bot> lock(*this);
		dispatch(msg);
	}
}
catch(const Internal &e)
{
	std::cout << "Dispatch worker exiting: " << e << std::endl;
	return;
}


Msg Bot::dispatch_next()
{
	std::unique_lock<std::mutex> lock(dispatch_mutex);
	dispatch_cond.wait(lock,[&]{ return !dispatch_queue.empty(); });
	const Msg ret = std::move(dispatch_queue.front());
	dispatch_queue.pop_front();
	return ret;
}


void Bot::dispatch(const Msg &msg)
try
{
	switch(msg.get_code())
	{
		case 0: switch(hash(msg.get_name()))
		{
			case hash("CTCP_ACTION"):      handle_ctcp_act(msg);            return;
			case hash("CTCP_REP"):         handle_ctcp_rep(msg);            return;
			case hash("CTCP_REQ"):         handle_ctcp_req(msg);            return;
			case hash("ACCOUNT"):          handle_account(msg);             return;
			case hash("INVITE"):           handle_invite(msg);              return;
			case hash("NOTICE"):           handle_notice(msg);              return;
			case hash("ACTION"):           handle_action(msg);              return;
			case hash("PRIVMSG"):          handle_privmsg(msg);             return;
			case hash("CHANNEL"):          handle_chanmsg(msg);             return;
			case hash("KICK"):             handle_kick(msg);                return;
			case hash("TOPIC"):            handle_topic(msg);               return;
			case hash("UMODE"):            handle_umode(msg);               return;
			case hash("MODE"):             handle_mode(msg);                return;
			case hash("PART"):             handle_part(msg);                return;
			case hash("JOIN"):             handle_join(msg);                return;
			case hash("QUIT"):             handle_quit(msg);                return;
			case hash("NICK"):             handle_nick(msg);                return;
			case hash("CAP"):              handle_cap(msg);                 return;
			case hash("AUTHENTICATE"):     handle_authenticate(msg);        return;
			case hash("CONNECT"):          handle_conn(msg);                return;
			case hash("ERROR"):            handle_error(msg);               return;
			default:                       handle_unhandled(msg);           return;
		}

		case LIBIRC_RFC_RPL_WELCOME:              handle_welcome(msg);                 return;
		case LIBIRC_RFC_RPL_YOURHOST:             handle_yourhost(msg);                return;
		case LIBIRC_RFC_RPL_CREATED:              handle_created(msg);                 return;
		case LIBIRC_RFC_RPL_MYINFO:               handle_myinfo(msg);                  return;
		case LIBIRC_RFC_RPL_BOUNCE:               handle_isupport(msg);                return;

		case LIBIRC_RFC_RPL_LIST:                 handle_list(msg);                    return;
		case LIBIRC_RFC_RPL_LISTEND:              handle_listend(msg);                 return;
		case LIBIRC_RFC_RPL_NAMREPLY:             handle_namreply(msg);                return;
		case LIBIRC_RFC_RPL_ENDOFNAMES:           handle_endofnames(msg);              return;
		case LIBIRC_RFC_RPL_UMODEIS:              handle_umodeis(msg);                 return;
		case LIBIRC_RFC_RPL_ISON:                 handle_ison(msg);                    return;
		case LIBIRC_RFC_RPL_AWAY:                 handle_away(msg);                    return;
		case LIBIRC_RFC_RPL_WHOREPLY:             handle_whoreply(msg);                return;
		case 354     /* RPL_WHOSPCRPL */:         handle_whospecial(msg);              return;
		case LIBIRC_RFC_RPL_WHOISUSER:            handle_whoisuser(msg);               return;
		case LIBIRC_RFC_RPL_WHOISIDLE:            handle_whoisidle(msg);               return;
		case LIBIRC_RFC_RPL_WHOISSERVER:          handle_whoisserver(msg);             return;
		case 671     /* RPL_WHOISSECURE */:       handle_whoissecure(msg);             return;
		case 330     /* RPL_WHOISACCOUNT */:      handle_whoisaccount(msg);            return;
		case LIBIRC_RFC_RPL_ENDOFWHOIS:           handle_endofwhois(msg);              return;
		case LIBIRC_RFC_RPL_WHOWASUSER:           handle_whowasuser(msg);              return;
		case LIBIRC_RFC_RPL_CHANNELMODEIS:        handle_channelmodeis(msg);           return;
		case LIBIRC_RFC_RPL_TOPIC:                handle_rpltopic(msg);                return;
		case LIBIRC_RFC_RPL_NOTOPIC:              handle_notopic(msg);                 return;
		case 333     /* RPL_TOPICWHOTIME */:      handle_topicwhotime(msg);            return;
		case 329     /* RPL_CREATIONTIME */:      handle_creationtime(msg);            return;
		case LIBIRC_RFC_RPL_BANLIST:              handle_banlist(msg);                 return;
		case LIBIRC_RFC_RPL_INVITELIST:           handle_invitelist(msg);              return;
		case LIBIRC_RFC_RPL_EXCEPTLIST:           handle_exceptlist(msg);              return;
		case 728     /* RPL_QUIETLIST */:         handle_quietlist(msg);               return;
		case 730     /* RPL_MONONLINE */:         handle_mononline(msg);               return;
		case 731     /* RPL_MONOFFLINE */:        handle_monoffline(msg);              return;
		case 732     /* RPL_MONLIST */:           handle_monlist(msg);                 return;
		case 733     /* RPL_ENDOFMONLIST */:      handle_endofmonlist(msg);            return;
		case 281     /* RPL_ACCEPTLIST */:        handle_acceptlist(msg);              return;
		case 282     /* RPL_ENDOFACCEPT */:       handle_endofaccept(msg);             return;
		case 710     /* RPL_KNOCK */:             handle_knock(msg);                   return;

		case 734     /* ERR_MONLISTFULL */:       handle_monlistfull(msg);             return;
		case 456     /* ERR_ACCEPTFULL */:        handle_acceptfull(msg);              return;
		case 457     /* ERR_ACCEPTEXIST */:       handle_acceptexist(msg);             return;
		case 458     /* ERR_ACCEPTNOT */:         handle_acceptnot(msg);               return;
		case LIBIRC_RFC_ERR_NOSUCHNICK:           handle_nosuchnick(msg);              return;
		case 714     /* ERR_ALREADYONCHAN */:     handle_alreadyonchan(msg);           return;
		case LIBIRC_RFC_ERR_USERONCHANNEL:        handle_useronchannel(msg);           return;
		case LIBIRC_RFC_ERR_USERNOTINCHANNEL:     handle_usernotinchannel(msg);        return;
		case LIBIRC_RFC_ERR_NICKNAMEINUSE:        handle_nicknameinuse(msg);           return;
		case LIBIRC_RFC_ERR_UNKNOWNMODE:          handle_unknownmode(msg);             return;
		case LIBIRC_RFC_ERR_CHANOPRIVSNEEDED:     handle_chanoprivsneeded(msg);        return;
		case LIBIRC_RFC_ERR_ERRONEUSNICKNAME:     handle_erroneusnickname(msg);        return;
		case LIBIRC_RFC_ERR_BANNEDFROMCHAN:       handle_bannedfromchan(msg);          return;
		default:                                  handle_unhandled(msg);               return;
	}
}
catch(const Internal &e)
{
	switch(e.code())
	{
		case -1:
			throw;

		default:
			std::cerr << "\033[1;37;41mDispatch Internal:\033[0m"
			          << " [\033[1;31m" << e << "\033[0m]"
			          << " Message: [\033[1;31m" << msg << "\033[0;0m]"
			          << std::endl;
	}
}
catch(const std::exception &e)
{
	std::cerr << "\033[1;37;41mDispatch Unhandled:\033[0m"
	          << " [\033[1;31m" << e.what() << "\033[0m]"
	          << " Message: [\033[1;31m" << msg << "\033[0;0m]"
	          << std::endl;
}


void Bot::handle_conn(const Msg &msg)
{
	log_handle(msg,"CONNECT");

	Sess &sess = get_sess();
	const Opts &opts = sess.get_opts();

	sess.cap("LS");
	sess.cap("REQ :account-notify extended-join");
	sess.cap("END");

	if(!opts["ns-acct"].empty() && !opts["ns-pass"].empty())
	{
		NickServ &ns = get_ns();
		ns.identify(opts["ns-acct"],opts["ns-pass"]);
		return;
	}

	Chans &chans = get_chans();
	chans.autojoin();
}


void Bot::handle_welcome(const Msg &msg)
{
	log_handle(msg,"WELCOME");

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

	Sess &sess = get_sess();
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

	Sess &sess = get_sess();
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
	using delim = boost::char_separator<char>;

	log_handle(msg,"CAP");

	Sess &sess = get_sess();
	Server &server = sess.server;

	static const delim sep(" ");
	const boost::tokenizer<delim> caps(msg[CAPLIST],sep);

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

	const std::string nick = msg.get_nick();
	const std::string &reason = msg[REASON];

	if(my_nick(nick))
		return;

	Users &users = get_users();
	User &user = users.get(nick);

	Chans &chans = get_chans();
	chans.for_each([&user,&msg]
	(Chan &chan)
	{
		chan.log(user,msg);
		chan.del(user);
	});
}


void Bot::handle_nick(const Msg &msg)
{
	using namespace fmt::NICK;

	log_handle(msg,"NICK");

	const std::string old_nick = msg.get_nick();
	const std::string &new_nick = msg[NICKNAME];

	Sess &sess = get_sess();
	if(my_nick(old_nick))
	{
		const bool regained = !sess.is_desired_nick();
		sess.set_nick(new_nick);

		if(regained)  // Nick was regained on connect; nothing will exist in Users/chans.
			return;
	}

	Users &users = get_users();
	users.rename(old_nick,new_nick);

	Chans &chans = get_chans();
	User &user = users.get(new_nick);
	chans.for_each([&user,&old_nick]
	(Chan &chan)
	{
		chan.rename(user,old_nick);
	});
}


void Bot::handle_account(const Msg &msg)
{
	using namespace fmt::ACCOUNT;

	log_handle(msg,"ACCOUNT");

	Users &users = get_users();
	User &user = users.add(msg.get_nick());
	user.set_acct(msg[ACCTNAME]);
}


void Bot::handle_join(const Msg &msg)
{
	using namespace fmt::JOIN;

	log_handle(msg,"JOIN");

	Sess &sess = get_sess();
	Chans &chans = get_chans();
	Users &users = get_users();

	User &user = users.add(msg.get_nick());
	if(sess.has_cap("extended-join"))
		user.set_acct(msg[ACCTNAME]);

	Chan &chan = chans.add(msg[CHANNAME]);
	chan.add(user);
	chan.log(user,msg);

	if(my_nick(msg.get_nick()))
	{
		chan.set_joined(true);
		chan.mode();
		chan.who();
		chan.csinfo();
		chan.banlist();
		chan.quietlist();
	}

	handle_join(msg,chan,user);
}


void Bot::handle_part(const Msg &msg)
{
	using namespace fmt::PART;

	log_handle(msg,"PART");

	Users &users = get_users();
	Chans &chans = get_chans();

	User &user = users.get(msg.get_nick());
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.log(user,msg);
	chan.del(user);

	if(my_nick(msg.get_nick()))
	{
		chans.del(chan);
		return;
	}

	const scope free([&]
	{
		if(user.num_chans() == 0)
			users.del(user);
	});

	handle_part(msg,chan,user);
}


void Bot::handle_mode(const Msg &msg)
{
	using namespace fmt::MODE;

	log_handle(msg,"MODE");

	// Our UMODE coming as MODE from the irclib
	if(msg.num_params() == 1)
	{
		handle_umode(msg);
		return;
	}

	// Our UMODE coming as MODE formatted as UMODEIS (was this protocol designed by committee?)
	if(my_nick(msg.get_nick()) && my_nick(msg[CHANNAME]))
	{
		handle_umodeis(msg);
		return;
	}

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);

	// Channel's own mode
	if(msg.num_params() < 3)
	{
		chan.delta_mode(msg[DELTASTR]);
		handle_mode(msg,chan);
		return;
	}

	// Channel mode apropos a user
	Users &users = get_users();
	const std::string sign(1,msg[DELTASTR].at(0));
	for(size_t d = 1, m = 2; m < msg.num_params(); d++, m++) try
	{
		// Associate each mode delta with its target
		const std::string delta = sign + msg[DELTASTR].at(d);
		const Mask &targ = msg[m];
		chan.delta_mode(delta,targ);          // Chan handles everything from here

		if(targ.get_form() == Mask::INVALID)  // Target is a straight nickname, we can call downstream
		{
			User &user = users.get(targ);
			handle_mode(msg,chan,user);
		}
	}
	catch(const std::exception &e)
	{
		std::cerr << "Mode update failed:"
		          << " chan: " << chan.get_name()
		          << " target[" << m << "]: " << msg[m]
		          << " (modestr: " << msg[DELTASTR] << ")"
		          << std::endl;
	}
}


void Bot::handle_umode(const Msg &msg)
{
	using namespace fmt::UMODE;

	log_handle(msg,"UMODE");

	if(!my_nick(msg.get_nick()))
		throw Exception("Server sent us umode for a different nickname");

	Sess &sess = get_sess();
	sess.delta_mode(msg[DELTASTR]);
}


void Bot::handle_umodeis(const Msg &msg)
{
	using namespace fmt::UMODEIS;

	log_handle(msg,"UMODEIS");

	if(!my_nick(msg[SELFNAME]))
		throw Exception("Server sent us umodeis for a different nickname");

	Sess &sess = get_sess();
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

	Users &users = get_users();
	User &user = users.get(msg[NICKNAME]);
	user.set_away(true);
}


void Bot::handle_channelmodeis(const Msg &msg)
{
	using namespace fmt::CHANNELMODEIS;

	log_handle(msg,"CHANNELMODEIS");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.delta_mode(msg[DELTASTR]);
}


void Bot::handle_chanoprivsneeded(const Msg &msg)
{
	using namespace fmt::CHANOPRIVSNEEDED;

	log_handle(msg,"CHANOPRIVSNEEDED");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan << Chan::NOTICE << "I need to be +o to do that. (" << msg[REASON] << ")" << Chan::flush;
}


void Bot::handle_creationtime(const Msg &msg)
{
	using namespace fmt::CREATIONTIME;

	log_handle(msg,"CREATION TIME");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.set_creation(msg.get<time_t>(TIME));
}


void Bot::handle_topicwhotime(const Msg &msg)
{
	using namespace fmt::TOPICWHOTIME;

	log_handle(msg,"TOPIC WHO TIME");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<Mask>(chan.get_topic()) = msg[MASK];
	std::get<time_t>(chan.get_topic()) = msg.get<time_t>(TIME);
}


void Bot::handle_banlist(const Msg &msg)
{
	using namespace fmt::BANLIST;

	log_handle(msg,"BAN LIST");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.bans.emplace(msg[BANMASK],msg[OPERATOR],msg.get<time_t>(TIME));
}


void Bot::handle_invitelist(const Msg &msg)
{
	using namespace fmt::INVITELIST;

	log_handle(msg,"INVITE LIST");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.invites.emplace(msg[MASK],msg[OPERATOR],msg.get<time_t>(TIME));
}


void Bot::handle_exceptlist(const Msg &msg)
{
	using namespace fmt::EXCEPTLIST;

	log_handle(msg,"EXEMPT LIST");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.excepts.emplace(msg[MASK],msg[OPERATOR],msg.get<time_t>(TIME));
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

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.quiets.emplace(msg[BANMASK],msg[OPERATOR],msg.get<time_t>(TIME));
}


void Bot::handle_topic(const Msg &msg)
{
	using namespace fmt::TOPIC;

	log_handle(msg,"TOPIC");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<std::string>(chan.get_topic()) = msg[TEXT];
}


void Bot::handle_rpltopic(const Msg &msg)
{
	using namespace fmt::RPLTOPIC;

	log_handle(msg,"RPLTOPIC");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<std::string>(chan.get_topic()) = msg[TEXT];
}


void Bot::handle_notopic(const Msg &msg)
{
	using namespace fmt::NOTOPIC;

	log_handle(msg,"NOTOPIC");

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	std::get<0>(chan.get_topic()) = {};
	std::get<1>(chan.get_topic()) = {};
	std::get<2>(chan.get_topic()) = {};
}


void Bot::handle_kick(const Msg &msg)
{
	using namespace fmt::KICK;

	log_handle(msg,"KICK");

	const std::string kicker = msg.get_nick();
	const std::string &kickee = msg.num_params() > 1? msg[KICK::TARGET] : kicker;

	Chans &chans = get_chans();
	Users &users = get_users();

	if(my_nick(kickee))
	{
		chans.del(msg[CHANNAME]);
		chans.join(msg[CHANNAME]);
		return;
	}

	User &user = users.get(kickee);
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.log(user,msg);
	chan.del(user);

	const scope free([&]
	{
		if(user.num_chans() == 0)
			users.del(user);
	});

	handle_kick(msg,chan,user);
}


void Bot::handle_chanmsg(const Msg &msg)
{
	using namespace fmt::CHANMSG;

	log_handle(msg,"CHANMSG");

	Chans &chans = get_chans();
	Users &users = get_users();

	User &user = users.get(msg.get_nick());
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.log(user,msg);

	handle_chanmsg(msg,chan,user);
}


void Bot::handle_privmsg(const Msg &msg)
{
	log_handle(msg,"PRIVMSG");

	Users &users = get_users();
	if(!users.has(msg.get_nick()))
	{
		// User is not in any channel with us.
		User user(get_adb(),get_sess(),get_ns(),msg.get_nick());
		handle_privmsg(msg,user);
		return;
	}

	User &user = users.get(msg.get_nick());
	handle_privmsg(msg,user);
}


void Bot::handle_cnotice(const Msg &msg)
{
	using namespace fmt::CNOTICE;

	Chans &chans = get_chans();

	if(msg[CHANNAME] == "$$*")
	{
		chans.for_each([&]
		(Chan &chan)
		{
			chan << msg[TEXT] << Chan::flush;
		});

		return;
	}

	const Sess &sess = get_sess();
	const Server &serv = sess.get_server();
	const bool wch_prefix = serv.has_prefix(msg[CHANNAME].at(0));
	const auto chname = wch_prefix? msg[CHANNAME].substr(1) : msg[CHANNAME];
	Chan &chan = chans.get(chname);

	if(msg.from_chanserv())
	{
		ChanServ &cs = get_cs();
		cs.handle_cnotice(msg,chan);
		return;
	}

	Users &users = get_users();
	User &user = users.get(msg.get_nick());
	chan.log(user,msg);
	handle_cnotice(msg,chan,user);
}


void Bot::handle_notice(const Msg &msg)
{
	using namespace fmt::NOTICE;

	log_handle(msg,"NOTICE");

	if(msg.from_server())
		return;

	if(!my_nick(msg[SELFNAME]))
	{
		handle_cnotice(msg);
		return;
	}

	if(msg.from_nickserv())
	{
		handle_notice_nickserv(msg);
		return;
	}

	if(msg.from_chanserv())
	{
		ChanServ &cs = get_cs();
		cs.handle(msg);
		return;
	}

	Users &users = get_users();
	if(!users.has(msg.get_nick()))
	{
		// User is not in any channel with us.
		User user(get_adb(),get_sess(),get_ns(),msg.get_nick());
		handle_notice(msg,user);
		return;
	}

	User &user = users.get(msg.get_nick());
	handle_notice(msg,user);
}


void Bot::handle_notice_nickserv(const Msg &msg)
{
	Sess &sess = get_sess();

	// Special case for joining
	if(!sess.is_identified() && msg[1].find("You are now identified") != std::string::npos)
	{
		NickServ &ns = get_ns();
		ns.clear_capture();
		ns.listchans();

		Chans &chans = get_chans();
		sess.set_identified(true);
		chans.autojoin();
		return;
	}

	NickServ &ns = get_ns();
	ns.handle(msg);
}


void Bot::handle_caction(const Msg &msg)
{
	using namespace fmt::CACTION;

	Chans &chans = get_chans();
	Users &users = get_users();

	User &user = users.get(msg.get_nick());
	Chan &chan = chans.get(msg[CHANNAME]);
	chan.log(user,msg);

	if(!msg[TEXT].empty() && user.is_owner())
		handle_caction_owner(msg,chan,user);

	handle_caction(msg,chan,user);
}


void Bot::handle_action(const Msg &msg)
{
	using namespace fmt::ACTION;

	log_handle(msg,"ACTION");

	if(!my_nick(msg[SELFNAME]))
	{
		handle_caction(msg);
		return;
	}

	Users &users = get_users();
	if(!users.has(msg.get_nick()))
	{
		// User is not in any channel with us.
		User user(get_adb(),get_sess(),get_ns(),msg.get_nick());
		handle_action(msg,user);
		return;
	}

	User &user = users.get(msg.get_nick());
	handle_action(msg,user);
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

	const Sess &sess = get_sess();
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

	Chans &chans = get_chans();
	chans.join(msg[CHANNAME]);
	throttle = time(NULL);
}


void Bot::handle_ctcp_req(const Msg &msg)
{
	log_handle(msg,"CTCP REQ");
}


void Bot::handle_ctcp_rep(const Msg &msg)
{
	log_handle(msg,"CTCP REP");
}


void Bot::handle_ctcp_act(const Msg &msg)
{
	log_handle(msg,"CTCP ACT");
}


void Bot::handle_namreply(const Msg &msg)
{
	using namespace fmt::NAMREPLY;

	log_handle(msg,"NAM REPLY");

	if(!my_nick(msg[NICKNAME]))
		throw Exception("Server replied to the wrong nickname");

	Users &users = get_users();
	Chans &chans = get_chans();
	Chan &chan = chans.add(msg[CHANNAME]);

	for(const auto &nick : tokens(msg[NAMELIST]))
	{
		const char f = Chan::nick_flag(nick);
		const char m = Chan::flag2mode(f);
		const std::string &rawnick = f? nick.substr(1) : nick;
		const std::string &modestr = m? std::string(1,m) : "";

		User &user = users.add(rawnick);
		chan.add(user,modestr);
	}
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

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan << "It seems " << msg[NICKNAME] << " " << msg[REASON] << "." << Chan::flush;
}


void Bot::handle_usernotinchannel(const Msg &msg)
{
	using namespace fmt::USERNOTINCHANNEL;

	log_handle(msg,"USERNOTINCHAN");

}


void Bot::handle_nicknameinuse(const Msg &msg)
{
	log_handle(msg,"NICK IN USE");

	Sess &sess = get_sess();
	const Opts &opts = sess.get_opts();

	if(!opts["ns-acct"].empty() && !opts["ns-pass"].empty())
	{
		NickServ &ns = get_ns();
		const std::string randy(randstr(14));
		sess.nick(randy);
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

			Users &users = get_users();
			User &user = users.get(nick);

			user.set_host(host);
			user.set_acct(acct);
			//user.set_idle(idle);

			if(user.is_logged_in() && !user.Acct::exists())
				user.set_val("first_seen",time(NULL));

			break;
		}

		default:
			throw Exception("WHO SPECIAL recipe unrecognized");
	}
}


void Bot::handle_whoisuser(const Msg &msg)
{
	log_handle(msg,"WHOIS USER");

}


void Bot::handle_whoisidle(const Msg &msg)
try
{
	using namespace fmt::WHOISIDLE;

	log_handle(msg,"WHOIS IDLE");

	Users &users = get_users();
	User &user = users.get(msg[NICKNAME]);
	user.set_idle(msg.get<time_t>(SECONDS));
	user.set_signon(msg.get<time_t>(SIGNON));
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

	Users &users = get_users();
	User &user = users.get(msg[NICKNAME]);
	user.set_secure(true);
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

	Users &users = get_users();
	User &user = users.get(msg[NICKNAME]);
	user.set_acct(msg[ACCTNAME]);
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

	Chans &chans = get_chans();
	Chan &chan = chans.get(msg[CHANNAME]);
	chan << "It seems " << msg[MASK] << " " << msg[REASON] << "." << Chan::flush;
}


void Bot::handle_nosuchnick(const Msg &msg)
{
	log_handle(msg,"NO SUCH NICK");

}


void Bot::handle_error(const Msg &msg)
{
	throw Internal(-1,msg[0]);
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
	std::cout << std::setw(15) << std::setfill(' ') << std::left << n;
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
