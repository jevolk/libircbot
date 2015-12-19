/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#define IRCBOT_FMT(type, ...)  namespace type { enum { __VA_ARGS__ }; }


inline namespace fmt
{
IRCBOT_FMT( HTTP,              CODE,     REASON    /* Not IRC proto; parsed when proxying */                   )
IRCBOT_FMT( PING,              SOURCE                                                                          )
IRCBOT_FMT( WELCOME,           TEXT                                                                            )
IRCBOT_FMT( YOURHOST,          SELFNAME, TEXT                                                                  )
IRCBOT_FMT( CREATED,           SELFNAME, TEXT                                                                  )
IRCBOT_FMT( MYINFO,            SELFNAME, SERVNAME, VERSION,  USERMODS, CHANMODS, CHANPARM                      )
IRCBOT_FMT( CAP,               SELFNAME, COMMAND,  CAPLIST                                                     )
IRCBOT_FMT( ACCOUNT,           ACCTNAME,                                                                       )
IRCBOT_FMT( NICK,              NICKNAME                                                                        )
IRCBOT_FMT( ISON,              SELFNAME, NICKLIST                                                              )
IRCBOT_FMT( LIST,              SELFNAME, CHANNAME, USERNUM,  TOPIC                                             )
IRCBOT_FMT( QUIT,              REASON                                                                          )
IRCBOT_FMT( JOIN,              CHANNAME, ACCTNAME, EGECOS                                                      )
IRCBOT_FMT( PART,              CHANNAME, REASON                                                                )
IRCBOT_FMT( MODE,              CHANNAME, DELTASTR                                                              )
IRCBOT_FMT( AWAY,              SELFNAME, NICKNAME, MESSAGE                                                     )
IRCBOT_FMT( UMODE,             DELTASTR                                                                        )
IRCBOT_FMT( UMODEIS,           SELFNAME, DELTASTR                                                              )
IRCBOT_FMT( INVITE,            SELFNAME, CHANNAME,                                                             )
IRCBOT_FMT( INVITING,          SELFNAME, TARGET,   CHANNAME,                                                   )
IRCBOT_FMT( NOTOPIC,           CHANNAME, INFO,                                                                 )
IRCBOT_FMT( TOPIC,             CHANNAME, TEXT,                                                                 )
IRCBOT_FMT( RPLTOPIC,          SELFNAME, CHANNAME, TEXT,                                                       )
IRCBOT_FMT( CHANNELMODEIS,     SELFNAME, CHANNAME, DELTASTR                                                    )
IRCBOT_FMT( CHANOPRIVSNEEDED,  SELFNAME, CHANNAME, REASON                                                      )
IRCBOT_FMT( NICKNAMEINUSE,     ASTERISK, NICKNAME, REASON                                                      )
IRCBOT_FMT( USERNOTINCHANNEL,  SELFNAME, NICKNAME, CHANNAME, REASON                                            )
IRCBOT_FMT( USERONCHANNEL,     SELFNAME, NICKNAME, CHANNAME, REASON                                            )
IRCBOT_FMT( ALREADYONCHAN,     SELFNAME, CHANNAME, REASON                                                      )
IRCBOT_FMT( BANNEDFROMCHAN,    SELFNAME, CHANNAME, REASON                                                      )
IRCBOT_FMT( CHANNELISFULL,     SELFNAME, CHANNAME, REASON                                                      )
IRCBOT_FMT( KICK,              CHANNAME, TARGET,   REASON                                                      )
IRCBOT_FMT( PRIVMSG,           SELFNAME, TEXT                                                                  )
IRCBOT_FMT( ACTION,            SELFNAME, TEXT                                                                  )
IRCBOT_FMT( NOTICE,            SELFNAME, TEXT                                                                  )
IRCBOT_FMT( NAMREPLY,          SELFNAME, TYPE,     CHANNAME, NAMELIST                                          )
IRCBOT_FMT( WHOREPLY,          SELFNAME, CHANNAME, USERNAME, HOSTNAME, SERVNAME, NICKNAME, FLAGS,    ADDL      )
IRCBOT_FMT( WHOISUSER,         SELFNAME, NICKNAME, USERNAME, HOSTNAME, ASTERISK, REALNAME                      )
IRCBOT_FMT( WHOISIDLE,         SELFNAME, NICKNAME, SECONDS,  SIGNON,   REMARKS                                 )
IRCBOT_FMT( WHOISSECURE,       SELFNAME, NICKNAME, REMARKS,                                                    )
IRCBOT_FMT( WHOISACCOUNT,      SELFNAME, NICKNAME, ACCTNAME, REMARKS                                           )
IRCBOT_FMT( HOSTHIDDEN,        SELFNAME, HOSTMASK, TEXT,                                                       )
IRCBOT_FMT( CREATIONTIME,      SELFNAME, CHANNAME, TIME,                                                       )
IRCBOT_FMT( TOPICWHOTIME,      SELFNAME, CHANNAME, MASK,     TIME,                                             )
IRCBOT_FMT( BANLIST,           SELFNAME, CHANNAME, BANMASK,  OPERATOR, TIME,                                   )
IRCBOT_FMT( EXCEPTLIST,        SELFNAME, CHANNAME, MASK,     OPERATOR, TIME,                                   )
IRCBOT_FMT( INVITELIST,        SELFNAME, CHANNAME, MASK,     OPERATOR, TIME,                                   )
IRCBOT_FMT( QUIETLIST,         SELFNAME, CHANNAME, MODECODE, BANMASK,  OPERATOR, TIME,                         )
IRCBOT_FMT( MONLIST,           SELFNAME, NICKLIST                                                              )
IRCBOT_FMT( MONONLINE,         SELFNAME, MASKLIST                                                              )
IRCBOT_FMT( MONOFFLINE,        SELFNAME, NICKLIST                                                              )
IRCBOT_FMT( ACCEPTLIST,        SELFNAME, /* ... */                                                             )
IRCBOT_FMT( ACCEPTEXIST,       SELFNAME, NICKNAME, REASON                                                      )
IRCBOT_FMT( ACCEPTNOT,         SELFNAME, NICKNAME, REASON                                                      )
IRCBOT_FMT( ACCEPTFULL,        SELFNAME                                                                        )
IRCBOT_FMT( KNOCK,             CHANNAME, CHANNAM2, MASK,     REASON                                            )
IRCBOT_FMT( MODEISLOCKED,      SELFNAME, CHANNAME, MODEUSED, LOCKLIST, REASON                                  )
}


