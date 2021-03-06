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
using Lambda = std::function<void (Chan &)>;
using Lambdas = std::forward_list<Lambda>;
using JoinThrottle = std::pair<uint,time_t>;

Type type(const char &c);
std::string nick_prefix(const Server &serv, const std::string &nick);
std::string gen_cs_cmd(const std::string &chan, const Delta &delta);
std::string gen_cs_cmd(const std::string &chan, const Deltas &delta);


class Chan : public Locutor,
             public Acct
{
	bool joined;                                            // Indication the server has sent us
	Mode _mode;                                             // Channel's mode state
	time_t creation;                                        // Timestamp for channel from server
	std::string pass;                                       // passkey for channel
	std::string forward;                                    // forward channel
	JoinThrottle join_throttle;                             // join throttle +j users:seconds
	uint limit;                                             // users +l limit
	Topic _topic;                                           // Topic state
	Info info;                                              // ChanServ info response
	Deltas opdo_deltas;                                     // OpDo Delta queue
	Lambdas opdo_lambdas;                                   // OpDo Lambda queue

  public:
	Users users;                                            // Users container direct interface
	Lists lists;                                            // Bans/Quiets/etc direct interface

	auto &get_name() const                                  { return Locutor::get_target();         }
	auto &is_joined() const                                 { return joined;                        }
	auto &get_mode() const                                  { return _mode;                         }
	auto &get_creation() const                              { return creation;                      }
	auto &get_pass() const                                  { return pass;                          }
	auto &get_forward() const                               { return forward;                       }
	auto &get_join_throttle() const                         { return join_throttle;                 }
	auto &get_limit() const                                 { return limit;                         }
	auto &get_topic() const                                 { return _topic;                        }
	auto &get_info() const                                  { return info;                          }
	auto &get_opdo_deltas() const                           { return opdo_deltas;                   }
	auto &get_opdo_lambdas() const                          { return opdo_lambdas;                  }
	bool has_mode(const char &mode) const                   { return get_mode().has(mode);          }

	// Convenience checks for ourself
	bool is_flag(const char &flag) const;
	bool is_voice() const;
	bool is_op() const;

  protected:
	bool run_opdo();
	void fetch_oplists();
	void event_opped();                                     // We have been opped up
	bool set_user_mode(const Delta &d);
	bool set_join_throttle(const Delta &d);

  public:
	void set_joined(const bool &joined)                     { this->joined = joined;                }
	void set_creation(const time_t &creation)               { this->creation = creation;            }
	void set_info(const decltype(info) &info)               { this->info = info;                    }
	auto &set_topic()                                       { return _topic;                        }
	bool set_mode(const Delta &d);

	// [SEND] Execution interface
	bool opdo(const Lambda &lambda);                        // sudo <something as op> (happens async)
	bool opdo(const Delta &delta);                          // sudo delta execution as op (happens async)
	bool opdo(const Deltas &deltas);                        // sudo deltas execution as op (happens async)
	bool csdo(const Delta &delta);                          // fed to chanserv (must have +r) (false if not avail)
	void csdo(const Deltas &deltas);                        // fed to chanserv (must have +r) (opdo if not avail)
	void operator()(const Deltas &deltas);                  // best possible deltas execution (cs or op)
	void operator()(const Delta &delta);                    // best possible deltas execution (cs or op)

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
	void csclear(const Mode &mode = {"bq"});                // clear a list with a Mode vector
	void akick_del(const Mask &mask);
	void akick_del(const User &user);
	void akick(const Mask &mask, const std::string &ts = "", const std::string &reason = "");
	void akick(const User &user, const std::string &ts = "", const std::string &reason = "");
	void flags(const User &user, const Deltas &deltas);
	void flags(const User &user, const Delta &delta)        { flags(user,Deltas{delta});            }
	void recover();                                         // recovery procedure
	void unban();                                           // unban self

	// [SEND] Main interface to channel
	void knock(const std::string &msg = "");
	void invite(const std::string &nick);
	void topic(const std::string &topic);
	void kick(const User &user, const std::string &reason = "");
	void remove(const User &user, const std::string &reason = "");
	Deltas unexcept(const User &user);
	Deltas except(const User &user);
	Deltas uninvex(const User &user);
	Deltas invex(const User &user);
	Deltas unquiet(const User &user);
	Deltas quiet(const User &user);
	Deltas unban(const User &user);
	Deltas ban(const User &user);
	Delta devoice(const User &user);
	Delta voice(const User &user);
	Delta deop(const User &user);
	Delta op(const User &user);
	void devoice();                                         // target is self
	void voice();                                           // target is self
	void deop();                                            // target is self
	void op();                                              // target is self

	void part();                                            // Leave channel
	void join();                                            // Enter channel

	friend Chan &operator<<(Chan &c, const User &user);     // append "nickname: " to locutor stream
	friend User &operator<<(User &u, const Chan &chan);     // for CNOTICE / CPRIVMSG
	friend User &operator<<(User &u, const Chan *const &c); // for CNOTICE / CPRIVMSG

	explicit Chan(const std::string &name, const std::string &pass = {});
	Chan(Chan &&chan) noexcept;
	Chan(const Chan &chan);
	virtual ~Chan() = default;

	friend std::ostream &operator<<(std::ostream &s, const Chan &chan);
};


