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

	extern std::mutex mutex;
	extern std::condition_variable cond;
	extern std::atomic<bool> interrupted;
	extern std::deque<Ent> queue;
	extern std::deque<Ent> slowq;
	extern std::thread thread;

	extern void interrupt();
	extern size_t send(Ent &ent);
	extern auto slowq_next();
	extern void slowq_process();
	extern void slowq_add(Ent &ent);
	extern void process(Ent &ent);
	extern void worker();
}


namespace recvq
{
	extern std::mutex mutex;
	extern std::atomic<bool> interrupted;
	extern boost::asio::io_service ios;
	extern std::vector<std::thread *> thread;

	extern void reset();
	extern void add_thread();
	extern void interrupt();
	extern void worker();
}


class Socket
{
	const Opts &opts;
	boost::asio::io_service::work work;
	boost::asio::ip::tcp::endpoint ep;
	boost::asio::ip::tcp::socket sd;
	boost::asio::basic_waitable_timer<steady_clock> timer;
	std::ostringstream sendq;
	milliseconds delay;
	Throttle throttle;
	int hold;                                         // makes operator<<(flush_t) ineffective

  public:
	using flush_t = Stream::flush_t;
	static constexpr flush_t flush {};

	auto &get_ep() const                              { return ep;                                }
	auto &get_sd() const                              { return sd;                                }
	auto &get_timer() const                           { return timer;                             }
	auto &get_delay() const                           { return delay;                             }
	auto &get_throttle() const                        { return throttle;                          }
	auto has_hold() const                             { return hold > 0;                          }
	auto has_pending() const                          { return !sendq.str().empty();              }
	bool is_connected() const;

	auto &get_ep()                                    { return ep;                                }
	auto &get_sd()                                    { return sd;                                }
	auto &get_timer()                                 { return timer;                             }
	void set_throttle(const milliseconds &inc)        { this->throttle.set_inc(inc);              }
	void set_delay(const milliseconds &delay)         { this->delay = delay;                      }
	void set_hold()                                   { this->hold++;                             }
	void unset_hold()                                 { this->hold--;                             }
	void clear();

	Socket &operator<<(const flush_t);
	template<class T> Socket &operator<<(const T &t);

	bool disconnect(const bool &fin = true);
	template<class Handler> void connect(Handler&& handler);
	void connect();

	Socket(const Opts &opts);
};


inline
Socket::Socket(const Opts &opts):
opts(opts),
work(recvq::ios),
ep([&]
{
	using namespace boost::asio::ip;

	const auto &host = opts.has("proxy-host")? opts["proxy-host"] : opts["host"];
	const auto &port = opts.has("proxy-port")? opts["proxy-port"] : opts["port"];

	tcp::resolver res(recvq::ios);
	tcp::resolver::query query(tcp::v4(),host,port,tcp::resolver::query::numeric_service);

	boost::system::error_code ec;
	const auto it = res.resolve(query,ec);

	if(ec)
		throw Internal(ec.value(),ec.message());

	return *it;
}()),
sd(recvq::ios),
timer(recvq::ios),
delay(0ms),
hold(0)
{

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


template<class Handler>
void Socket::connect(Handler&& handler)
{
	sd.async_connect(ep,std::forward<Handler>(handler));
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
	if(has_hold())
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
