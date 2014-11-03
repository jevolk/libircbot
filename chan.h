/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


enum Type
{
	SECRET,
	PRIVATE,
	PUBLIC
};

using Info = std::map<std::string, std::string>;
using Topic = std::tuple<std::string, Mask, time_t>;

Type type(const char &c);
char name_hat(const Server &serv, const std::string &nick); // "@nickname" then output = '@' (or null)


class Chan : public Locutor,
             public Acct
{
	Service *chanserv;
	Log _log;

	// State
	bool joined;                                            // Indication the server has sent us
	Mode _mode;                                             // Channel's mode state
	time_t creation;                                        // Timestamp for channel from server
	std::string pass;                                       // passkey for channel
	Topic _topic;                                           // Topic state
	Info info;                                              // ChanServ info response
	OpDo opq;                                               // Queue of things to do when op'ed

  public:
	Users users;                                            // Users container direct interface
	Lists lists;                                            // Bans/Quiets/etc direct interface

	auto &get_name() const                                  { return Locutor::get_target();         }
	auto &get_cs() const                                    { return *chanserv;                     }
	auto &get_log() const                                   { return _log;                          }
	auto &is_joined() const                                 { return joined;                        }
	auto &get_mode() const                                  { return _mode;                         }
	auto &get_creation() const                              { return creation;                      }
	auto &get_pass() const                                  { return pass;                          }
	auto &get_topic() const                                 { return _topic;                        }
	auto &get_info() const                                  { return info;                          }
	auto &get_opq() const                                   { return opq;                           }

	bool has_mode(const char &mode) const                   { return get_mode().has(mode);          }
	bool is_voice() const;
	bool is_op() const;

  protected:
	auto &get_cs()                                          { return *chanserv;                     }
	auto &get_log()                                         { return _log;                          }

	void run_opdo();
	void fetch_oplists();
	void event_opped();                                     // We have been opped up

  public:
	void log(const User &user, const Msg &msg)              { _log(user,msg);                       }
	void set_joined(const bool &joined)                     { this->joined = joined;                }
	void set_creation(const time_t &creation)               { this->creation = creation;            }
	void set_info(const decltype(info) &info)               { this->info = info;                    }
	auto &set_topic()                                       { return _topic;                        }
	bool set_mode(const Delta &d);
	template<class F> bool opdo(F&& f);                     // sudo <something as op> (happens async)

	// [SEND] State update interface
	void who(const std::string &fl = User::WHO_FORMAT);     // Update state of users in channel (goes into Users->User)
	void accesslist();                                      // ChanServ access list update
	void flagslist();                                       // ChanServ flags list update
	void akicklist();                                       // ChanServ akick list update
	void invitelist();                                      // INVEX update
	void exceptlist();                                      // EXCEPTS update
	void quietlist();                                       // 728 q list
	void banlist();
	void csinfo();                                          // ChanServ info update
	void names();                                           // Update user list of channel (goes into this->users)

	// [SEND] ChanServ interface to channel
	void csclear(const Mode &mode = "bq");                  // clear a list with a Mode vector
	void akick_del(const Mask &mask);
	void akick_del(const User &user);
	void akick(const Mask &mask, const std::string &ts = "", const std::string &reason = "");
	void akick(const User &user, const std::string &ts = "", const std::string &reason = "");
	void csunquiet(const Mask &mask);
	void csunquiet(const User &user);
	void csquiet(const Mask &user);
	void csquiet(const User &user);
	void csdevoice(const User &user);
	void csvoice(const User &user);
	void csdeop(const User &user);
	void csop(const User &user);
	void recover();                                         // recovery procedure
	void unban();                                           // unban self
	void csdevoice();                                       // target is self
	void csdeop();                                          // target is self
	void devoice();                                         // target is self
	void voice();                                           // target is self
	void deop();                                            // target is self
	void op();                                              // target is self

	// [SEND] Direct interface to channel
	void knock(const std::string &msg = "");
	void invite(const std::string &nick);
	void topic(const std::string &topic);
	void kick(const User &user, const std::string &reason = "");
	void remove(const User &user, const std::string &reason = "");
	Deltas unquiet(const User &user);
	Deltas quiet(const User &user);
	Deltas unban(const User &user);
	Deltas ban(const User &user);
	Delta devoice(const User &user);
	Delta voice(const User &user);
	Delta deop(const User &user);
	Delta op(const User &user);
	void part();                                            // Leave channel
	void join();                                            // Enter channel

	friend Chan &operator<<(Chan &c, const User &user);     // append "nickname: " to locutor stream
	friend User &operator<<(User &u, const Chan &chan);     // for CNOTICE / CPRIVMSG

	Chan(Adb *const &adb, Sess *const &sess, Service *const &chanserv, const std::string &name, const std::string &pass = "");
	virtual ~Chan() = default;

	friend std::ostream &operator<<(std::ostream &s, const Chan &chan);
};


