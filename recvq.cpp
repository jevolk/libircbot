/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


#include "bot.h"

using namespace irc::bot;


decltype(recvq::mutex)        recvq::mutex;
decltype(recvq::interrupted)  recvq::interrupted;
decltype(recvq::ios)          recvq::ios;
decltype(recvq::thread)       recvq::thread;
static const scope join([]
{
	recvq::interrupt();
	for(auto &thread : recvq::thread)
	{
		thread->join();
		delete thread;
	}
});


void recvq::interrupt()
{
	interrupted.store(true,std::memory_order_release);
	ios.stop();
}


void recvq::reset()
{
	const std::lock_guard<decltype(mutex)> lock(mutex);
	if(thread.size() <= 1)
		ios.reset();
}


size_t recvq::num_threads()
{
	const std::lock_guard<decltype(mutex)> lock(mutex);
	return thread.size();
}


void recvq::add_thread(const size_t &num)
{
	const std::lock_guard<decltype(mutex)> lock(mutex);
	for(size_t i(0); i < num; ++i)
		thread.emplace_back(new std::thread(&recvq::worker));
}


void recvq::min_threads(const size_t &num)
{
	const std::lock_guard<decltype(mutex)> lock(mutex);
	for(size_t i(thread.size()); i < num; ++i)
		thread.emplace_back(new std::thread(&recvq::worker));
}


void recvq::worker()
{
	const boost::asio::io_service::work work(ios);

	while(!interrupted.load(std::memory_order_consume)) try
	{
		const scope r(std::bind(&recvq::reset));
		ios.run();
	}
	catch(const Interrupted &e)
	{
		continue;
	}
}
