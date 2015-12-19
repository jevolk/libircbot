/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Deltas : std::vector<Delta>
{
	bool all(const bool &sign) const;
	bool all(const char &mode) const;
	bool all(const Mask &mask) const;

	bool too_many(const Server &s) const         { return size() > s.isupport.get_or_max("MODES"); }
	void validate_chan(const Server &s) const;
	void validate_user(const Server &s) const;

	std::string substr(const const_iterator &begin, const const_iterator &end) const;
	operator std::string() const;

	template<class... T> Deltas &operator<<(T&&... t);
	Deltas &set_signs(const bool &sign);
	Deltas operator~() const &;                  // invert the signs
	Deltas operator~() &&;                       // invert the signs

	Deltas() = default;
	Deltas(std::vector<Delta> &&vec):       std::vector<Delta>(std::move(vec)) {}
	Deltas(const std::vector<Delta> &vec):  std::vector<Delta>(vec) {}

	//NOTE: Modes with arguments must only be chan_pmodes for now
	Deltas(const std::string &delts, const Server *const &serv = nullptr);
	Deltas(const std::string &delts, const Server &serv): Deltas(delts,&serv) {}

	friend std::ostream &operator<<(std::ostream &s, const Deltas &d);
};


inline
Deltas::Deltas(const std::string &delts,
               const Server *const &serv)
try
{
	const auto tok(tokens(delts));
	const auto &ms(tok.at(0));

	// Handle an empty mode string or simply a "+" string.
	if(ms.empty() || (Delta::is_sign(ms.at(0)) && ms.size() == 1))
		return;

	auto sign(Delta::sign(ms.at(0)));
	for(size_t i(0), a(1); i < ms.size(); i++)
	{
		if(Delta::is_sign(ms.at(i)))
			sign = Delta::sign(ms.at(i++));

		const auto &mode(ms.at(i));
		const auto has_arg(serv? Delta::has_arg(*serv,mode,sign) : tok.size() > a);
		emplace_back(sign,mode,has_arg? tok.at(a++) : std::string{});
	}
}
catch(const std::out_of_range &e)
{
	throw Exception("Improperly formatted deltas string.");
}


inline
Deltas Deltas::operator~()
&&
{
	for(auto &d : *this)
		std::get<d.SIGN>(d) =! std::get<d.SIGN>(d);

	return std::move(*this);
}


inline
Deltas Deltas::operator~()
const &
{
	Deltas ret(*this);
	for(auto &d : ret)
		std::get<d.SIGN>(d) =! std::get<d.SIGN>(d);

	return ret;
}


inline
Deltas &Deltas::set_signs(const bool &sign)
{
	for(auto &d : *this)
		std::get<d.SIGN>(d) = sign;

	return *this;
}


template<class... T>
Deltas &Deltas::operator<<(T&&... t)
{
	emplace_back(std::forward<T>(t)...);
	return *this;
}


inline
void Deltas::validate_user(const Server &s)
const
{
	if(too_many(s))
		throw Exception("Server doesn't support changing this many modes at once.");

	for(const Delta &delta : *this)
		delta.validate_user(s);
}


inline
void Deltas::validate_chan(const Server &s)
const
{
	if(too_many(s))
		throw Exception("Server doesn't support changing this many modes at once.");

	for(const Delta &delta : *this)
		delta.validate_chan(s);
}


inline
bool Deltas::all(const bool &sign)
const
{
	return std::all_of(begin(),end(),[&sign]
	(const Delta &d)
	{
		return std::get<d.SIGN>(d) == sign;
	});
}


inline
bool Deltas::all(const char &mode)
const
{
	return std::all_of(begin(),end(),[&mode]
	(const Delta &d)
	{
		return std::get<d.MODE>(d) == mode;
	});
}


inline
bool Deltas::all(const Mask &mask)
const
{
	return std::all_of(begin(),end(),[&mask]
	(const Delta &d)
	{
		return std::get<d.MASK>(d) == mask;
	});
}


inline
Deltas::operator std::string()
const
{
	return string(*this);
}


inline
std::string Deltas::substr(const Deltas::const_iterator &begin,
                           const Deltas::const_iterator &end)
const
{
	std::stringstream s;

	for(auto it(begin); it != end; ++it)
	{
		const auto &d(*it);
		s << d.sign(std::get<d.SIGN>(d)) << std::get<d.MODE>(d);
	}

	for(auto it(begin); it != end; ++it)
	{
		const auto &d(*it);
		s << " " << std::get<d.MASK>(d);
	}

	return s.str();
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Deltas &deltas)
{
	s << deltas.substr(deltas.begin(),deltas.end());
	return s;
}
