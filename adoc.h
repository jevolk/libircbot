/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Adoc : public boost::property_tree::ptree
{
	operator std::string() const;

	bool has(const std::string &key) const               { return !get(key,std::string{}).empty();  }
	bool has_child(const std::string &key) const         { return count(key) > 0;                   }
	auto operator[](const std::string &key) const        { return get(key,std::string{});           }

	void recurse(const std::function<void (const std::string &, const Adoc &)> &func) const;
	void for_each(const std::function<void (const std::string &, const std::string &)> &func) const;

	// Array document utils
	template<class C> C into() const;
	template<class C, class It> auto into(C &c, It&& i) const;

	template<class T> Adoc &push(const T &val) &;
	template<class I> Adoc &push(const I &beg, const I &end) &;

	bool remove(const std::string &key) &;
	Adoc &merge(const Adoc &src) &;                      // src takes precedence over this

	// Special ctor for key/vals i.e "--foo=bar" to {"foo": "bar"}
	static constexpr const struct arg_ctor_t {} arg_ctor {};
	Adoc(arg_ctor_t, const std::string &str, const std::string &keyed = "--", const std::string &valued = "=", const std::string &toksep = " ");

	// Primary ctors
	Adoc(const std::string &str = "{}");
	Adoc(boost::property_tree::ptree &&p):               boost::property_tree::ptree(std::move(p)) {}
	Adoc(const boost::property_tree::ptree &p):          boost::property_tree::ptree(p) {}
	template<class It> Adoc(It&& begin, It&& end);

	friend std::ostream &operator<<(std::ostream &s, const Adoc &adoc);
};


template<class It>
Adoc::Adoc(It&& beg,
           It&& end)
{
	std::for_each(beg,end,[this]
	(const auto &val)
	{
		this->push(val);
	});
}


inline
Adoc::Adoc(const std::string &str)
try:
boost::property_tree::ptree([str]
{
	std::stringstream ss(str);
	boost::property_tree::ptree ret;
	boost::property_tree::json_parser::read_json(ss,ret);
	return ret;
}())
{

}
catch(const boost::property_tree::json_parser::json_parser_error &e)
{
	throw Exception(e.what());
}


inline
Adoc::Adoc(arg_ctor_t,
           const std::string &str,
           const std::string &keyed,
           const std::string &valued,
           const std::string &toksep)
{
	parse_args(str,keyed,valued,toksep,[&]
	(const auto &kv)
	{
		this->put(kv.first,kv.second);
	});
}


inline
Adoc &Adoc::merge(const Adoc &src)
&
{
	const std::function<void (const Adoc &src, Adoc &dst)> recurse = [&recurse]
	(const Adoc &src, Adoc &dst)
	{
		for(const auto &pair : src)
		{
			const auto &key(pair.first);
			const auto &sub(pair.second);

			if(!sub.empty())
			{
				Adoc child(dst.get_child(key,Adoc{}));
				recurse(sub,child);

				if(key.empty())
					dst.push(child);
				else
					dst.put_child(key,child);
			}
			else if(key.empty())
				dst.push(sub.get("",std::string{}));
			else
				dst.put_child(key,sub);
		}
	};

	recurse(src,*this);
	return *this;
}


inline
bool Adoc::remove(const std::string &key)
&
{
	const auto pos(key.rfind("."));
	if(pos == std::string::npos)
		return erase(key);

	const auto path(key.substr(0,pos));
	const auto skey(key.substr(pos+1));

	if(skey.empty())
		erase(path);

	auto &doc(get_child(path));
	const auto ret(doc.erase(skey));

	if(doc.empty())
		erase(path);

	return ret;
}


template<class I>
Adoc &Adoc::push(const I &beg,
                 const I &end)
&
{
	std::for_each(beg,end,[this]
	(const auto &val)
	{
		this->push(val);
	});

	return *this;
}


template<class T>
Adoc &Adoc::push(const T &val)
&
{
	Adoc tmp;
	tmp.put("",val);
	push_back({"",tmp});
	return *this;
}


template<class C>
C Adoc::into()
const
{
	C ret;
	into(ret,ret.begin());
	return ret;
}


template<class C,
         class It>
auto Adoc::into(C &c,
                It&& i)
const
{
	return std::transform(begin(),end(),std::inserter(c,i),[]
	(const auto &p)
	{
		return p.second.get<typename C::value_type>("");
	});
}


inline
void Adoc::for_each(const std::function<void (const std::string &key, const std::string &val)> &func)
const
{
	recurse([&]
	(const std::string &key, const Adoc &doc)
	{
		func(key,doc[key]);
	});
}


inline
void Adoc::recurse(const std::function<void (const std::string &key, const Adoc &doc)> &func)
const
{
	const std::function<void (const Adoc &)> re = [&re,&func]
	(const Adoc &doc)
	{
		for(const auto &pair : doc)
		{
			const auto &key(pair.first);
			const auto &sub(pair.second);
			func(key,doc);
			re(sub);
		}
	};

	re(*this);
}


inline
Adoc::operator std::string()
const
{
	std::stringstream ss;
	ss << (*this);
	return ss.str();
}


inline
std::ostream &operator<<(std::ostream &s,
                         const Adoc &doc)
{
	boost::property_tree::write_json(s,doc,false);
	return s;
}
