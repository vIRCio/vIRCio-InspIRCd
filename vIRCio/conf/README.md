# Configuração da Rede vIRCio

Esta pasta contém os arquivos de configuração versionados da Rede vIRCio
para InspIRCd 4.

## Arquivos comuns

    inspircd.conf
    vIRCio.modules
    vIRCio.services
    vIRCio.helpop
    vIRCio.filters
    vIRCio.motd
    vIRCio.opermotd

`vIRCio.modules` define os módulos e configurações comuns utilizados pela
rede.

`vIRCio.helpop` contém o sistema de ajuda e a compatibilidade com
`/HELPOP`.

## Configuração por IRCd

    ircd.conf.example

Deve ser usado como base para o arquivo local:

    ircd.conf

Contém parâmetros específicos do node, como nome, SID, binds,
certificados, links e gateways.

## Operadores

    vIRCio.roots.example

Deve ser usado como base para:

    vIRCio.roots

Contém tipos, classes e O-Lines administrativas.

Credenciais reais não devem ser versionadas.

## Runtime

Arquivos gerados pelo InspIRCd não fazem parte dos templates, incluindo:

    vIRCio.xline
    vIRCio.permchannels

## Segredos

Nunca versionar:

    cloak keys
    hashes e senhas reais
    credenciais de links
    credenciais WEBIRC
    tokens
    chaves privadas TLS
    secrets de serviços externos
