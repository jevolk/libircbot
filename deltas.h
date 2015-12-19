/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Deltas : std::vector<Delta>
{
	operator std::string() const                 { return string(*this);                           }

	bool all(const bool &sign) const;
	bool all(const char &mode) const;
	bool all(const Mask &mask) const;

	std::string substr(const const_iterator &begin, const const_iterator &end) const;
	Deltas &set_signs(const bool &sign);

	Deltas(void) = default;
	Deltas(std::vector<Delta> &&vec): std::vector<Delta>(std::move(vec)) {}
	Deltas(const std::vector<Delta> &vec): std::vector<Delta>(vec) {}
	Deltas(const std::string &delts, const Server *const &serv = nullptr, const bool &valid = false);
	Deltas(const std::string &delts, const Server &serv, const bool &valid = false): Deltas(delts,&serv,valid) {}

	friend std::ostream &operator<<(std::ostream &s, const Deltas &d);
};


inline
Deltas::Deltas(const std::string &delts,
               const Server *const &serv,
               const bool &valid)
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
		const auto has_arg(serv? serv->mode_has_arg(mode,sign) : tok.size() > a);
		emplace_back(sign,mode,has_arg? tok.at(a++) : std::string{});
	}

	if(serv && valid)
		for(const auto &delta : *this)
			serv->valid(delta);
}
catch(const std::out_of_range &e)
{
	throw Exception("Improperly formatted deltas string.");
}


inline
Deltas &Deltas::set_signs(const bool &sign)
{
	for(auto &d : *this)
		std::get<d.SIGN>(d) = sign;

	return *this;
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
std::ostream &operator<<(std::ostream &s, const Deltas &deltas)
{
	s << deltas.substr(deltas.begin(),deltas.end());
	return s;
}


inline
Deltas operator~(Deltas &&deltas)
{
	for(auto &d : deltas)
		std::get<d.SIGN>(d) =! std::get<d.SIGN>(d);

	return std::move(deltas);
}


inline
Deltas operator~(const Deltas &deltas)
{
	Deltas ret(deltas);
	for(auto &r : ret)
		std::get<r.SIGN>(r) =! std::get<r.SIGN>(r);

	return ret;
}


inline
bool too_many(const Server &s,
              const Deltas &deltas)
{
	return deltas.size() > s.isupport.get_or_max("MODES");
}


inline
void valid(const Server &s,
           const Deltas &deltas)
{
	if(too_many(s,deltas))
		throw Exception("Server doesn't support changing this many modes at once.");

	for(const Delta &d : deltas)
		s.valid(d);
}
