/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */

struct Delta : std::tuple<bool,char,Mask>
{
	enum Field                                   { SIGN, MODE, MASK                                };
	enum class Type : uint
	{
		A,   // Chan Mode that adds or removes a nick or address to a list. Always has a parameter s2c.
		B,   // Chan Mode that changes a setting and always has a parameter. May be in isup[PREFIX]
		C,   // Chan Mode that changes a setting and only has a parameter when set.
		D,   // Mode that changes a setting and never has a parameter. Defaults
	};

	static char sign(const bool &b)              { return b? '+' : '-';                            }
	static bool sign(const char &s)              { return s != '-';                                }
	static bool is_sign(const char &s)           { return s == '-' || s == '+';                    }

	explicit operator const bool &() const       { return std::get<SIGN>(*this);                   }
	explicit operator const char &() const       { return std::get<MODE>(*this);                   }
	operator std::string() const                 { return string(*this);                           }

	explicit Delta(const std::string &delta);
	Delta(const char *const &delta): Delta(std::string(delta)) {}
	Delta(const std::string &mode_delta, const Mask &mask);
	Delta(const bool &sign, const char &mode, const Mask &mask);
	Delta(const char &sign, const char &mode, const Mask &mask);

	friend std::ostream &operator<<(std::ostream &s, const Delta &delta);
};


inline
Delta::Delta(const std::string &delta)
try:
Delta(delta.substr(0,delta.find(" ")),
      delta.size() > 2? delta.substr(delta.find(" ")+1) : "")
{
}
catch(const std::out_of_range &e)
{
	throw Exception("Improperly formatted delta string (no space?).");
}


inline
Delta::Delta(const std::string &mode_delta,
             const Mask &mask)
try:
Delta(sign(mode_delta.at(0)),
      mode_delta.at(1),
      mask)
{
	if(mode_delta.size() != 2)
		throw Exception("Bad mode delta: Need two characters: [+/-][mode].");
}
catch(const std::out_of_range &e)
{
	throw Exception("Bad mode delta: Need two characters: [+/-][mode] (had less).");
}


inline
Delta::Delta(const char &sc,
             const char &mode,
             const Mask &mask):
Delta(sign(sc),mode,mask)
{
}


inline
Delta::Delta(const bool &sign,
             const char &mode,
             const Mask &mask):
std::tuple<bool,char,Mask>(sign,mode,mask)
{
}


inline
Delta operator~(Delta &&d)
{
	std::get<d.SIGN>(d) =! std::get<d.SIGN>(d);
	return std::move(d);
}


inline
Delta operator~(const Delta &d)
{
	Delta r(d);
	std::get<r.SIGN>(r) =! std::get<r.SIGN>(r);
	return r;
}


inline
bool operator==(const Delta &d, const Mask::Form &form)
{
	return std::get<d.MASK>(d) == form;
}


inline
bool operator==(const Delta &d, const char &mode)
{
	return Delta::is_sign(mode)? std::get<d.SIGN>(d) == Delta::sign(mode):
	                             std::get<d.MODE>(d) == mode;
}


inline
std::ostream &operator<<(std::ostream &s, const Delta &d)
{
	using std::get;

	s << d.sign(get<d.SIGN>(d))
	  << get<d.MODE>(d);

	if(!get<d.MASK>(d).empty())
		s << " " << get<d.MASK>(d);

	return s;
}
