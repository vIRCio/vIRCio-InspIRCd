# vIRCio InspIRCd — Agent Instructions

Este repositório contém o InspIRCd v4 da Rede vIRCio.

## Antes de trabalhar

Leia, nesta ordem:

1. este `AGENTS.md`;
2. `.opencode/skills/inspircd-module-development/SKILL.md`.

Não iniciar trabalho relevante antes dessas duas leituras.

## Manutenção deste arquivo

Este arquivo deve permanecer curto e operacional.

Sempre que uma regra, decisão arquitetural, estado de implementação ou
procedimento aqui documentado mudar, atualizar também este `AGENTS.md`.

Remover informações obsoletas em vez de apenas acumulá-las.

## Estado atual

Branch:

    v4

Baseline upstream:

    InspIRCd 4.12.1
    4785cd00553ac81cfbef7f0278803ebec42225f3

Baseline vIRCio:

    tag: vIRCio-baseline-v4.12.1
    commit: 7aaad71ef9d46d1e7ef1980fed1718913876a6f0

## Paths

Source:

    /home/vircio/vIRCio-InspIRCd-v2

Runtime:

    /home/vircio/inspircd

Runtime conf:

    /home/vircio/inspircd/conf

Runtime modules:

    /home/vircio/inspircd/modules

Logs:

    /home/vircio/inspircd/logs

## Controle do daemon

Usar somente:

    /home/vircio/inspircd/inspircd start
    /home/vircio/inspircd/inspircd stop
    /home/vircio/inspircd/inspircd status
    /home/vircio/inspircd/inspircd rehash

Não iniciar diretamente:

    /home/vircio/inspircd/bin/inspircd

Não usar `kill -9` salvo recuperação emergencial explicitamente autorizada.

## Política de implementação

Ordem de preferência:

1. recurso nativo do InspIRCd;
2. contrib mantido;
3. módulo próprio vIRCio;
4. mudança upstream quando a necessidade for genérica;
5. alteração direta do core somente excepcionalmente.

Para código próprio:

- não portar módulo v2 linha por linha;
- extrair a intenção funcional e reimplementar contra a API pública v4;
- usar C++17 e o estilo nativo do projeto;
- não usar `VF_VENDOR` em módulos próprios;
- evitar patches no core quando um módulo puder resolver;
- projetar já pensando no upgrade v4 -> v5, isolando dependências da API e
  evitando acoplamento a internals para permitir reaproveitamento do código
  com o mínimo possível de mudanças estruturais.

## Integridade upstream

Manter o core upstream limpo para funcionalidades específicas da vIRCio.

Atualizações do InspIRCd devem integrar o release oficial completo.

Contrib pode receber patch local quando houver bug confirmado, desde que seja:

- mínimo;
- documentado;
- acompanhado upstream quando aplicável;
- removido quando a correção oficial for incorporada.

## Configuração

Arquivos comuns principais:

    vIRCio/conf/inspircd.conf
    vIRCio/conf/vIRCio.modules
    vIRCio/conf/vIRCio.helpop
    vIRCio/conf/vIRCio.opermotd

Node-specific:

    ircd.conf

Secrets/runtime:

    vIRCio.roots
    keys
    passwords
    tokens
    cloak keys

Nunca exibir ou commitar secrets.

## Build e implantação

Do source:

    make -j5
    make install

Contrib:

    ./modulemanager install <module>
    make -j5 install

Quando uma configuração versionada também for implantada no runtime:

    cp vIRCio/conf/<arquivo> /home/vircio/inspircd/conf/<arquivo>

Depois usar rehash quando apropriado.

Ao substituir core/binário, fazer restart controlado.

## Git

Antes de editar:

    git status --short

Não sobrescrever alterações desconhecidas.

Commits devem ser focados por feature/módulo.

Não mover nem reescrever tags históricas.

Tags importantes:

    vIRCio-baseline-v4.12.0
    vIRCio-config-v4.12.0
    vIRCio-baseline-v4.12.1

Não fazer commit ou push sem autorização explícita.

## Contrib implementados

Administrativos:

    clones
    stats_unlinked
    jumpserver
    lockserv
    xlinetools

UX/proteção:

    defaulttopic
    autoaway
    hideidle
    autodrop
    tgchange

SWHOIS:

    swhois_ext

`swhois_ext` substitui o `m_swhois` nativo.

Existe patch local temporário para `DoClear` / `DoDel`, que devem operar
sobre `target` e não `source`.

Issue upstream:

    #2232

Remover o patch quando a correção oficial for incorporada.

## Módulos próprios vIRCio

Implementados e habilitados:

    m_vircio_ircops.cpp
    m_vircio_root.cpp
    m_vircio_pretenduser.cpp
    m_vircio_invisible.cpp
    m_vircio_zombie.cpp

Estado:

- `m_vircio_ircops`: `/IRCOPS`, `VF_OPTCOMMON`;
- `m_vircio_root`: proteção administrativa de Services Root, `VF_COMMON`;
- `m_vircio_pretenduser`: PRETENDUSER com privilégios e proteções próprias,
  `VF_OPTCOMMON`;
- `m_vircio_invisible`: usermode `+Q`, `VF_COMMON`;
- `m_vircio_zombie`: quarentena usermode `+Z`, `VF_COMMON`.

O lado InspIRCd do Zombie está implementado.

Integração positiva completa com Anope/OperServ/ZombieGringo será feita em
fase própria.

Os antigos modos administrativos `+N`, `+A` e `+O` não são dependência dos
módulos atuais e permanecem fora desta fase.

## Fase atual

Revisão integrada dos cinco módulos próprios, configuração e documentação.

Objetivos imediatos:

    revisar código em conjunto
    build completo
    git diff --check
    eliminar inconsistências residuais
    preparar versionamento somente após revisão

## Próxima fase

Integração e homologação completa com:

    múltiplos IRCds
    Anope / OperServ
    ZombieGringo
    testes de burst, split e relink
    bateria pré-produção

## Testes

Evitar testes repetitivos sem ganho.

Módulo local:

    build/load
    comportamento nominal
    erro/permissão relevante

Módulo distribuído:

    dois servidores
    burst
    split
    relink

Testes extensos de regressão ficam para a homologação pré-produção.

## Estilo de execução

Preferir uma rodada completa quando as operações forem seguras para agrupar.

Pedir output apenas quando ele determinar a próxima decisão.

Se algo passou, registrar e seguir.

Não inventar falhas.
