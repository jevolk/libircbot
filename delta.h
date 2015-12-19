/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */

struct Delta : std::tuple<bool,char,Mask>
{
	enum Field                                      { SIGN, MODE, MASK                             };
	enum Valid                                      { VALID, NOT_FOUND, NEED_MASK, CANT_MASK       };
	enum class Type : uint
	{
		A,   // Chan Mode that adds or removes a nick or address to a list. Always has a parameter s2c.
		B,   // Chan Mode that changes a setting and always has a parameter. May be in isup[PREFIX]
		C,   // Chan Mode that changes a setting and only has a parameter when set.
		D,   // Mode that changes a setting and never has a parameter. Defaults
	};

	static char sign(const bool &b)                 { return b? '+' : '-';                         }
	static bool sign(const char &s)                 { return s != '-';                             }
	static bool is_sign(const char &s)              { return s == '-' || s == '+';                 }

	static Type type(const Server &s, const char &m);
	static bool has_arg(const Server &s, const char &m, const bool &sign);
	static bool has_invalid_mask(const char &m);

	Type type(const Server &s) const                { return type(s,std::get<MODE>(*this));        }
	bool need_mask_chan(const Server &s) const;
	bool need_mask_user(const Server &s) const;
	bool exists_chan(const Server &s) const;
	bool exists_user(const Server &s) const;
	bool empty_mask() const                         { return std::get<MASK>(*this).empty();        }

	// returns reason
	Valid check_chan(const Server &s) const;
	Valid check_user(const Server &s) const;

	// false on not VALID
	bool valid_chan(const Server &s) const          { return check_chan(s) == VALID;               }
	bool valid_user(const Server &s) const          { return check_user(s) == VALID;               }

	// Throws on not VALID
	void validate_chan(const Server &s) const;
	void validate_user(const Server &s) const;

	void check() const;                             // Throws why failed

	bool operator==(const Mask::Form &form) const   { return std::get<MASK>(*this) == form;        }
	bool operator==(const Mask &mask) const         { return std::get<MASK>(*this) == mask;        }
	bool operator==(const bool &sign) const         { return std::get<SIGN>(*this) == sign;        }
	bool operator==(const char &mode) const;        // compares with sign (+/-) or mode char

	explicit operator const bool &() const          { return std::get<SIGN>(*this);                }
	explicit operator const char &() const          { return std::get<MODE>(*this);                }
	operator std::string() const;

	Delta operator~() const &;                      // Invert the sign
	Delta operator~() &&;                           // Invert the sign

	Delta(const bool &sign, const char &mode, const Mask &mask);
	Delta(const char &sign, const char &mode, const Mask &mask);
	Delta(const std::string &mode_delta, const Mask &mask);
	Delta(const std::string &delta);

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
Delta Delta::operator~()
&&
{
	std::get<SIGN>(*this) =! std::get<SIGN>(*this);
	return std::move(*this);
}


inline
Delta Delta::operator~()
const &
{
	Delta ret(*this);
	std::get<SIGN>(ret) =! std::get<SIGN>(ret);
	return ret;
}


inline
Delta::operator std::string()
const
{
	return string(*this);
}


inline
bool Delta::operator==(const char &mode)
const
{
	return is_sign(mode)? std::get<SIGN>(*this) == sign(mode):
	                      std::get<MODE>(*this) == mode;
}


inline
void Delta::check()
const
{
	if(!empty_mask())
	{
		if(has_invalid_mask(std::get<MODE>(*this)) && std::get<MASK>(*this) != Mask::INVALID)
			throw Exception("Mode does not require a hostmask argument.");
	}
}


inline
void Delta::validate_user(const Server &s)
const
{
	switch(check_user(s))
	{
		case NOT_FOUND:       throw Exception("Mode is not valid for users on this server.");
		case NEED_MASK:       throw Exception("Mode requries an argument.");
		case CANT_MASK:       throw Exception("Mode does not have an argument.");
		case VALID:
		default:              return;
	};
}


inline
void Delta::validate_chan(const Server &s)
const
{
	switch(check_chan(s))
	{
		case NOT_FOUND:       throw Exception("Mode is not valid for channels on this server.");
		case NEED_MASK:       throw Exception("Mode requries an argument.");
		case CANT_MASK:       throw Exception("Mode does not have an argument.");
		case VALID:
		default:              return;
	};
}


inline
Delta::Valid Delta::check_user(const Server &s)
const
{
	if(!exists_user(s))
		return NOT_FOUND;

	if(empty_mask() && need_mask_user(s))
		return NEED_MASK;

	if(!empty_mask() && !need_mask_user(s))
		return CANT_MASK;

	return VALID;
}

inline
Delta::Valid Delta::check_chan(const Server &s)
const
{
	if(!exists_chan(s))
		return NOT_FOUND;

	if(empty_mask() && need_mask_chan(s))
		return NEED_MASK;

	if(!empty_mask() && !need_mask_chan(s))
		return CANT_MASK;

	return VALID;
}


inline
bool Delta::need_mask_chan(const Server &s)
const
{
	return has_arg(s,std::get<MODE>(*this),std::get<SIGN>(*this));
}


inline
bool Delta::need_mask_user(const Server &s)
const
{
	return s.user_pmodes.find(std::get<MODE>(*this)) != std::string::npos;
}


inline
bool Delta::exists_chan(const Server &s)
const
{
	return s.chan_modes.find(std::get<MODE>(*this)) != std::string::npos;
}


inline
bool Delta::exists_user(const Server &s)
const
{
	return s.user_modes.find(std::get<MODE>(*this)) != std::string::npos;
}


inline
bool Delta::has_invalid_mask(const char &m)
{
	switch(m)
	{
		case 'o':
		case 'v':
		case 'l':
		case 'j':
		case 'f':
			return true;

		default:
			return false;
	}
}


inline
bool Delta::has_arg(const Server &serv,
                    const char &mode,
                    const bool &sign)
{
	switch(type(serv,mode))
	{
		case Type::A:     return true;
		case Type::B:     return true;
		case Type::C:     return sign;
		default:
		case Type::D:     return false;
	}
}


inline
Delta::Type Delta::type(const Server &serv,
                        const char &mode)
{
	size_t i(0);
	const auto &isup(serv.isupport);
	const auto cm(tokens(isup["CHANMODES"],","));
	for(; i < 4; ++i)
		if(cm.size() > i && cm.at(i).find(mode) != std::string::npos)
			break;
		else if(i == 1 && serv.has_prefix(mode))
			break;

	return Type(i);
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Delta &d)
{
	using std::get;

	s << d.sign(get<d.SIGN>(d));
	s << get<d.MODE>(d);

	if(!d.empty_mask())
		s << " " << get<d.MASK>(d);

	return s;
}
