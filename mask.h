/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Mask : std::string
{
	enum Form                             { INVALID, CANONICAL, EXTENDED                          };
	enum Type                             { NICK, USER, HOST, ACCT, JOIN, GECOS, FULL, SSL        };

	auto num(const char &c) const         { return std::count(begin(),end(),c);                   }
	bool has(const char &c) const         { return find(c) != std::string::npos;                  }

	// Canonical mask related
	auto get_nick() const                 { return substr(0,find('!'));                           }
	auto get_user() const                 { return substr(find('!')+1,find('@') - find('!') - 1); }
	auto get_host() const                 { return substr(find('@')+1,npos);                      }
	bool has_ident() const                { return !has('~');                                     }
	bool has_all_wild() const             { return std::string(*this) == "*!*@*";                 }
	bool has_wild(const Type &t) const;
	bool is_canonical() const;

	// Extended mask utils
	bool has_negation() const             { return has('~');                                      }
	bool has_mask() const                 { return has(':');                                      }
	auto get_mask() const                 { return has_mask()? substr(find(':')+1,npos) : "";     }
	auto &get_exttype() const             { return has_negation()? at(2) : at(1);                 }
	bool has_wild_mask() const            { return get_mask().empty() || get_mask() == "*";       }
	bool is_extended() const              { return size() > 1 && at(0) == '$';                    }

	Mask(void) = default;
	Mask(const char *const &s): std::string(s) {}
	Mask(const std::string &s): std::string(s) {}
	Mask(std::string &&s): std::string(std::move(s)) {}
	Mask(Mask &&) = default;
	Mask(const Mask &) = default;
	Mask &operator=(Mask &&) = default;
	Mask &operator=(const Mask &) = default;
};


inline
bool Mask::is_canonical()
const
{
	return has('!') &&
	       has('@') &&
	       num('!') == 1 &&
	       num('@') == 1 &&
	       find('!') < find('@') &&
	       !get_nick().empty() &&
	       !get_user().empty() &&
	       !get_host().empty();
}


inline
bool Mask::has_wild(const Type &t)
const
{
	switch(t)
	{
		case NICK:   return get_nick() == "*";
		case USER:   return get_user() == "*";
		case HOST:   return get_host() == "*";
		default:     return false;
	}
}


inline
Mask::Form form(const Mask &mask)
{
	return mask.is_canonical()?  Mask::CANONICAL:
	       mask.is_extended()?   Mask::EXTENDED:
	                             Mask::INVALID;
}


inline
bool operator==(const Mask &a, const Mask::Form &f)
{
	return form(a) == f;
}


inline
bool operator!=(const Mask &a, const Mask::Form &f)
{
	return form(a) != f;
}


inline
bool operator==(const Mask &a, const Mask &b)
{
	return tolower(a) == tolower(b);
}


inline
bool operator!=(const Mask &a, const Mask &b)
{
	return tolower(a) != tolower(b);
}


inline
bool operator<=(const Mask &a, const Mask &b)
{
	return tolower(a) <= tolower(b);
}


inline
bool operator>=(const Mask &a, const Mask &b)
{
	return tolower(a) >= tolower(b);
}


inline
bool operator<(const Mask &a, const Mask &b)
{
	return tolower(a) < tolower(b);
}


inline
bool operator>(const Mask &a, const Mask &b)
{
	return tolower(a) > tolower(b);
}


inline
bool operator==(const Mask &a, const std::string &b)
{
	return tolower(a) == tolower(b);
}


inline
bool operator!=(const Mask &a, const std::string &b)
{
	return tolower(a) != tolower(b);
}


inline
bool operator<=(const Mask &a, const std::string &b)
{
	return tolower(a) <= tolower(b);
}


inline
bool operator>=(const Mask &a, const std::string &b)
{
	return tolower(a) >= tolower(b);
}


inline
bool operator<(const Mask &a, const std::string &b)
{
	return tolower(a) < tolower(b);
}


inline
bool operator>(const Mask &a, const std::string &b)
{
	return tolower(a) > tolower(b);
}
