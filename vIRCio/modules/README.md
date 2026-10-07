# Módulos próprios da Rede vIRCio

Os módulos próprios da vIRCio são implementados sem alterar o core
upstream do InspIRCd.

Os sources compiláveis ficam em:

    src/modules/

## Implementados

### m_vircio_ircops.cpp

Fornece `/IRCOPS` e o usermode não-oper `+h`, exibindo IRCops e Helpers da
rede conforme OperType, nível administrativo e regras de visibilidade.

### m_vircio_admin.cpp

Protege IRC Admins elegíveis contra ações administrativas forçadas por
operadores sem autorização equivalente. A elegibilidade usa o nível mínimo
configurado e o privilégio `users/vircio-admin`; não depende do nome literal
de um OperType.

### m_vircio_pretenduser.cpp

Fornece `PRETENDUSER`, permitindo execução administrativa de uma linha
IRC em nome de outro usuário, com controles de permissão e hierarquia.

### m_vircio_invisible.cpp

Fornece o usermode `+Q`, usado por IRCops humanos para ocultar sua
presença em canais de usuários sem `users/auspex`.

### m_vircio_zombie.cpp

Fornece o usermode `+Z` e a quarentena Zombie/Gringo.

Enquanto `+Z` está ativo, o usuário fica restrito ao canal de quarentena
e aos Services. A integração positiva com Anope/ZombieGringo será
homologada separadamente.

## Desenvolvimento

Ordem de preferência:

1. recurso nativo do InspIRCd;
2. módulo contrib mantido;
3. módulo próprio da vIRCio.

Não portar código legado linha por linha.

Reimplementar o comportamento necessário usando a API atual do
InspIRCd, mantendo o código preparado para futura migração para v5.

Alterações diretas no core upstream devem ser evitadas.
