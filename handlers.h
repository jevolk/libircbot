/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


enum Special
{
	ALL,         // Called on anything no matter what
	MISS,        // Called if no mapped handlers found (one-times discarded even if not called)

	_NUM_SPECIAL
};


template<class Handler>
class Handlers
{
	template<class T> static std::string name_cast(const T &num);

	std::multimap<std::string, Handler> handlers;
	std::vector<std::list<Handler>> specials            { _NUM_SPECIAL                       };

  public:
	template<class... Args> auto &add(const Special &special, Args&&... args);
	template<class... Args> auto &add(const std::string &event, Args&&... args);
	template<class... Args> auto &add(const char *const &event, Args&&... args);
	template<class T, class... Args> auto &add(const T &num, Args&&... args);

	template<class... Args> void operator()(const std::string &name, Args&&... args);
	template<class... Args> void operator()(const Msg &msg, Args&&... args);
	template<class T, class... Args> void operator()(const T &num, Args&&... args);

	void clear(const std::string &event)                { handlers.erase(event);             }
	void clear(const Special &special)                  { specials[special].clear();         }
	void clear(const Prio &prio);                       // clears handlers by priority num
	void clear_handlers()                               { handlers.clear();                  }
	void clear_specials();                              // clears all Special handlers
	void clear();                                       // clears everything
};


template<class Handler>
void Handlers<Handler>::clear()
{
	clear_specials();
	clear_handlers();
}


template<class Handler>
void Handlers<Handler>::clear_specials()
{
	for(auto &s : specials)
		s.clear();
}


template<class Handler>
void Handlers<Handler>::clear(const Prio &prio)
{
	for(auto it(handlers.begin()); it != handlers.end(); )
		if(it->second.get_prio() == prio)
			handlers.erase(it++);
		else
			++it;

	for(auto &spec : specials)
		spec.remove_if([&prio](const Handler &handler)
		{
			return handler.get_prio() == prio;
		});
}


template<class Handler>
template<class T,
         class... Args>
void Handlers<Handler>::operator()(const T &num,
                                   Args&&... args)
{
	const auto name = name_cast(num);
	operator()(name,std::forward<Args>(args)...);
}


template<class Handler>
template<class... Args>
void Handlers<Handler>::operator()(const Msg &msg,
                                   Args&&... args)
{
	const auto &name = msg.get_name();
	operator()(name,msg,std::forward<Args>(args)...);
}


template<class Handler>
template<class... Args>
void Handlers<Handler>::operator()(const std::string &name,
                                   Args&&... args)
{
	// Find out how many handlers will be called
	const auto itp = handlers.equal_range(name);
	const size_t itp_sz = std::distance(itp.first,itp.second);
	const size_t vec_sz = specials[ALL].size() + (itp_sz? itp_sz : specials[MISS].size());

	// Gather pointers to all relevant handlers
	std::vector<const Handler *> vec(vec_sz);
	auto vit = pointers(specials[ALL].begin(),specials[ALL].end(),vec.begin());

	if(itp_sz)
		std::transform(itp.first,itp.second,vit,[](const auto &vt) { return &vt.second; });
	else
		pointers(specials[MISS].begin(),specials[MISS].end(),vit);

	// Sort calling order based on handler priority
	std::sort(vec.begin(),vec.end(),[]
	(const auto &a, const auto &b)
	{
		return a->get_prio() < b->get_prio();
	});

	// Call handlers
	for(const Handler *const &handler : vec)
		(*handler)(std::forward<Args>(args)...);

	// Erase one-time mapped handlers
	for(auto it = itp.first; it != itp.second; )
		if(!it->second.is(RECURRING))
			handlers.erase(it++);
		else
			++it;

	// Erase one-time Special handlers (note: one-time MISS handlers erased even if never called)
	for(auto &s : specials)
		s.remove_if([](const auto &handler) { return !handler.is(RECURRING); });
}


template<class Handler>
template<class T,
         class... Args>
auto &Handlers<Handler>::add(const T &num,
                             Args&&... args)
{
	const auto event(name_cast(num));
	return add(event,std::forward<Args>(args)...);
}


template<class Handler>
template<class... Args>
auto &Handlers<Handler>::add(const std::string &event,
                             Args&&... args)
{
	auto iit(handlers.emplace(event,Handler{std::forward<Args>(args)...}));
	return iit->second;
}


template<class Handler>
template<class... Args>
auto &Handlers<Handler>::add(const char *const &event,
                             Args&&... args)
{
	auto iit(handlers.emplace(event,Handler{std::forward<Args>(args)...}));
	return iit->second;
}


template<class Handler>
template<class... Args>
auto &Handlers<Handler>::add(const Special &spec,
                             Args&&... args)
{
	specials.at(spec).emplace_back(Handler{std::forward<Args>(args)...});
	return specials.at(spec).back();
}


template<class Handler>
template<class T>
std::string Handlers<Handler>::name_cast(const T &num)
{
	std::stringstream name;
	name << std::setw(3) << std::setfill('0') << ssize_t(num);
	return name.str();
}
