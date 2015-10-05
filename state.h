/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


enum Flag : uint16_t
{
	CONNECTED          = 0x0001,         // TCP handshake complete
	PROXIED            = 0x0002,         // If proxying, got 200 OK
	NEGOTIATED         = 0x0004,         // CAP registration has ended
	WELCOMED           = 0x0008,         // USER/ircd registration taken place
	IDENTIFIED         = 0x0010,         // NickServ identification confirmed

	// Faults
	TIMEOUT            = 0x0100,         // Timer expired triggered fault state
	SOCKERR            = 0x0200,         // Any socket/connection error
	SERVERR            = 0x0400,         // Server triggered the fault state

	// Masks                             // (not a flag)
	NONE               = 0x0000,         // Mask for no flags
	FAULT              = 0xff00,         // Mask for fault flags
	ALL                = 0xffff,         // Mask for all flags
};


enum class State : int
{
	FAULT              = -1,             // Handling of exceptional events
	INACTIVE           = 0,              // The session is inactive or was terminated
	CONNECTING         = 1,              // Attemping a TCP connection to target
	PROXYING           = 2,              // If proxying, sent a CONNECT, awaiting response
	NEGOTIATING        = 3,              // CAP registration is open and has not ended
	REGISTERING        = 4,              // USER/ircd registration in progress
	IDENTIFYING        = 5,              // Services identification in progress
	ACTIVE             = 6,              // Normal handling of IRC events
};


using state_t = std::underlying_type<State>::type;
using flag_t = std::underlying_type<Flag>::type;
