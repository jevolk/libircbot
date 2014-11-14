/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


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
