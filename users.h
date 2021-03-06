/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Users
{
	using Cmp = CaseInsensitiveEqual<std::string>;
	std::unordered_map<std::string, User, Cmp, Cmp> users;

  public:
	// Observers
	const User &get(const std::string &nick) const;
	bool has(const std::string &nick) const              { return users.count(nick);          }
	auto num() const                                     { return users.size();               }

	// Closures
	void for_each(const std::function<void (const User &)> &c) const;
	void for_each(const std::function<void (User &)> &c);

	// Manipulators
	User &get(const std::string &nick);
	void rename(const std::string &old_nick, const std::string &new_nick);
	template<class... Args> User &add(const std::string &nick, Args&&... args);
	bool del(const User &user);

	friend std::ostream &operator<<(std::ostream &s, const Users &u);
};


inline
bool Users::del(const User &user)
{
	return users.erase(user.get_nick());
}


template<class... Args>
User &Users::add(const std::string &nick,
                 Args&&... args)
{
	const auto &iit(users.emplace(std::piecewise_construct,
	                              std::forward_as_tuple(nick),
	                              std::forward_as_tuple(nick,std::forward<Args>(args)...)));
	return iit.first->second;
}


inline
void Users::rename(const std::string &old_nick,
                   const std::string &new_nick)
{
	User &old_user(users.at(old_nick));
	User tmp_user(std::move(old_user));
	tmp_user.set_nick(new_nick);
	users.erase(old_nick);
	users.emplace(new_nick,std::move(tmp_user));
}


inline
User &Users::get(const std::string &nick)
try
{
	return users.at(nick);
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
const User &Users::get(const std::string &nick)
const try
{
	return users.at(nick);
}
catch(const std::out_of_range &e)
{
	throw Exception("User not found");
}


inline
void Users::for_each(const std::function<void (User &)> &closure)
{
	for(auto &userp : users)
		closure(userp.second);
}


inline
void Users::for_each(const std::function<void (const User &)> &closure)
const
{
	for(const auto &userp : users)
		closure(userp.second);
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Users &u)
{
	s << "Users(" << u.num() << ")" << std::endl;
	for(const auto &userp : u.users)
		s << userp.second << std::endl;

	return s;
}
