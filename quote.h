/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Quote
{
	const char *const &cmd;

  public:
	using flush_t = Stream::flush_t;
	static constexpr flush_t flush = Stream::flush;

	auto &get_cmd() const                           { return cmd;                }
	bool has_cmd() const                            { return strnlen(cmd,64);    }

  protected:
	auto &get_cmd()                                 { return cmd;                }

  public:
	// Send to server
	Quote &operator<<(const flush_t);
	Quote &operator()(const std::string &str);      // flush automatically
	Quote &operator()();                            // flush automatically

	// Append to stream
	template<class T> Quote &operator<<(const T &t);

	Quote(const char *const &cmd = "", const milliseconds &delay = 0ms);
	~Quote() noexcept;
};


inline
Quote::Quote(const char *const &cmd,
             const milliseconds &delay):
cmd(cmd)
{
	auto &sock(get_sock());
	sock.set_delay(delay);

	if(has_cmd())
		sock << cmd << " ";
}


inline
Quote::~Quote()
noexcept
{
	auto &sock(get_sock());
	if(std::uncaught_exception())
	{
		sock.clear();
		return;
	}

	if(sock.has_pending())
		operator<<(flush);
}


inline
Quote &Quote::operator()()
{
	operator<<(flush);
	return *this;
}


inline
Quote &Quote::operator()(const std::string &str)
{
	operator<<(str);
	operator<<(flush);
	return *this;
}


template<class T>
Quote &Quote::operator<<(const T &t)
{
	auto &sock(get_sock());
	if(has_cmd() && !sock.has_pending())
		sock << cmd << " ";

	sock << t;
	return *this;
}


inline
Quote &Quote::operator<<(const flush_t)
{
	auto &sock(get_sock());
	sock << flush;
	return *this;
}
