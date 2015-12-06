/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


enum Flag : uint8_t
{
	RECURRING = 0x01,
};


enum Prio : uint8_t
{
	HOOK      = 0,
	LIB       = 64,
	USER      = 128,
};


using flag_t = std::underlying_type<Flag>::type;
using prio_t = std::underlying_type<Prio>::type;


template<class Prototype>
struct Handler
{
	std::function<Prototype> func;
	flag_t flags;
	prio_t prio;

  public:
	auto &get_flags() const                   { return flags;                             }
	auto &get_prio() const                    { return prio;                              }
	bool is(const flag_t &flags) const        { return (this->flags & flags) == flags;    }

	template<class... Args> void operator()(Args&&... args) const;

	Handler(const std::function<Prototype> &func    = nullptr,
	        const flag_t &flags                     = 0,
	        const prio_t &prio                      = Prio::USER);
};


template<class Prototype>
Handler<Prototype>::Handler(const std::function<Prototype> &func,
                            const flag_t &flags,
                            const prio_t &prio):
func(func),
flags(flags),
prio(prio)
{
}


template<class Prototype>
template<class... Args>
void Handler<Prototype>::operator()(Args&&... args)
const try
{
	func(std::forward<Args>(args)...);
}
catch(const Internal &e)
{
	std::cerr << "HANDLER INTERNAL: \033[1;45;37m" << e << "\033[0m" << std::endl;
	Stream::clear();
}
catch(const Interrupted &)
{
	Stream::clear();
	throw;
}
catch(const std::exception &e)
{
	std::cerr << "UNHANDLED: \033[1;41;37m" << e.what() << "\033[0m" << std::endl;
	Stream::clear();
}
