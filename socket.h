/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


namespace sendq
{
	struct Ent
	{
		time_point absolute;
		boost::asio::ip::tcp::socket *sd;
		std::string pck;
	};

	using ECb = std::function<void (const boost::system::error_code &)>;

	extern std::mutex mutex;
	extern std::condition_variable cond;
	extern std::atomic<bool> interrupted;
	extern std::map<const void *, ECb> ecbs;
	extern std::deque<Ent> queue;
	extern std::deque<Ent> slowq;
	extern std::thread thread;

	void set_ecb(const void *const &p, const ECb &c); // No lock required.
	void purge(const void *const &p);                 // No lock required.
	size_t send(Ent &ent);                            // Lock required (internal usage)
	void slowq_add(Ent &ent);                         // Lock required (internal usage)
	void process(Ent &ent);                           // Lock required (internal usage)
	auto next_event();                                // Lock required (internal usage)
	void interrupt();                                 // No lock required.
	void worker();                                    // Static initialized. Not advised to call.
}


namespace recvq
{
	extern std::mutex mutex;
	extern std::atomic<bool> interrupted;
	extern boost::asio::io_service ios;
	extern std::vector<std::thread *> thread;

	void reset();                                     // No lock required.
	size_t num_threads();                             // No lock required.
	void add_thread(const size_t &num = 1);           // No lock required.
	void min_threads(const size_t &num = 0);          // No lock required.
	void interrupt();                                 // No lock required.
	void worker();                                    // User may call in own threads. No lock required.
}


class Socket
{
	const Opts &opts;
	boost::asio::ip::tcp::endpoint ep;
	boost::asio::ip::tcp::socket sd;
	boost::asio::steady_timer timer;
	std::ostringstream sendq;
	milliseconds delay;
	Throttle throttle;
	int cork;                                         // makes operator<<(flush_t) ineffective

  public:
	using flush_t = Stream::flush_t;
	static constexpr flush_t flush {};

	auto &get_ep() const                              { return ep;                                }
	auto &get_sd() const                              { return sd;                                }
	auto &get_timer() const                           { return timer;                             }
	auto &get_delay() const                           { return delay;                             }
	auto &get_throttle() const                        { return throttle;                          }
	auto has_cork() const                             { return cork > 0;                          }
	auto has_pending() const                          { return !sendq.str().empty();              }
	bool is_connected() const;

	auto &get_ep()                                    { return ep;                                }
	auto &get_sd()                                    { return sd;                                }
	auto &get_timer()                                 { return timer;                             }
	void set_ecb(const sendq::ECb &cb)                { sendq::set_ecb(&get_sd(),cb);             }
	void set_throttle(const milliseconds &inc)        { this->throttle.set_inc(inc);              }
	void set_delay(const milliseconds &delay)         { this->delay = delay;                      }
	void set_cork()                                   { this->cork++;                             }
	void unset_cork()                                 { this->cork--;                             }
	void purge()                                      { sendq::purge(&get_sd());                  }
	void clear();                                     // Clears the instance sendq buffer

	Socket &operator<<(const flush_t);
	template<class T> Socket &operator<<(const T &t);

	bool disconnect(const bool &fin = true);
	void connect();

	Socket(const Opts &opts);
	~Socket() noexcept;
};


inline
Socket::Socket(const Opts &opts):
opts(opts),
ep([&]() -> decltype(ep)
{
	using namespace boost::asio::ip;

	const auto &host = opts.has("proxy")? split(opts["proxy"],":").first : opts["host"];
	const auto &port = opts.has("proxy")? split(opts["proxy"],":").second : opts["port"];

	boost::system::error_code ec;
	tcp::resolver res(recvq::ios);
	const tcp::resolver::query query(tcp::v4(),host,port,tcp::resolver::query::numeric_service);
	const auto it = res.resolve(query,ec);

	if(ec)
		throw Internal(ec.value(),ec.message());

	return *it;
}()),
sd(recvq::ios),
timer(recvq::ios),
delay(0ms),
cork(0)
{

}


inline
Socket::~Socket()
noexcept
{
	purge();
}


inline
void Socket::connect()
try
{
	sd.open(boost::asio::ip::tcp::v4());
	sd.connect(ep);
}
catch(const boost::system::system_error &e)
{
	throw Internal(e.what());
}


inline
bool Socket::disconnect(const bool &fin)
try
{
	if(!sd.is_open())
		return false;

	if(fin)
	{
		boost::system::error_code ec;
		sd.shutdown(boost::asio::ip::tcp::socket::shutdown_type::shutdown_both,ec);
	}

	sd.close();
	timer.cancel();
	return true;
}
catch(const boost::system::system_error &e)
{
	std::cerr << "Socket::disconnect(): " << e.what() << std::endl;
	return false;
}


template<class T>
Socket &Socket::operator<<(const T &t)
{
	sendq << t;
	return *this;
}


inline
Socket &Socket::operator<<(const flush_t)
{
	if(has_cork())
	{
		sendq << "\r\n";
		return *this;
	}

	const scope clr(std::bind(&Socket::clear,this));
	const auto xmit_time = delay == 0ms? throttle.next_abs():
	                                     steady_clock::now() + delay;

	const std::lock_guard<decltype(sendq::mutex)> lock(sendq::mutex);
	sendq::queue.push_back({xmit_time,&sd,sendq.str()});
	sendq::cond.notify_one();
	return *this;
}


inline
void Socket::clear()
{
	sendq.clear();
	sendq.str(std::string());
	delay = 0ms;
}


inline
bool Socket::is_connected()
const try
{
	return sd.is_open();
}
catch(const boost::system::system_error &e)
{
	return false;
}
