/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2022 delthas
 *   Copyright (C) 2013, 2018-2023 Sadie Powell <sadie@witchery.services>
 *   Copyright (C) 2012, 2015, 2018 Attila Molnar <attilamolnar@hush.com>
 *
 * This file is part of InspIRCd.  InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "inspircd.h"
#include "clientprotocolevent.h"
#include "modules/account.h"
#include "modules/away.h"
#include "modules/cap.h"
#include "modules/ircv3.h"
#include "modules/monitor.h"

class AwayMessage final
	: public ClientProtocol::Message
{
public:
	AwayMessage(User* user)
		: ClientProtocol::Message("AWAY", user)
	{
		SetParams(user, user->away);
	}

	AwayMessage()
		: ClientProtocol::Message("AWAY")
	{
	}

	void SetParams(User* user, const std::optional<AwayState>& awaystate)
	{
		// Going away: 1 parameter which is the away reason
		// Back from away: no parameter
		if (awaystate)
			PushParam(awaystate->message);
	}
};

class JoinHook final
	: public ClientProtocol::EventHook
{
private:
	Account::API accountapi;
	ClientProtocol::Events::Join extendedjoinmsg;

public:
	const std::string asterisk = "*";
	ClientProtocol::EventProvider awayprotoev;
	AwayMessage awaymsg;
	Cap::Capability extendedjoincap;
	Cap::Capability awaycap;

	JoinHook(Module* mod)
		: ClientProtocol::EventHook(mod, "JOIN")
		, accountapi(mod)
		, awayprotoev(mod, "AWAY")
		, extendedjoincap(mod, "extended-join")
		, awaycap(mod, "away-notify")
	{
	}

	void OnEventInit(const ClientProtocol::Event& ev) override
	{
		const ClientProtocol::Events::Join& join = static_cast<const ClientProtocol::Events::Join&>(ev);

		// An extended join has two extra parameters:
		// First the account name of the joining user or an asterisk if the user is not logged in.
		// The second parameter is the realname of the joining user.

		Membership* const memb = join.GetMember();
		const std::string* account = &asterisk;
		if (accountapi)
		{
			const std::string* accountname = accountapi->GetAccountName(memb->user);
			if (accountname)
				account = accountname;
		}

		extendedjoinmsg.ClearParams();
		extendedjoinmsg.SetSource(join);
		extendedjoinmsg.PushParamRef(memb->chan->name);
		extendedjoinmsg.PushParamRef(*account);
		extendedjoinmsg.PushParamRef(memb->user->GetRealName());

		awaymsg.ClearParams();
		if ((memb->user->IsAway()) && (awaycap.IsActive()))
		{
			awaymsg.SetSource(join);
			awaymsg.SetParams(memb->user, memb->user->away);
		}
	}

	ModResult OnPreEventSend(LocalUser* user, const ClientProtocol::Event& ev, ClientProtocol::MessageList& messagelist) override
	{
		if (extendedjoincap.IsEnabled(user))
			messagelist.front() = &extendedjoinmsg;

		if ((!awaymsg.GetParams().empty()) && (awaycap.IsEnabled(user)))
			messagelist.push_back(&awaymsg);

		return MOD_RES_PASSTHRU;
	}
};

class ModuleIRCv3 final
	: public Module
	, public Account::EventListener
	, public Away::EventListener
{
private:
	Cap::Capability cap_accountnotify;
	JoinHook joinhook;
	ClientProtocol::EventProvider accountprotoev;
	Monitor::API monitorapi;
	Cap::Capability stdrplcap;

public:
	ModuleIRCv3()
		: Module(VF_VENDOR, "Provides the IRCv3 account-notify, away-notify, extended-join, and standard-replies client capabilities.")
		, Account::EventListener(this)
		, Away::EventListener(this)
		, cap_accountnotify(this, "account-notify")
		, joinhook(this)
		, accountprotoev(this, "ACCOUNT")
		, monitorapi(this)
		, stdrplcap(this, "standard-replies")
	{
	}

	void ReadConfig(ConfigStatus& status) override
	{
		const auto& conf = ServerInstance->Config->ConfValue("ircv3");
		cap_accountnotify.SetActive(conf->getBool("accountnotify", true));
		joinhook.awaycap.SetActive(conf->getBool("awaynotify", true));
		joinhook.extendedjoincap.SetActive(conf->getBool("extendedjoin", true));
		stdrplcap.SetActive(conf->getBool("standardreplies", true));
	}

	void OnAccountChange(User* user, const std::string& newaccount) override
	{
		if (!(user->connected & User::CONN_NICKUSER))
			return;

		// Logged in: 1 parameter which is the account name
		// Logged out: 1 parameter which is a "*"
		ClientProtocol::Message msg("ACCOUNT", user);
		const std::string& param = (newaccount.empty() ? joinhook.asterisk : newaccount);
		msg.PushParamRef(param);
		ClientProtocol::Event accountevent(accountprotoev, msg);
		IRCv3::WriteNeighborsWithCap res(user, accountevent, cap_accountnotify, true);
		Monitor::WriteWatchersWithCap(monitorapi, user, accountevent, cap_accountnotify, res.GetAlreadySentId());
	}

	void OnUserAway(User* user, const std::optional<AwayState>& prevstate) override
	{
		if (!joinhook.awaycap.IsActive())
			return;

		// Going away: n!u@h AWAY :reason
		AwayMessage msg(user);
		ClientProtocol::Event awayevent(joinhook.awayprotoev, msg);
		IRCv3::WriteNeighborsWithCap res(user, awayevent, joinhook.awaycap);
		Monitor::WriteWatchersWithCap(monitorapi, user, awayevent, joinhook.awaycap, res.GetAlreadySentId());
	}

	void OnUserBack(User* user, const std::optional<AwayState>& prevstate) override
	{
		// Back from away: n!u@h AWAY
		OnUserAway(user, prevstate);
	}
};

MODULE_INIT(ModuleIRCv3)
