/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Server
{
	std::string name;
	std::string vers;
	ISupport isupport;
	std::string user_modes, user_pmodes;
	std::string chan_modes, chan_pmodes;
	std::string serv_modes, serv_pmodes;
	std::set<std::string> caps;

	bool has_cap(const std::string &cap) const       { return caps.count(cap);                        }

	// 3.2 CHANLIMIT
	size_t get_chanlimit(const char &prefix) const;

	// 3.10 MAXLIST
	std::string get_maxlist_assoc(const char &prefix) const;
	size_t get_maxlist(const char &prefix) const;

	// 3.14 PREFIX
	bool has_prefix(const char &prefix) const;
	bool has_prefix_mode(const char &mode) const;
	char prefix_to_mode(const char &prefix) const;
	char mode_to_prefix(const char &mode) const;

	// Delta/mode rules and validations
	Delta::Type mode_type(const char &mode) const;
	Delta::Type mode_type(const Delta &delta) const;
	bool mode_has_arg(const char &mode, const bool &sign) const;
	bool mode_has_invmask(const char &m) const;
	bool need_mask(const Delta &d) const;
	bool user_mode(const char &m) const;
	bool user_mode(const Delta &d) const;
	bool chan_mode(const char &m) const;
	bool chan_mode(const Delta &d) const;
	const char *valid_arg(const Delta &delta) const;                     // static string on invalid, else null
	const char *valid(const Delta &delta, const std::nothrow_t) const;   // static string on invalid, else null
	void valid(const Delta &delta) const;                                // throws reason for invalid

	friend std::ostream &operator<<(std::ostream &s, const Server &srv);
};



inline
void Server::valid(const Delta &d)
const
{
	const auto reason(valid(d,std::nothrow));

	if(reason)
		throw Exception(reason);
}


inline
const char *Server::valid(const Delta &d,
                          const std::nothrow_t)
const
{
	if(!user_mode(d) && !chan_mode(d))
		return "Mode is not valid on this server.";

	return valid_arg(d);
}


inline
const char *Server::valid_arg(const Delta &d)
const
{
	using std::get;

	if(get<d.MASK>(d).empty() && need_mask(d))
		return "Mode requries an argument.";

	if(!get<d.MASK>(d).empty() && !need_mask(d))
		return "Mode requires no argument.";

	if(!mode_has_invmask(get<d.MODE>(d)) && get<d.MASK>(d) == Mask::INVALID)
		return "Mode requires a valid mask argument.";

	switch(get<d.MODE>(d))
	{
		case 'l':   return isnumeric(get<d.MASK>(d))? nullptr : "argument must be numeric.";
		case 'j':
		{
			const auto arg(split(get<d.MASK>(d),":"));
			return arg.first.empty() || !isnumeric(arg.first)?    "join throttle user count invalid.":
			       arg.second.empty() || !isnumeric(arg.second)?  "join throttle seconds count invalid.":
			                                                      nullptr;
		}

		default:    return nullptr;
	}
}


inline
bool Server::need_mask(const Delta &d)
const
{
	return mode_has_arg(std::get<d.MODE>(d),std::get<d.SIGN>(d));
}


inline
bool Server::chan_mode(const Delta &d)
const
{
	return chan_mode(std::get<d.MODE>(d));
}


inline
bool Server::chan_mode(const char &m)
const
{
	return chan_modes.find(m) != std::string::npos ||
	       chan_pmodes.find(m) != std::string::npos;
}


inline
bool Server::user_mode(const Delta &d)
const
{
	return user_mode(std::get<d.MODE>(d));
}


inline
bool Server::user_mode(const char &m)
const
{
	return user_modes.find(m) != std::string::npos ||
	       user_pmodes.find(m) != std::string::npos;
}


inline
bool Server::mode_has_invmask(const char &m)
const
{
	switch(m)
	{
		case 'o':
		case 'v':
		case 'l':
		case 'j':
		case 'f':  return true;
		default:   return false;
	}
}


inline
bool Server::mode_has_arg(const char &mode,
                          const bool &sign)
