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
 * 2018-12-29
 * 		Add. Zombie/gringo
 *
 */

#include "inspircd.h"
#include <stdarg.h>

#define ZOMBIE_MESSAGE(source) source->WriteServ("NOTICE %s :*** \002Atenção:\002 É necessário que você registre ou identifique-se junto ao NickServ para poder entrar nos canais ou falar com alguém. Para registrar seu nick, digite: \002/NICKSERV REGISTER senha email\002", source->nick.c_str());
#define NICKSERV_NAME "nickserv"
#define ZOMBIE_CHAN "#vircio"
/**
 * User mode +Z - mark a user as Zombie / Gringo
 */
class User_Z : public ModeHandler
{

public:
	User_Z(Module* Creator) : ModeHandler(Creator, "u_services_zombie", 'Z', PARAM_NONE, MODETYPE_USER) { }

	ModeAction OnModeChange(User* source, User* dest, Channel* channel, std::string &parameter, bool adding)
	{
		if (!IS_LOCAL(source))
		{
			if (dest->IsModeSet('r'))
				return MODEACTION_DENY;

			if ((adding != dest->IsModeSet('Z')))
			{
				ZOMBIE_MESSAGE(dest);
				dest->SetMode('Z',adding);

                if (IS_LOCAL(dest)) {
                    Channel::JoinUser(dest, ZOMBIE_CHAN, false, "", false, ServerInstance->Time());
                }

				return MODEACTION_ALLOW;
			}
		}
		else
		{
			source->WriteNumeric(500, "%s :Only a server may modify the +Z user mode", source->nick.c_str());
		}
		return MODEACTION_DENY;
	}
};

class ModuleVircio : public Module
{
	User_Z user_Z;

public:
	ModuleVircio() : user_Z(this)
	{
	}

	void init()
	{
		ServiceProvider* providerlist[] = { &user_Z};
		ServerInstance->Modules->AddServices(providerlist, sizeof(providerlist)/sizeof(ServiceProvider*));

		Implementation eventlist[] = { I_OnWhois, I_OnUserPreMessage, I_OnUserPreNotice, I_OnUserPreJoin };
		ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));
	}

	ModResult OnUserPreMessage(User *user, void *dest, int target_type, std::string &text, char status, CUList &exempt_list)
	{
		if(user->IsModeSet('Z')) {
            std::string nickChan = "";

            if (target_type == TYPE_CHANNEL) {
                Channel *d = (Channel *) dest;
                nickChan = d->name.c_str();
            } else {
                User *d = (User *) dest;
                nickChan = d->nick.c_str();
            }

            std::transform( nickChan.begin(), nickChan.end(), nickChan.begin(), ::tolower );

			if(nickChan == NICKSERV_NAME || nickChan == ZOMBIE_CHAN)
				return MOD_RES_PASSTHRU;

			ZOMBIE_MESSAGE(user);
			return MOD_RES_DENY;
		}

		return MOD_RES_PASSTHRU;
	}

	ModResult OnUserPreNotice(User* user,void* dest,int target_type, std::string &text, char status, CUList &exempt_list)
	{
		if(user->IsModeSet('Z')) {
			ZOMBIE_MESSAGE(user);
			return MOD_RES_DENY;
		}

		return MOD_RES_PASSTHRU;
	}

    ModResult OnUserPreJoin(User* user, Channel* chan, const char* cname, std::string &privs, const std::string &keygiven) {
        if(user->IsModeSet('Z')) {
            std::string channame = cname;
            std::transform( channame.begin(), channame.end(), channame.begin(), ::tolower );

            if(channame == ZOMBIE_CHAN)
                return MOD_RES_PASSTHRU;

			ZOMBIE_MESSAGE(user);
            return MOD_RES_DENY;
        }

        return MOD_RES_PASSTHRU;
	}


	Version GetVersion()
	{
		return Version("Provides support for VIRCIO Network user modes", VF_OPTCOMMON|VF_VENDOR);
	}
};

MODULE_INIT(ModuleVircio)
