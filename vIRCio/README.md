# vIRCio

Arquivos específicos da Rede vIRCio utilizados sobre a base oficial do
InspIRCd.

As customizações da rede são mantidas separadas do core upstream sempre
que possível.

## Estrutura

    vIRCio/
    ├── conf/
    └── modules/

## conf/

Templates e arquivos comuns de configuração da Rede vIRCio.

Consulte:

    conf/README.md

## modules/

Documentação dos módulos próprios da Rede vIRCio.

Os sources compiláveis ficam em:

    src/modules/m_vircio_*.cpp

Consulte:

    modules/README.md

## Runtime

Dados gerados em execução, certificados, chaves e segredos não pertencem
ao source e não devem ser versionados.
