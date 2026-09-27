# Configuração padrão da Rede vIRCio

Esta pasta contém os templates oficiais da configuração da Rede vIRCio
para InspIRCd 4.

Baseline:

- InspIRCd 4.12.0
- branch: v4
- tag: vIRCio-baseline-v4.12.0

## Arquivos

### inspircd.conf

Configuração padrão e comum a todos os IRCds da Rede vIRCio.

Não deve conter configurações específicas de um node.

### ircd.conf.example

Template das configurações específicas de cada IRCd.

Antes de utilizar, copiar para:

    ircd.conf

e ajustar:

- serverName
- serverId
- bindIPv4 / IPv6
- cloakKey
- certificados TLS
- links entre IRCds
- link com Services
- gateways WEBIRC
- O-Lines locais, quando aplicável

A cloakKey real nunca deve ser versionada.

Todos os IRCds da mesma rede devem utilizar a mesma cloakKey para gerar
cloaks consistentes.

### vIRCio.modules

Módulos oficiais do InspIRCd utilizados pela Rede vIRCio e suas
configurações.

### vIRCio.services

Aliases, reservas de nicks e integração do InspIRCd com Services.

O link físico com o Anope deve permanecer no ircd.conf de cada node.

### vIRCio.roots.example

Classes, tipos e O-Lines administrativas da Rede vIRCio.

Antes do uso, copiar para:

    vIRCio.roots

Os hashes reais dos OPERs não são armazenados no Git.

### vIRCio.helpop

Sistema de ajuda baseado no help oficial do InspIRCd 4.

Inclui compatibilidade com /HELPOP.

### vIRCio.filters

Filtros de spam, vírus e avisos administrativos.

Utiliza PCRE2.

### vIRCio.motd

MOTD público da Rede vIRCio.

### vIRCio.opermotd

MOTD exclusivo dos IRCops.

## Arquivos runtime não versionados

Os seguintes arquivos são gerados pelo próprio InspIRCd e não fazem
parte dos templates:

    vIRCio.xline
    vIRCio.permchannels

O banco de X-Lines é mantido pelo módulo xline_db.

O banco de canais permanentes é mantido pelo módulo permchannels.

## Segredos

Nunca versionar:

- cloakKey real
- hashes reais de OPER
- senhas de links
- senhas WEBIRC
- tokens
- chaves privadas TLS
- secrets de serviços externos

Em produção esses dados devem ser fornecidos separadamente da
configuração versionada.
