---
name: inspircd-module-development
description: Engenharia, port, revisão e manutenção de módulos InspIRCd no estilo nativo do projeto, com foco na baseline InspIRCd 4.12.1, compatibilidade S2S, lifecycle, segurança, configuração e migração futura para v5.
compatibility: InspIRCd 4.12.1
metadata:
  project: "vIRCio"
  baseline: "4.12.1"
---

# InspIRCd Module Development Skill

## Baseline normativa

A implementação atual da Rede vIRCio usa:

- InspIRCd 4.12.1
- upstream commit:
  4785cd00553ac81cfbef7f0278803ebec42225f3

Para APIs, assinaturas, lifecycle e comportamento, o source exato dessa
baseline é a autoridade principal.

InspIRCd 5/master serve para entender a direção futura do projeto.

Todo código novo deve compilar e operar contra a API pública da baseline
v4.12.1, mas deve ser estruturado pensando na futura migração para v5:

- preferir APIs públicas e abstrações estáveis;
- evitar dependência de internals;
- isolar pontos específicos da API v4;
- evitar desenho que exija reescrita estrutural no upgrade.

Nunca use API exclusiva do v5 em código v4 sem uma camada explícita e
verificada de compatibilidade.

## Ordem das fontes

Quando houver divergência:

1. source exato InspIRCd 4.12.1;
2. documentação oficial v4;
3. source exato do módulo contrib utilizado;
4. changelog/breaking changes;
5. issues, PRs e histórico oficial;
6. source v3/v2 para arqueologia;
7. v5/master para direção futura;
8. material secundário apenas como contexto.

Não aceite automaticamente código gerado por IA.

Confira símbolos e assinaturas nos headers reais da baseline.

## Princípio central

Portar um módulo InspIRCd não significa traduzir C++ antigo.

O módulo legado serve como especificação funcional.

Antes de escrever código:

1. identificar a função real do módulo antigo;
2. verificar o que já existe nativamente;
3. verificar contrib mantido;
4. identificar somente a funcionalidade residual da vIRCio;
5. definir autoridade/localidade;
6. definir modelo de estado;
7. definir sincronização S2S;
8. definir limites de confiança;
9. considerar a portabilidade futura para v5;
10. só então implementar.

Quando duas soluções forem equivalentes na v4, preferir a que:

- usa APIs públicas;
- possui menor acoplamento ao core;
- isola detalhes específicos da v4;
- tende a exigir menos mudanças estruturais na migração para v5.

Não sacrificar correção da v4 para antecipar APIs do v5.

## Hierarquia de implementação

Preferir:

1. funcionalidade nativa do InspIRCd;
2. contrib mantido;
3. pequeno módulo próprio vIRCio usando APIs públicas;
4. PR/hook upstream quando a necessidade for genérica;
5. alteração direta de core apenas excepcionalmente.

Evitar fork desnecessário do core.

## Arquitetura

InspIRCd moderno é orientado a módulos e services.

Preferir composição de:

- Module;
- Command / SplitCommand;
- ModeHandler;
- ExtBan;
- ExtensionItem;
- DataProvider;
- dynamic_reference;
- EventListener;
- timers;
- ServerProtocol listeners.

Evitar:

- globals;
- casts para implementação interna de outro módulo;
- dependência direta entre .so;
- duplicação de API já pública.

Consumir APIs públicas quando disponíveis:

- Account::API;
- Geolocation::API;
- Regex::EngineReference;
- hash providers;
- cloak engines;
- APIs de TLS/certificado;
- demais DataProviders oficiais.

## ModuleFlags v4

VF_NONE:
sem requisito especial.

VF_CORE:
coremod.

VF_VENDOR:
módulo distribuído oficialmente pelo InspIRCd.
NÃO usar em módulo próprio vIRCio.

VF_COMMON:
deve estar presente de forma compatível em toda a rede.

VF_OPTCOMMON:
deve normalmente existir em todos os servidores para consistência,
mas não é requisito absoluto de link.

VF_DEPRECATED:
módulo upstream depreciado.

Escolher COMMON/OPTCOMMON por semântica da rede.

## Lifecycle v4

Modelo:

constructor
→ registro/attach de services
→ init()
→ ReadConfig()
→ runtime
→ Cull()
→ destruição/RAII

Constructor:

- construir members;
- commands;
- modes;
- providers;
- listeners;
- referências leves.

init():

- setup que depende do ambiente do servidor já existir.

ReadConfig(ConfigStatus&):

- parse;
- validação;
- rehash.

Prioritize():

- somente quando ordem de hooks for semanticamente necessária.

Cull():

- cleanup específico antes da destruição quando necessário;
- se sobrescrever, finalizar chamando/retornando Module::Cull().

IMPORTANTE:

Não existe um callback genérico OnUnload() para cleanup do próprio módulo
na API v4.12.1.

OnUnloadModule(Module*) informa outros módulos que determinado módulo
está sendo descarregado.

Não confundir os dois conceitos.

## Config e rehash

Preferir getters tipados:

- getString;
- getBool;
- getNum;
- getDuration;
- getCharacter;
- getEnum.

