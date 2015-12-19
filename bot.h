/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#ifndef LIBIRCBOT_INCLUDE
#define LIBIRCBOT_INCLUDE


#include <stdint.h>
#include <set>
#include <map>
#include <list>
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

// boost
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio.hpp>

// leveldb
#include <stldb/stldb.h>

// dry
#define INCLUDED_config_h
#include "ircd-seven/include/numeric.h"
#undef INCLUDED_config_h

// irc::bot
namespace irc {
namespace bot {

using namespace std::literals::chrono_literals;
using milliseconds = std::chrono::milliseconds;
using steady_clock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<steady_clock>;

namespace colors
{
	#include "colors.h"
}
#include "exception.h"
#include "util.h"
#include "opts.h"
#include "mask.h"
#include "throttle.h"
#include "isupport.h"
#include "server.h"
#include "delta.h"
#include "deltas.h"
#include "mode.h"
#include "ban.h"
#include "flags.h"
#include "akick.h"
#include "adoc.h"
#include "msg.h"
#include "state.h"
#include "stream.h"
namespace handler
{
	#include "handler.h"
	#include "handlers.h"
}

#include "adb.h"
extern thread_local Adb *adb;
inline auto &get_adb()                 { assert(adb); return *adb;             }
#include "acct.h"
namespace sendq
{
	#include "sendq.h"
}
namespace recvq
{
	#include "recvq.h"
}
#include "socket.h"
#include "sess.h"
extern thread_local Sess *sess;
inline auto &get_sess()                { assert(sess); return *sess;           }
inline auto &get_opts()                { return get_sess().get_opts();         }
inline auto &get_sock()                { return get_sess().get_socket();       }
#include "floodguard.h"
#include "cork.h"
#include "quote.h"
#include "cmds.h"
#include "locutor.h"
#include "service.h"
extern thread_local Service *chanserv;
extern thread_local Service *nickserv;
inline auto &get_cs()                  { assert(chanserv); return *chanserv;   }
inline auto &get_ns()                  { assert(nickserv); return *nickserv;   }
#include "user.h"
namespace chan
{
	class Chan;
	#include "chan_lists.h"
	#include "chan_users.h"
	#include "chan.h"
}
using Chan = chan::Chan;
#include "users.h"
extern thread_local Users *users;
inline auto &get_users()               { assert(users); return *users;         }
#include "chans.h"
extern thread_local Chans *chans;
inline auto &get_chans()               { assert(chans); return *chans;         }
#include "events.h"
#include "nickserv.h"
#include "chanserv.h"


/**
 * Primary libircbot object
 *
 * Usage:
 *	0. #include this file, and only this file, in your project.
 *	1. Add event handlers to the appropriate events structure.
 *	2. Fill in an 'Opts' options structure (opts.h) and instance of this bot in your project.
 *	3. Operate the controls:
 *		connect() - initiate the connection to server. Async (returns immediately).
 *		operator() - runs the event processing for the instance.
 *		
 * This class is protected by a simple mutex via the lock()/unlock() concept:
 *	- Mutex is locked when handling events.
 *		+ The handlers operate under this lock when called.
 *	- If you access this class asynchronously outside of the handler stack you must lock.
 *
 * To destruct the Bot instance from inside a handler, pass a lambda of your destruction
 * routine to sess.post().
 */
struct Bot : public std::mutex
{
	Opts opts;                                        // Options for this session
	Adb adb;                                          // Document database (local ldb)
	Sess sess;                                        // IRC client session
	Events events;                                    // Event handler registry
	Users users;                                      // Users state
	Chans chans;                                      // Channels state
	NickServ ns;                                      // NickServ service parser
	ChanServ cs;                                      // ChanServ service parser

	void set_tls_context();                           // Direct thread-locals at this instance.

  private:
	static void log(const State &state, const std::string &remarks = "");
	static void log(const Msg &m, const std::string &name = "");

