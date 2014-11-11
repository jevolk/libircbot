/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"

using namespace irc::bot;


decltype(sendq::mutex)        sendq::mutex;
decltype(sendq::cond)         sendq::cond;
decltype(sendq::interrupted)  sendq::interrupted;
decltype(sendq::queue)        sendq::queue;
decltype(sendq::slowq)        sendq::slowq;
decltype(sendq::thread)       sendq::thread {&sendq::worker};
static const scope join([]
{
	sendq::interrupt();
	sendq::thread.join();
});


void sendq::interrupt()
{
	interrupted.store(true,std::memory_order_release);
	cond.notify_all();
}


void sendq::purge(const boost::asio::ip::tcp::socket *const &ptr)
{
	const std::unique_lock<decltype(mutex)> lock(mutex);

	const auto queue_end = std::remove_if(queue.begin(),queue.end(),[&ptr]
	(const auto &ent)
	{
		return ent.sd == ptr;
	});

	const auto slowq_end = std::remove_if(slowq.begin(),slowq.end(),[&ptr]
	(const auto &ent)
	{
		return ent.sd == ptr;
	});

	queue.erase(queue_end,queue.end());
	slowq.erase(slowq_end,slowq.end());
}


size_t sendq::send(Ent &ent)
try
{
	static const boost::asio::const_buffer terminator{"\r\n",2};
	const std::array<boost::asio::const_buffer,2> buf
	{
		boost::asio::const_buffer{ent.pck.data(),ent.pck.size()},
		terminator
	};

	std::cout << "\033[1;36m>> " << ent.pck << "\033[0m" << std::endl;
	return ent.sd->send(buf);
}
catch(const boost::system::system_error &e)
{
	std::cerr << "sendq::send(): " << e.what() << std::endl;
	return 0;
}


void sendq::slowq_add(Ent &ent)
{
	slowq.emplace_back(ent);
	std::sort(slowq.begin(),slowq.end(),[]
	(const Ent &a, const Ent &b)
	{
		return a.absolute < b.absolute;
	});
}


void sendq::process(Ent &ent)
{
	if(ent.absolute > steady_clock::now())
		slowq_add(ent);
	else
		send(ent);
}


auto sendq::slowq_next()
{
	using namespace std::chrono;

	if(slowq.empty())
		return milliseconds(std::numeric_limits<uint32_t>::max());

	const auto now = steady_clock::now();
	const auto &abs = slowq.front().absolute;
	if(abs < now)
		return milliseconds(0);

	return duration_cast<milliseconds>(abs.time_since_epoch()) -
	       duration_cast<milliseconds>(now.time_since_epoch());
}


void sendq::worker()
try
{
	while(1)
	{
		std::unique_lock<decltype(mutex)> lock(mutex);
		cond.wait_for(lock,slowq_next());

		if(interrupted.load(std::memory_order_consume))
			throw Interrupted("Interrupted");

		while(!queue.empty())
		{
			const scope pf([]{ queue.pop_front(); });
			process(queue.front());
		}

		while(!slowq.empty())
		{
			Ent &ent = slowq.front();
			if(ent.absolute > steady_clock::now())
				break;

			send(ent);
			slowq.pop_front();
		}
    }
}
catch(const Internal &e)
{
	std::cerr << "\033[1;31m[sendq]: " << e << "\033[0m" << std::endl;
	throw;
}
catch(const Interrupted &e)
{
	return;
}
