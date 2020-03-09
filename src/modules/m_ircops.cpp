/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2012 Shawn Smith <shawn@inspircd.org>
 *   Copyright (C) 2009 Daniel De Graaf <danieldg@inspircd.org>
 *   Copyright (C) 2006-2008 Robin Burchell <robin+git@viroteck.net>
 *   Copyright (C) 2008 Pippijn van Steenhoven <pip88nl@gmail.com>
 *   Copyright (C) 2006, 2008 Craig Edwards <craigedwards@brainbox.cc>
 *   Copyright (C) 2007 Dennis Friis <peavey@inspircd.org>
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


/**
 *
 * $ModDesc: Provides support for vIRCio Network user modes
 * Author: Diego Agudo <diego@agudo.eti.br>
 *
 */

#include "inspircd.h"


/**
 * User mode +O - mark a user as services operator
 */
class User_O : public ModeHandler
{

public:
	User_O(Module* Creator) : ModeHandler(Creator, "u_services_oper", 'O', PARAM_NONE, MODETYPE_USER) { }

	ModeAction OnModeChange(User* source, User* dest, Channel* channel, std::string &parameter, bool adding)
	{
		if (!IS_LOCAL(source))
		{
			if ((adding != dest->IsModeSet('O')))
			{
				dest->SetMode('O',adding);
				return MODEACTION_ALLOW;
			}
		}
		else
		{
			source->WriteNumeric(500, "%s :Only a server may modify the +O user mode", source->nick.c_str());
		}
		return MODEACTION_DENY;
	}
};

/**
 * User mode +A - mark a user as services admin
 */
class User_A : public ModeHandler
{

public:
	User_A(Module* Creator) : ModeHandler(Creator, "u_services_admin", 'A', PARAM_NONE, MODETYPE_USER) { }

	ModeAction OnModeChange(User* source, User* dest, Channel* channel, std::string &parameter, bool adding)
	{
		if (!IS_LOCAL(source))
		{
			if ((adding != dest->IsModeSet('A')))
			{
				dest->SetMode('A',adding);
				return MODEACTION_ALLOW;
			}
		}
		else
		{
			source->WriteNumeric(500, "%s :Only a server may modify the +A user mode", source->nick.c_str());
		}
		return MODEACTION_DENY;
	}
};

/**
 * User mode +N - mark a user as services admin
 */
class User_N : public ModeHandler
{

public:
	User_N(Module* Creator) : ModeHandler(Creator, "u_services_root", 'N', PARAM_NONE, MODETYPE_USER) { }

	ModeAction OnModeChange(User* source, User* dest, Channel* channel, std::string &parameter, bool adding)
	{
		if (!IS_LOCAL(source))
		{
			if ((adding != dest->IsModeSet('N')))
			{
				dest->SetMode('N',adding);
				return MODEACTION_ALLOW;
			}
		}
		else
		{
			source->WriteNumeric(500, "%s :Only a server may modify the +N user mode", source->nick.c_str());
		}
		return MODEACTION_DENY;
	}
};



/**
 * Handles /IRCOPS
 */
class CommandIRCOps : public Command
{

public:

	CommandIRCOps (Module* Creator) : Command(Creator,"IRCOPS",0)
	{

	}

	CmdResult Handle (const std::vector<std::string>&, User *user)
	{
		if (user) {
			user->WriteServ ("292 %s :\2Nick                        Status\2", user->nick.c_str());
			user->WriteServ ("292 %s :------------------------------------------", user->nick.c_str());

			int total = 0, total_away = 0;
			for (std::list<User*>::iterator i = ServerInstance->Users->all_opers.begin(); i != ServerInstance->Users->all_opers.end(); i++) {
				User* oper = *i;

				/**
                 * Ignore U-Line Services and Oper Hide
                 */
				if (!ServerInstance->ULine(oper->server) && !oper->IsModeSet('H')) {
					if(!oper->awaymsg.empty()) {
						total_away++;
						user->WriteServ ("292 %s :%s                        %s (Away)", user->nick.c_str(), (oper->nick.c_str()), (oper->oper->NameStr()) );
					} else {
						user->WriteServ ("292 %s :%s                        %s (Available for help)", user->nick.c_str(), (oper->nick.c_str()), (oper->oper->NameStr()) /*, oper->server.c_str() */);

					}

					total++;
				}
			}

			user->WriteServ ("292 %s :There are %d IRCop(s) Online and %d Away(s)", user->nick.c_str(), total, total_away);
			user->WriteServ ("292 %s :End of /IRCOPS", user->nick.c_str());
		}


		return CMD_SUCCESS;
	}

};

class ModuleIRCops : public Module
{
	User_O user_O;
	User_A user_A;
	User_N user_N;
	CommandIRCOps cmdircops;


public:
	ModuleIRCops() : user_O(this), user_A(this), user_N(this), cmdircops(this)
	{
	}

	void init()
	{
		ServiceProvider* providerlist[] = { &user_O, &user_A, &user_N};
		ServerInstance->Modules->AddServices(providerlist, sizeof(providerlist)/sizeof(ServiceProvider*));
		ServerInstance->Modules->AddService(cmdircops);

		Implementation eventlist[] = { I_OnWhois, I_OnUserPreMessage, I_OnUserPreNotice, I_OnUserPreJoin };
		ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));
	}

	void OnWhois(User* source, User* dest)
	{
		if(!dest->IsModeSet('H')) {
			if (dest->IsModeSet('O'))
			{
				ServerInstance->SendWhoisLine(source, dest, 313, "%s %s :is a Services Operator on %s",
											  source->nick.c_str(), dest->nick.c_str(),
											  ServerInstance->Config->Network.c_str());
			} else if (dest->IsModeSet('A')) {
				ServerInstance->SendWhoisLine(source, dest, 313, "%s %s :is a Services Administrator on %s",
											  source->nick.c_str(), dest->nick.c_str(),
											  ServerInstance->Config->Network.c_str());
			} else if (dest->IsModeSet('N')) {
				ServerInstance->SendWhoisLine(source, dest, 313, "%s %s :is a Services Root on %s",
											  source->nick.c_str(), dest->nick.c_str(),
											  ServerInstance->Config->Network.c_str());
			}
		}

	}

	Version GetVersion()
	{
		return Version("Provides support for VIRCIO Network user modes", VF_OPTCOMMON|VF_VENDOR);
	}
};

MODULE_INIT(ModuleIRCops)
