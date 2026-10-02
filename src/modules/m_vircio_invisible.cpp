/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 * Historical functionality based on the InspIRCd 2.x m_invisible module.
 *
 * Copyright (C) 2002-2010 InspIRCd Development Team
 * Copyright (C) 2026 vIRCio contributors
 *
 * This file is part of InspIRCd. InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 */

#include "inspircd.h"
#include "clientprotocolevent.h"
#include "modules/ctctags.h"
#include "modules/names.h"
#include "modules/who.h"
#include "modules/whois.h"

class ModuleVircioInvisible;

namespace
{

class InvisibleMode final
	: public SimpleUserMode
{
private:
	ModuleVircioInvisible* const parent;

public:
	InvisibleMode(ModuleVircioInvisible* Parent);

	bool OnModeChange(
		User* source,
		User* dest,
		Channel* channel,
		Modes::Change& change
	) override;
};


class JoinHook final
	: public ClientProtocol::EventHook
{
private:
	ModuleVircioInvisible* const parent;
	bool active = false;

public:
	JoinHook(ModuleVircioInvisible* Parent);

	void OnEventInit(
		const ClientProtocol::Event& ev
	) override;

	ModResult OnPreEventSend(
		LocalUser* user,
		const ClientProtocol::Event& ev,
		ClientProtocol::MessageList& messagelist
	) override;
};

}