const
{
	switch(mode_type(mode))
	{
		case Delta::Type::A:     return true;
		case Delta::Type::B:     return true;
		case Delta::Type::C:     return sign;
		default:
		case Delta::Type::D:     return false;
	}
}


inline
Delta::Type Server::mode_type(const Delta &d)
const
{
	return mode_type(std::get<d.MODE>(d));
}


inline
Delta::Type Server::mode_type(const char &mode)
const
{
	size_t i(0);
	const auto cm(tokens(isupport["CHANMODES"],","));
	for(; i < 4; ++i)
		if(cm.size() > i && cm.at(i).find(mode) != std::string::npos)
			break;
		else if(i == 1 && has_prefix_mode(mode))
			break;

	return Delta::Type(i);
}


inline
char Server::mode_to_prefix(const char &prefix)
const
{
	const auto &pxs(isupport["PREFIX"]);
	const auto modes(between(pxs,"(",")"));
	const auto prefx(split(pxs,")").second);
	const auto pos(modes.find(prefix));
	return pos != std::string::npos? prefx.at(pos) : '\0';
}


inline
char Server::prefix_to_mode(const char &mode)
const
{
	const auto &pxs(isupport["PREFIX"]);
	const auto modes(between(pxs,"(",")"));
	const auto prefx(split(pxs,")").second);
	const auto pos(prefx.find(mode));
	return pos != std::string::npos? modes.at(pos) : '\0';
}


inline
bool Server::has_prefix(const char &prefix)
const
{
	return prefix_to_mode(prefix) != '\0';
}


inline
bool Server::has_prefix_mode(const char &mode)
const
{
	const auto &pxs(isupport["PREFIX"]);
	const auto modes(between(pxs,"(",")"));
	return modes.find(mode) != std::string::npos;
}


inline
size_t Server::get_maxlist(const char &prefix)
const
{
	for(const auto &tok : tokens(isupport["MAXLIST"],","))
	{
		const auto kv(split(tok,":"));
		if(kv.first.find(prefix) != std::string::npos)
			return kv.second.empty()? std::numeric_limits<size_t>::max():
			                          lex_cast<size_t>(kv.second);
	}

	return std::numeric_limits<size_t>::max();
}


inline
std::string Server::get_maxlist_assoc(const char &prefix)
const
{
	for(const auto &tok : tokens(isupport["MAXLIST"],","))
	{
		const auto kv(split(tok,":"));
		if(kv.first.find(prefix) != std::string::npos)
			return kv.first;
	}

	return {};
}


inline
size_t Server::get_chanlimit(const char &prefix)
const
{
	for(const auto &tok : tokens(isupport["CHANLIMIT"],","))
	{
		const auto kv(split(tok,":"));
		if(kv.first.find(prefix) != std::string::npos)
			return kv.second.empty()? std::numeric_limits<size_t>::max():
			                          lex_cast<size_t>(kv.second);
	}

	return std::numeric_limits<size_t>::max();
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Server &srv)
{
	s << "[Server Modes]: " << std::endl;
	s << "name:           " << srv.name << std::endl;
	s << "vers:           " << srv.vers << std::endl;
	s << "user modes:     [" << srv.user_modes << "]" << std::endl;
	s << "   w/ parm:     [" << srv.user_pmodes << "]" << std::endl;
	s << "chan modes:     [" << srv.chan_modes << "]" << std::endl;
	s << "   w/ parm:     [" << srv.chan_pmodes << "]" << std::endl;
	s << "serv modes:     [" << srv.serv_modes << "]" << std::endl;
	s << "   w/ parm:     [" << srv.serv_pmodes << "]" << std::endl;
	s << std::endl;

	s << "[Server Configuration]:" << std::endl;
	for(const auto &p : srv.isupport)
		s << std::setw(16) << p.first << " = " << p.second << std::endl;

	s << std::endl;

	s << "[Server Extended Capabilities]:" << std::endl;
	for(const auto &cap : srv.caps)
		s << cap << std::endl;

	return s;
}
