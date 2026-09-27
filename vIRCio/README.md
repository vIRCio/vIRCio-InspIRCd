# vIRCio

Esta pasta contém os arquivos específicos da Rede vIRCio utilizados
sobre a base oficial do InspIRCd.

O objetivo é manter as customizações da rede separadas do core upstream
sempre que possível.

## Estrutura

    vIRCio/
    ├── conf/
    └── modules/

## conf/

Contém os templates de configuração utilizados pela Rede vIRCio.

Os arquivos desta pasta representam a configuração padrão da rede e não
devem conter segredos reais.

Arquivos específicos de cada IRCd devem ser adaptados a partir dos
templates disponibilizados.

Consulte:

    conf/README.md

## modules/

Contém os módulos próprios desenvolvidos para necessidades específicas
da Rede vIRCio.

A ordem de preferência para implementar funcionalidades é:

1. recurso nativo do InspIRCd;
2. módulo contrib existente;
3. módulo próprio da vIRCio.

Alterações diretas no core upstream devem ser evitadas.

Consulte:

    modules/README.md

## Runtime

Arquivos gerados durante a execução do IRCd não são mantidos nesta
pasta.

Exemplos:

    vIRCio.xline
    vIRCio.permchannels
    logs
    certificados
    chaves privadas
    arquivos PID

Esses dados pertencem ao ambiente de execução e não ao source.

## Segredos

Nunca devem ser versionados:

    cloakKey real
    hashes reais de OPER
    senhas de links
    credenciais WEBIRC
    tokens
    chaves privadas TLS

Os templates versionados devem utilizar placeholders quando necessário.
