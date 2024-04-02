# INF01151 - Sistemas Operacionais II

Pode ser feito build com o comando `make`, ou executado com containers.

### Containers

Por padrão os comandos make usam o nerdCTL e Containerd para executar os containers. Isso pode ser configurado no arquivo Makefile

Cria a imagem e inicia um container interativo com o Sleep Server. Pode passar argumentos para o comando ./sleep_server:

```
$ make run _=manager
$ make run _="manager -p 5000"
```

Executa vários containers replicados:

```
$ docker compose up --profile=manager -d
$ docker compose up --profile=host -d
```
