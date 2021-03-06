/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class User : public Locutor,
             public Acct
{
	// nick -> Locutor::target                         // who 'n'
	std::string host;                                  // who 'h'
	std::string acct;                                  // who 'a' (account name)
	bool secure;                                       // WHOISSECURE (ssl)
	time_t signon;                                     // WHOISIDLE
	time_t idle;                                       // who 'l' or WHOISIDLE
	bool away;
	size_t chans;                                      // reference counter for number of channels

  public:
	static constexpr int WHO_RECIPE                    = 0;
	static constexpr const char *const WHO_FORMAT      = "%tnha,0";       // ID must match WHO_RECIPE

	// Observers
	auto &get_nick() const                             { return Locutor::get_target();               }
	auto &get_host() const                             { return host;                                }
	auto &get_acct() const                             { return acct;                                }
	auto &is_secure() const                            { return secure;                              }
	auto &is_away() const                              { return away;                                }
	auto &get_signon() const                           { return signon;                              }
	auto &get_idle() const                             { return idle;                                }
	auto &num_chans() const                            { return chans;                               }
	bool is_myself() const                             { return get_nick() == get_sess().get_nick(); }
	bool is_logged_in() const;
	bool is_owner() const;
	Mask mask(const Mask::Type &t) const;              // Generate a mask from *this members
	bool is_myself(const Mask &mask) const;            // Test if mask can match us

	Delta unexcept(const Quiet::Type &type) const      { return {"-e",mask(type)};                   }
	Delta except(const Quiet::Type &type) const        { return {"+e",mask(type)};                   }
	Delta uninvex(const Quiet::Type &type) const       { return {"-I",mask(type)};                   }
	Delta invex(const Quiet::Type &type) const         { return {"+I",mask(type)};                   }
	Delta unquiet(const Quiet::Type &type) const       { return {"-q",mask(type)};                   }
	Delta quiet(const Quiet::Type &type) const         { return {"+q",mask(type)};                   }
	Delta unban(const Ban::Type &type) const           { return {"-b",mask(type)};                   }
	Delta ban(const Ban::Type &type) const             { return {"+b",mask(type)};                   }
	Delta devoice() const                              { return {"-v",get_nick()};                   }
	Delta voice() const                                { return {"+v",get_nick()};                   }
	Delta deop() const                                 { return {"-o",get_nick()};                   }
	Delta op() const                                   { return {"+o",get_nick()};                   }

	// [RECV] Handlers may call to update state
	void set_nick(const std::string &nick)             { Locutor::set_target(nick);                  }
	void set_acct(const std::string &acct)             { this->acct = tolower(acct);                 }
	void set_host(const std::string &host)             { this->host = host;                          }
	void set_secure(const bool &secure)                { this->secure = secure;                      }
	void set_signon(const time_t &signon)              { this->signon = signon;                      }
	void set_idle(const time_t &idle)                  { this->idle = idle;                          }
	void set_away(const bool &away)                    { this->away = away;                          }
	void inc_chans(const size_t &n = 1)                { chans += n;                                 }
	void dec_chans(const size_t &n = 1)                { chans -= n;                                 }

	// [SEND] Controls
	void who(const std::string &flags = WHO_FORMAT);   // Requests who with flags we need by default
	void info();                                       // Update acct["info"] from nickserv

	explicit User(const std::string &nick, const std::string &host = {}, const std::string &acct = {});
	User(User &&user) noexcept;
	User(const User &user);
	User &operator=(User &&) noexcept;
	User &operator=(const User &);

	friend std::ostream &operator<<(std::ostream &s, const User &u);
};


inline
User::User(const std::string &nick,
           const std::string &host,
           const std::string &acct):
Locutor(nick),
Acct(&this->acct),
host(host),
acct(tolower(acct)),
secure(false),
signon(0),
idle(0),
away(false),
chans(0)
{
}


inline
User::User(const User &user):
Locutor(user),
Acct(&this->acct),
host(user.host),
acct(user.acct),
secure(user.secure),
signon(user.signon),
idle(user.idle),
away(user.away),
chans(user.chans)
{
}


inline
User::User(User &&user)
noexcept:
Locutor(std::move(user)),
Acct(&this->acct),
host(std::move(user.host)),
acct(std::move(user.acct)),
secure(std::move(user.secure)),
signon(std::move(user.signon)),
idle(std::move(user.idle)),
away(std::move(user.away)),
chans(std::move(user.chans))
{
}


inline
User &User::operator=(const User &o)
{
	static_cast<Locutor &>(*this) = o;
	host = o.host;
	acct = o.acct;
	secure = o.secure;
	signon = o.signon;
	idle = o.idle;
	away = o.away;
	chans = o.chans;
	return *this;
}


inline
User &User::operator=(User &&o)
noexcept
{
	static_cast<Locutor &>(*this) = std::move(o);
	host = std::move(o.host);
	acct = std::move(o.acct);
	secure = std::move(o.secure);
	signon = std::move(o.signon);
	idle = std::move(o.idle);
	away = std::move(o.away);
	chans = std::move(o.chans);
	return *this;
}


inline
void User::info()
{
	Service &ns(get_ns());
	ns << "info " << acct << flush;
	ns.terminator_next("*** End of Info ***");
}


inline
void User::who(const std::string &flags)
{
	Quote out("WHO");
	out << get_nick() << " " << flags << flush;
}


inline
bool User::is_owner()
const
{
	const auto &opts(get_opts());
	return is_logged_in() && (get_acct() == opts["owner"] || get_nick() == opts["owner"]);
}


inline
bool User::is_logged_in()
const
{
	return acct.size() > 0 && acct.at(0) != '0' && acct.at(0) != '*';
}


inline
bool User::is_myself(const Mask &mask)
const
{
	switch(form(mask))
	{
		case Mask::CANONICAL:
			if(mask.has_all_wild())
				return true;

			if(tolower(mask.get_host()) == tolower(get_host()))
				return true;

			if(tolower(mask.get_nick()) == tolower(get_nick()))
				return true;

		case Mask::EXTENDED:
			return tolower(mask.get_mask()) == get_acct();

		case Mask::INVALID:
			return tolower(mask) == tolower(get_nick());

		default:
			throw Exception("Mask format unrecognized.");
	}
}


inline
Mask User::mask(const Mask::Type &recipe)
const
{
	std::stringstream s;
	switch(recipe)
	{
		case Mask::NICK:
			s << get_nick() << "!*@*";
			break;

		case Mask::HOST:
			s << "*!*@" << get_host();
			break;

		case Mask::ACCT:
			if(!is_logged_in())
				throw Assertive("Can't mask ACCT: user not logged in");

			s << "$a:" << get_acct();
			break;

		default:
			break;
	}

	return s.str();
}


inline
std::ostream &operator<<(std::ostream &s,
                         const User &u)
{
	s << std::setw(18) << std::left << u.get_nick();
	s << "@: " << std::setw(64) << std::left << u.get_host();
	s << "ssl: " << std::boolalpha << std::setw(8) << std::left << u.is_secure();
	s << "idle: " << std::setw(7) << std::left <<  u.get_idle();
	s << "chans: " << std::setw(3) << std::left << u.num_chans();

	if(u.is_logged_in())
		s << "$a: " << std::setw(18) << std::left << u.get_acct();

	if(u.is_myself())
		s << "<MYSELF>";

	return s;
}
