# vIRCio-InspIRCd

InspIRCd utilizado pela [Rede vIRCio](https://vircio.net).

Este repositório é um fork do projeto
[InspIRCd](https://github.com/inspircd/inspircd), adaptado para a
infraestrutura e necessidades da Rede vIRCio.

A base atual utiliza **InspIRCd 4.12.0** e mantém o core upstream o mais
limpo possível. Funcionalidades específicas da rede são implementadas
através de configuração, módulos contrib ou módulos próprios da vIRCio,
evitando alterações diretas no core sempre que possível.

## Estado atual

Branch de desenvolvimento atual:

    v4

Base upstream:

    InspIRCd 4.12.0

Baseline homologada da vIRCio:

    vIRCio-baseline-v4.12.0

A baseline foi auditada e submetida a testes funcionais antes do início
da integração dos módulos específicos da Rede vIRCio.

## Estrutura do projeto

As customizações específicas da Rede vIRCio ficam concentradas em:

    vIRCio/

Estrutura atual:

    vIRCio/
    ├── conf/
    └── modules/

### vIRCio/conf/

Contém os templates das configurações padrão da Rede vIRCio.

Dados sensíveis e arquivos gerados em runtime não são versionados.

### vIRCio/modules/

Área destinada aos módulos próprios da Rede vIRCio.

A política do projeto é, nesta ordem:

1. utilizar funcionalidades nativas do InspIRCd;
2. avaliar módulos contrib existentes;
3. desenvolver módulos próprios da vIRCio quando necessário.

Não serão feitas alterações no core upstream quando a funcionalidade
puder ser implementada adequadamente por módulo.

## Módulos próprios planejados

Entre as funcionalidades históricas que serão portadas ou
reimplementadas para InspIRCd 4 estão:

- `m_vircio_oper`
- `m_vircio_zombie`
- `m_vircio_invisible`
- `cmd_vircio_pretenduser`
- `m_vircio_serverprotect`

O comportamento das implementações antigas será revisado antes do port
para aproveitar recursos nativos existentes no InspIRCd 4.

## Instalação

Consulte:

    INSTALL.md

O documento contém o procedimento utilizado para preparar, compilar,
instalar e executar a versão da vIRCio.

## Configuração

Os templates da configuração da rede estão em:

    vIRCio/conf/

Os arquivos de exemplo não contêm credenciais reais.

Nunca devem ser versionados:

- `cloakKey` real;
- hashes reais de operadores;
- senhas de links entre servidores;
- credenciais WEBIRC;
- tokens;
- certificados privados;
- chaves privadas TLS;
- bancos de X-Lines;
- bancos de canais permanentes;
- logs e demais dados de runtime.

## Upstream

Projeto original:

https://github.com/inspircd/inspircd

Documentação oficial:

https://docs.inspircd.org/4/

O README original correspondente à base upstream utilizada por este
fork é preservado em:

    README.inspircd.md

## Histórico

A branch `master` deste repositório preserva a antiga implementação da
Rede vIRCio baseada em InspIRCd 2.

O InspIRCd 2 está obsoleto e é mantido apenas como referência histórica
para a migração das funcionalidades específicas da rede.

O desenvolvimento atual ocorre na branch `v4`.

## Licença

Este projeto é derivado do InspIRCd e permanece sujeito aos termos da
GNU General Public License versão 2 aplicáveis ao projeto upstream.

Consulte também os arquivos de licença presentes no repositório.