	// IRC Handlers
	void handle_unhandled(const Msg &m);
	void handle_caction_owner(const Msg &m, Chan &c, User &u);
	void handle_ctcp_version(const Msg &m);
	void handle_ctcp(const Msg &m);
	void handle_modeislocked(const Msg &m);
	void handle_cannotsendtochan(const Msg &m);
	void handle_bannedfromchan(const Msg &m);
	void handle_chanoprivsneeded(const Msg &m);
	void handle_usernotinchannel(const Msg &m);
	void handle_channelisfull(const Msg &m);
	void handle_useronchannel(const Msg &m);
	void handle_alreadyonchan(const Msg &m);
	void handle_channelmodeis(const Msg &m);
	void handle_topicwhotime(const Msg &m);
	void handle_creationtime(const Msg &m);
	void handle_hosthidden(const Msg &m);
	void handle_endofnames(const Msg &m);
	void handle_namreply(const Msg &m);
	void handle_invitelist(const Msg &m);
	void handle_exceptlist(const Msg &m);
	void handle_quietlist(const Msg &m);
	void handle_banlist(const Msg &m);
	void handle_chan_privmsg(const Msg &m);
	void handle_chan_notice(const Msg &m);
	void handle_chan_action(const Msg &m);
	void handle_privmsg(const Msg &m);
	void handle_notice(const Msg &m);
	void handle_action(const Msg &m);
	void handle_inviting(const Msg &m);
	void handle_knock(const Msg &m);
	void handle_notopic(const Msg &m);
	void handle_rpltopic(const Msg &m);
	void handle_topic(const Msg &m);
	void handle_mode(const Msg &m);
	void handle_kick(const Msg &m);
	void handle_part(const Msg &m);
	void handle_join(const Msg &m);
	void handle_acceptnot(const Msg &m);
	void handle_acceptexist(const Msg &m);
	void handle_acceptfull(const Msg &m);
	void handle_endofaccept(const Msg &m);
	void handle_acceptlist(const Msg &m);
	void handle_monlistfull(const Msg &m);
	void handle_endofmonlist(const Msg &m);
	void handle_monoffline(const Msg &m);
	void handle_mononline(const Msg &m);
	void handle_monlist(const Msg &m);
	void handle_unknownmode(const Msg &m);
	void handle_nicknameinuse(const Msg &m);
	void handle_erroneusnickname(const Msg &m);
	void handle_nosuchnick(const Msg &m);
	void handle_whowasuser(const Msg &m);
	void handle_endofwhois(const Msg &m);
	void handle_whoischannels(const Msg &m);
	void handle_whoisaccount(const Msg &m);
	void handle_whoissecure(const Msg &m);
	void handle_whoisserver(const Msg &m);
	void handle_whoisidle(const Msg &m);
	void handle_whoisuser(const Msg &m);
	void handle_whospecial(const Msg &m);
	void handle_whoreply(const Msg &m);
	void handle_umodeis(const Msg &m);
	void handle_invite(const Msg &m);
	void handle_umode(const Msg &m);
	void handle_ison(const Msg &m);
	void handle_away(const Msg &m);
	void handle_nick(const Msg &m);
	void handle_quit(const Msg &m);
	void handle_authenticate(const Msg &m);
	void handle_account(const Msg &m);
	void handle_listend(const Msg &m);
	void handle_list(const Msg &m);
	void handle_cap(const Msg &m);
	void handle_myinfo(const Msg &m);
	void handle_created(const Msg &m);
	void handle_isupport(const Msg &m);
	void handle_yourhost(const Msg &m);
	void handle_welcome(const Msg &m);
	void handle_error(const Msg &m);
	void handle_ping(const Msg &m);
	void handle_http(const Msg &m);

	// Session state handlers
	void enter_state_unhandled(const State &s);
	void enter_state_active(const State &s);
	void enter_state_identifying(const State &s);
	void enter_state_registering(const State &s);
	void leave_state_negotiating(const State &s);
	void enter_state_negotiating(const State &s);
	void enter_state_proxying(const State &s);
	void enter_state_connecting(const State &s);
	void enter_state_inactive(const State &s);
	void enter_state_fault(const State &s);

	// IO Service handlers
	void handle_pck(const boost::system::error_code &e, const size_t size, const std::shared_ptr<boost::asio::streambuf> sbuf);
	void handle_conn(const boost::system::error_code &e);
	void handle_timeout(const boost::system::error_code &e);
	void handle_socket_ecb(const boost::system::error_code &e);

	// Inits
	void init_state_handlers();
	void init_irc_handlers();

  protected:
	// Controls
	void state(const State &state);                   // Jump to a State and call handlers
	void state_next();                                // Increment to next State and call handlers
	bool cancel_handle();                             // Cancel socket handles
	bool cancel_timer(const bool &all = false);       // Cancel pending timer(s)
	void set_handle(const std::shared_ptr<boost::asio::streambuf> buf);
	void set_timer(const milliseconds &ms);           // set a timer for anything
	void set_timeout();                               // set_timer(opts["timeout"])
	void new_handle();

  public:
	// Controls
	void connect();                                                              // LOCK OPTIONAL
	void disconnect();                                                           // LOCK REQUIRED
	void join(const std::string &chan)                { chans.join(chan);     }  // LOCK REQUIRED
	void quit();                                                                 // LOCK OPTIONAL

	// Execution
	enum Loop { FOREGROUND, BACKGROUND };
	void operator()(const Loop &loop = FOREGROUND);   // Run worker loop
	void operator()(const Msg &msg)                   { events.msg(msg);      }  // manual dispatch (lock required)

	Bot(void) = delete;
	Bot(const Opts &opts, boost::asio::io_service *const &ios = nullptr);
	Bot(Bot &&) = delete;
	Bot(const Bot &) = delete;
	Bot &operator=(Bot &&) = delete;
	Bot &operator=(const Bot &) = delete;

	friend std::ostream &operator<<(std::ostream &s, const Bot &bot);
};


}       // namespace bot
}       // namespace irc

#endif  // LIBIRCBOT_INCLUDE