Config inválida deve usar ModuleException quando apropriado.

Rehash seguro:

parse
→ construir estado temporário
→ validar
→ abrir recursos/providers necessários
→ swap/commit
→ liberar estado antigo

Não alterar metade do estado ativo antes de uma validação que pode falhar.

## Localidade

LocalUser*:

usuário cujo socket pertence ao servidor local.

User*:

usuário local ou remoto.

Membership*:

relação específica entre user e channel.

Channel*:

entidade canal.

Server*:

topologia/link.

Antes de escrever um hook pergunte:

- a decisão ocorre na origem local?
- ocorre no servidor que possui o target?
- é apenas observação de fato já aceito?

Não reaplicar política de origem em cada servidor remoto.

Use IS_LOCAL(user) quando precisar obter LocalUser*.

Use user->IsLocal() quando bastar saber a localidade.

## ModResult

MOD_RES_PASSTHRU:
o módulo não tomou uma decisão.

MOD_RES_DENY:
veto explícito.

MOD_RES_ALLOW:
autorização explícita e, dependendo do hook, pode bypassar outras
restrições.

Regra:

"não quero bloquear" normalmente significa PASSTHRU, não ALLOW.

## Commands

SplitCommand NÃO existe para dividir parâmetros.

O parser já divide os parâmetros.

SplitCommand existe para separar a origem:

- HandleLocal(LocalUser*);
- HandleRemote(RemoteUser*);
- HandleServer(FakeUser*).

Para comandos v4:

- definir min/max params;
- preencher syntax;
- usar CmdAccess;
- usar HasPrivPermission para privilégios granulares;
- definir translation quando necessário;
- definir routing conscientemente;
- usar CmdResult::SUCCESS/FAILURE/INVALID.

Para identidade distribuída, preferir UUID.

TR_NICK permite tradução nick → UUID no S2S.

Não usar APIs antigas sem conferir:

- flags_needed;
- CMD_SUCCESS;
- CMD_FAILURE;
- Version GetVersion() override;
- APIs v2/v3 de lookup;
- callbacks antigos de metadata.

## Replies

Separar:

- resposta protocolar;
- notice humano;
- logging/snomask.

Usar numeric existente quando adequado.

Quando apropriado usar IRCv3 standard replies:

- IRCv3::Replies::Fail;
- IRCv3::Replies::Note;
- IRCv3::Replies::Warn;
- IRCv3::Replies::CapReference.

Não inventar numeric sem necessidade.

## Extension state

SimpleExtItem<T> é genérico.

Não é apenas uma flag.

Tipos relevantes:

- SimpleExtItem<T>;
- StringExtItem;
- BoolExtItem;
- IntExtItem;
- ListExtItem<T>;
- ExtensionItem custom.

Perguntar:

- estado apenas local?
- outros servidores precisam dele?
- precisa sobreviver a burst/relink?
- pertence a User, Channel ou Membership?

Para valor simples distribuído, preferir ExtensionItem sincronizado em vez
de metadata manual duplicada.

Sempre considerar também unset/delete.

## APIs entre módulos

Preferir:

ServiceProvider
→ DataProvider
→ API pública
→ dynamic_reference/API wrapper no consumidor

Evitar global pointer para outro módulo.

Dependência opcional:

- usar referência apropriada;
- testar disponibilidade.

Dependência obrigatória:

- validar deliberadamente;
- definir comportamento de falha.

Não armazenar raw pointer de provider além do lifetime garantido.

## Modes e ExtBans

Usar frameworks nativos.

Antes de escolher letra:

- verificar modes carregados;
- prefix modes;
- ranks;
- extbans;
- configuração da rede;
- módulos vIRCio planejados.

Parameter modes distribuídos devem possuir comportamento consistente em
merge.

Não implementar parser próprio de extban quando o framework cobre o caso.

## S2S

Estado distribuído é protocolo.

Para toda mutação definir:

- origem autoritativa;
- identificador canônico;
- propagação incremental;
- burst;
- unset/delete;
- split;
- relink;
- idempotência;
- compatibilidade;
- peer sem módulo;
- proteção contra feedback loop.

Identidade de usuário distribuída:

UUID, não nick.

Estado de canal pode depender de TS.

Estado de membership pode depender de membership ID.

Ao receber state remoto não retransmitir automaticamente como nova ação local.

Usar ServerProtocol::SyncEventListener quando realmente necessário.

## Link compatibility

VF_COMMON quando semântica incompatível impedir operação correta da rede.

Usar GetLinkData/CompareLinkData quando presença do módulo não bastar e
configuração também precisar ser compatível.

Nunca sincronizar secret apenas para comparar configuração.

Comparar propriedades derivadas não reversíveis quando necessário.

## Segurança

Tratar S2S como input estruturado que ainda exige validação.

Validar:

- quantidade de parâmetros;
- UUID;
- SID;
- timestamps;
- enum/action;
- lengths;
- target existente;
- Services authority.

Não assumir que qualquer origem SERVER é Services.

Verificar server->IsService() quando o padrão oficial exigir.

