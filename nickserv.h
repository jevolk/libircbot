/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


class NickServ : public Service
{
	Users &users;
	Chans &chans;
	Events &events;

	void handle_identified(const Capture &capture);
	void handle_listchans(const Capture &capture);
	void handle_info(const Capture &capture);
	void captured(const Capture &capture) override;        // Fully collected messages from Service
	NickServ &operator<<(const flush_t f) override;

  public:
	void identify(const std::string &acct, const std::string &pass);
	void regain(const std::string &nick, const std::string &pass = "");
	void ghost(const std::string &nick, const std::string &pass);
	void listchans();

	NickServ(Users &users, Chans &chans, Events &events);
};


inline
NickServ::NickServ(Users &users,
                   Chans &chans,
                   Events &events):
Service("NickServ"),
users(users),
chans(chans),
events(events)
{
}


inline
void NickServ::listchans()
{
	Stream &out(*this);
	out << "LISTCHANS" << flush;
	terminator_next("channel access match");
}


inline
void NickServ::identify(const std::string &acct,
                        const std::string &pass)
{
	Stream &out(*this);
	out << "identify" << " " << acct << " " << pass << flush;
	terminator_next("You are now identified");
}


inline
void NickServ::ghost(const std::string &nick,
                     const std::string &pass)
{
	Stream &out(*this);
	out << "ghost" << " " << nick << " " << pass << flush;
	terminator_any();
}


inline
void NickServ::regain(const std::string &nick,
                      const std::string &pass)
{
	Stream &out(*this);
	out << "regain" << " " << nick << " " << pass << flush;
	terminator_any();
}


inline
void NickServ::captured(const Capture &msg)
{
	const auto &header(msg.front());
	if(header.find("Information on") == 0)
		handle_info(msg);
	else if(header.find("Access flag(s)") == 0)
		handle_listchans(msg);
	else if(header.find("You are now identified") == 0)
		handle_identified(msg);
	else
		throw Exception("Unhandled NickServ capture.");
}


inline
void NickServ::handle_info(const Capture &msg)
{
	const auto tok(tokens(msg.front()));
	const auto &name(tolower(tok.at(2)));
	const auto &primary(tolower(tok.at(4).substr(0,tok.at(4).size()-2)));  // Chop off "):]"

	Acct acct(&name);
	Adoc info(acct.get("info"));
	info.put("account",primary);
	info.put("_fetched_",time(NULL));

	auto it(msg.begin());
	for(++it; it != msg.end(); ++it)
	{
		const auto kv(split(*it," : "));
		const auto &key(chomp(chomp(kv.first),"."));
		const auto &val(kv.second);
		info.put(key,val);
	}

	acct.set("info",info);
}


inline
void NickServ::handle_listchans(const Capture &msg)
{
	auto &sess(get_sess());

	for(const auto &line : msg)
	{
		const auto tok(tokens(line));
		const Mode flags(tok.at(2).substr(1)); // chop leading +
		const auto chan(tok.at(4));
		sess.access[chan] = flags;
	}

	// Automatically join the channels where we have access
	if(sess.has_opt("as-a-service") && (!sess.has_opt("cloaked") || sess.is(CLOAKED)))
		chans.servicejoin();
}


inline
void NickServ::handle_identified(const Capture &capture)
{
	auto &sess(get_sess());
	sess.set(IDENTIFIED);
}


inline
NickServ &NickServ::operator<<(const flush_t)
{
	const scope clear([&]
	{
		Stream::clear();
	});

	Quote ns("ns");
	ns << get_str() << flush;
	return *this;
}


inline
std::ostream &operator<<(std::ostream &s,
                         const NickServ &ns)
{
	s << dynamic_cast<const Service &>(ns) << std::endl;
	return s;
}