class ModuleVircioInvisible final
	: public Module
	, public CTCTags::EventListener
	, public Names::EventListener
	, public Who::EventListener
	, public Who::VisibleEventListener
	, public Whois::LineEventListener
{
private:
	InvisibleMode invisiblemode;
	JoinHook joinhook;

	bool hidejoin = true;
	bool hidenames = true;
	bool hidewho = true;
	bool hidewhois = true;
	bool hidemsg = true;

	std::string seepriv = "users/auspex";

public:
	/*
	 * Visibility predicates are intentionally exposed to the protocol hook.
	 *
	 * They are read-only queries; all state mutation remains private to the
	 * module and its mode handler.
	 */
	bool IsInvisible(const User* user) const
	{
		return user && user->IsModeSet(invisiblemode);
	}

	bool CanSeeInvisible(User* viewer, User* target) const
	{
		if (!viewer || !target)
			return false;

		/*
		 * A user must always be able to see their own membership.
		 */
		if (viewer == target)
			return true;

		/*
		 * Services are trusted protocol actors and must not have their view
		 * of the network artificially restricted.
		 */
		if (viewer->server->IsService())
			return true;

		return viewer->HasPrivPermission(seepriv);
	}

	bool ShouldHideJoin(Membership* memb) const
	{
		return hidejoin
			&& memb
			&& IsInvisible(memb->user);
	}

private:
	void BuildExcept(
		Membership* memb,
		CUList& excepts
	)
	{
		if (!ShouldHideJoin(memb))
			return;

		for (const auto& [viewer, _] : memb->chan->GetUsers())
		{
			if (!IS_LOCAL(viewer))
				continue;

			if (!CanSeeInvisible(viewer, memb->user))
				excepts.insert(viewer);
		}
	}

	void SendVisibilityTransition(
		User* target,
		bool becominginvisible
	)
	{
		if (!hidejoin)
			return;

		/*
		 * +Q is a user state but clients model channel membership locally.
		 *
		 * When +Q is enabled, normal local clients which could previously
		 * see the target receive a synthetic PART.
		 *
		 * When +Q is disabled, those clients receive a synthetic JOIN.
		 *
		 * The real Membership is never destroyed or recreated.
		 */
		for (auto* memb : target->chans)
		{
			if (becominginvisible)
			{
				const std::string reason;
				ClientProtocol::Messages::Part partmsg(memb, reason);
				ClientProtocol::Event partevent(
					ServerInstance->GetRFCEvents().part,
					partmsg
				);

				for (const auto& [viewer, _] : memb->chan->GetUsers())
				{
					auto* localviewer = IS_LOCAL(viewer);

					if (!localviewer || viewer == target)
						continue;

					if (CanSeeInvisible(viewer, target))
						continue;

					localviewer->Send(partevent);
				}
			}
			else
			{
				/*
				 * Use the real v4 JOIN event rather than constructing a raw
				 * IRC line. Core JOIN hooks automatically restore prefix
				 * modes and cooperate with IRCv3 extensions.
				 */
				ClientProtocol::Events::Join joinevent(memb);

				for (const auto& [viewer, _] : memb->chan->GetUsers())
				{
					auto* localviewer = IS_LOCAL(viewer);

					if (!localviewer || viewer == target)
						continue;

					if (CanSeeInvisible(viewer, target))
						continue;

					localviewer->Send(joinevent);
				}
			}
		}
	}

	ModResult HandlePrivateMessage(
		User* source,
		const MessageTarget& target
	)
	{
		if (!hidemsg)
			return MOD_RES_PASSTHRU;

		/*
		 * Enforce this at the command origin only. Remote servers receive
		 * the result of an already accepted message and should not repeat
		 * local policy.
		 */
		if (!IS_LOCAL(source))
			return MOD_RES_PASSTHRU;

		if (target.type != MessageTarget::TYPE_USER)
			return MOD_RES_PASSTHRU;

		auto* targetuser = target.Get<User>();

		if (!IsInvisible(targetuser))
			return MOD_RES_PASSTHRU;

		if (CanSeeInvisible(source, targetuser))
			return MOD_RES_PASSTHRU;

		source->WriteNumeric(
			ERR_NOSUCHNICK,
			targetuser->nick,
			"No such nick/channel"
		);

		return MOD_RES_DENY;
	}

public:
	ModuleVircioInvisible()
		: Module(
			VF_COMMON,
			"Adds vIRCio operator invisibility using user mode +Q."
		)
		, CTCTags::EventListener(this)
		, Names::EventListener(this)
		, Who::EventListener(this)
		, Who::VisibleEventListener(this)
		, Whois::LineEventListener(this)
		, invisiblemode(this)
		, joinhook(this)
	{
	}

	void ReadConfig(ConfigStatus&) override
	{
		const auto& tag =
			ServerInstance->Config->ConfValue("vircioinvisible");

		hidejoin =
			tag->getBool("join", true);

		/*
		 * "list" is accepted as a compatibility alias for the historical
		 * m_invisible option. The modern name is "names".
		 */
		hidenames =
			tag->getBool(
				"names",
				tag->getBool("list", true)
			);

		hidewho =
			tag->getBool("who", true);

		hidewhois =
			tag->getBool("whois", hidewho);

		hidemsg =
			tag->getBool("msg", true);

		seepriv =
			tag->getString(
				"seepriv",
				"users/auspex"
			);

		if (seepriv.empty())
		{
			throw ModuleException(
				this,
				"<vircioinvisible:seepriv> must not be empty"
			);
		}
	}

	void OnVisibilityChange(
		User* source,
		User* target,
		bool invisible
	)
	{
		SendVisibilityTransition(
			target,
			invisible
		);

		/*
		 * Log only at the server where a local actor initiated the change.
		 * Remote copies of the mode should not duplicate the audit notice.
		 */
		if (IS_LOCAL(source))
		{
			ServerInstance->SNO.WriteGlobalSno(
				'a',
				"{} made {} {} using vIRCio user mode {}Q",
				source->nick,
				target->nick,
				invisible ? "invisible" : "visible",
				invisible ? '+' : '-'
			);
		}
	}

	ModResult OnNamesListItem(
		LocalUser* issuer,
		Membership* memb,
		std::string& prefixes,
		std::string& nick
	) override
	{
		if (!hidenames)
			return MOD_RES_PASSTHRU;

		if (!IsInvisible(memb->user))
			return MOD_RES_PASSTHRU;

		if (CanSeeInvisible(issuer, memb->user))
			return MOD_RES_PASSTHRU;

		return MOD_RES_DENY;
	}

	ModResult OnWhoLine(
		const Who::Request& request,
		LocalUser* source,
		User* user,
		Membership* memb,
		Numeric::Numeric& numeric
	) override
	{
		if (!hidewho)
			return MOD_RES_PASSTHRU;

		if (!IsInvisible(user))
			return MOD_RES_PASSTHRU;

		if (CanSeeInvisible(source, user))
			return MOD_RES_PASSTHRU;

		return MOD_RES_DENY;
	}

	ModResult OnWhoVisible(
		const Who::Request& request,
		LocalUser* source,
		Membership* memb
	) override
	{
		if (!hidewho)
			return MOD_RES_PASSTHRU;

		if (!IsInvisible(memb->user))
			return MOD_RES_PASSTHRU;

		if (CanSeeInvisible(source, memb->user))
			return MOD_RES_PASSTHRU;

		return MOD_RES_DENY;
	}

	ModResult OnWhoisLine(
		Whois::Context& whois,
		Numeric::Numeric& numeric
	) override
	{
		if (!hidewhois)
			return MOD_RES_PASSTHRU;

		if (numeric.GetNumeric() != RPL_WHOISCHANNELS
			&& numeric.GetNumeric() != RPL_CHANNELSMSG)
		{
			return MOD_RES_PASSTHRU;
		}

		User* target = whois.GetTarget();
		User* source = whois.GetSource();

		if (!IsInvisible(target))
			return MOD_RES_PASSTHRU;

		if (CanSeeInvisible(source, target))
			return MOD_RES_PASSTHRU;

		return MOD_RES_DENY;
	}

	void OnUserPart(
		Membership* memb,
		std::string& partmessage,
		CUList& excepts
	) override
	{
		BuildExcept(
			memb,
			excepts
		);
	}

	void OnUserKick(
		User* source,
		Membership* memb,
		const std::string& reason,
		CUList& excepts
	) override
	{
		BuildExcept(
			memb,
			excepts
		);
	}

	void OnBuildNeighborList(
		User* source,
		User::NeighborList& include,
		User::NeighborExceptions& exceptions
	) override
	{
		/*
		 * NICK, QUIT, host cycle and similar neighbor events must not reveal
		 * an invisible membership to clients which never saw the JOIN.
		 */
		if (!hidejoin || !IsInvisible(source))
			return;

		for (
			auto iter = include.begin();
			iter != include.end();
		)
		{
			Membership* memb = *iter;

			iter = include.erase(iter);

			for (const auto& [viewer, _] : memb->chan->GetUsers())
			{
				if (!IS_LOCAL(viewer))
					continue;

				if (CanSeeInvisible(viewer, source))
					exceptions[viewer] = true;
			}
		}
	}

	ModResult OnUserPreMessage(
		User* user,
		MessageTarget& target,
		MessageDetails& details
	) override
	{
		return HandlePrivateMessage(
			user,
			target
		);
	}

	ModResult OnUserPreTagMessage(
		User* user,
		MessageTarget& target,
		CTCTags::TagMessageDetails& details
	) override
	{
		return HandlePrivateMessage(
			user,
			target
		);
	}
};


