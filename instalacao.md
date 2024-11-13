# Guia de Instalação do vIRCio-InspIRCd

## Passo 1: Download do Arquivo

Para começar, faça o download do repositório principal:

```bash
wget https://github.com/vIRCio/vIRCio-InspIRCd/archive/master.zip
```

## Passo 2: Descompactar

Descompacte o arquivo baixado:

```bash
unzip master.zip
```

## Passo 3: Entrar na Pasta do Projeto

Entre na pasta descompactada:

```bash
cd vIRCio-InspIRCd-master
```

## Passo 4: Compilar o Projeto

Configure e compile o projeto utilizando os comandos abaixo:

```bash
./configure
./configure --prefix=/home/vircio/ircd/ --enable-extras=m_geoip.cpp,m_mysql.cpp,m_regex_pcre.cpp,m_ssl_openssl.cpp
make
make install
```

## Configuração dos Arquivos de Configuração

### 1. Preparar a Pasta de Configuração

Entre na pasta do IRCd:

```bash
cd ~/ircd
```

Apague a pasta padrão de configuração:

```bash
rm -rf conf/
```

Copie a pasta de configuração modelo do vIRCio:

```bash
cp -R ~/vIRCio-InspIRCd-master/conf ~/ircd/conf
```

### 2. Editar os Arquivos de Configuração

Entre na pasta de configuração:

```bash
cd conf
```

Edite os seguintes arquivos de configuração conforme necessário:

- **ircd.conf**: Configure o nome do servidor e os IPs para IPv4 e IPv6.
  
  ```bash
  nano ircd.conf
  ```

- **links.conf**: Adicione os dados necessários para realizar a ligação com outros servidores.
  
  ```bash
  nano links.conf
  ```

- **ircops.conf**: Configure as permissões de operadores (O-lines).

  ```bash
  nano ircops.conf
  ```

> **Nota**: Não edite os demais arquivos sem autorização da equipe. É importante que todos os servidores linkados tenham a mesma configuração para evitar incompatibilidades.
```