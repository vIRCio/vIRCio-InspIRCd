/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 * Copyright (C) 2009 Chernov-Phoenix Alexey <phoenix@pravmail.ru>
 * Copyright (C) 2012-2013 Attila Molnar <attilamolnar@hush.com>
 * Copyright (C) 2026 vIRCio contributors
 *
 * This file is part of InspIRCd. InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 */

#include "inspircd.h"

namespace
{
	class ActiveGuard final
	{
	private:
		bool& active;

	public:
		explicit ActiveGuard(bool& Active)
			: active(Active)
		{
			active = true;
		}

		~ActiveGuard()
		{
			active = false;
		}
	};
}

class CommandPretendUser final
	: public Command
{
private:
	bool active = false;

	unsigned long GetOperLevel(const User* user) const
	{
		if (!user || !user->IsOper())
			return 0;

		/*
		 * Remote OperAccount objects contain the oper type but do not
		 * necessarily contain arbitrary local config fields such as level.
		 *
		 * Resolve the type against our shared configuration first. This keeps
		 * oper hierarchy meaningful when PRETENDUSER crosses a server link.
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

	static std::string GetPretendedCommand(const std::string& line)
	{
		irc::tokenstream stream(line);
		std::string command;

		if (!stream.GetMiddle(command))
			return "";

		std::transform(
			command.begin(),
			command.end(),
			command.begin(),
			::toupper
		);

		return command;
	}

	bool CheckSource(User* source) const
	{
		/*
		 * Local command parsing already checks CmdAccess and command
		 * permissions. Repeat the important checks here because the command
		 * can also arrive over S2S from a remote operator.
		 *
		 * SpanningTree synchronizes remote oper command and privilege lists.
		 */
		return source->IsOper()
			&& source->HasCommandPermission("PRETENDUSER")
			&& source->HasPrivPermission("users/pretenduser");
	}

public:
	CommandPretendUser(Module* Creator)
		: Command(Creator, "PRETENDUSER", 2, 2)
	{
		access_needed = CmdAccess::OPERATOR;
		syntax = { "<nick> :<command-line>" };

		/*
		 * Translate the target nick to its UUID for S2S transmission.
		 * The command line itself must remain opaque text.
		 */
		translation = {
			TR_NICK,
			TR_TEXT
		};
	}

	CmdResult Handle(User* source, const Params& parameters) override
	{
		if (!CheckSource(source))
		{
			source->WriteNumeric(
				ERR_NOPRIVILEGES,
				"Permission denied - the users/pretenduser privilege is required"
			);

			return CmdResult::FAILURE;
		}

		/*
		 * Prevent direct and indirect PRETENDUSER recursion.
		 */
		if (active)
		{
			source->WriteNotice(
				"*** PRETENDUSER recursion is not permitted."
			);

			return CmdResult::FAILURE;
		}

		auto* target =
			ServerInstance->Users.Find(parameters[0], true);

		if (!target || IS_SERVER(target))
		{
			source->WriteNumeric(
				ERR_NOSUCHNICK,
				parameters[0],
				"No such nick"
			);

			return CmdResult::FAILURE;
		}

		if (source == target)
		{
			source->WriteNotice(
				"*** PRETENDUSER cannot target yourself."
			);

			return CmdResult::FAILURE;
		}

		/*
		 * Services pseudoclients are protocol actors, not normal clients.
		 * PRETENDUSER must never be used to impersonate them.
		 */
		if (target->server->IsService())
		{
			source->WriteNumeric(
				ERR_NOPRIVILEGES,
				INSP_FORMAT(
					"Permission denied - {} is a Services pseudoclient",
					target->nick
				)
			);

			return CmdResult::FAILURE;
		}

		/*
		 * Preserve the historic oper hierarchy rule:
		 * an operator may not impersonate an operator with a higher level.
		 *
		 * Equal levels retain the legacy behaviour. Protected IRC Admins receive
		 * stronger protection separately from m_vircio_admin.
		 */
		if (target->IsOper())
		{
			const auto sourcelevel = GetOperLevel(source);
			const auto targetlevel = GetOperLevel(target);

			if (targetlevel > sourcelevel)
			{
				source->WriteNumeric(
					ERR_NOPRIVILEGES,
					INSP_FORMAT(
						"Permission denied - oper {} has a higher level than you",
						target->nick
					)
				);

				if (IS_LOCAL(source))
				{
					ServerInstance->SNO.WriteGlobalSno(
						'a',
						"{} (level {}) attempted PRETENDUSER against higher level oper {} (level {})",
						source->nick,
						sourcelevel,
						target->nick,
						targetlevel
					);
				}

				return CmdResult::FAILURE;
			}
		}

		const std::string& line = parameters[1];

		if (line.empty())
		{
			source->WriteNotice(
				"*** PRETENDUSER requires a command line."
			);

			return CmdResult::FAILURE;
		}

		if (line.find_first_of("\r\n") != std::string::npos)
		{
			source->WriteNotice(
				"*** PRETENDUSER command line contains invalid characters."
			);

			return CmdResult::FAILURE;
		}

		const std::string pretendcommand =
			GetPretendedCommand(line);

		if (irc::equals(pretendcommand, "PRETENDUSER"))
		{
			source->WriteNotice(
				"*** Nested PRETENDUSER commands are not permitted."
			);

			return CmdResult::FAILURE;
		}

		/*
		 * If the target is remote then there is nothing to execute on this
		 * server. GetRouting() will forward PRETENDUSER to the server which
		 * owns the target's connection.
		 */
		auto* localtarget = IS_LOCAL(target);

		if (!localtarget)
			return CmdResult::SUCCESS;

		/*
		 * Audit the operation without logging the full payload. The payload
		 * could contain passwords or private message contents.
		 */
		ServerInstance->SNO.WriteGlobalSno(
			'a',
			"{} used PRETENDUSER to make {} execute {}",
			source->nick,
			target->nick,
			pretendcommand.empty() ? "<unknown>" : pretendcommand
		);

		/*
		 * Execute through the normal client parser as the target LocalUser.
		 *
		 * This deliberately preserves normal target-side command parsing,
		 * hooks, permissions, aliases, flood accounting, and module policy.
		 */
		ActiveGuard guard(active);
		ServerInstance->Parser.ProcessBuffer(localtarget, line);

		return CmdResult::SUCCESS;
	}

	RouteDescriptor GetRouting(
		User*,
		const Params& parameters
	) override
	{
		/*
		 * TR_NICK translates parameters[0] to UUID for transmission.
		 * ROUTE_OPT_UCAST sends the command only to the target's server.
		 */
		return ROUTE_OPT_UCAST(parameters[0]);
	}
};

class ModuleVircioPretendUser final
	: public Module
{
private:
	CommandPretendUser cmdpretenduser;

public:
	ModuleVircioPretendUser()
		: Module(
			VF_OPTCOMMON,
			"Adds the vIRCio PRETENDUSER command for executing an IRC command on behalf of another user."
		)
		, cmdpretenduser(this)
	{
	}
};

MODULE_INIT(ModuleVircioPretendUser)
