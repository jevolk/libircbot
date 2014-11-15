/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Sess
{
	friend class Bot;                                  // Handler accesses
	friend class NickServ;                             // Handler access

	Opts &opts;                                        // downstream reference to Bot Opts instance
	std::mutex &mutex;                                 // downstream reference to Bot mutex
	boost::asio::io_service &ios;                      // Associated IOService
	boost::asio::steady_timer timer;                   // Session's timer
	boost::asio::strand strand;                        // Session events' mutex
	State state;                                       // Session State
	flag_t flags;                                      // Session flags indicator
	Socket socket;
	Server server;                                     // Filled at connection time
	Mode mode;                                         // UMODE
	std::set<std::string> caps;                        // registered capabilities (full LS in Server)
	std::string nick;                                  // NICK reply
	std::map<std::string,Mode> access;                 // Our channel access (/ns LISTCHANS)
	std::exception_ptr eptr;                           // Storage for a fault handler

  public:
	auto &get_opts() const                             { return opts;                               }
	auto &get_mutex() const                            { return const_cast<std::mutex &>(mutex);    }
	auto &get_ios() const                              { return ios;                                }
	auto &get_timer() const                            { return timer;                              }
	auto &get_strand() const                           { return strand;                             }
	auto &get_state() const                            { return state;                              }
	auto &get_flags() const                            { return flags;                              }
	auto &get_socket() const                           { return socket;                             }
	auto &get_server() const                           { return server;                             }
	auto &get_isupport() const                         { return get_server().isupport;              }
	auto &get_nick() const                             { return nick;                               }
	auto &get_mode() const                             { return mode;                               }
	auto &get_access() const                           { return access;                             }
	auto &get_eptr() const                             { return eptr;                               }

	bool is(const State &state) const                  { return this->state == state;               }
	bool is(const Flag &flags) const                   { return (this->flags & flags) == flags;     }
	auto has_opt(const std::string &key) const         { return opts.get<bool>(key);                }
	bool has_cap(const std::string &cap) const         { return caps.count(cap);                    }
	bool has_exception() const                         { return bool(eptr);                         }
	auto isupport(const std::string &key) const        { return get_server().isupport(key);         }
	auto is_desired_nick() const                       { return nick == opts["nick"];               }
	auto get_acct() const                              { return tolower(opts["ns-acct"]);           }

	auto &get_opts()                                   { return opts;                               }
	auto &get_mutex()                                  { return mutex;                              }
	auto &get_timer()                                  { return timer;                              }
	auto &get_strand()                                 { return strand;                             }
	auto &get_socket()                                 { return socket;                             }

	void set(const State &state)                       { this->state = state;                       }
	void set(const Flag &flags)                        { this->flags |= flags;                      }
	void unset(const Flag &flags)                      { this->flags &= ~flags;                     }
	void set_nick(const std::string &nick)             { this->nick = nick;                         }
	void delta_mode(const std::string &mode)           { this->mode.delta(mode);                    }

	[[noreturn]] void rethrow_exception()              { std::rethrow_exception(eptr);              }
	void set_current_exception()                       { this->eptr = std::current_exception();     }
	template<class E> void set_exception(E&& e)        { this->eptr = std::make_exception_ptr(e);   }
	template<class F> void dispatch(F&& func)          { strand.dispatch(std::forward<F>(func));    }
	template<class F> void post(F&& func)              { strand.post(std::forward<F>(func));        }
	template<class F> auto wrap(F&& func)              { return strand.wrap(std::forward<F>(func)); }

	void umode(const std::string &m);
	void umode();

	Sess(Opts &opts, std::mutex &mutex, boost::asio::io_service &ios);
	Sess(const Sess &) = delete;
	Sess &operator=(const Sess &) = delete;

	friend std::ostream &operator<<(std::ostream &s, const Sess &sess);
};


inline
Sess::Sess(Opts &opts,
           std::mutex &mutex,
           boost::asio::io_service &ios):
opts(opts),
mutex(mutex),
ios(ios),
timer(ios),
strand(ios),
state(State::INACTIVE),
flags(Flag::NONE),
socket(this->opts,ios),
nick(this->opts["nick"])
{
	// Use the same global locale for each session for now.
	// Raise an issue if you have a case for this being a problem.
	irc::bot::locale = std::locale(this->opts["locale"].c_str());
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
