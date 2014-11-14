/**
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


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
