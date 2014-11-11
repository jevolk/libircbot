/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Sess
{
  public:
	enum State : uint16_t
	{
		ERRONEOUS          = 0x01,                     // The session cannot continue normally
		CONNECTED          = 0x02,                     // Logical indication (socket indicator in Socket)
		PROXIED            = 0x04,                     // If proxying, got 200 OK
		CAP_REG            = 0x08,                     // CAP registration is open and has not ended
		REGISTERED         = 0x10,                     // USER / ircd registration has taken place
		IDENTIFIED         = 0x20,                     // NickServ identification confirmed
	};

	using state_t = std::underlying_type<State>::type;

  private:
	std::mutex &mutex;                                 // Bot mutex
	Opts &opts;
	state_t state;                                     // State flags
	Socket socket;
	Server server;                                     // Filled at connection time
	std::set<std::string> caps;                        // registered capabilities (full LS in Server)
	std::string nickname;                              // NICK reply
	Mode mymode;                                       // UMODE
	std::map<std::string,Mode> access;                 // Our channel access (/ns LISTCHANS)

	// Handler accesses
	friend class Bot;
	friend class NickServ;
	void set_nick(const std::string &nickname)         { this->nickname = nickname;                 }
	void delta_mode(const std::string &str)            { mymode.delta(str);                         }

  public:
	auto &get_mutex() const                            { return const_cast<std::mutex &>(mutex);    }
	auto &get_opts() const                             { return opts;                               }
	auto &get_state() const                            { return state;                              }
	auto &get_socket() const                           { return socket;                             }
	auto &get_server() const                           { return server;                             }
	auto &get_isupport() const                         { return get_server().isupport;              }
	auto &get_nick() const                             { return nickname;                           }
	auto &get_mode() const                             { return mymode;                             }
	auto &get_access() const                           { return access;                             }

	auto has_opt(const std::string &key) const         { return opts.get<bool>(key);                }
	bool has_cap(const std::string &cap) const         { return caps.count(cap);                    }
	auto isupport(const std::string &key) const        { return get_server().isupport(key);         }
	auto is_desired_nick() const                       { return nickname == opts["nick"];           }
	auto get_acct() const                              { return tolower(opts["ns-acct"]);           }

	bool is(const State &state) const                  { return (this->state & state) == state;     }
	void set(const State &state)                       { this->state |= state;                      }
	void unset(const State &state)                     { this->state &= ~state;                     }

	Socket &get_socket()                               { return socket;                             }
	auto &get_mutex()                                  { return mutex;                              }
	auto &get_opts()                                   { return opts;                               }

	void umode(const std::string &m);
	void umode();

	void proxy();
	void reg();
	void cap();

	Sess(std::mutex &mutex, Opts &opts);
	Sess(const Sess &) = delete;
	Sess &operator=(const Sess &) = delete;

	friend std::ostream &operator<<(std::ostream &s, const Sess &sess);
};


inline
Sess::Sess(std::mutex &mutex,
           Opts &opts):
mutex(mutex),
opts(opts),
state(0),
socket(this->opts),
nickname(this->opts["nick"])
{
	// Use the same global locale for each session for now.
	// Raise an issue if you have a case for this being a problem.
	irc::bot::locale = std::locale(this->opts["locale"].c_str());

}


inline
void Sess::cap()
{
	socket << "CAP LS" << socket.flush;
	socket << "CAP REQ :account-notify extended-join multi-prefix" << socket.flush;
	socket << "CAP END" << socket.flush;
}


inline
void Sess::reg()
{
	const auto &username = opts.has("user")? opts["user"] : "nobody";
	const auto &gecos = opts.has("gecos")? opts["gecos"] : "nowhere";

	socket << "NICK " << get_nick() << socket.flush;
	socket << "USER " << username << " unknown unknown :" << gecos << socket.flush;

	set(REGISTERED);
}


inline
void Sess::proxy()
{
	const auto &host = opts["host"];
	const auto &port = opts["port"];

	// socket.flush adds the second CR-LF
	socket << "CONNECT " << host << ":" << port << " HTTP/1.0\r\n" << socket.flush;
}


inline
void Sess::umode()
{
	socket << "MODE " << get_nick() << socket.flush;
}


inline
void Sess::umode(const std::string &str)
{
	socket << "MODE " << get_nick() << " " << str << socket.flush;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Sess &ss)
{
	s << "Opts:            " << std::endl << ss.get_opts() << std::endl;
	s << "server:          " << ss.get_server() << std::endl;
	s << "nick:            " << ss.get_nick() << std::endl;
	s << "mode:            " << ss.get_mode() << std::endl;

	s << "caps:            ";
	for(const auto &cap : ss.caps)
		s << "[" << cap << "]";
	s << std::endl;

	for(const auto &p : ss.access)
		s << "access:          " << p.second << "\t => " << p.first << std::endl;

	return s;
}