inline
Chan::Chan(const std::string &name,
           const std::string &pass):
Locutor(name),
Acct(&Locutor::get_target()),
joined(false),
creation(0),
pass(pass),
join_throttle{0,0},
limit(0)
{
}


inline
Chan::Chan(const Chan &chan):
Locutor(chan),
Acct(&Locutor::get_target()),
joined(chan.joined),
_mode(chan._mode),
creation(chan.creation),
pass(chan.pass),
forward(chan.forward),
join_throttle(chan.join_throttle),
limit(chan.limit),
_topic(chan._topic),
info(chan.info),
opdo_deltas(chan.opdo_deltas),
opdo_lambdas(chan.opdo_lambdas),
users(chan.users),
lists(chan.lists)
{
}


inline
Chan::Chan(Chan &&chan)
noexcept:
Locutor(std::move(chan)),
Acct(&Locutor::get_target()),
joined(std::move(chan.joined)),
_mode(std::move(chan._mode)),
creation(std::move(chan.creation)),
pass(std::move(chan.pass)),
forward(std::move(chan.forward)),
join_throttle(std::move(chan.join_throttle)),
limit(std::move(chan.limit)),
_topic(std::move(chan._topic)),
info(std::move(chan.info)),
opdo_deltas(std::move(chan.opdo_deltas)),
opdo_lambdas(std::move(chan.opdo_lambdas)),
users(std::move(chan.users)),
lists(std::move(chan.lists))
{
}