inline
Chan::Chan(Adb *const &adb,
           Sess *const &sess,
           Service *const &chanserv,
           const std::string &name,
           const std::string &pass):
Locutor(sess,name),
Acct(adb,&Locutor::get_target()),
chanserv(chanserv),
_log(sess,name),
joined(false),
creation(0),
pass(pass)
{


}


inline
User &operator<<(User &user,
                 const Chan &chan)
{
	const Sess &sess = chan.get_sess();

	if(!chan.is_op())
		return user;

	if(user.get_meth() == user.NOTICE && !sess.isupport("CNOTICE"))
		return user;

	if(user.get_meth() == user.PRIVMSG && !sess.isupport("CPRIVMSG"))
		return user;

	user.Stream::clear();
	user << user.CMSG;
	user << chan.get_target() << "\n";
	return user;
}


inline
Chan &operator<<(Chan &chan,
                 const User &user)
{
	chan << user.get_nick() << ": ";
	return chan;
}


inline
void Chan::join()
{
	Quote out(get_sess(),"JOIN");
	out << get_name();

	if(!get_pass().empty())
		out << " " << get_pass();
}


inline
void Chan::part()
{
	Quote(get_sess(),"PART") << get_name();
}


inline
Delta Chan::op(const User &u)
{
	const Delta d = u.op();
	opdo(d);
	return d;
}


inline
Delta Chan::deop(const User &u)
{
	const Delta d = u.deop();
	opdo(d);
	return d;
}


inline
Delta Chan::voice(const User &u)
{
	Delta d = u.voice();
	opdo(d);
	return d;
}


inline
Delta Chan::devoice(const User &u)
{
	Delta d = u.devoice();
	opdo(d);
	return d;
}


inline
Deltas Chan::ban(const User &u)
{
	Deltas d;
	d.emplace_back(u.ban(Mask::HOST));

	if(u.is_logged_in())
		d.emplace_back(u.ban(Mask::ACCT));

	opdo(d);
	return d;
}


inline
Deltas Chan::unban(const User &u)
{
	Deltas d;

	if(lists.bans.count(u.mask(Mask::NICK)))
		d.emplace_back(u.unban(Mask::NICK));

	if(lists.bans.count(u.mask(Mask::HOST)))
		d.emplace_back(u.unban(Mask::HOST));

	if(u.is_logged_in() && lists.bans.count(u.mask(Mask::ACCT)))
		d.emplace_back(u.unban(Mask::ACCT));

	opdo(d);
	return d;
}


inline
Deltas Chan::quiet(const User &u)
{
	Deltas d;
	d.emplace_back(u.quiet(Mask::HOST));

	if(u.is_logged_in())
		d.emplace_back(u.quiet(Mask::ACCT));

	opdo(d);
	return d;
}


inline
Deltas Chan::unquiet(const User &u)
{
	Deltas d;

	if(lists.quiets.count(u.mask(Mask::NICK)))
		d.emplace_back(u.unquiet(Mask::NICK));

	if(lists.quiets.count(u.mask(Mask::HOST)))
		d.emplace_back(u.unquiet(Mask::HOST));

	if(u.is_logged_in() && lists.quiets.count(u.mask(Mask::ACCT)))
		d.emplace_back(u.unquiet(Mask::ACCT));

	opdo(d);
	return d;
}


inline
void Chan::remove(const User &user,
                  const std::string &reason)
{
	const auto &nick = user.get_nick();
	opdo([nick,reason](Chan &chan)
	{
		Quote(chan.get_sess(),"REMOVE") << chan.get_name() << " "  << nick << " :" << reason;
	});
}


inline
void Chan::kick(const User &user,
                const std::string &reason)
{
	const auto &nick = user.get_nick();
	opdo([nick,reason](Chan &chan)
	{
		Quote(chan.get_sess(),"KICK") << chan.get_name() << " "  << nick << " :" << reason;
	});
}


inline
void Chan::invite(const std::string &nick)
{

	const auto func = [nick](Chan &chan)
	{
		Quote(chan.get_sess(),"INVITE") << chan.get_name() << " " << nick;
	};

	if(has_mode('g'))
		func(*this);
	else
		opdo(func);
}


inline
void Chan::topic(const std::string &text)
{
	opdo([text](Chan &chan)
	{
		Quote out(chan.get_sess(),"TOPIC");
		out << chan.get_name();

		if(!text.empty())
			out << " :" << text;
	});
}


inline
void Chan::knock(const std::string &msg)
{
	Quote(get_sess(),"KNOCK") << get_name() << " :" << msg;
}


