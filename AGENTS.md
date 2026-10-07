# vIRCio InspIRCd — instruções operacionais

## Antes de trabalhar

Leia, nesta ordem:

1. este `AGENTS.md`;
2. `.opencode/skills/inspircd-module-development/SKILL.md`.

## Estado e paths

Branches:

    v2 = legado
    v4 = linha atual e branch padrão
    v5 = futura linha baseada no upstream InspIRCd 5

Baseline da v4:

    InspIRCd 4.12.1
    upstream: 4785cd00553ac81cfbef7f0278803ebec42225f3
    baseline vIRCio: vIRCio-baseline-v4.12.1

Source:

    /home/vircio/vIRCio-InspIRCd

Runtime:

    /home/vircio/inspircd

Runtime conf, módulos e logs:

    /home/vircio/inspircd/conf
    /home/vircio/inspircd/modules
    /home/vircio/inspircd/logs

Fase atual: integração e homologação com Anope 2.1. A arquitetura está
definida; a homologação da integração ainda é pendente.

## Rede e Services

Portas:

    6667 = clientes IRC plaintext
    6697 = clientes IRC TLS
    7002 = WebSocket
    7007 = exclusivamente IRCd <-> IRCd S2S com TLS
    7008 = exclusivamente Anope <-> InspIRCd, 127.0.0.1, type=servers, sem TLS

Anope 2.1.27 inicia a conexão com o IRCd em 7008. Não usar `<autoconnect>`
para `services.vircio.net`. A primeira implantação usa `pt_BR.UTF-8` e
`db_json`; JSON-RPC é planejado. StatServ/irc2sql não pertence a essa fase.

No IRCd, os OperTypes atuais são:

    IRC Admin  = nível 100, acesso total, users/vircio-admin
    IRC Oper  = nível 50, operação global cotidiana

Services Root, Services Admin, Services Oper e Helper pertencem
exclusivamente à hierarquia do Anope. Helper não é OperType do IRCd:
`m_vircio_ircops` o identifica, quando não-oper, por `+h` ou por `@op` ou
superior em canal oficial de ajuda.

`m_vircio_admin` protege IRC Admins elegíveis por nível mínimo (padrão 100) e
privilégio `users/vircio-admin`; não depende do nome literal de um OperType.

## Controle do daemon

Usar somente:

    /home/vircio/inspircd/inspircd start
    /home/vircio/inspircd/inspircd stop
    /home/vircio/inspircd/inspircd status
    /home/vircio/inspircd/inspircd rehash

Não iniciar diretamente `/home/vircio/inspircd/bin/inspircd`. Não usar
`kill -9` salvo recuperação emergencial explicitamente autorizada.

## Desenvolvimento e segurança

Ordem de preferência:

1. recurso nativo do InspIRCd;
2. contrib mantido;
3. módulo próprio vIRCio;
4. mudança upstream quando genérica;
5. core somente excepcionalmente.

Código próprio usa C++17, API pública v4 e estilo nativo; não portar v2 linha
por linha, não usar `VF_VENDOR` e não criar patch de core quando um módulo
resolve. Isolar dependências da v4 para a futura migração à v5.

## Source, runtime e configuração

O source é versionado; o runtime não é. Configuração comum versionável fica em
`vIRCio/conf/`. `ircd.conf` e `vIRCio.roots` reais são específicos do nó e
ficam somente no runtime. Compare a semântica antes de instalar; nunca copie
um secret do runtime para um template versionado.

O link local de Anope usa 7008 e é iniciado pelo Anope; o IRCd não deve ter
`<autoconnect>` para Services. Os módulos próprios ativos são:

    m_vircio_admin        VF_COMMON
    m_vircio_ircops       VF_OPTCOMMON
    m_vircio_invisible    VF_COMMON
    m_vircio_pretenduser  VF_OPTCOMMON
    m_vircio_zombie       VF_COMMON

`m_vircio_root` foi removido. Não o restaure nem crie OperTypes administrativos
para Services Root, Services Admin, Services Oper ou Helper. Apenas IRC Admin
(nível 100) e IRC Oper (nível 50) são OperTypes deste IRCd.

## Build, instalação e testes

Antes de editar, confira `git status --short`; preserve trabalho desconhecido.
Antes de propor uma alteração, valide contra headers e módulos oficiais desta
baseline e execute `git diff --check`.

Esta baseline usa o build nativo Make; `build/GCC-14.2` contém os artefatos:

    make -j4
    make install

Instale arquivos versionáveis no runtime de forma explícita, sem sobrescrever
`ircd.conf`, `vIRCio.roots` ou materiais TLS. Use rehash para mudanças de
configuração e restart controlado somente quando binário ou módulos carregados
exigirem. Após a mudança, confirme daemon, listeners, módulos e logs.

Para módulos `VF_COMMON`, estado distribuído ou decisões por OperType remoto,
planeje também cenário real de dois IRCds (propagação, burst, split e relink).
Não alegue essa cobertura sem executá-la. Testes de Services devem verificar a
origem legítima e o link ativo, sem alterar source ou configuração do Anope.

Nunca exibir ou commitar senhas, tokens, chaves privadas, cloak keys,
credenciais de links/WEBIRC ou outros secrets. Preserve alterações
desconhecidas; não faça commit ou push sem autorização explícita.
