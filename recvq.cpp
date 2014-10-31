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


void recvq::add_thread()
{
	const std::lock_guard<decltype(mutex)> lock(mutex);
	thread.emplace_back(new std::thread(&recvq::worker));
}


void recvq::interrupt()
{
	interrupted.store(true,std::memory_order_release);
	ios.stop();
}


void recvq::reset()
{
	const std::lock_guard<decltype(mutex)> lock(mutex);
	if(thread.size() == 1)
		ios.reset();
}


void recvq::worker()
{
	while(!interrupted.load(std::memory_order_consume)) try
	{
		const scope r(std::bind(&recvq::reset));
		ios.run();
	}
	catch(const Internal &e)
	{
		std::cerr << "INTERNAL: \033[1;41;37m" << e << "\033[0m" << std::endl;
		continue;
	}
	catch(const Interrupted &e)
	{
		return;
	}
	catch(const std::exception &e)
	{
		std::cerr << "UNHANDLED: \033[1;45;37m" << e.what() << "\033[0m" << std::endl;
		continue;
	}
}
