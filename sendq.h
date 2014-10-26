/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


extern std::mutex coarse_flood_mutex;
extern std::condition_variable coarse_flood_cond;

void coarse_flood_wait();
void coarse_flood_done();


class SendQ
{
	struct Ent
	{
		irc_session_t *sess;
		std::string pck;
	};

	static std::mutex mutex;
	static std::condition_variable cond;
	static std::atomic<bool> interrupted;
	static std::deque<Ent> queue;

  public:
	static void interrupt();
	static void worker();

  private:
	irc_session_t *sess;
	std::ostringstream sendq;

  public:
	using flush_t = Stream::flush_t;
	static constexpr flush_t flush {};

	auto &get_sess() const                    { return *sess;                                       }
	auto has_pending() const                  { return !sendq.str().empty();                        }

	void clear();
	SendQ &operator<<(const flush_t);
	template<class T> SendQ &operator<<(const T &t);

	SendQ(irc_session_t *const &sess): sess(sess) {}
};


template<class T>
SendQ &SendQ::operator<<(const T &t)
{
	sendq << t;
	return *this;
}


inline
SendQ &SendQ::operator<<(const flush_t)
{
	const scope clr(std::bind(&SendQ::clear,this));
	const std::lock_guard<decltype(mutex)> lock(mutex);
	queue.push_back({sess,sendq.str()});
	cond.notify_one();
	return *this;
}


inline
void SendQ::clear()
{
	sendq.clear();
	sendq.str(std::string());
}
