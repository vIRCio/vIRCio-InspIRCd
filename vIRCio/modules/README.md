# Módulos próprios da Rede vIRCio

Os módulos exclusivos da Rede vIRCio devem ser implementados aqui sem
alterar diretamente o core oficial do InspIRCd.

Módulos planejados:

## m_vircio_oper.cpp

Responsável pelas funcionalidades administrativas específicas da rede.

Previsto inicialmente:

- Services Root: +N
- Services Administrator: +A
- Services Operator: +O
- comportamento equivalente ao antigo /IRCOPS, quando necessário

## m_vircio_zombie.cpp

Port do antigo modo Zombie +Z.

O comportamento histórico será reavaliado antes da implementação.

## m_vircio_invisible.cpp

Port da funcionalidade histórica de invisibilidade +Q.

O comportamento será comparado com os recursos atuais do InspIRCd 4
antes da implementação.

## cmd_vircio_pretenduser.cpp

Port do comando PRETENDUSER utilizado pela Rede vIRCio.

## m_vircio_serverprotect.cpp

Substituto da personalização histórica feita no antigo servprotect.

A implementação deverá considerar a integração com o Anope atual e
evitar alterações diretas em módulos oficiais.

## Regra de desenvolvimento

Não modificar arquivos do core oficial para implementar funcionalidades
da vIRCio.

Sempre que possível:

1. utilizar primeiro funcionalidade nativa do InspIRCd;
2. depois avaliar módulos contrib;
3. somente então criar módulo próprio vIRCio.
