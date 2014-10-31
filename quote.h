/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Quote
{
	Sess &sess;
	Socket &socket;
	const char *const &cmd;

  public:
	using flush_t = Stream::flush_t;
	static constexpr flush_t flush = Stream::flush;

	auto &get_sess() const                          { return sess;               }
	auto &get_socket() const                        { return socket;             }
	auto &get_cmd() const                           { return cmd;                }
	bool has_cmd() const                            { return strnlen(cmd,64);    }

  protected:
	auto &get_sess()                                { return sess;               }
	auto &get_socket()                              { return socket;             }
	auto &get_cmd()                                 { return cmd;                }

  public:
	// Send to server
	Quote &operator<<(const flush_t);
	Quote &operator()(const std::string &str);      // flush automatically
	Quote &operator()();                            // flush automatically

	// Append to stream
	template<class T> Quote &operator<<(const T &t);

	Quote(Sess &sess, const char *const &cmd = "", const milliseconds &delay = 0ms);
	~Quote() noexcept;
};


inline
Quote::Quote(Sess &sess,
             const char *const &cmd,
             const milliseconds &delay):
sess(sess),
socket(sess.get_socket()),
cmd(cmd)
{
	socket.set_delay(delay);

	if(has_cmd())
		socket << cmd << " ";
}


inline
Quote::~Quote()
noexcept
{
	if(std::uncaught_exception())
	{
		socket.clear();
		return;
	}

	if(socket.has_pending())
		operator<<(flush);
}


inline
Quote &Quote::operator()()
{
	auto &socket = get_socket();
	operator<<(flush);
	return *this;
}


inline
Quote &Quote::operator()(const std::string &str)
{
	auto &socket = get_socket();
	operator<<(str);
	operator<<(flush);
	return *this;
}


template<class T>
Quote &Quote::operator<<(const T &t)
{
	auto &socket = get_socket();
	if(has_cmd() && !socket.has_pending())
		socket << cmd << " ";

	socket << t;
	return *this;
}


inline
Quote &Quote::operator<<(const flush_t)
{
	auto &socket = get_socket();
	socket << flush;
	return *this;
}
