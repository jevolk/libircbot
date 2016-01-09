/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


template<class Value> using List = std::set<Value>;
template<class R, class List> using Closure = std::function<R (const typename List::value_type &)>;

template<class List> void for_each(const List &list, const Mask &match, const Closure<void,List> &func);
template<class List> void for_each(const List &list, const Mask &match, const Closure<bool,List> &func);
template<class List> Deltas compose(const List &list, const User &user, const Delta &delta);
template<class List> size_t count(const List &list, const Mask &match);
template<class List> size_t count(const List &list, const User &user);
template<class List> bool exists(const List &list, const Mask &match);
template<class List> bool exists(const List &list, const User &user);


struct Lists
{
	List<Ban> bans;
	List<Quiet> quiets;
	List<Except> excepts;
	List<Invite> invites;
	List<AKick> akicks;
	List<Flags> flags;

	// Convenience access for the flags list only
	const Flags &get_flag(const Mask &m) const;
	const Flags &get_flag(const User &u) const;
	bool has_flag(const Mask &m) const;
	bool has_flag(const User &u) const;
	bool has_flag(const Mask &m, const char &flag) const;
	bool has_flag(const User &u, const char &flag) const;

	// Finds existence of user in appropriate list by letters
	bool has_mode(const User &u, const char &list) const;
	bool has_mode(const User &u, const Mode &lists = {"bqeI"}) const;

	// Mutators
	bool set_mode(const Delta &delta);
	void delta_flag(const Mask &m, const std::string &delta);

	friend std::ostream &operator<<(std::ostream &s, const Lists &lists);
};


inline
void Lists::delta_flag(const Mask &mask,
                       const std::string &delta)
{
	if(mask.empty())
		throw Assertive("Lists::delta_flag on empty Mask");

	auto it(flags.find(Flags{mask}));
	if(it == flags.end())
		it = flags.emplace(mask).first;

	auto &f(const_cast<Flags &>(*it));
	f.delta(delta);
	f.update(time(NULL));
}


inline
bool Lists::set_mode(const Delta &d)
{
	using std::get;

	switch(char(d))
	{
		case 'b':     return bool(d)? bans.emplace(get<d.MASK>(d)).second:
		                              bans.erase(get<d.MASK>(d));

		case 'q':     return bool(d)? quiets.emplace(get<d.MASK>(d)).second:
		                              quiets.erase(get<d.MASK>(d));

		case 'e':     return bool(d)? excepts.emplace(get<d.MASK>(d)).second:
		                              excepts.erase(get<d.MASK>(d));

		case 'I':     return bool(d)? invites.emplace(get<d.MASK>(d)).second:
		                              invites.erase(get<d.MASK>(d));

		default:      return false;
	}
}


inline
bool Lists::has_mode(const User &user,
                     const Mode &lists)
const
{
	for(const auto &list : lists)
		if(has_mode(user,list))
			return true;

	return false;
}


inline
bool Lists::has_mode(const User &user,
                     const char &list)
const
{
	switch(list)
	{
		case 'b':   return exists(bans,user);
		case 'q':   return exists(quiets,user);
		case 'e':   return exists(excepts,user);
		case 'I':   return exists(invites,user);
		default:    return false;
	}
}


inline
bool Lists::has_flag(const User &u,
                     const char &flag)
const
{
	return u.is_logged_in()? has_flag(u.get_acct(),flag) : false;
}


inline
bool Lists::has_flag(const Mask &m,
                     const char &flag)
const
{
	const auto it(flags.find(Flags{m}));
	return it != flags.end()? it->get_flags().has(flag) : false;
}


inline
bool Lists::has_flag(const User &u)
const
{
	return u.is_logged_in()? has_flag(u.get_acct()) : false;
}


inline
bool Lists::has_flag(const Mask &m)
const
{
	return flags.count(Flags{m});
}


inline
const Flags &Lists::get_flag(const User &u)
const
{
	return get_flag(u.get_acct());
}


inline
const Flags &Lists::get_flag(const Mask &m)
const
{
	const auto it(flags.find(Flags{m}));
	if(it == flags.end())
		throw Exception("No flags matching this user.");

	return *it;
}


inline
std::ostream &operator<<(std::ostream &s, const Lists &l)
{
	s << "bans:      \t" << l.bans.size() << std::endl;
	for(const auto &b : l.bans)
		s << "\t+b " << b << std::endl;

	s << "quiets:    \t" << l.quiets.size() << std::endl;
	for(const auto &q : l.quiets)
		s << "\t+q " << q << std::endl;

	s << "excepts:   \t" << l.excepts.size() << std::endl;
	for(const auto &e : l.excepts)
		s << "\t+e " << e << std::endl;

	s << "invites:   \t" << l.invites.size() << std::endl;
	for(const auto &i : l.invites)
		s << "\t+I " << i << std::endl;

	s << "flags:     \t" << l.flags.size() << std::endl;
	for(const auto &f : l.flags)
		s << "\t"<< f << std::endl;

	s << "akicks:    \t" << l.akicks.size() << std::endl;
	for(const auto &a : l.akicks)
		s << "\t"<< a << std::endl;

	return s;
}


template<class List>
bool exists(const List &list,
            const User &user)
{
	if(exists(list,user.mask(Mask::NICK)))
		return true;

	if(exists(list,user.mask(Mask::HOST)))
		return true;

	if(user.is_logged_in() && exists(list,user.mask(Mask::ACCT)))
		return true;

	return false;
}


template<class List>
bool exists(const List &list,
            const Mask &match)
{
	return std::any_of(std::begin(list),std::end(list),[&match]
	(const auto &element)
	{
		return Mask(element) == match;
	});
}


template<class List>
size_t count(const List &list,
             const User &user)
{
	size_t ret(0);
	ret += count(list,user.mask(Mask::NICK));
	ret += count(list,user.mask(Mask::HOST));

	if(user.is_logged_in())
		ret += count(list,user.mask(Mask::ACCT));

	return ret;
}


template<class List>
size_t count(const List &list,
             const Mask &match)
{
	return std::count_if(std::begin(list),std::end(list),[&match]
	(const auto &element)
	{
		return Mask(element) == match;
	});
}


template<class List>
Deltas compose(const List &list,
               const User &user,
               const Delta &delta)
{
	Deltas ret;
	const auto lambda([&ret,&delta]
	(const auto &element)
	{
		ret.emplace_back(string(delta),Mask(element));
	});

	if(user.is_logged_in())
		for_each(list,user.mask(Mask::ACCT),lambda);

	for_each(list,user.mask(Mask::NICK),lambda);
	for_each(list,user.mask(Mask::HOST),lambda);

	return ret;
}


template<class List>
void for_each(const List &list,
              const Mask &match,
              const Closure<void,List> &func)
{
	for(const auto &elem : list)
		if(elem == match)
			func(elem);
}


template<class List>
void for_each(const List &list,
              const Mask &match,
              const Closure<bool,List> &func)
{
	for(const auto &elem : list)
		if(elem == match)
			if(!func(elem))
				return;
}
