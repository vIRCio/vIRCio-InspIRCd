/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 * Copyright (C) 2026 vIRCio contributors
 *
 * This file is part of InspIRCd. InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 */

#include "inspircd.h"

class ModuleVircioAdmin final
	: public Module
{
private:
	unsigned long minlevel = 100;
	std::string privilege = "users/vircio-admin";

	unsigned long GetOperLevel(const User* user) const
	{
		if (!user || !user->IsOper())
			return 0;

		/*
		 * Remote OperAccount objects contain the oper type but do not
		 * necessarily contain arbitrary local config fields such as level.
		 * Resolve the type against our shared configuration first so IRC Admin
		 * protection remains effective across server links.
		 */
		const auto iter =
			ServerInstance->Config->OperTypes.find(user->oper->GetType());

		if (iter != ServerInstance->Config->OperTypes.end())
		{
			return iter->second->GetConfig()
				->getNum<unsigned long>("level", 0);
		}

		/*
		 * Fallback for unusual/local oper types which are not represented in
		 * the type map available on this server.
		 */
		return user->oper->GetConfig()
			->getNum<unsigned long>("level", 0);
	}

	bool IsProtectedAdmin(const User* user) const
	{
		if (!user || !user->IsOper())
			return false;

		const auto level = GetOperLevel(user);

		if (level < minlevel)
			return false;

		return user->HasPrivPermission(privilege);
	}

	void ReportDenied(User* source, User* target, const std::string& action)
	{
		if (IS_LOCAL(source))
		{
			source->WriteNumeric(
				ERR_NOPRIVILEGES,
				INSP_FORMAT(
					"Permission denied - {} is a protected vIRCio IRC Admin",
					target->nick
				)
			);
		}

		ServerInstance->SNO.WriteGlobalSno(
			'a',
			"{} attempted {} against protected IRC Admin {}",
			source->nick,
			action,
			target->nick
		);
	}

	User* FindForcedCommandTarget(
		const std::string& command,
		const CommandBase::Params& parameters
	) const
	{
		if (irc::equals(command, "SAJOIN"))
		{
			/*
			 * SAJOIN #channel acts on the source itself.
			 * SAJOIN nick #channel acts on another user.
			 */
			if (parameters.size() < 2)
				return nullptr;

			return ServerInstance->Users.FindNick(parameters[0], true);
		}

		if (irc::equals(command, "SAKICK"))
		{
			if (parameters.size() < 2)
				return nullptr;

			return ServerInstance->Users.FindNick(parameters[1], true);
		}

		if (irc::equals(command, "SANICK")
			|| irc::equals(command, "SAPART")
			|| irc::equals(command, "SAQUIT"))
		{
			if (parameters.empty())
				return nullptr;

			return ServerInstance->Users.FindNick(parameters[0], true);
		}

		if (irc::equals(command, "SAMODE"))
		{
			if (parameters.empty() || parameters[0].empty())
				return nullptr;

			/*
			 * A channel target is not a user and is outside the scope of
			 * IRC Admin protection.
			 */
			if (ServerInstance->Channels.IsPrefix(parameters[0][0]))
				return nullptr;

			return ServerInstance->Users.FindNick(parameters[0], true);
		}

		if (irc::equals(command, "PRETENDUSER"))
		{
			if (parameters.empty())
				return nullptr;

			return ServerInstance->Users.Find(parameters[0], true);
		}

		if (irc::equals(command, "REMOVE"))
		{
			if (parameters.size() < 2)
				return nullptr;

			return ServerInstance->Users.FindNick(parameters[1], true);
		}

		return nullptr;
	}

public:
	ModuleVircioAdmin()
		: Module(
			VF_COMMON,
			"Protects privileged vIRCio IRC Admins from forced administrative actions."
		)
	{
	}

	void ReadConfig(ConfigStatus&) override
	{
		const auto& tag =
			ServerInstance->Config->ConfValue("vircioadmin");

		minlevel =
			tag->getNum<unsigned long>("minlevel", 100, 1);

		privilege =
			tag->getString("privilege", "users/vircio-admin");

		if (privilege.empty())
		{
			throw ModuleException(
				this,
				"<vircioadmin:privilege> must not be empty"
			);
		}
	}

	ModResult OnKill(
		User* source,
		User* dest,
		const std::string&
	) override
	{
		if (!source || source == dest || !IsProtectedAdmin(dest))
			return MOD_RES_PASSTHRU;

		ReportDenied(source, dest, "KILL");
		return MOD_RES_DENY;
	}

	ModResult OnUserPreKick(
		User* source,
		Membership* memb,
		const std::string&
	) override
	{
		User* target = memb->user;

		if (source == target || !IsProtectedAdmin(target))
			return MOD_RES_PASSTHRU;

		ReportDenied(source, target, "KICK");
		return MOD_RES_DENY;
	}

	ModResult OnRawMode(
		User* source,
		Channel* chan,
		const Modes::Change& change
	) override
	{
		if (!IS_LOCAL(source) || !chan)
			return MOD_RES_PASSTHRU;

		if (change.adding || change.param.empty())
			return MOD_RES_PASSTHRU;

		const auto* prefixmode = change.mh->IsPrefixMode();
		if (!prefixmode)
			return MOD_RES_PASSTHRU;

		auto* target =
			ServerInstance->Users.Find(change.param, true);

		if (!target || source == target || !IsProtectedAdmin(target))
			return MOD_RES_PASSTHRU;

		auto* memb = chan->GetUser(target);
		if (!memb || !memb->HasMode(prefixmode))
			return MOD_RES_PASSTHRU;

		ReportDenied(
			source,
			target,
			INSP_FORMAT(
				"removal of channel prefix +{} on {}",
				change.mh->GetModeChar(),
				chan->name
			)
		);

		return MOD_RES_DENY;
	}

	ModResult OnPreCommand(
		std::string& command,
		CommandBase::Params& parameters,
		LocalUser* source,
		bool validated
	) override
	{
		if (!validated)
			return MOD_RES_PASSTHRU;

		auto* target =
			FindForcedCommandTarget(command, parameters);

		if (!target || source == target || !IsProtectedAdmin(target))
			return MOD_RES_PASSTHRU;

		ReportDenied(source, target, command);
		return MOD_RES_DENY;
	}
};

MODULE_INIT(ModuleVircioAdmin)
