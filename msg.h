/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#define SPQF_FMT(type, ...)  namespace type { enum { __VA_ARGS__ }; }


inline namespace fmt
{
SPQF_FMT( HTTP,              CODE,     REASON    /* Not IRC proto; parsed when proxying */                   )
SPQF_FMT( PING,              SOURCE                                                                          )
SPQF_FMT( WELCOME,           TEXT                                                                            )
SPQF_FMT( YOURHOST,          SELFNAME, TEXT                                                                  )
SPQF_FMT( CREATED,           SELFNAME, TEXT                                                                  )
SPQF_FMT( MYINFO,            SELFNAME, SERVNAME, VERSION,  USERMODS, CHANMODS, CHANPARM                      )
SPQF_FMT( CAP,               SELFNAME, COMMAND,  CAPLIST                                                     )
SPQF_FMT( ACCOUNT,           ACCTNAME,                                                                       )
SPQF_FMT( NICK,              NICKNAME                                                                        )
SPQF_FMT( ISON,              SELFNAME, NICKLIST                                                              )
SPQF_FMT( LIST,              SELFNAME, CHANNAME, USERNUM,  TOPIC                                             )
SPQF_FMT( QUIT,              REASON                                                                          )
SPQF_FMT( JOIN,              CHANNAME, ACCTNAME, EGECOS                                                      )
SPQF_FMT( PART,              CHANNAME, REASON                                                                )
SPQF_FMT( MODE,              CHANNAME, DELTASTR                                                              )
SPQF_FMT( AWAY,              SELFNAME, NICKNAME, MESSAGE                                                     )
SPQF_FMT( UMODE,             DELTASTR                                                                        )
SPQF_FMT( UMODEIS,           SELFNAME, DELTASTR                                                              )
SPQF_FMT( INVITE,            SELFNAME, CHANNAME,                                                             )
SPQF_FMT( NOTOPIC,           CHANNAME, INFO,                                                                 )
SPQF_FMT( TOPIC,             CHANNAME, TEXT,                                                                 )
SPQF_FMT( RPLTOPIC,          SELFNAME, CHANNAME, TEXT,                                                       )
SPQF_FMT( CHANNELMODEIS,     SELFNAME, CHANNAME, DELTASTR                                                    )
SPQF_FMT( CHANOPRIVSNEEDED,  SELFNAME, CHANNAME, REASON                                                      )
SPQF_FMT( NICKNAMEINUSE,     ASTERISK, NICKNAME, REASON                                                      )
SPQF_FMT( USERNOTINCHANNEL,  SELFNAME, NICKNAME, CHANNAME, REASON                                            )
SPQF_FMT( USERONCHANNEL,     SELFNAME, NICKNAME, CHANNAME, REASON                                            )
SPQF_FMT( ALREADYONCHAN,     SELFNAME, CHANNAME, REASON                                                      )
SPQF_FMT( BANNEDFROMCHAN,    SELFNAME, CHANNAME, REASON                                                      )
SPQF_FMT( CHANNELISFULL,     SELFNAME, CHANNAME, REASON                                                      )
SPQF_FMT( KICK,              CHANNAME, TARGET,   REASON                                                      )
SPQF_FMT( PRIVMSG,           SELFNAME, TEXT                                                                  )
SPQF_FMT( ACTION,            SELFNAME, TEXT                                                                  )
SPQF_FMT( NOTICE,            SELFNAME, TEXT                                                                  )
SPQF_FMT( NAMREPLY,          SELFNAME, TYPE,     CHANNAME, NAMELIST                                          )
SPQF_FMT( WHOREPLY,          SELFNAME, CHANNAME, USERNAME, HOSTNAME, SERVNAME, NICKNAME, FLAGS,    ADDL      )
SPQF_FMT( WHOISUSER,         SELFNAME, NICKNAME, USERNAME, HOSTNAME, ASTERISK, REALNAME                      )
SPQF_FMT( WHOISIDLE,         SELFNAME, NICKNAME, SECONDS,  SIGNON,   REMARKS                                 )
SPQF_FMT( WHOISSECURE,       SELFNAME, NICKNAME, REMARKS,                                                    )
SPQF_FMT( WHOISACCOUNT,      SELFNAME, NICKNAME, ACCTNAME, REMARKS                                           )
SPQF_FMT( HOSTHIDDEN,        SELFNAME, HOSTMASK, TEXT,                                                       )
SPQF_FMT( CREATIONTIME,      SELFNAME, CHANNAME, TIME,                                                       )
SPQF_FMT( TOPICWHOTIME,      SELFNAME, CHANNAME, MASK,     TIME,                                             )
SPQF_FMT( BANLIST,           SELFNAME, CHANNAME, BANMASK,  OPERATOR, TIME,                                   )
SPQF_FMT( EXCEPTLIST,        SELFNAME, CHANNAME, MASK,     OPERATOR, TIME,                                   )
SPQF_FMT( INVITELIST,        SELFNAME, CHANNAME, MASK,     OPERATOR, TIME,                                   )
SPQF_FMT( QUIETLIST,         SELFNAME, CHANNAME, MODECODE, BANMASK,  OPERATOR, TIME,                         )
SPQF_FMT( MONLIST,           SELFNAME, NICKLIST                                                              )
SPQF_FMT( MONONLINE,         SELFNAME, MASKLIST                                                              )
SPQF_FMT( MONOFFLINE,        SELFNAME, NICKLIST                                                              )
SPQF_FMT( ACCEPTLIST,        SELFNAME, /* ... */                                                             )
SPQF_FMT( ACCEPTEXIST,       SELFNAME, NICKNAME, REASON                                                      )
SPQF_FMT( ACCEPTNOT,         SELFNAME, NICKNAME, REASON                                                      )
SPQF_FMT( ACCEPTFULL,        SELFNAME                                                                        )
SPQF_FMT( KNOCK,             CHANNAME, CHANNAM2, MASK,     REASON                                            )
SPQF_FMT( MODEISLOCKED,      SELFNAME, CHANNAME, MODEUSED, LOCKLIST, REASON                                  )
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
