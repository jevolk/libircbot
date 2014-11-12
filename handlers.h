/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


enum Special
{
	ALL,
	MISSING,
};


template<class Handler>
class Handlers
{
	static std::string name_cast(const uint &num);

	std::multimap<std::string, Handler> handlers;
	std::list<Handler> any, miss;

  public:
	template<class... Args> auto &add(const Special &special, Args&&... args);
	template<class... Args> auto &add(const std::string &event, Args&&... args);
	template<class... Args> auto &add(const char *const &event, Args&&... args);
	template<class... Args> auto &add(const uint &rpl_num, Args&&... args);

	template<class... Args> void operator()(const std::string &name, Args&&... args);
	template<class... Args> void operator()(const uint &num, Args&&... args);
	template<class... Args> void operator()(const Msg &msg, Args&&... args);
};


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
void Handlers<Handler>::operator()(const uint &num,
                                   Args&&... args)
{
	const auto name = name_cast(num);
	operator()(name,std::forward<Args>(args)...);
}


template<class Handler>
template<class... Args>
void Handlers<Handler>::operator()(const std::string &name,
                                   Args&&... args)
{
	const auto itp = handlers.equal_range(name);
	const size_t itp_sz = std::distance(itp.first,itp.second);
	std::vector<const Handler *> vec(any.size() + (itp_sz? itp_sz : miss.size()));
	auto vit = pointers(any.begin(),any.end(),vec.begin());
	if(itp_sz)
		std::transform(itp.first,itp.second,vit,[](const auto &vt)
		{
			return &(vt.second);
		});
	else
		pointers(miss.begin(),miss.end(),vit);

	std::sort(vec.begin(),vec.end(),[]
	(const auto &a, const auto &b)
	{
		return a->get_prio() < b->get_prio();
	});

	for(const Handler *handler : vec)
		(*handler)(std::forward<Args>(args)...);

	for(auto it = itp.first; it != itp.second; )
	{
		const auto &handler = it->second;
		if(!handler.is(RECURRING))
			handlers.erase(it++);
		else
			++it;
	}

	any.remove_if([](const auto &handler)
	{
		return !handler.is(RECURRING);
	});

	if(!itp_sz)
		miss.remove_if([](const auto &handler)
		{
			return !handler.is(RECURRING);
		});
}


template<class Handler>
template<class... Args>
auto &Handlers<Handler>::add(const uint &num,
                             Args&&... args)
{
	const auto event = name_cast(num);
	return add(event,Handler{std::forward<Args>(args)...});
}


template<class Handler>
template<class... Args>
auto &Handlers<Handler>::add(const std::string &event,
                             Args&&... args)
{
	auto iit = handlers.emplace(event,Handler{std::forward<Args>(args)...});
	return iit->second;
}


template<class Handler>
template<class... Args>
auto &Handlers<Handler>::add(const char *const &event,
                             Args&&... args)
{
	auto iit = handlers.emplace(event,Handler{std::forward<Args>(args)...});
	return iit->second;
}


template<class Handler>
template<class... Args>
auto &Handlers<Handler>::add(const Special &spec,
                             Args&&... args)
{
	switch(spec)
	{
		case ALL:
			any.emplace_back(Handler{std::forward<Args>(args)...});
			return any.back();

		case MISSING:
			miss.emplace_back(Handler{std::forward<Args>(args)...});
			return miss.back();

		default:
			throw Assertive("Special event type not known");
	}
}


template<class Handler>
std::string Handlers<Handler>::name_cast(const uint &num)
{
	std::stringstream name;
	name << std::setw(3) << std::setfill('0') << num;
	return name.str();
}
