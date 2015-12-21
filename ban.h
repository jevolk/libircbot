/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Ban
{
	Mask mask;
	Mask oper;
	time_t time;

  public:
	using Type = Mask::Type;

	auto &get_mask() const                       { return mask;                                    }
	auto &get_oper() const                       { return oper;                                    }
	auto &get_time() const                       { return time;                                    }

	explicit operator const Mask &() const       { return get_mask();                              }

	bool operator<(const Ban &o) const           { return get_mask() < std::string(o.get_mask());  }
	bool operator==(const Ban &o) const          { return get_mask() == o.get_mask();              }

	Ban(const Mask &mask, const Mask &oper = "", const time_t &time  = 0);

	friend std::ostream &operator<<(std::ostream &s, const Ban &ban);
};


inline
Ban::Ban(const Mask &mask,
         const Mask &oper,
         const time_t &time):
mask(mask),
oper(oper),
time(time)
{
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Ban &ban)
{
	s << std::setw(64) << std::left << ban.get_mask();
	s << "by: " << std::setw(64) << std::left << ban.get_oper();
	s << "(" << ban.get_time() << ")";
	return s;
}
