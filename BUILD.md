# Build - compilando e executando OVMF

## Ambiente utilizado
- Sistema operacional: Ubuntu 24.04 LTS (outras distribuições devem funcionar com os devidos ajustes)
- Arquitetura do host: x86_64 (aka amd64)
- Compilador: GCC
- Firmware: OVMF (EDK II)
- Emulador QEMU

## 1 - Instalação de dependências

No terminal, instale as dependências a seguir:

```sudo apt update```

```
sudo apt install -y \
    build-essential \
    git \
    uuid-dev \
    acpica-tools \
    nasm \
    python3 \
    python-is-python3 \
    qemu-system-x86
```

Observação: o QEMU não é necessário para compilar o firmware mas será utilizado para executar a OVMF.

## 2 - Obtendo o código fonte do EDK II

No diretório de trabalho, clone o repositório oficial do projeto:

```git clone --recurse-submodules https://github.com/tianocore/edk2.git```

## 3 - Compilando as ferramentas do EDK II

Para compilar as ferramentas auxiliares do próprio EDK II, é necessário rodar na raíz do repositório:

```make -C BaseTools```

## 4 - Preparando o ambiente de compilação

Carregas as configurações no shell atual (não é necessário toda vez)

```. ./edksetup.sh```

## 5 - Compilando o OVMF

Compilação propriamente dita

```build \
    -p OvmfPkg/OvmfPkgX64.dsc \
    -a X64 \
    -t GCC \
    -b DEBUG```

O parâmetro -n pode ser incluindo para especificar o número de threads utilizadas.
Se a compilação terminar sem erros, os artefatos serão encontrados em
```Build/OvmfX64/DEBUG_GCC/FV/```

- OVMF.fd: imagem completa do firmware
- OVMF_CODE.fd: região de código do firmware
- OVMF_VARS.fd: modelo inicial da área de variáveis UEFI

## 6 - Executando o OVMF no QEMU

Execute o seguinte comando:

```qemu-system-x86_64 \
    -machine q35 \
    -m 2048 \
    -drive if=pflash,format=raw,unit=0,readonly=on,file=Build/OvmfX64/DEBUG_GCC/FV/OVMF_CODE.fd \
    -drive if=pflash,format=raw,unit=1,file=run-ovmf/OVMF_VARS.fd \
    -net none```

Uma máquina virtual deve inicializar com a OVMF recém-compilada.
Nos parâmetros, estão especificados 2GB RAM e sem interface de rede.

