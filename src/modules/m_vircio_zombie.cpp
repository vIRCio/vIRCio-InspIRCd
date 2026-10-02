/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 * Historical functionality based on the vIRCio InspIRCd 2.x
 * Zombie/Gringo user mode.
 *
 * Copyright (C) 2026 vIRCio contributors
 *
 * This file is part of InspIRCd. InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 */

#include "inspircd.h"
#include "modules/account.h"
#include "modules/ctctags.h"

class ModuleVircioZombie;


class ZombieMode final
	: public SimpleUserMode
{
private:
	ModuleVircioZombie* const parent;

public:
	ZombieMode(ModuleVircioZombie* Parent);

	bool OnModeChange(
		User* source,
		User* dest,
		Channel* channel,
		Modes::Change& change
	) override;
};


class ModuleVircioZombie final
	: public Module
	, public Account::EventListener
	, public CTCTags::EventListener
{
private:
	friend class ZombieMode;

	Account::API accountapi;
	ZombieMode zombiemode;

	std::string quarantinechannel = "#vIRCio";

	std::string restrictionnotice =
		"*** Atencao: identifique-se ou registre uma conta nos Services "
		"para acessar os demais canais e usuarios. "
		"Use /NICKSERV HELP para obter ajuda.";

	bool IsZombie(const User* user) const
	{
		return user
			&& user->IsModeSet(zombiemode);
	}

	bool IsAuthenticated(const User* user) const
	{
		return accountapi
			&& accountapi->GetAccountName(user);
	}

	bool IsQuarantineChannel(const std::string& name) const
	{
		return irc::equals(
			name,
			quarantinechannel
		);
	}

	bool IsAllowedTarget(
		const MessageTarget& target
	) const
	{
		switch (target.type)
		{
			case MessageTarget::TYPE_CHANNEL:
			{
				const auto* channel =
					target.Get<Channel>();

				return IsQuarantineChannel(
					channel->name
				);
			}

			case MessageTarget::TYPE_USER:
			{
				const auto* user =
					target.Get<User>();

				/*
				 * Do not hardcode NickServ.
				 *
				 * A quarantined user may communicate with any real
				 * Services pseudoclient.
				 */
				return user->server->IsService();
			}

			case MessageTarget::TYPE_SERVER:
				break;
		}

		return false;
	}

	void NotifyRestriction(User* user) const
	{
		auto* localuser = IS_LOCAL(user);

		if (!localuser)
			return;

		if (!restrictionnotice.empty())
			localuser->WriteNotice(restrictionnotice);
	}

	bool CanEnableZombie(User* target) const
	{
		if (!target)
			return false;

		/*
		 * Services pseudoclients are protocol actors and must never enter
		 * the Zombie state.
		 */
		if (target->server->IsService())
			return false;

		/*
		 * If the account module/API is unavailable then fail closed.
		 * Automatic removal after authentication is a required property
		 * of the Zombie state.
		 */
		if (!accountapi)
			return false;

		/*
		 * An already authenticated user must not be quarantined.
		 */
		if (IsAuthenticated(target))
			return false;

		return true;
	}

	void OnZombieEnabled(
		User* source,
		User* target
	)
	{
		auto* localtarget = IS_LOCAL(target);

		if (!localtarget)
			return;

		NotifyRestriction(localtarget);

		/*
		 * Force the local user into the quarantine channel.
		 *
		 * override=true intentionally bypasses bans, keys, invite-only,
		 * limits, etc. The quarantine channel must always be reachable.
		 *
		 * If the user is already on the channel JoinUser() simply does
		 * nothing.
		 */
		Channel::JoinUser(
			localtarget,
			quarantinechannel,
			true
		);

		ServerInstance->SNO.WriteGlobalSno(
			'a',
			"{} placed {} into vIRCio Zombie quarantine (+Z)",
			source->nick,
			target->nick
		);
	}

	ModResult HandleMessage(
		User* source,
		const MessageTarget& target
	)
	{
		/*
		 * Policy is enforced at the user's home server. The accepted or
		 * rejected result then propagates normally through the network.
		 */
		if (!IS_LOCAL(source))
			return MOD_RES_PASSTHRU;

		if (!IsZombie(source))
			return MOD_RES_PASSTHRU;

		if (IsAllowedTarget(target))
			return MOD_RES_PASSTHRU;

		NotifyRestriction(source);

		return MOD_RES_DENY;
	}

public:
	ModuleVircioZombie()
		: Module(
			VF_COMMON,
			"Adds the vIRCio Zombie/Gringo quarantine using user mode +Z."
		)
		, Account::EventListener(this)
		, CTCTags::EventListener(this)
		, accountapi(this)
		, zombiemode(this)
	{
	}

	void ReadConfig(ConfigStatus&) override
	{
		if (!accountapi)
		{
			throw ModuleException(
				this,
				"m_vircio_zombie requires the account module"
			);
		}

		const auto& tag =
			ServerInstance->Config->ConfValue(
				"virciozombie"
			);

		const std::string newchannel =
			tag->getString(
				"channel",
				"#vIRCio"
			);

		if (!ServerInstance->Channels.IsChannel(newchannel))
		{
			throw ModuleException(
				this,
				"<virciozombie:channel> is not a valid channel name"
			);
		}

		quarantinechannel = newchannel;

		restrictionnotice =
			tag->getString(
				"notice",
				"*** Atencao: identifique-se ou registre uma conta nos Services "
				"para acessar os demais canais e usuarios. "
				"Use /NICKSERV HELP para obter ajuda."
			);
	}

	ModResult OnUserPreJoin(
		LocalUser* user,
		Channel* channel,
		const std::string& cname,
		std::string& privs,
		const std::string& keygiven,
		bool override
	) override
	{
		if (!IsZombie(user))
			return MOD_RES_PASSTHRU;

		if (IsQuarantineChannel(cname))
			return MOD_RES_PASSTHRU;

		NotifyRestriction(user);

		return MOD_RES_DENY;
	}

	ModResult OnUserPreMessage(
		User* user,
		MessageTarget& target,
		MessageDetails& details
	) override
	{
		return HandleMessage(
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
		return HandleMessage(
			user,
			target
		);
	}

	void OnAccountChange(
		User* user,
		const std::string& account
	) override
	{
		/*
		 * Ignore logout events.
		 */
		if (account.empty())
			return;

		/*
		 * Only the user's home server initiates the mode removal.
		 * The resulting MODE then propagates normally to the network.
		 */
		auto* localuser = IS_LOCAL(user);

		if (!localuser)
			return;

		if (!IsZombie(localuser))
			return;

		Modes::ChangeList changelist;
		changelist.push_remove(
			&zombiemode
		);

		ServerInstance->Modes.Process(
			ServerInstance->FakeClient,
			nullptr,
			localuser,
			changelist
		);

		localuser->WriteNotice(
			"*** Identificacao confirmada. A restricao Zombie (+Z) foi removida."
		);

		ServerInstance->SNO.WriteGlobalSno(
			'a',
			"Zombie quarantine (+Z) was removed from {} after account login as {}",
			localuser->nick,
			account
		);
	}
};


ZombieMode::ZombieMode(
	ModuleVircioZombie* Parent
)
	: SimpleUserMode(
		Parent,
		"u_services_zombie",
		'Z'
	)
	, parent(Parent)
{
}


bool ZombieMode::OnModeChange(
	User* source,
	User* dest,
	Channel* channel,
	Modes::Change& change
)
{
	/*
	 * Adding +Z is exclusively controlled by Services.
	 *
	 * This replaces the historical and unsafe test:
	 *
	 *     !IS_LOCAL(source)
	 *
	 * which trusted every remote source.
	 */
	if (change.adding)
	{
		if (!source->server->IsService())
		{
			if (IS_LOCAL(source))
			{
				source->WriteNumeric(
					ERR_NOPRIVILEGES,
					"Only Services may add user mode +Z"
				);
			}

			return false;
		}

		if (!parent->CanEnableZombie(dest))
			return false;
	}
	else
	{
		/*
		 * Normal removal is also a Services action.
		 *
		 * Server sources are additionally allowed to remove the mode so the
		 * account event can safely clear +Z and the core can clean up the
		 * mode during module unload.
		 */
		if (!source->server->IsService()
			&& !IS_SERVER(source))
		{
			if (IS_LOCAL(source))
			{
				source->WriteNumeric(
					ERR_NOPRIVILEGES,
					"Only Services may remove user mode +Z"
				);
			}

			return false;
		}
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

	if (change.adding)
	{
		parent->OnZombieEnabled(
			source,
			dest
		);
	}

	return true;
}


MODULE_INIT(ModuleVircioZombie)
