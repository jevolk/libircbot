/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class Acct
{
	Adb *adb;
	const std::string *acct;                             // the document key ; subclass holds data

	auto &get_adb()                                     { return *adb;                              }

  public:
	auto &get_adb() const                               { return *adb;                              }
	auto &get_acct() const                              { return *acct;                             }
	bool exists() const                                 { return adb->exists(get_acct());           }

	// Get document
	Adoc get() const                                    { return adb->get(std::nothrow,get_acct()); }
	Adoc get()                                          { return adb->get(std::nothrow,get_acct()); }
	Adoc get(const std::string &key) const              { return get().get_child(key,Adoc{});       }
	Adoc get(const std::string &key)                    { return get().get_child(key,Adoc{});       }
	Adoc operator[](const std::string &key) const       { return get(key);                          }
	Adoc operator[](const std::string &key)             { return get(key);                          }

	// Get value of document
	template<class T = std::string> T get_val(const std::string &key, const T &def) const;
	template<class T = std::string> T get_val(const std::string &key, const T &def);
	template<class T = std::string> T get_val(const std::string &key) const;
	template<class T = std::string> T get_val(const std::string &key);

	// Check if document exists
	bool has(const std::string &key) const              { return !get_val(key).empty();             }

	// Set document
	void set(const Adoc &doc)                           { adb->set(get_acct(),doc);                 }
	void set(const std::string &key, const Adoc &doc);

	// Convenience for single key => value
	template<class It> void set_val(const std::string &key, It&& begin, It &&end);
	template<class T> void set_val(const std::string &key, const T &t);

	Acct(const std::string *const &acct, Adb *const &adb = bot::adb);
	Acct(Acct &&) = delete;
	Acct(const Acct &) = delete;
	Acct &operator=(Acct &&) = default;
	Acct &operator=(const Acct &) = default;
	virtual ~Acct() = default;
};


inline
Acct::Acct(const std::string *const &acct,
           Adb *const &adb):
adb(adb),
acct(acct)
{
}


template<> inline
void Acct::set_val(const std::string &key,
                   const Adoc &val)
{
	Adoc doc(get());
	doc.put_child(key,val);
	set(doc);
}


template<class T>
void Acct::set_val(const std::string &key,
                   const T &t)
{
	Adoc doc(get());
	doc.put(key,t);
	set(doc);
}


template<class It>
void Acct::set_val(const std::string &key,
                   It&& begin,
                   It&& end)
{
	Adoc doc;
	for(auto it(begin); it != end; ++it)
		doc.push(*it);

	set_val(key,doc);
}


inline
void Acct::set(const std::string &key,
               const Adoc &doc)
{
	Adoc main(get());
	main.put_child(key,doc);
	set(main);
}


template<class T>
T Acct::get_val(const std::string &key)
{
	return get_val(key,T{});
}


template<class T>
T Acct::get_val(const std::string &key)
const
{
	return get_val(key,T{});
}


template<class T>
T Acct::get_val(const std::string &key,
                const T &def)
{
	const Adoc doc(get(key));
	return doc.get_value<T>(def);
}


template<class T>
T Acct::get_val(const std::string &key,
                const T &def)
const
{
	const Adoc doc(get(key));
	return doc.get_value<T>(def);
}
