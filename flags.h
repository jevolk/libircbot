/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Flags
{
	Mask mask;
	Mode flags;
	time_t time;
	bool founder;

  public:
	using Type = Mask::Type;

	auto &get_mask() const                    { return mask;                     }
	auto &get_flags() const                   { return flags;                    }
	auto &get_time() const                    { return time;                     }
	auto &is_founder() const                  { return founder;                  }
	bool has(const char &c) const             { return flags.has(c);             }

	bool operator<(const Flags &o) const      { return mask < o.mask;            }
	bool operator<(const Mask &o) const       { return mask < o;                 }
	bool operator==(const Flags &o) const     { return mask == o.mask;           }
	bool operator==(const Mask &o) const      { return mask == o;                }

	template<class... Delta> Flags &operator+=(Delta&&... delta) &;
	template<class... Delta> Flags &operator-=(Delta&&... delta) &;

	bool delta(const std::string &str) &      { return flags.delta(str);         }
	void update(const time_t &time) &         { this->time = time;               }

	Flags(const Mask &mask     = Mask(),
	      const Mode &flags    = Mode(),
	      const time_t &time   = 0,
	      const bool &founder  = false):
	      mask(mask), flags(flags), time(time), founder(founder) {}

	Flags(const Delta &delta,
	      const time_t &time,
	      const bool &founder  = false);

	Flags(const Deltas &deltas,
	      const time_t &time,
	      const bool &founder  = false);

	friend std::ostream &operator<<(std::ostream &s, const Flags &f);
};


inline
Flags::Flags(const Delta &delta,
             const time_t &time,
             const bool &founder):
Flags(std::get<Delta::MASK>(delta),
      std::get<Delta::MODE>(delta),
      time,
      founder)
{
	if(!std::get<Delta::SIGN>(delta))
		throw Assertive("No reason to construct a negative flag at this time.");
}


inline
Flags::Flags(const Deltas &deltas,
            const time_t &time,
            const bool &founder):
mask(deltas.empty()? Mask() : std::get<Delta::MASK>(deltas.at(0))),
time(time),
founder(founder)
{
	if(std::count(deltas.begin(),deltas.end(),false))
		throw Assertive("No reason to construct a negative flag at this time.");

	for(const Delta &d : deltas)
		flags.push_back(std::get<Delta::MODE>(d));
}


template<class... Delta>
Flags &Flags::operator+=(Delta&&... delta) &
{
	flags.operator+=(std::forward<Delta>(delta)...);
	return *this;
}


template<class... Delta>
Flags &Flags::operator-=(Delta&&... delta) &
{
	flags.operator-=(std::forward<Delta>(delta)...);
	return *this;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Flags &f)
{
	std::string mstr = f.get_flags();
	std::sort(mstr.begin(),mstr.end());
	s << "+" << std::setw(16) << std::left << mstr;
	s << " " << std::setw(64) << std::left << f.get_mask();
	s << " (" << f.get_time() << ")";

	if(f.is_founder())
		s << " (FOUNDER)";

	return s;
}
