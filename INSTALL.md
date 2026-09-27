# Instalação do vIRCio-InspIRCd 4

Este documento descreve o procedimento utilizado pela Rede vIRCio para
preparar, compilar e instalar o InspIRCd 4.

A base atual do projeto é:

    InspIRCd 4.12.0

Branch:

    v4

Baseline homologada:

    vIRCio-baseline-v4.12.0


## Repositórios

Projeto oficial InspIRCd:

https://github.com/inspircd/inspircd

Fork da Rede vIRCio:

https://github.com/vIRCio/vIRCio-InspIRCd-v2


## Sistema utilizado

A instalação atual da Rede vIRCio utiliza Debian GNU/Linux.

Os exemplos deste documento consideram o usuário:

    vircio

Código fonte:

    /home/vircio/vIRCio-InspIRCd-v2

Instalação:

    /home/vircio/inspircd


## Dependências

Atualize os índices dos pacotes:

    sudo apt update

Instale as dependências:

    sudo apt install -y \
        git \
        ca-certificates \
        build-essential \
        pkg-config \
        perl \
        libssl-dev \
        libpcre2-dev \
        libmaxminddb-dev


## Clonar o repositório

Entre no diretório do usuário:

    cd /home/vircio

Clone o repositório:

    git clone git@github.com:vIRCio/vIRCio-InspIRCd-v2.git

Entre no source:

    cd /home/vircio/vIRCio-InspIRCd-v2

Selecione a branch atual:

    git checkout v4


## Extras utilizados pela vIRCio

Antes da configuração principal, habilite os módulos extras utilizados
pela Rede vIRCio:

    ./configure --enable-extras regex_pcre2,geo_maxmind,ssl_openssl,sslrehashsignal

Atualmente são preparados:

- regex_pcre2
- geo_maxmind
- ssl_openssl
- sslrehashsignal

O fato de um extra estar compilado não significa necessariamente que
ele já esteja ativo na configuração da rede.


## Configurar a compilação

Configure o prefixo da instalação:

    ./configure --prefix=/home/vircio/inspircd


## Compilar e instalar

Compile e instale:

    make -j5 install


## Verificar a versão

Execute:

    /home/vircio/inspircd/bin/inspircd --version

Resultado esperado:

    InspIRCd-4.12.0


## Diretórios

Código fonte:

    /home/vircio/vIRCio-InspIRCd-v2

Instalação:

    /home/vircio/inspircd

Configuração:

    /home/vircio/inspircd/conf

Módulos:

    /home/vircio/inspircd/modules

Dados:

    /home/vircio/inspircd/data

Logs:

    /home/vircio/inspircd/logs


## Configuração da Rede vIRCio

Os templates versionados ficam em:

    /home/vircio/vIRCio-InspIRCd-v2/vIRCio/conf

A configuração operacional fica em:

    /home/vircio/inspircd/conf

Os arquivos comuns da Rede vIRCio devem ser copiados a partir dos
templates versionados.

Arquivos específicos do servidor devem ser ajustados individualmente.

Exemplos:

    ircd.conf.example
    vIRCio.roots.example

devem dar origem respectivamente a:

    ircd.conf
    vIRCio.roots

Nunca copie placeholders diretamente para produção sem revisar os
valores específicos do servidor.


## Segredos

Os templates do Git não contêm os segredos reais.

Nunca versione:

- cloakKey real;
- hashes reais de OPER;
- senhas entre IRCds;
- credenciais WEBIRC;
- tokens;
- chaves privadas TLS;
- certificados privados;
- outros secrets da infraestrutura.

Os valores reais devem ser configurados diretamente no ambiente de
execução.


## Certificado TLS de laboratório

Durante desenvolvimento pode ser utilizado um certificado
autoassinado.

Crie o diretório:

    mkdir -p /home/vircio/inspircd/conf/certs

Exemplo:

    openssl req \
        -x509 \
        -newkey rsa:4096 \
        -sha256 \
        -days 365 \
        -nodes \
        -keyout /home/vircio/inspircd/conf/certs/irc-dev.key \
        -out /home/vircio/inspircd/conf/certs/irc-dev.crt

Ajuste as permissões da chave privada:

    chmod 600 /home/vircio/inspircd/conf/certs/irc-dev.key

Em produção devem ser utilizados certificados apropriados para os
nomes reais dos servidores, por exemplo certificados Let's Encrypt.


## Iniciar o InspIRCd

Utilize sempre o script de controle instalado:

    /home/vircio/inspircd/inspircd start


## Verificar o status

    /home/vircio/inspircd/inspircd status


## Rehash

Depois de alterações de configuração:

    /home/vircio/inspircd/inspircd rehash


## Parar o InspIRCd

    /home/vircio/inspircd/inspircd stop


## Reiniciar

Quando for necessário um restart completo:

    /home/vircio/inspircd/inspircd stop

    /home/vircio/inspircd/inspircd start

Evite iniciar diretamente:

    /home/vircio/inspircd/bin/inspircd

O binário não deve ser utilizado como substituto do script de controle
da instalação.


## Portas atualmente utilizadas

A configuração padrão da Rede vIRCio utiliza:

    6667    IRC sem TLS
    6697    IRC com TLS
    7002    WebSocket seguro / WSS
    7007    links entre servidores via TLS

As portas podem ser alteradas na configuração específica de cada node.


## Verificação rápida

Processo:

    ps x | grep '[i]nspircd'

Listeners:

    ss -lntp | grep -E ':(6667|6697|7002|7007)\b'

Log:

    tail -n 100 /home/vircio/inspircd/logs/inspircd.log


## Atualizações

O core upstream deve permanecer o mais próximo possível do InspIRCd
oficial.

A ordem de preferência para funcionalidades específicas da Rede vIRCio
é:

1. recurso nativo do InspIRCd;
2. módulo contrib;
3. módulo próprio da vIRCio.

Alterações diretas no core upstream devem ser evitadas.


## Referências

README do fork:

    README.md

README original do InspIRCd:

    README.inspircd.md

Configurações da Rede vIRCio:

    vIRCio/conf/

Módulos próprios:

    vIRCio/modules/