inline
void Chan::op()
{
	Service &cs = get_cs();
	cs << "OP " << get_name() << " " << get_my_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::deop()
{
	mode(Delta("-o",get_my_nick()));
}


inline
void Chan::voice()
{
	Service &cs = get_cs();
	cs << "VOICE " << get_name() << " " << get_my_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::devoice()
{
	opdo(Delta("-v",get_my_nick()));
}


inline
void Chan::csdeop()
{
	Service &cs = get_cs();
	cs << "DEOP " << get_name() << " " << get_my_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::csdevoice()
{
	Service &cs = get_cs();
	cs << "DEVOICE " << get_name() << " " << get_my_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::unban()
{
	Service &cs = get_cs();
	cs << "UNBAN " << get_name() << flush;
	cs.terminator_errors();
}


inline
void Chan::recover()
{
	Service &cs = get_cs();
	cs << "RECOVER " << get_name() << flush;
	cs.terminator_errors();
}


inline
void Chan::csop(const User &user)
{
	Service &cs = get_cs();
	cs << "OP " << get_name() << " " << user.get_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::csdeop(const User &user)
{
	Service &cs = get_cs();
	cs << "DEOP " << get_name() << " " << user.get_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::csvoice(const User &user)
{
	Service &cs = get_cs();
	cs << "VOICE " << get_name() << " " << user.get_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::csdevoice(const User &user)
{
	Service &cs = get_cs();
	cs << "DEVOICE " << get_name() << " " << user.get_nick() << flush;
	cs.terminator_errors();
}


inline
void Chan::csquiet(const User &user)
{
	csquiet(user.mask(Mask::HOST));

//	if(user.is_logged_in())
//		csquiet(user.get_acct());
}


inline
void Chan::csunquiet(const User &user)
{
	csunquiet(user.mask(Mask::HOST));

//	if(user.is_logged_in())
//		csunquiet(user.get_acct());
}


inline
void Chan::csquiet(const Mask &mask)
{
	Service &cs = get_cs();
	cs << "QUIET " << get_name() << " " << mask << flush;
	cs.terminator_errors();
}


inline
void Chan::csunquiet(const Mask &mask)
{
	Service &cs = get_cs();
	cs << "UNQUIET " << get_name() << " " << mask << flush;
	cs.terminator_errors();
}


inline
void Chan::akick(const User &user,
                 const std::string &ts,
                 const std::string &reason)
{
	akick(user.mask(Mask::HOST),ts,reason);

	if(user.is_logged_in())
		akick(user.mask(Mask::ACCT),ts,reason);
}


inline
void Chan::akick(const Mask &mask,
                 const std::string &ts,
                 const std::string &reason)
{
	Service &cs = get_cs();
	cs << "AKICK " << get_name() << " ADD " << mask;

	if(!ts.empty())
		cs << " !T " << ts;
	else
		cs << " !P";

	cs << " " << reason;
	cs << flush;
	cs.terminator_any();
}


inline
void Chan::akick_del(const Mask &mask)
{
	Service &cs = get_cs();
	cs << "AKICK " << get_name() << " DEL " << mask << flush;
	cs.terminator_any();
}


inline
void Chan::csclear(const Mode &mode)
{
	Service &cs = get_cs();
	cs << "clear " << get_name() << " BANS " << mode << flush;
	cs.terminator_any();
}


inline
void Chan::csinfo()
{
	Service &cs = get_cs();
	cs << "info " << get_name() << flush;
	cs.terminator_next("*** End of Info ***");
}


inline
void Chan::names()
{
	Quote out(get_sess(),"NAMES");
	out << get_name() << flush;
}


inline
void Chan::banlist()
{
	const auto &sess = get_sess();
	const auto &serv = sess.get_server();
	if(serv.chan_pmodes.find('b') == std::string::npos)
		return;

	mode("+b");
}


inline
void Chan::quietlist()
{
	const auto &sess = get_sess();
	const auto &serv = sess.get_server();
	if(serv.chan_pmodes.find('q') == std::string::npos)
		return;

	mode("+q");
}


inline
void Chan::exceptlist()
{
	const auto &sess = get_sess();
	const auto &isup = sess.get_isupport();
	mode(std::string("+") + isup.get("EXCEPTS",'e'));
}


inline
void Chan::invitelist()
{
	const auto &sess = get_sess();
	const auto &isup = sess.get_isupport();
	mode(std::string("+") + isup.get("INVEX",'I'));
}


inline
void Chan::flagslist()
{
	Service &cs = get_cs();
	cs << "flags " << get_name() << flush;

	std::stringstream ss;
	ss << "End of " << get_name() << " FLAGS listing.";
	cs.terminator_next(ss.str());
}


inline
void Chan::accesslist()
{
	Service &cs = get_cs();
	cs << "access " << get_name() << " list" << flush;

	std::stringstream ss;
	ss << "End of " << get_name() << " FLAGS listing.";
	cs.terminator_next(ss.str());
}


inline
void Chan::akicklist()
{
	Service &cs = get_cs();
	cs << "akick " << get_name() << " list" << flush;

	// This is the best we can do right now
	std::stringstream ss;
	ss << "Total of ";
	cs.terminator_next(ss.str());
}


inline
void Chan::who(const std::string &flags)
{
	Quote out(get_sess(),"WHO");
	out << get_name() << " " << flags << flush;
}


inline
bool Chan::set_mode(const Delta &delta)
try
{
	const auto &mask = std::get<Delta::MASK>(delta);
	const auto &sign = std::get<Delta::SIGN>(delta);
	const auto &mode = std::get<Delta::MODE>(delta);

	// Mode is for channel (TODO: special arguments)
	if(mask.empty())
	{
		_mode += delta;
		return true;
	}

	// Target is a straight nickname, not a Mask
	if(mask == Mask::INVALID)
	{
		if(mask == get_my_nick() && sign && mode == 'o')
			event_opped();

		users.mode(mask) += delta;
		return true;
	}

	return lists.set_mode(delta);
}
catch(const Exception &e)
{
	// Ignore user's absence from channel, though this shouldn't happen.
	std::cerr << "Chan: " << get_name() << " "
	          << "set_mode: " << delta << " "
	          << e << std::endl;

	return false;
}


template<class F>
bool Chan::opdo(F&& f)
{
	if(!is_op() && opq.empty())
		op();

	opq(std::forward<F>(f));

	if(!is_op())
		return false;

	run_opdo();
	return true;
}


inline
void Chan::event_opped()
{
	const Sess &sess = get_sess();
	const Opts &opts = sess.get_opts();
	if(opq.empty() && opts.get<bool>("chan-fetch-lists"))
		fetch_oplists();

	run_opdo();
}


inline
void Chan::fetch_oplists()
{
	const Sess &sess = get_sess();

	if(sess.isupport("INVEX"))
		invitelist();

	if(sess.isupport("EXCEPTS"))
		exceptlist();

	if(sess.has_opt("services"))
	{
		flagslist();
		akicklist();
	}
}


inline
void Chan::run_opdo()
{
	const scope clear([&]
	{
		opq.clear();
	});

	for(const auto &lambda : opq.get_lambdas()) try
	{
		lambda(*this);
	}
	catch(const Exception &e)
	{
		std::cerr << "Chan::run_opdo() lambda exception: " << e << std::endl;
	}

	auto &deltas = opq.get_deltas();

	const auto &sess = get_sess();
	const auto &acct = sess.get_acct();
	if(!acct.empty() && lists.has_flag(acct) && !lists.has_flag(acct,'O'))
		deltas.emplace_back("-o",get_my_nick());

	mode(deltas);
}


inline
bool Chan::is_voice()
const
{
	const auto &mode = users.mode(get_my_nick());
	return mode.has('v');
}


inline
bool Chan::is_op()
const
{
	const auto &mode = users.mode(get_my_nick());
	return mode.has('o');
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Chan &c)
{
	s << "name:       \t" << c.get_name() << std::endl;
	s << "passkey:    \t" << c.get_pass() << std::endl;
	s << "logfile:    \t" << c.get_log().get_path() << std::endl;
	s << "mode:       \t" << c.get_mode() << std::endl;
	s << "creation:   \t" << c.get_creation() << std::endl;
	s << "joined:     \t" << std::boolalpha << c.is_joined() << std::endl;
	s << "my privs:   \t" << (c.is_op()? "I am an operator." : "I am not an operator.") << std::endl;
	s << "topic:      \t" << std::get<std::string>(c.get_topic()) << std::endl;
	s << "topic by:   \t" << std::get<Mask>(c.get_topic()) << std::endl;
	s << "topic time: \t" << std::get<time_t>(c.get_topic()) << std::endl;

	s << "info:       \t" << c.info.size() << std::endl;
	for(const auto &kv : c.info)
		s << "\t" << kv.first << ":\t => " << kv.second << std::endl;

	s << c.lists << std::endl;
	s << c.users << std::endl;
	return s;
}



inline
Type type(const char &c)
{
	switch(c)
	{
		case '@':     return SECRET;
		case '*':     return PRIVATE;
		case '=':
		default:      return PUBLIC;
	};
}


inline
char name_hat(const Server &serv,
              const std::string &nick)
try
{
	const char &c = nick.at(0);
	return serv.has_prefix(c)? c : '\0';
}
catch(const std::out_of_range &e)
{
	return '\0';
}
