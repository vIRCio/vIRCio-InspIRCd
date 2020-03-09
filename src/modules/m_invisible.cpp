/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2010 InspIRCd Development Team
 * See: http://wiki.inspircd.org/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include <stdarg.h>

/* $ModDesc: Allows for opered clients to join channels without being seen, similar to unreal 3.1 +I mode */
/* $ModDepends: core 2.0 */

class InvisibleMode : public ModeHandler
{
 public:
	InvisibleMode(Module* Creator) : ModeHandler(Creator, "invis-oper", 'Q', PARAM_NONE, MODETYPE_USER)
	{
		oper = true;
	}

	~InvisibleMode()
	{
	}

	ModeAction OnModeChange(User* source, User* dest, Channel* channel, std::string &parameter, bool adding)
	{
		if (dest->IsModeSet('Q') != adding)
		{
			dest->SetMode('Q', adding);

			/* Fix for bug #379 reported by stealth. On +/-Q make m_watch think the user has signed on/off */
			Module* m = ServerInstance->Modules->Find("m_watch.so");

			/* This must come before setting/unsetting the handler */
			if (m && adding)
				m->OnUserQuit(dest, "Connection closed", "Connection closed");

			/* User appears to vanish or appear from nowhere */
			for (UCListIter f = dest->chans.begin(); f != dest->chans.end(); f++)
			{
				const UserMembList *ulist = (*f)->GetUsers();
				char tb[MAXBUF];

				snprintf(tb,MAXBUF,":%s %s %s", dest->GetFullHost().c_str(), adding ? "PART" : "JOIN", (*f)->name.c_str());
				std::string out = tb;
				Membership* memb = (**f).GetUser(dest);
				std::string ms = memb->modes;
				for(unsigned int i=0; i < memb->modes.length(); i++)
					ms.append(" ").append(dest->nick);


				for (UserMembCIter i = ulist->begin(); i != ulist->end(); i++)
				{
					/* User only appears to vanish for non-opers */
					if (IS_LOCAL(i->first) && !IS_OPER(i->first))
					{
						i->first->Write(out);
						if (!ms.empty() && !adding)
							i->first->WriteServ("MODE %s +%s", (**f).name.c_str(), ms.c_str());
					}
				}
			}

			ServerInstance->SNO->WriteToSnoMask('a', "\2NOTICE\2: Oper %s has become %svisible (%cQ)", dest->GetFullHost().c_str(), adding ? "in" : "", adding ? '+' : '-');
			return MODEACTION_ALLOW;
		}
		else
		{
			return MODEACTION_DENY;
		}
	}
};

class ModuleInvisible : public Module
{
 private:
	InvisibleMode qm;
	bool hidejoin;
	bool hidelist;
	bool hidewho;
	bool hidemsg;
 public:
	ModuleInvisible() : qm(this)
	{
	}

	void init()
	{
		ServerInstance->Modules->AddService(qm);

		/* Yeah i know people can take this out. I'm not about to obfuscate code just to be a pain in the ass. */
		ServerInstance->Users->ServerNoticeAll("*** m_invisible.so has just been loaded on this network. For more information, please visit http://inspircd.org/wiki/Modules/invisible");
		Implementation eventlist[] = {
			I_OnUserPreMessage, I_OnUserPreNotice, I_OnUserJoin,
			I_OnBuildNeighborList, I_OnSendWhoLine, I_OnNamesListItem,
			I_OnRehash
		};
		ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));
		OnRehash(NULL);
	}

	void OnRehash(User*)
	{
		ConfigTag* tag = ServerInstance->Config->ConfValue("invisible");
		hidejoin = tag->getBool("join");
		hidelist = tag->getBool("list");
		hidewho = tag->getBool("who");
		hidemsg = tag->getBool("msg");
	}
	Version GetVersion();
	void OnUserJoin(Membership* memb, bool sync, bool created, CUList& excepts);
	void OnBuildNeighborList(User* source, UserChanList &include, std::map<User*,bool> &exceptions);
	ModResult OnUserPreNotice(User* user,void* dest,int target_type, std::string &text, char status, CUList &exempt_list);
	ModResult OnUserPreMessage(User* user,void* dest,int target_type, std::string &text, char status, CUList &exempt_list);
	void OnSendWhoLine(User* source, const std::vector<std::string>&, User* user, std::string& line);
	void OnNamesListItem(User* issuer, Membership* memb, std::string &prefixes, std::string &nick);
};

Version ModuleInvisible::GetVersion()
{
	return Version("Allows opers to join channels invisibly", VF_COMMON);
}

static void BuildExcept(Membership* memb, CUList& excepts)
{
	const UserMembList* users = memb->chan->GetUsers();
	for(UserMembCIter i = users->begin(); i != users->end(); i++)
	{
		if (IS_LOCAL(i->first) && i->first->HasPrivPermission("invisible/see"))
			excepts.insert(i->first);
	}
}

void ModuleInvisible::OnUserJoin(Membership* memb, bool sync, bool created, CUList& excepts)
{
	if (hidejoin && memb->user->IsModeSet('Q'))
	{
		BuildExcept(memb, excepts);
		ServerInstance->SNO->WriteToSnoMask('a', "\2NOTICE\2: Oper %s has joined %s invisibly (+Q)",
			memb->user->GetFullHost().c_str(), memb->chan->name.c_str());
	}
}

void ModuleInvisible::OnBuildNeighborList(User* source, UserChanList &include, std::map<User*,bool> &exceptions)
{
	if (hidewho && source->IsModeSet('Q'))
	{
		include.clear();
	}
}

/* No privmsg response when hiding - submitted by Eric at neowin */
ModResult ModuleInvisible::OnUserPreNotice(User* user,void* dest,int target_type, std::string &text, char status, CUList &exempt_list)
{
	if ((target_type == TYPE_USER) && (IS_LOCAL(user)))
	{
		User* target = (User*)dest;
		if(hidemsg && target->IsModeSet('Q') && !IS_OPER(user))
		{
			user->WriteNumeric(401, "%s %s :No such nick/channel",user->nick.c_str(), target->nick.c_str());
			return MOD_RES_DENY;
		}
	}
	return MOD_RES_PASSTHRU;
}

ModResult ModuleInvisible::OnUserPreMessage(User* user,void* dest,int target_type, std::string &text, char status, CUList &exempt_list)
{
	return OnUserPreNotice(user, dest, target_type, text, status, exempt_list);
}

void ModuleInvisible::OnSendWhoLine(User* source, const std::vector<std::string>& params, User* user, std::string& line)
{
	if (hidewho && user->IsModeSet('Q') && !IS_OPER(source))
		line.clear();
}

void ModuleInvisible::OnNamesListItem(User* issuer, Membership* memb, std::string &prefixes, std::string &nick)
{
	if (hidelist && memb->user->IsModeSet('Q') && !IS_OPER(issuer))
		nick.clear();
}

MODULE_INIT(ModuleInvisible)
