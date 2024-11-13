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

// Define macro para enviar  mensagem ao user "Zombie"
#define ZOMBIE_MESSAGE(source) source->WriteServ("NOTICE %s :*** \002Atenção:\002 É necessário que você registre ou identifique-se junto ao NickServ para poder entrar nos canais ou falar com alguém. Para registrar seu nick, digite: \002/NICKSERV REGISTER senha email\002", source->nick.c_str());

// Define serviço NickServ
#define NICKSERV_NAME "nickserv"

// Define o canal para os "Zombies" entrar
#define ZOMBIE_CHAN "#vircio"

// Classe que representa user mode +Z (Zombie/Gringo)
class User_Z : public ModeHandler
{
public:
    // Construtor da classe que inicia o modo +Z
    User_Z(Module* Creator) : ModeHandler(Creator, "u_services_zombie", 'Z', PARAM_NONE, MODETYPE_USER) { }

    // Método chamado quando o modo +Z é alterado no user
    ModeAction OnModeChange(User* source, User* dest, Channel* channel, std::string &parameter, bool adding)
    {
        // Verifica se o comando é originado de outro servidor e não de um usuário local
        if (!IS_LOCAL(source))
        {
            // Impede a definição do modo +Z em users já registrados (+r)
            if (dest->IsModeSet('r'))
                return MODEACTION_DENY;

            // Verifica se o modo +Z está sendo adicionado e o usuário ainda não possui esse modo
            if ((adding != dest->IsModeSet('Z')))
            {
                // Envia mensagem de aviso ao usuário
                ZOMBIE_MESSAGE(dest);
                // Define o modo +Z para o usuário
                dest->SetMode('Z', adding);

                // Se o usuário for local, força entrada no canal para "Zombies"
                if (IS_LOCAL(dest)) {
                    Channel::JoinUser(dest, ZOMBIE_CHAN, false, "", false, ServerInstance->Time());
                }

                return MODEACTION_ALLOW;
            }
        }
        else
        {
            // Se o user tentar definir manualmente o modo +Z, retorna o erro de que apenas o servidor pode modificar esse modo
            source->WriteNumeric(500, "%s :Only a server may modify the +Z user mode", source->nick.c_str());
        }
        return MODEACTION_DENY;
    }
};

// Classe principal do módulo que define e gerencia o modo +Z
class ModuleVircio : public Module
{
    // Instância do modo de usuário +Z
    User_Z user_Z;

public:
    // Construtor da classe que inicializa a instância do modo +Z
    ModuleVircio() : user_Z(this)
    {
    }

    // Método de inicialização do módulo
    void init()
    {
        // Registra o modo +Z no servidor
        ServiceProvider* providerlist[] = { &user_Z};
        ServerInstance->Modules->AddServices(providerlist, sizeof(providerlist)/sizeof(ServiceProvider*));

        // Anexa os eventos ao módulo
        Implementation eventlist[] = { I_OnWhois, I_OnUserPreMessage, I_OnUserPreNotice, I_OnUserPreJoin };
        ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));
    }

    // Evento chamado antes de um usuário enviar uma mensagem
    ModResult OnUserPreMessage(User *user, void *dest, int target_type, std::string &text, char status, CUList &exempt_list)
    {
        // Verifica se o usuário está no modo +Z
        if(user->IsModeSet('Z')) {
            std::string nickChan = "";

            // Identifica o destino da mensagem, se é um canal ou outro usuário
            if (target_type == TYPE_CHANNEL) {
                Channel *d = (Channel *) dest;
                nickChan = d->name.c_str();
            } else {
                User *d = (User *) dest;
                nickChan = d->nick.c_str();
            }

            // Converte o destino para minúsculas para comparação
            std::transform( nickChan.begin(), nickChan.end(), nickChan.begin(), ::tolower );

            // Permite mensagens apenas para o NickServ ou o canal ZOMBIE_CHAN
            if(nickChan == NICKSERV_NAME || nickChan == ZOMBIE_CHAN)
                return MOD_RES_PASSTHRU;

            // Se o destino for diferente, bloqueia a mensagem e envia um aviso
            ZOMBIE_MESSAGE(user);
            return MOD_RES_DENY;
        }

        return MOD_RES_PASSTHRU;
    }

    // Evento chamado antes de um usuário enviar um aviso (notice)
    ModResult OnUserPreNotice(User* user, void* dest, int target_type, std::string &text, char status, CUList &exempt_list)
    {
        // Se o usuário está no modo +Z, bloqueia o envio do aviso e envia uma mensagem de alerta
        if(user->IsModeSet('Z')) {
            ZOMBIE_MESSAGE(user);
            return MOD_RES_DENY;
        }

        return MOD_RES_PASSTHRU;
    }

    // Evento chamado antes de um usuário entrar em um canal
    ModResult OnUserPreJoin(User* user, Channel* chan, const char* cname, std::string &privs, const std::string &keygiven)
    {
        // Verifica se o usuário está no modo +Z
        if(user->IsModeSet('Z')) {
            std::string channame = cname;
            // Converte o nome do canal para minúsculas para comparação
            std::transform( channame.begin(), channame.end(), channame.begin(), ::tolower );

            // Permite a entrada apenas no canal ZOMBIE_CHAN
            if(channame == ZOMBIE_CHAN)
                return MOD_RES_PASSTHRU;

            // Se o canal for diferente, bloqueia a entrada e envia um aviso
            ZOMBIE_MESSAGE(user);
            return MOD_RES_DENY;
        }

        return MOD_RES_PASSTHRU;
    }

    // Método que retorna a versão do módulo
    Version GetVersion()
    {
        return Version("Fornece suporte aos modos de usuário da rede vIRCio", VF_OPTCOMMON|VF_VENDOR);
    }
};

// Macro para inicializar o módulo no InspIRCd
MODULE_INIT(ModuleVircio)