## WEBIRC e IP

Módulo dependente de IP deve considerar OnChangeRemoteAddress.

Exemplos:

- GeoIP;
- ASN;
- anti-abuse;
- per-IP tracking;
- cloak;
- limits.

Nunca assumir que o peer TCP original continua sendo o endereço real do
cliente após gateway/WEBIRC.

## TLS

Consumir APIs de TLS/certificado do InspIRCd.

Não inferir TLS apenas pela porta.

## C++ e estilo

Baseline v4:

C++17.

Seguir o source oficial:

- tabs;
- opening brace em linha própria;
- PascalCase para classes/métodos;
- ModuleFoo;
- CommandFoo;
- final quando apropriado;
- override;
- initializer lists;
- early return;
- const/ref;
- auto quando claro;
- STL/insp containers;
- lambdas;
- INSP_FORMAT/fmt.

Raw pointers para:

User*
Channel*
Membership*
Server*

normalmente são views non-owning.

Não deletar objetos pertencentes ao core.

## Contrib metadata

Conhecer:

$ModAuthor
$ModDesc
$ModConfig
$ModDepends
$ModConflicts
$ModMask
$CompilerFlags
$LinkerFlags
$PackageInfo

Não transportar automaticamente build glue do v4 para v5.

## v2/v3

Nunca fazer port linha a linha.

Classificar cada comportamento antigo:

- ainda necessário;
- virou recurso nativo;
- virou provider/API;
- virou ExtensionItem;
- virou listener;
- virou contrib;
- ficou obsoleto.

Código antigo é arqueologia funcional.

## v5

v5 é apenas referência arquitetural futura.

Direções atuais incluem:

- C++20;
- CMake;
- shared_ptr em mais lifecycles;
- mudanças de Extension ownership;
- IS_* migrando para Is*/As*;
- reorganização de services;
- mudanças de command translation;
- reorganização de Module properties/ABI.

Não escrever "v5 dentro do v4".

## Anti-patterns

Rejeitar:

- copiar módulo v2 literalmente;
- modificar core quando hook resolve;
- nick como identidade S2S;
- política local repetida em usuários remotos;
- estado global apenas em map local sem sync;
- pointer global entre módulos;
- provider pointer cacheado após unload;
- rehash parcial;
- ALLOW quando deveria ser PASSTHRU;
- IsOper() como substituto de privilege granular;
- confiar em qualquer SERVER como Services;
- ignorar WEBIRC/change address;
- metadata remota sem validação;
- numeric inventado;
- mode/extban letter não verificado;
- secret em LinkData;
- VF_VENDOR em módulo vIRCio.

## Armadilhas comuns em código gerado por IA

Verificar imediatamente qualquer exemplo que use:

- Version GetVersion() override;
- flags_needed;
- CMD_SUCCESS;
- CMD_FAILURE;
- ExtensionItem::EXT_USER;
- `.set()` / `.get()` em APIs que no v4 usam Set/Get;
- OnSyncUser diretamente em Module sem conferir arquitetura;
- SendMetaData de branches antigos;
- RegexFactory em vez da API Regex atual;
- VF_VENDOR em módulo custom.

Compilar mentalmente contra os headers 4.12.1 e depois realmente compilar.

## Workflow para módulo vIRCio

1. ler source legado integralmente;
2. inventariar comportamento;
3. inventariar estado;
4. definir autoridade/localidade;
5. buscar recurso nativo 4.12.1;
6. buscar contrib;
7. mapear funcionalidade residual para API v4;
8. desenhar S2S antes do C++;
9. revisar segurança;
10. implementar mínimo necessário;
11. comparar com 2 ou 3 módulos oficiais semelhantes;
12. compilar;
13. fazer teste funcional suficiente;
14. para state distribuído, testar dois servidores/burst/split/relink;
15. revisar portabilidade futura para v5;
16. registrar apenas adaptações inevitáveis específicas da API v4.

Na revisão de portabilidade, verificar principalmente se o código depende
desnecessariamente de internals, lifecycle específico, ownership específico
ou APIs que já possuem direção de mudança conhecida no v5.

## Política de teste

Não repetir auditorias sem necessidade.

Módulo local pequeno:

- build/load;
- teste nominal;
- erro/permissão relevante.

Configurável:

- um rehash válido.

State distribuído:

- dois servidores;
- propagação;
- burst;
- split/relink;
- unset/delete.

IP-sensitive:

- conexão direta;
- WEBIRC/change address.

Services-sensitive:

- Services online/offline;
- origem legítima;
- origem SERVER não-Services.

## vIRCio

Ao trabalhar neste projeto:

- leia `AGENTS.md`;
- preserve o core upstream;
- não revele secrets;
- use os paths e workflow definidos no `AGENTS.md`.

## Referências

https://github.com/inspircd/inspircd
https://github.com/inspircd/inspircd-contrib
https://docs.inspircd.org/4/
https://docs.inspircd.org/4/breaking-changes/
https://docs.inspircd.org/4/module-manager/
https://docs.inspircd.org/5/change-log/