InvisibleMode::InvisibleMode(
	ModuleVircioInvisible* Parent
)
	: SimpleUserMode(
		Parent,
		"vircio-invisible",
		'Q',
		true
	)
	, parent(Parent)
{
}


bool InvisibleMode::OnModeChange(
	User* source,
	User* dest,
	Channel* channel,
	Modes::Change& change
)
{
	/*
	 * +Q represents vIRCio operator invisibility.
	 *
	 * The oper-only flag on SimpleUserMode restricts who may SET the mode,
	 * but SAMODE could otherwise apply it to an ordinary user. Do not allow
	 * that: only real human IRC operators may receive +Q.
	 */
	if (change.adding
		&& (!dest->IsOper() || dest->server->IsService()))
	{
		if (IS_LOCAL(source))
		{
			source->WriteNumeric(
				ERR_NOPRIVILEGES,
				"Permission denied - user mode +Q can only be set on a human IRC operator"
			);
		}

		return false;
	}

	if (!SimpleUserMode::OnModeChange(
		source,
		dest,
		channel,
		change
	))
	{
		return false;
	}

	parent->OnVisibilityChange(
		source,
		dest,
		change.adding
	);

	return true;
}


JoinHook::JoinHook(
	ModuleVircioInvisible* Parent
)
	: ClientProtocol::EventHook(
		Parent,
		"JOIN",
		10
	)
	, parent(Parent)
{
}


void JoinHook::OnEventInit(
	const ClientProtocol::Event& ev
)
{
	const auto& join =
		static_cast<
			const ClientProtocol::Events::Join&
		>(ev);

	active =
		parent->ShouldHideJoin(
			join.GetMember()
		);
}


ModResult JoinHook::OnPreEventSend(
	LocalUser* user,
	const ClientProtocol::Event& ev,
	ClientProtocol::MessageList& messagelist
)
{
	if (!active)
		return MOD_RES_PASSTHRU;

	const auto& join =
		static_cast<
			const ClientProtocol::Events::Join&
		>(ev);

	Membership* memb =
		join.GetMember();

	if (parent->CanSeeInvisible(
		user,
		memb->user
	))
	{
		return MOD_RES_PASSTHRU;
	}

	return MOD_RES_DENY;
}


MODULE_INIT(ModuleVircioInvisible)
