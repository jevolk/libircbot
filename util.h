/**
 *	COPYRIGHT 2014 (C) Jason Volk
 *	COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *	DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


extern std::locale locale;   // bot.cpp


constexpr
size_t hash(const char *const &str,
            const size_t i = 0)
{
    return !str[i]? 5381ULL : (hash(str,i+1) * 33ULL) ^ str[i];
}


inline
size_t hash(const std::string &str,
            const size_t i = 0)
{
    return i >= str.size()? 5381ULL : (hash(str,i+1) * 33ULL) ^ str.at(i);
}


template<class T = std::string,
         class... Args>
auto lex_cast(Args&&... args)
{
	return boost::lexical_cast<T>(std::forward<Args>(args)...);
}


template<class T>
std::string string(const T &t)
{
	return static_cast<std::stringstream &>(std::stringstream() << t).str();
}


template<class I,
         class O>
auto pointers(const I &begin,
              const I &end,
              O &&out)
{
	return std::transform(begin,end,out,[]
	(auto &t)
	{
		return &t;
	});
}


template<class R,
         class I>
R pointers(I&& begin,
           I&& end)
{
	R ret;
	std::transform(begin,end,std::inserter(ret,ret.end()),[]
	(typename std::iterator_traits<I>::reference t)
	{
		return &t;
	});

	return ret;
}


template<class R,
         class C>
R pointers(C&& c)
{
	R ret;
	std::transform(std::begin(c),std::end(c),std::inserter(ret,std::end(ret)),[]
	(typename C::iterator::value_type &t)
	{
		return &t;
	});

	return ret;
}


template<class I>
bool isnumeric(const I &beg,
               const I &end)
{
	return std::all_of(beg,end,[]
	(auto&& c)
	{
		return std::isdigit(c,locale);
	});
}


template<class I>
bool isalpha(const I &beg,
             const I &end)
{
	return std::all_of(beg,end,[]
	(auto&& c)
	{
		return std::isalpha(c,locale);
	});
}


template<class I,
         class T>
bool isspec(const I &beg,
            const I &end,
            const std::set<T> &spec)
{
	return std::all_of(beg,end,[&spec]
	(auto&& c)
	{
		return spec.count(c);
	});
}


template<class I>
bool isspec(const I &beg,
            const I &end,
            const std::string &spec)
{
	return isspec(beg,end,std::set<char>(std::begin(spec),std::end(spec)));
}


template<class T>
bool isalpha(const T &val)
{
	return isalpha(std::begin(val),std::end(val));
}


template<class T>
bool isnumeric(const T &val)
{
	return isnumeric(std::begin(val),std::end(val));
}


template<class T,
         class S>
bool isspec(const T &val,
            const S &spec)
{
	return isspec(std::begin(val),std::end(val),spec);
}


inline
std::string chomp(const std::string &str,
                  const std::string &c = " ")
{
	const auto pos(str.find_last_not_of(c));
	return pos == std::string::npos? str : str.substr(0,pos+1);
}


inline
std::pair<std::string, std::string> split(const std::string &str,
                                          const std::string &delim = " ")
{
	const auto pos(str.find(delim));
	return pos == std::string::npos?
	              std::make_pair(str,std::string()):
	              std::make_pair(str.substr(0,pos), str.substr(pos+delim.size()));
}


inline
std::string between(const std::string &str,
                    const std::string &a = "(",
                    const std::string &b = ")")
{
	return split(split(str,a).second,b).first;
}


inline
bool endswith(const std::string &str,
              const std::string &val)
{
	const auto pos(str.find(val));
	return pos != std::string::npos && pos == str.size() - val.size();
}


template<class It>
bool endswith_any(const std::string &str,
                  const It &begin,
                  const It &end)
{
	return std::any_of(begin,end,[&]
	(const auto &val)
	{
		return endswith(str,val);
	});
}


inline
void tokens(const std::string &str,
            const char *const &sep,
            const std::function<void (std::string)> &func)
{
	using delim = boost::char_separator<char>;

	const delim d(sep);
	const boost::tokenizer<delim> tk(str,d);
	std::for_each(std::begin(tk),std::end(tk),func);
}


template<template<class,class>
         class C = std::vector,
         class T = std::string,
         class A = std::allocator<T>>
C<T,A> tokens(const std::string &str,
              const char *const &sep = " ")
{
	using delim = boost::char_separator<char>;

	const delim d(sep);
	const boost::tokenizer<delim> tk(str,d);
	return {std::begin(tk),std::end(tk)};
}


template<class It>
std::string detok(const It &begin,
                  const It &end,
                  const std::string &sep = " ")
{
	std::ostringstream str;
	std::for_each(begin,end,[&str,sep]
	(const auto &val)
	{
		str << val << sep;
	});

	return str.str().substr(0,ssize_t(str.tellp()) - sep.size());
}


inline
bool is_arg(const std::string &token,
            const std::string &keyed = "--")
{
	return token.size() > keyed.size() && token.find(keyed) == 0;
}


template<class It>
void parse_args(const It &begin,
                const It &end,
                const std::string &keyed,   //  = "--",
                const std::string &valued,  //  = "=",
                const std::function<void (std::pair<std::string,std::string>)> &func)
{
	std::for_each(begin,end,[&keyed,&valued,&func]
	(const auto &token)
	{
		if(is_arg(token,keyed))
			func(split(token.substr(keyed.size()),valued));
	});
}


inline
void parse_args(const std::string &str,
                const std::string &keyed,   //  = "--"
                const std::string &valued,  //  = "="
                const std::string &toksep,  //  = " "
                const std::function<void (std::pair<std::string,std::string>)> &func)
{
	tokens(str,toksep.c_str(),[&keyed,&valued,&func]
	(const auto &token)
	{
		parse_args(&token,&token+1,keyed,valued,func);
	});
}


template<class It,
         class Func>
void parse_opargs(const It &begin,
                  const It &end,
                  const std::string &keyed,                //  = "--"
                  const std::vector<std::string> &ops,     //  = { "=", "!=", "<", ... } MUST BE SORTED BY LENGTH
                  Func&& func)                             // void (string key, string op, string val)
{
	std::for_each(begin,end,[&keyed,&ops,&func]
	(const auto &token)
	{
		if(!is_arg(token,keyed))
			return;

		const auto &kov(token.substr(keyed.size()));
		for(const auto &op : ops)
		{
			if(op.empty())
			{
				func(kov,op,std::string());
				break;
			}
			else if(kov.find(op) != std::string::npos)
			{
				const auto &kv(split(kov,op));
				func(kv.first,op,kv.second);
				break;
			}
		}
	});
}


template<class Func>
void parse_opargs(const std::string &str,
                  const std::string &keyed,                //  = "--"
                  const std::vector<std::string> &ops,     //  = { "=", "!=", "<", ... } MUST BE SORTED BY LENGTH
                  const std::string &toksep,               //  = " "
                  Func&& func)                             // void (string key, string op, string val)
{
	tokens(str,toksep.c_str(),[&keyed,&ops,&func]
	(const auto &token)
	{
		parse_opargs(&token,&token+1,keyed,ops,std::forward<Func>(func));
	});
}


template<class Func>
void parse_args(const std::string &str,
                const std::string &keyed,                  //  = "--"
                std::vector<std::string> ops,              //  = { "=", "!=", "<", ... }
                const std::string &toksep,                 //  = " "
                Func&& func)                               // void (string key, string op, string val)
{
	// ops are sorted from longest to shortest, longest being the hardest to match.
	std::sort(std::begin(ops),std::end(ops),[]
	(const auto &a, const auto &b)
	{
		return a.size() > b.size();
	});

	parse_opargs(str,keyed,ops,toksep,std::forward<Func>(func));
}


inline
std::string strip_args(const std::string &str,
                       const std::string &keyed = "--",
                       const std::string &toksep = " ")
{
	std::stringstream ret;
	tokens(str,toksep.c_str(),[&ret,&keyed,&toksep]
	(const auto &token)
	{
		if(!is_arg(token,keyed))
			ret << token << toksep;
	});

	return ret.str().substr(0,ssize_t(ret.tellp()) - toksep.size());
}


inline
time_t secs_cast(const std::string &dur)
{
	if(dur.empty())
		return 0;

	if(isnumeric(dur))
		return lex_cast<time_t>(dur);

	if(!isnumeric(dur.begin(),dur.begin()+dur.size()-1))
		throw Exception("Improperly formatted duration: random non-numerics");

	const char &postfix = dur.at(dur.size() - 1);
	if(!std::isalpha(postfix,locale))
		throw Exception("Improperly formatted duration: postfix must be a letter");

	time_t ret = lex_cast<time_t>(dur.substr(0,dur.size()-1));
	switch(postfix)
	{
		case 'y':  ret *= 12;
		case 'M':  ret *= 4;
		case 'w':  ret *= 7;
		case 'd':  ret *= 24;
		case 'h':  ret *= 60;
		case 'm':  ret *= 60;
		case 's':  return ret;
		default:   throw Exception("Duration postfix not recognized");
	}
}


inline
std::string secs_cast(const time_t &t)
{
	std::stringstream ret;

	const time_t minutes   { t / (60L)                              };
	const time_t hours     { t / (60L * 60L)                        };
	const time_t days      { t / (60L * 60L * 24L)                  };
	const time_t weeks     { t / (60L * 60L * 24L * 7L)             };
	const time_t months    { t / (60L * 60L * 24L * 7L * 4L)        };
	const time_t years     { t / (60L * 60L * 24L * 7L * 4L * 12L)  };

	if(years >= 1)
		ret << years << " years";
	else if(months >= 1)
		ret << months << " months";
	else if(weeks >= 1)
		ret << weeks << " weeks";
	else if(days >= 1)
		ret << days << " days";
	else if(hours >= 1)
		ret << hours << " hours";
	else if(minutes >= 1)
		ret << minutes << " minutes";
	else if(t >= 1)
		ret << t << " seconds";
	else
		ret << "now";

	return ret.str();
}


inline
std::string packetize(std::string &&str,
                      const size_t &max = 390)
{
	for(size_t i = 0, j = 0; i < str.size(); i++, j++)
	{
		if(j > max)
			str.insert(i,1,'\n');

		if(str[i] == '\n')
			j = 0;
	}

	return std::move(str);
}


inline
std::string packetize(const std::string &str,
                      const size_t &max = 390)
{
	std::string ret(str);
	return packetize(std::move(ret),max);
}


template<size_t SIZE>
const char *tolower(char (&buf)[SIZE])
{
	const size_t len = strnlen(buf,SIZE);
	std::transform(buf,buf+len,buf,[]
	(const char &c)
	{
		return std::tolower(c,locale);
	});

	return buf;
}


inline
std::string tolower(const std::string &str)
{
	std::string ret(str.size(),char());
	std::transform(str.begin(),str.end(),ret.begin(),[]
	(const char &c)
	{
		return std::tolower(c,locale);
	});

	return ret;
}


template<class T = std::string>
struct CaseInsensitiveEqual
{
	auto operator()(const T &a, const T &b) const    { return tolower(a) == tolower(b);   }
	auto operator()(const T &a) const                { return std::hash<T>()(tolower(a)); }
};


template<class T = std::string>
struct CaseInsensitiveLess
{
	auto operator()(const T &a, const T &b) const    { return tolower(a) < tolower(b);    }
};


inline
std::string decolor(const std::string &str)
{
	std::string ret(str);
	const auto end = std::remove_if(ret.begin(),ret.end(),[]
	(const char &c)
	{
		switch(c)
		{
			case '\x02':
			case '\x03':
				return true;

			default:
				return false;
		}
	});

	ret.erase(end,ret.end());
	return ret;
}


inline
std::string randstr(const size_t &len)
{
	std::string buf;
	buf.resize(len);
	std::generate(buf.begin(),buf.end(),[]
	{
		static const char *const randy = "abcdefghijklmnopqrstuvwxyz";
		return randy[rand() % strlen(randy)];
	});

	return buf;
}


inline
std::string randword(const std::string &dict = "/usr/share/dict/words")
{
	std::ifstream file(dict);
	file.seekg(0,std::ios_base::end);
	file.seekg(rand() % file.tellg(),std::ios_base::beg);

	std::string ret;
	std::getline(file,ret);
	std::getline(file,ret);
	return chomp(ret,"'s");
}


struct scope
{
	typedef std::function<void ()> Func;
	const Func func;

	scope(const Func &func): func(func) {}
	~scope() { func(); }
};


template<class M>
class unlock_guard
{
	M &m;

  public:
	unlock_guard(M &m): m(m)                       { m.unlock();  }
	unlock_guard(unlock_guard &&) = delete;
	unlock_guard(const unlock_guard &) = delete;
	~unlock_guard()                                { m.lock();    }
};
