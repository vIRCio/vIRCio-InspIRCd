# Instalação do vIRCio-InspIRCd 4

Procedimento para preparar, compilar e instalar o InspIRCd utilizado pela
Rede vIRCio.

Base atual:

    InspIRCd 4.12.1
    branch: v4
    tag: vIRCio-baseline-v4.12.1

Ambiente de referência:

    Debian GNU/Linux 13 (Trixie)
    usuário: vircio

Source:

    /home/vircio/vIRCio-InspIRCd-v2

Runtime:

    /home/vircio/inspircd


## Dependências

Instale as ferramentas de compilação e as bibliotecas exigidas pelos extras
utilizados pela vIRCio:

    sudo apt update

    sudo apt install -y \
        git \
        ca-certificates \
        build-essential \
        pkg-config \
        perl \
        openssl \
        libssl-dev \
        libpcre2-dev \
        libmaxminddb-dev

Essas dependências atendem aos extras atualmente preparados:

    ssl_openssl
    regex_pcre2
    geo_maxmind
    sslrehashsignal

O InspIRCd 4 requer compilador com suporte a C++17 e Perl 5.26 ou superior.


### Module Manager

Os contrib homologados pela vIRCio já são versionados no repositório e não
precisam ser baixados novamente durante uma instalação normal.

Caso seja necessário usar `./modulemanager` para adicionar ou atualizar
contribs, instale também:

    sudo apt install -y \
        libwww-perl \
        libio-socket-ssl-perl


## Clonar o repositório

    cd /home/vircio

    git clone --branch v4 \
        git@github.com:vIRCio/vIRCio-InspIRCd-v2.git

    cd /home/vircio/vIRCio-InspIRCd-v2


## Extras

A vIRCio compila explicitamente:

    regex_pcre2
    geo_maxmind
    ssl_openssl
    sslrehashsignal

Configure a compilação:

    ./configure \
        --prefix=/home/vircio/inspircd \
        --disable-auto-extras \
        --enable-extras "regex_pcre2 geo_maxmind ssl_openssl sslrehashsignal"

Nem todo extra compilado precisa estar ativo na configuração da rede.


## Compilar e instalar

    make -j5 install


## Contribs

Os módulos contrib utilizados pela vIRCio ficam versionados em
`src/modules/` junto com o source homologado.

Os módulos abaixo são os sources oficiais para InspIRCd 4 mantidos em
`inspircd/inspircd-contrib`, sem modificações locais da Rede vIRCio:

    clones
    stats_unlinked
    jumpserver
    lockserv
    xlinetools
    defaulttopic
    autoaway
    hideidle
    autodrop
    tgchange
    swhois_ext

Também é mantido no source, mas não está ativo na configuração atual:

    delayuse

O `swhois_ext` já contém upstream a correção dos comandos CLEAR e DEL
reportada pela vIRCio e não possui patch local.

Se um contrib precisar de alteração específica da Rede vIRCio, a
modificação deve ser identificada explicitamente como local e documentada.

O `modulemanager` só é necessário para adicionar ou atualizar contribs:

    ./modulemanager list
    ./modulemanager install <modulo>
    ./modulemanager upgrade

Após alteração:

    make -j5 install


## Módulos próprios vIRCio

Os módulos compiláveis próprios da rede ficam em:

    src/modules/m_vircio_*.cpp

Atualmente:

    m_vircio_ircops.cpp
    m_vircio_root.cpp
    m_vircio_pretenduser.cpp
    m_vircio_invisible.cpp
    m_vircio_zombie.cpp

A documentação específica desses módulos fica em:

    vIRCio/modules/


## Verificar a instalação

Versão:

    /home/vircio/inspircd/bin/inspircd --version

Esperado:

    InspIRCd-4.12.1

Diretórios:

    source:   /home/vircio/vIRCio-InspIRCd-v2
    runtime:  /home/vircio/inspircd
    config:   /home/vircio/inspircd/conf
    modules:  /home/vircio/inspircd/modules
    data:     /home/vircio/inspircd/data
    logs:     /home/vircio/inspircd/logs


## Configuração vIRCio

Templates e arquivos comuns versionados:

    /home/vircio/vIRCio-InspIRCd-v2/vIRCio/conf

Configuração operacional:

    /home/vircio/inspircd/conf

Arquivos específicos de cada IRCd devem ser criados a partir dos respectivos
templates.

Exemplos:

    ircd.conf.example  -> ircd.conf
    vIRCio.roots.example -> vIRCio.roots

Nunca copie placeholders para produção sem revisar os valores do servidor.


## Segredos

Nunca versionar:

    senhas e hashes reais
    cloak keys
    credenciais entre IRCds
    credenciais WEBIRC
    tokens
    chaves privadas TLS
    outros secrets da infraestrutura

Os valores reais pertencem apenas ao ambiente operacional.


## TLS de laboratório

Para desenvolvimento pode ser usado certificado autoassinado:

    mkdir -p /home/vircio/inspircd/conf/certs

    openssl req \
        -x509 \
        -newkey rsa:4096 \
        -sha256 \
        -days 365 \
        -nodes \
        -keyout /home/vircio/inspircd/conf/certs/irc-dev.key \
        -out /home/vircio/inspircd/conf/certs/irc-dev.crt

    chmod 600 \
        /home/vircio/inspircd/conf/certs/irc-dev.key

Em produção devem ser utilizados certificados apropriados aos nomes reais dos
servidores.


## Controle do daemon

Usar sempre:

    /home/vircio/inspircd/inspircd start
    /home/vircio/inspircd/inspircd status
    /home/vircio/inspircd/inspircd rehash
    /home/vircio/inspircd/inspircd stop

Para restart:

    /home/vircio/inspircd/inspircd stop
    /home/vircio/inspircd/inspircd start

Não iniciar o daemon diretamente por:

    /home/vircio/inspircd/bin/inspircd


## Portas

Configuração atual da vIRCio:

    6667    IRC
    6697    IRC/TLS
    7002    WSS
    7007    link IRCd/TLS

Os valores podem variar por node.


## Verificação rápida

Processo:

    ps x | grep '[i]nspircd'

Listeners:

    ss -lntp | grep -E ':(6667|6697|7002|7007)\b'

Log:

    tail -n 100 \
        /home/vircio/inspircd/logs/inspircd.log


## Referências

Projeto oficial:

    https://github.com/inspircd/inspircd

Fork vIRCio:

    https://github.com/vIRCio/vIRCio-InspIRCd-v2

Documentação:

    https://docs.inspircd.org/4/

Arquivos do projeto:

    AGENTS.md
    README.md
    README.inspircd.md
    vIRCio/conf/
    vIRCio/modules/