class Msg
{
	using Params = std::vector<std::string>;

	static void consume_any(std::istream &stream, const char &c);
	static void consume_until(std::istream &stream, const char &c);

	Mask origin;
	std::string name;
	uint32_t code;
	Params params;

  public:
	auto &get_code() const                                  { return code;                      }
	auto &get_name() const                                  { return name;                      }
	auto &get_origin() const                                { return origin;                    }
	auto &get_params() const                                { return params;                    }
	auto num_params() const                                 { return get_params().size();       }

	auto get_nick() const                                   { return origin.get_nick();         }
	auto get_user() const                                   { return origin.get_user();         }
	auto get_host() const                                   { return origin.get_host();         }

	auto &get(const size_t &i) const                        { return get_params().at(i);        }
	auto &operator[](const size_t &i) const;                // returns empty str for outofrange
	template<class R> R get(const size_t &i) const;         // throws for range or bad cast
	template<class R> R operator[](const size_t &i) const   { return get<R>(i);                 }
	auto begin() const                                      { return params.begin();            }
	auto end() const                                        { return params.end();              }

	bool from(const Mask &mask) const;
	bool from_server() const;

	Msg(const uint32_t &code, const std::string &origin, const Params &params);
	Msg(const uint32_t &code, const char *const &origin, const char **const &params, const size_t &count);
	Msg(const std::string &name, const std::string &origin, const Params &params);
	Msg(const char *const &name, const char *const &origin, const char **const &params, const size_t &count);
	Msg(std::istream &stream);

	friend std::ostream &operator<<(std::ostream &s, const Msg &m);
};


inline
Msg::Msg(std::istream &stream):
origin([&]() -> Mask
{
	if(stream.eof() || stream.peek() != ':')
		return {};

	Mask ret;
	stream.ignore(1,':');
	std::getline(stream,ret,' ');
	return ret;
}()),
name([&]
{
	std::string ret;
	consume_any(stream,' ');
	std::getline(stream,ret,' ');
	return ret;
}()),
code(isnumeric(name)? lex_cast<decltype(code)>(name) : 0),
params([&]() -> Params
{
	using delim = boost::char_separator<char>;

	std::string str;
	std::getline(stream,str,'\r');
	if(str.empty())
		return {};

	if(str[0] == ':')
		return {{str.substr(1)}};

	static const delim d(" ");
	const auto col(str.find(" :"));
	if(col == std::string::npos)
	{
		const boost::tokenizer<delim> tk(str,d);
		return {tk.begin(),tk.end()};
	}

	const auto &par(str.substr(0,col));
	const auto &trail(str.substr(col+2));
	const boost::tokenizer<delim> tk(par,d);
	Params ret(tk.begin(),tk.end());
	ret.emplace_back(trail);
	return ret;
}())
{
	consume_any(stream,'\r');
	consume_until(stream,'\n');
}


inline
Msg::Msg(const char *const &name,
         const char *const &origin,
         const char **const &params,
         const size_t &count):
Msg(origin? origin : std::string{},
    name,
    {params,params+count})
{

}


inline
Msg::Msg(const std::string &name,
         const std::string &origin,
         const Params &params):
origin(origin),
name(name),
code(0),
params(params)
{

}


inline
Msg::Msg(const uint32_t &code,
         const char *const &origin,
         const char **const &params,
         const size_t &count):
Msg(code,
    origin? origin : std::string{},
    {params,params+count})
{

}


inline
Msg::Msg(const uint32_t &code,
         const std::string &origin,
         const Params &params):
origin(origin),
code(code),
params(params)
{

}


inline
bool Msg::from_server()
const
{
	return !get_origin().has('@') && !get_origin().has('!');
}


inline
bool Msg::from(const Mask &mask)
const
{
	return mask == origin || tolower(mask.get_nick()) == tolower(origin.get_nick());
}


template<class R>
R Msg::get(const size_t &i)
const
{
	return lex_cast<R>(params.at(i));
}


inline
auto &Msg::operator[](const size_t &i)
const try
{
	return get(i);
}
catch(const std::out_of_range &e)
{
	static const std::string empty;
	return empty;
}


inline
void Msg::consume_any(std::istream &stream,
                      const char &c)
{
	while(!stream.eof() && stream.peek() == c)
		stream.ignore(1,c);
}


inline
void Msg::consume_until(std::istream &stream,
                        const char &c)
{
	stream.ignore(512,c);
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Msg &m)
{
	s << "(" << std::setw(3) << m.get_code() << ")";
	s << " " << std::setw(27) << std::setfill(' ') << std::left << m.get_origin();
	s << " [" << std::setw(2) << m.num_params() << "]: ";

	for(const auto &param : m.get_params())
		s << "[" << param << "]";

	return s;
}
