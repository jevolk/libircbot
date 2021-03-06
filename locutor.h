/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Locutor : Stream
{
	enum Method
	{
		NOTICE,
		PRIVMSG,
		ACTION,
		CTCP,
	};

	enum MethodEx
	{
		NONE,
		CMSG,
		WALLCHOPS,
		WALLVOICE,
	};

	static const Method DEFAULT_METHOD                  { NOTICE                                     };
	static const MethodEx DEFAULT_METHODEX              { NONE                                       };

  private:
	Method meth;                                        // Stream state for current method
	MethodEx methex;                                    // Stream state for extension to method
	colors::FG fg;                                      // Stream state for foreground color
	std::string target;
	Throttle throttle;

  public:
	auto &get_meth() const                              { return meth;                               }
	auto &get_methex() const                            { return methex;                             }
	auto &get_target() const                            { return target;                             }
	auto &get_my_nick() const                           { return get_sess().get_nick();              }
	auto &get_throttle() const                          { return throttle;                           }

	void set_target(const std::string &target)          { this->target = target;                     }
	void reset();

  protected:
	auto &get_throttle()                                { return throttle;                           }
	void msg(const char *const &cmd);

  public:
	// [SEND] stream interface                          // Defaults back to DEFAULT_METHOD after flush
	Locutor &operator<<(const flush_t) override;
	Locutor &operator<<(const Method &method);          // Set method for this message
	Locutor &operator<<(const MethodEx &methodex);      // Set method extension for this message
	Locutor &operator<<(const colors::Mode &mode);      // Color controls
	Locutor &operator<<(const colors::FG &fg);          // Insert foreground color
	Locutor &operator<<(const colors::BG &fg);          // Insert background color
	template<class T> Locutor &operator<<(const T &t)   { Stream::operator<<<T>(t);   return *this;  }

	// [SEND] Controls / Utils
	void mode(const std::string &mode);                 // Raw mode command
	void mode(const Deltas &deltas);                    // Execute any number of deltas
	void whois();                                       // Sends whois query
	void mode();                                        // Sends mode query

	explicit Locutor(const std::string &target);
	virtual ~Locutor() = default;
};


inline
Locutor::Locutor(const std::string &target):
meth(DEFAULT_METHOD),
methex(DEFAULT_METHODEX),
fg(colors::FG::BLACK),
target(target),
throttle(get_opts().get<uint>("throttle-msg"))
{
}


inline
void Locutor::mode()
{
	Quote("MODE") << get_target();
}


inline
void Locutor::whois()
{
	Quote("WHOIS") << get_target();
}


inline
void Locutor::mode(const Deltas &deltas)
{
	auto &sess(get_sess());
	const auto &isup(sess.get_isupport());
	const size_t max(isup.get("MODES",3));

	for(size_t i(0); i < deltas.size(); i += max)
	{
		const auto beg(deltas.begin() + i);
		const auto end(deltas.begin() + std::min(deltas.size(),i + max));
		mode(deltas.substr(beg,end));
	}
}


inline
void Locutor::mode(const std::string &str)
{
	Quote("MODE") << get_target() << " " << str;
}


inline
Locutor &Locutor::operator<<(const colors::FG &fg)
{
	auto &out(*this);
	out << "\x03" << std::setfill('0') << std::setw(2) << std::right << int(fg);
	this->fg = fg;
	return *this;
}


inline
Locutor &Locutor::operator<<(const colors::BG &bg)
{
	auto &out(*this);
	out << "\x03" << std::setfill('0') << std::setw(2) << std::right << int(fg);
	out << "," << std::setfill('0') << std::setw(2) << std::right << int(bg);
	return *this;
}


inline
Locutor &Locutor::operator<<(const colors::Mode &mode)
{
	auto &out(*this);
	out << (unsigned char)mode;
	return *this;
}


inline
Locutor &Locutor::operator<<(const Method &meth)
{
	this->meth = meth;
	return *this;
}


inline
Locutor &Locutor::operator<<(const MethodEx &methex)
{
	this->methex = methex;
	return *this;
}


inline
Locutor &Locutor::operator<<(const flush_t)
{
	const scope reset([this]
	{
		this->reset();
	});

	switch(meth)
	{
		case NOTICE:     msg(methex == CMSG? "CNOTICE" : "NOTICE");      break;
		case PRIVMSG:    msg(methex == CMSG? "CPRIVMSG" : "PRIVMSG");    break;
		case ACTION:     msg("ACTION");                                  break;
		case CTCP:       msg("CTCP");                                    break;
		default:
			throw Assertive("Unsupported locution method");
	}

	return *this;
}


inline
void Locutor::msg(const char *const &cmd)
{
	auto &throttle(get_throttle());
	const auto toks(tokens(packetize(get_str()),"\n"));

	switch(methex)
	{
		case WALLCHOPS:
		case WALLVOICE:
		{
			const auto prefix(methex == WALLCHOPS? '@' : '+');
			for(const auto &token : toks)
				Quote(cmd,throttle.next()) << prefix << get_target() << " :" << token;

			break;
		}

		case CMSG:
		{
			const auto &chan(toks.at(0));
			for(auto it(toks.begin()+1); it != toks.end(); ++it)
				Quote(cmd,throttle.next()) << get_target() << " "  << chan << " :" << *it;

			break;
		}

		case NONE:
		default:
		{
			for(const auto &token : toks)
				Quote(cmd,throttle.next()) << get_target() << " :" << token;

			break;
		}
	}
}


inline
void Locutor::reset()
{
	clear();
	meth = DEFAULT_METHOD;
	methex = DEFAULT_METHODEX;
}


inline
bool operator==(const Locutor &a, const Locutor &b)
{
	return a.get_target() == b.get_target();
}


inline
bool operator!=(const Locutor &a, const Locutor &b)
{
	return a.get_target() != b.get_target();
}


inline
bool operator<=(const Locutor &a, const Locutor &b)
{
	return a.get_target() <= b.get_target();
}


inline
bool operator>=(const Locutor &a, const Locutor &b)
{
	return a.get_target() >= b.get_target();
}


inline
bool operator<(const Locutor &a, const Locutor &b)
{
	return a.get_target() < b.get_target();
}


inline
bool operator>(const Locutor &a, const Locutor &b)
{
	return a.get_target() > b.get_target();
}


inline
bool operator==(const Locutor &a, const std::string &b)
{
	return tolower(a.get_target()) == tolower(b);
}


inline
bool operator!=(const Locutor &a, const std::string &b)
{
	return tolower(a.get_target()) != tolower(b);
}


inline
bool operator<=(const Locutor &a, const std::string &b)
{
	return tolower(a.get_target()) <= tolower(b);
}


inline
bool operator>=(const Locutor &a, const std::string &b)
{
	return tolower(a.get_target()) >= tolower(b);
}


inline
bool operator<(const Locutor &a, const std::string &b)
{
	return tolower(a.get_target()) < tolower(b);
}


inline
bool operator>(const Locutor &a, const std::string &b)
{
	return tolower(a.get_target()) > tolower(b);
}