inline
User &operator<<(User &user,
                 const Chan &chan)
{
	const auto &sess(get_sess());

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
User &operator<<(User &user,
                 const Chan *const &chan)
{
	return chan? (user << (*chan)) : user;
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
	Quote out("JOIN");
	out << get_name();

	if(!get_pass().empty())
		out << " " << get_pass();
}


inline
void Chan::part()
{
	Quote("PART") << get_name();
}


inline
void Chan::op()
{
	csdo(Delta("+o",get_my_nick()));
}


inline
void Chan::deop()
{
	mode(Delta("-o",get_my_nick()));
}


inline
void Chan::voice()
{
	operator()(Delta("+v",get_my_nick()));
}


inline
void Chan::devoice()
{
	operator()(Delta("-v",get_my_nick()));
}


inline
Delta Chan::op(const User &u)
{
	const Delta d(u.op());
	operator()(d);
	return d;
}


inline
Delta Chan::deop(const User &u)
{
	const Delta d(u.deop());
	operator()(d);
	return d;
}


inline
Delta Chan::voice(const User &u)
{
	const Delta d(u.voice());
	operator()(d);
	return d;
}


inline
Delta Chan::devoice(const User &u)
{
	const Delta d(u.devoice());
	operator()(d);
	return d;
}


inline
Deltas Chan::invex(const User &u)
{
	Deltas d;

	if(u.is_logged_in())
		d.emplace_back(u.invex(Mask::ACCT));
	else
		d.emplace_back(u.invex(Mask::HOST));

	operator()(d);
	return d;
}


inline
Deltas Chan::uninvex(const User &user)
{
	const auto deltas(compose(lists.invites,user,"-I"));
	operator()(deltas);
	return deltas;
}


inline
Deltas Chan::except(const User &u)
{
	Deltas d;

	if(u.is_logged_in())
		d.emplace_back(u.except(Mask::ACCT));
	else
		d.emplace_back(u.except(Mask::HOST));

	operator()(d);
	return d;
}


inline
Deltas Chan::unexcept(const User &user)
{
	const auto deltas(compose(lists.excepts,user,"-e"));
	operator()(deltas);
	return deltas;
}


inline
Deltas Chan::ban(const User &u)
{
	Deltas d;
	d.emplace_back(u.ban(Mask::HOST));

	if(u.is_logged_in())
		d.emplace_back(u.ban(Mask::ACCT));

	operator()(d);
	return d;
}


inline
Deltas Chan::unban(const User &user)
{
	const auto deltas(compose(lists.bans,user,"-b"));
	operator()(deltas);
	return deltas;
}


inline
Deltas Chan::quiet(const User &u)
{
	Deltas d;
	d.emplace_back(u.quiet(Mask::HOST));

	if(u.is_logged_in())
		d.emplace_back(u.quiet(Mask::ACCT));

	if(users.has(u) && users.mode(u).has('v'))
		d.emplace_back(u.devoice());

	operator()(d);
	return d;
}


inline
Deltas Chan::unquiet(const User &user)
{
	const auto deltas(compose(lists.quiets,user,"-q"));
	operator()(deltas);
	return deltas;
}


inline
void Chan::remove(const User &user,
                  const std::string &reason)
{
	const auto &nick(user.get_nick());
	opdo([nick,reason](Chan &chan)
	{
		Quote("REMOVE") << chan.get_name() << " "  << nick << " :" << reason;
	});
}


inline
void Chan::kick(const User &user,
                const std::string &reason)
{
	const auto &nick(user.get_nick());
	opdo([nick,reason](Chan &chan)
	{
		Quote("KICK") << chan.get_name() << " "  << nick << " :" << reason;
	});
}


inline
void Chan::invite(const std::string &nick)
{

	const auto func([nick](Chan &chan)
	{
		Quote("INVITE") << nick << " " << chan.get_name();
	});

	if(has_mode('g'))
		func(*this);
	else
		opdo(func);
}


inline
void Chan::topic(const std::string &text)
{
	if(is_flag('t'))
	{
		Service &cs(get_cs());
		cs << "TOPIC " << get_name() << " " << text << flush;
		cs.terminator_errors();
		return;
	}

	opdo([text](Chan &chan)
	{
		Quote out("TOPIC");
		out << chan.get_name();

		if(!text.empty())
			out << " :" << text;
	});
}


inline
void Chan::knock(const std::string &msg)
{
	Quote("KNOCK") << get_name() << " :" << msg;
}


inline
void Chan::unban()
{
	Service &cs(get_cs());
	cs << "UNBAN " << get_name() << flush;
	cs.terminator_next("Unbanned");
}


inline
void Chan::recover()
{
	Service &cs(get_cs());
	cs << "RECOVER " << get_name() << flush;
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
	Service &cs(get_cs());
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
	Service &cs(get_cs());
	cs << "AKICK " << get_name() << " DEL " << mask << flush;
	cs.terminator_any();
}


inline
void Chan::flags(const User &user,
                 const Deltas &deltas)
{
	if(!user.is_logged_in())
		throw Assertive("Can't set flags on user: not logged in");

	Service &cs(get_cs());
	cs << "FLAGS " << get_name() << " " << user.get_acct() << " " << deltas << flush;

	std::stringstream terminator;
	terminator << "Flags " << deltas << " were set on " << user.get_acct() << " in " << get_name();
	cs.terminator_next(terminator.str());
}


inline
void Chan::csclear(const Mode &mode)
{
	Service &cs(get_cs());
	cs << "clear " << get_name() << " BANS " << mode << flush;
	cs.terminator_any();
}


inline
void Chan::csinfo()
{
	Service &cs(get_cs());
	cs << "info " << get_name() << flush;
	cs.terminator_next("*** End of Info ***");
}


inline
void Chan::names()
{
	Quote out("NAMES");
	out << get_name() << flush;
}


inline
void Chan::banlist()
{
	const auto &sess(get_sess());
	const auto &serv(sess.get_server());
	if(serv.chan_pmodes.find('b') == std::string::npos)
		return;

	mode("+b");
}


inline
void Chan::quietlist()
{
	const auto &sess(get_sess());
	const auto &serv(sess.get_server());
	if(serv.chan_pmodes.find('q') == std::string::npos)
		return;

	mode("+q");
}


inline
void Chan::exceptlist()
{
	const auto &sess(get_sess());
	const auto &isup(sess.get_isupport());
	mode(std::string("+") + isup.get("EXCEPTS",'e'));
}


inline
void Chan::invitelist()
{
	const auto &sess(get_sess());
	const auto &isup(sess.get_isupport());
	mode(std::string("+") + isup.get("INVEX",'I'));
}


inline
void Chan::flagslist()
{
	Service &cs(get_cs());
	cs << "flags " << get_name() << flush;

	std::stringstream ss;
	ss << "End of " << get_name() << " FLAGS listing.";
	cs.terminator_next(ss.str());
}


inline
void Chan::accesslist()
{
	Service &cs(get_cs());
	cs << "access " << get_name() << " list" << flush;

	std::stringstream ss;
	ss << "End of " << get_name() << " FLAGS listing.";
	cs.terminator_next(ss.str());
}


inline
void Chan::akicklist()
{
	Service &cs(get_cs());
	cs << "akick " << get_name() << " list" << flush;

	// This is the best we can do right now
	std::stringstream ss;
	ss << "Total of ";
	cs.terminator_next(ss.str());
}


inline
void Chan::who(const std::string &flags)
{
	Quote out("WHO");
	out << get_name() << " " << flags << flush;
}


inline
bool Chan::set_mode(const Delta &d)
try
{
	using std::get;

	const auto &sess(get_sess());
	const auto &serv(sess.get_server());
	switch(serv.mode_type(d))
	{
		case Delta::Type::A:
			return lists.set_mode(d);

		case Delta::Type::B: switch(get<d.MODE>(d))
		{
			case 'k':
				_mode += 'k';
				pass = get<d.MASK>(d);
				return true;

			default:
				if(get<d.MASK>(d) == Mask::INVALID)
					return set_user_mode(d);

				return false;
		}

		case Delta::Type::C: switch(get<d.MODE>(d))
		{
			case 'l':
				limit = bool(d)? lex_cast<uint>(get<d.MASK>(d)) : 0U;
				return true;

			case 'f':
				forward = bool(d)? get<d.MASK>(d) : Mask{};
				return true;

			case 'j':
				return set_join_throttle(d);

			default:
				return false;
		}

		default:
		case Delta::Type::D:
			_mode += d;
			return true;
	}
}
catch(const Exception &e)
{
	std::cerr << "Chan: " << get_name() << " "
	          << "set_mode: [" << d << "] "
	          << e << std::endl;

	return false;
}


inline
bool Chan::set_join_throttle(const Delta &d)
{
	if(bool(d))
	{
		const auto arg(split(std::get<d.MASK>(d),":"));
		join_throttle.first = lex_cast<uint>(arg.first);
		join_throttle.second = lex_cast<time_t>(arg.second);
	}
	else join_throttle = {0,0};

	return true;
}


inline
bool Chan::set_user_mode(const Delta &d)
try
{
	using std::get;

	users.mode(get<d.MASK>(d)) += d;
	if(get<d.MASK>(d) == get_my_nick() && get<d.SIGN>(d) && get<d.MODE>(d) == 'o')
		event_opped();

	return true;
}
catch(const std::out_of_range &e)
{
	std::cerr << get_name() << " set_user_mode(" << d << ") failed" << std::endl;
	return false;
}


inline
void Chan::operator()(const Deltas &deltas)
{
	csdo(deltas);
}


inline
void Chan::operator()(const Delta &delta)
{
	if(!csdo(delta))
		opdo(delta);
}


inline
void Chan::csdo(const Deltas &deltas)
{
	const auto cmd(gen_cs_cmd(get_name(),deltas));
	if(!cmd.empty())
	{
		Service &cs(get_cs());
		cs << cmd << flush;
		cs.terminator_errors();
		return;
	}

	Deltas remain;
	for(const auto &delta : deltas)
		if(!csdo(delta))
			remain.emplace_back(delta);

	opdo(remain);
}


inline
bool Chan::csdo(const Delta &delta)
{
	using std::get;

	const auto cmd(gen_cs_cmd(get_name(),delta));
	if(cmd.empty())
		return false;

	Service &cs(get_cs());
	cs << cmd << flush;
	cs.terminator_errors();
	return true;
}


inline
bool Chan::opdo(const Deltas &deltas)
{
	if(!is_op() && opdo_deltas.empty() && opdo_lambdas.empty())
		op();

	opdo_deltas.insert(opdo_deltas.end(),deltas.begin(),deltas.end());
	return run_opdo();
}


inline
bool Chan::opdo(const Delta &delta)
{
	if(!is_op() && opdo_deltas.empty() && opdo_lambdas.empty())
		op();

	opdo_deltas.emplace_back(delta);
	return run_opdo();
}


inline
bool Chan::opdo(const Lambda &lambda)
{
	if(!is_op() && opdo_deltas.empty() && opdo_lambdas.empty())
		op();

	opdo_lambdas.emplace_front(lambda);
	return run_opdo();
}


inline
void Chan::event_opped()
{
	const Sess &sess(get_sess());
	const Opts &opts(sess.get_opts());
	const auto opq_empty(opdo_deltas.empty() && opdo_lambdas.empty());
	if(opq_empty && opts.get<bool>("chan-fetch-lists"))
		fetch_oplists();

	run_opdo();
}


inline
void Chan::fetch_oplists()
{
	const Sess &sess(get_sess());

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
bool Chan::run_opdo()
{
	if(!is_op())
		return false;

	const scope clear([&]
	{
		opdo_deltas.clear();
		opdo_lambdas.clear();
	});

	for(const auto &lambda : opdo_lambdas) try
	{
		lambda(*this);
	}
	catch(const Exception &e)
	{
		std::cerr << "Chan::run_opdo() lambda exception: " << e << std::endl;
	}

	const auto &sess(get_sess());
	const auto &acct(sess.get_acct());
	if(!acct.empty() && lists.has_flag(acct) && !is_flag('O'))
		opdo_deltas.emplace_back("-o",get_my_nick());

	mode(opdo_deltas);
	return true;
}


inline
bool Chan::is_flag(const char &flag)
const
{
	const auto &sess(get_sess());
	const auto &acct(sess.get_acct());
	return lists.has_flag(acct,flag);
}


inline
bool Chan::is_voice()
const
{
	return users.mode(get_my_nick()).has('v');
}


inline
bool Chan::is_op()
const
{
	return users.mode(get_my_nick()).has('o');
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Chan &c)
{
	s << "name:       \t" << c.get_name() << std::endl;
	s << "passkey:    \t" << c.get_pass() << std::endl;
	s << "forward:    \t" << c.get_forward() << std::endl;
	s << "jthrottle:  \t" << c.get_join_throttle().first << ":" << c.get_join_throttle().second << std::endl;
	s << "limit:      \t" << c.get_limit() << std::endl;
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


/**
 * This only works if this vector shares the same signs and modes. If so, we can simply
 * append the different masks to the chanserv command. If modes or signs differ, this
 * function also returns an empty string. If gen_cs_cmd() does not work either,
 * returns an empty string.
 */
inline
std::string gen_cs_cmd(const std::string &chan,
                       const Deltas &deltas)
try
{
	using std::get;

	std::stringstream s;
	const auto &sign(get<Delta::SIGN>(deltas.at(0)));
	const auto &mode(get<Delta::MODE>(deltas.at(0)));
	if(!deltas.all(sign) || !deltas.all(mode))
		return s.str();

	s << gen_cs_cmd(chan,deltas.at(0));
	if(s.tellp() == 0)
		return s.str();

	for(auto it(std::next(deltas.begin())); it != deltas.end(); ++it)
		s << " " << std::get<Delta::MASK>(*it);

	return s.str();
}
catch(const std::out_of_range &e)
{
	return std::string {};
}


inline
std::string gen_cs_cmd(const std::string &chan,
                       const Delta &delta)
{
	using std::get;

	std::stringstream s;
	const auto &mask(get<Delta::MASK>(delta));
	const auto &sign(get<Delta::SIGN>(delta));
	const auto &mode(get<Delta::MODE>(delta));

	switch(mode)
	{
		case 'o':     s << (sign? "OP"     : "DEOP"     );     break;
		case 'v':     s << (sign? "VOICE"  : "DEVOICE"  );     break;
		case 'q':     s << (sign? "QUIET"  : "UNQUIET"  );     break;
		default:      return s.str();
	}

	s << " " << chan << " " << mask;
	return s.str();
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
std::string nick_prefix(const Server &serv,
                        const std::string &nick)
{
	std::string ret;
	for(const char &c : nick)
		if(serv.has_prefix(c))
			ret.push_back(c);
		else
			break;

	return ret;
}
