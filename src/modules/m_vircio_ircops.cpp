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

namespace
{
	constexpr unsigned int RPL_IRCOPS = 292;
	constexpr size_t NICK_WIDTH = 24;
	constexpr size_t ROLE_WIDTH = 28;

	std::string FitColumn(std::string value, size_t width)
	{
		if (value.length() > width)
		{
			if (width > 3)
			{
				value.resize(width - 3);
				value.append("...");
			}
			else
				value.resize(width);
		}

		if (value.length() < width)
			value.append(width - value.length(), ' ');

		return value;
	}

	struct StaffEntry final
	{
		User* user;
		unsigned long level;
		std::string role;
		bool hidden;
	};
}

class CommandIRCOps final
	: public Command
{
private:
	UserModeReference hideopermode;
	UserModeReference helpopmode;

	bool IsAutomaticHelper(User* user) const
	{
		/*
		 * Um usuario nao-oper e considerado Helper automaticamente
		 * quando possui rank de @op ou superior em um dos canais
		 * oficiais de ajuda configurados.
		 *
		 * OP_VALUE = 30000.
		 * Founder/Admin customizados possuem rank superior e tambem
		 * satisfazem este criterio.
		 */
		for (const auto* memb : user->chans)
		{
			if (memb->GetRank() < OP_VALUE)
				continue;

			for (const auto& helperchannel : helperchannels)
			{
				if (irc::equals(memb->chan->name, helperchannel))
					return true;
			}
		}

		return false;
	}

	bool IsHelper(User* user) const
	{
		/*
		 * +h continua sendo a forma explicita de marcar um Helper.
		 */
		if (helpopmode && user->IsModeSet(helpopmode))
			return true;

		/*
		 * Alternativamente, @op ou superior em canal oficial de ajuda
		 * concede status automatico de Helper no /IRCOPS.
		 */
		return IsAutomaticHelper(user);
	}

public:
	unsigned long helperlevel = 20;
	std::vector<std::string> helperchannels;

	CommandIRCOps(Module* Creator)
		: Command(Creator, "IRCOPS")
		, hideopermode(Creator, "hideoper")
		, helpopmode(Creator, "helpop")
	{
	}

	CmdResult Handle(User* source, const Params&) override
	{
		std::vector<StaffEntry> entries;

		const bool canseehidden =
			source->HasPrivPermission("users/auspex");

		/*
		 * IRCops reais.
		 *
		 * all_opers contem operadores locais e remotos, portanto
		 * /IRCOPS representa toda a rede.
		 */
		for (auto* oper : ServerInstance->Users.all_opers)
		{
			if (!oper || !oper->IsFullyConnected())
				continue;

			if (oper->server->IsService())
				continue;

			const bool hidden = oper->IsModeSet(hideopermode);

			/*
			 * +H oculta o IRCop para usuarios sem users/auspex.
			 * O proprio usuario continua podendo visualizar sua linha.
			 */
			if (hidden && !canseehidden && source != oper)
				continue;

			const auto level =
				oper->oper->GetConfig()->getNum<unsigned long>("level", 0);

			/*
			 * GetType() retorna o tipo administrativo real:
			 *
			 * Services Root
			 * Services Administrator
			 * Services Operator
			 * NetAdmin
			 * GlobalOp
			 *
			 * GetName() retornaria o nome da conta/O-Line,
			 * por exemplo "cirinho".
			 */
			entries.push_back({
				oper,
				level,
				oper->oper->GetType(),
				hidden
			});
		}

		/*
		 * Helpers nao-oper.
		 *
		 * Sao reconhecidos por:
		 *
		 * 1. usermode +h (helpop); OU
		 * 2. @op ou superior nos canais oficiais de ajuda.
		 *
		 * IRCops nao sao duplicados como Helpers.
		 */
		for (const auto& [_, helper] : ServerInstance->Users.GetUsers())
		{
			if (!helper || !helper->IsFullyConnected())
				continue;

			if (helper->server->IsService())
				continue;

			if (helper->IsOper())
				continue;

			if (!IsHelper(helper))
				continue;

			entries.push_back({
				helper,
				helperlevel,
				"Helper",
				false
			});
		}

		/*
		 * Ordem:
		 *
		 * 1. level administrativo decrescente;
		 * 2. nick alfabetico usando o casemapping IRC.
		 */
		std::sort(entries.begin(), entries.end(),
			[](const StaffEntry& lhs, const StaffEntry& rhs)
			{
				if (lhs.level != rhs.level)
					return lhs.level > rhs.level;

				return irc::insensitive_swo()(lhs.user->nick, rhs.user->nick);
			});

		const std::string header = INSP_FORMAT(
			"{}  {}  {}",
			FitColumn("Nick", NICK_WIDTH),
			FitColumn("Status", ROLE_WIDTH),
			"Availability"
		);

		source->WriteNumeric(
			RPL_IRCOPS,
			std::string("\x02") + header + "\x02"
		);

		source->WriteNumeric(
			RPL_IRCOPS,
			std::string(NICK_WIDTH + ROLE_WIDTH + 30, '-')
		);

		size_t away = 0;

		for (const auto& entry : entries)
		{
			std::string availability;

			if (entry.user->IsAway())
			{
				availability = "Away";
				away++;
			}
			else
				availability = "Available for help";

			if (entry.hidden && (canseehidden || source == entry.user))
				availability.append(" [Hidden]");

			source->WriteNumeric(
				RPL_IRCOPS,
				INSP_FORMAT(
					"{}  {}  {}",
					FitColumn(entry.user->nick, NICK_WIDTH),
					FitColumn(entry.role, ROLE_WIDTH),
					availability
				)
			);
		}

		const size_t available = entries.size() - away;

		source->WriteNumeric(
			RPL_IRCOPS,
			INSP_FORMAT(
				"{} staff member(s) online - {} available - {} away",
				entries.size(),
				available,
				away
			)
		);

		source->WriteNumeric(
			RPL_IRCOPS,
			"End of /IRCOPS"
		);

		return CmdResult::SUCCESS;
	}
};

class ModuleVircioIRCOps final
	: public Module
{
private:
	CommandIRCOps cmdircops;

public:
	ModuleVircioIRCOps()
		: Module(
			VF_OPTCOMMON,
			"Adds the vIRCio /IRCOPS command for displaying network staff."
		)
		, cmdircops(this)
	{
	}

	void ReadConfig(ConfigStatus&) override
	{
		const auto& tag =
			ServerInstance->Config->ConfValue("vircioircops");

		cmdircops.helperlevel =
			tag->getNum<unsigned long>("helperlevel", 20, 0);

		cmdircops.helperchannels.clear();

		const auto channels =
			tag->getString(
				"helperchannels",
				"#Help #Ajuda #vIRCio"
			);

		irc::spacesepstream stream(channels);

		for (std::string channel; stream.GetToken(channel); )
		{
			if (!ServerInstance->Channels.IsChannel(channel))
			{
				throw ModuleException(
					this,
					"Invalid helper channel in <vircioircops:helperchannels>: "
						+ channel
				);
			}

			cmdircops.helperchannels.push_back(channel);
		}
	}
};

MODULE_INIT(ModuleVircioIRCOps)
