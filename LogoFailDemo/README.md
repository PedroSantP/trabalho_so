# LogoFAILDemo - demonstração didática da primitiva LogoFAIL em OVMF/EDK II

> **Repositório privado.** Material de avaliação acadêmica. Não redistribuir fora do escopo do projeto.

## Visão geral

Este repositório contém um módulo DXE didático que demonstra a **primitiva** associada à classe de falhas LogoFAIL: um arquivo BMP não assinado, presente na ESP, é processado pelo parser do firmware e leva a uma **transferência de controle** para um marcador interno de simulação.

O módulo **não** implementa execução arbitrária de código (ACE). O segundo estágio da cadeia — conversão da primitiva em execução não assinada — é **citado e analisado conceitualmente** no relatório do trabalho, mas **não é reproduzido nem redistribuído** neste artefato.

## O que é demonstrado

Cadeia coberta pelo módulo:

BMP na ESP -> parser -> modelagem aritmética de alocação -> detecção de inconsistência -> transferência de controle para marcador interno (função SimulatedAceMarker)

O marcador apenas registra em log/console que o desvio foi alcançado. Não há corrupção de memória, desvio real de ponteiro de instrução, nem payload.

## O que NÃO é demonstrado (e por quê)

- **Conversão da primitiva em ACE**: fora do escopo aprovado; analisada conceitualmente no relatório com base em literatura pública.
- **Anulação de `gSecurity2` / desabilitação de verificação de assinatura**: o trecho correspondente é obra derivada de código GPL-3.0 de terceiros e **não é redistribuído** aqui, por razões éticas, de segurança e de licença.
- **Execução de binário não assinado**: não incluída.

## Escopo e postura

- O código presente neste repositório é **read-only** quanto a variáveis de segurança (`SecureBoot`, `BootOrder` são apenas lidos e registrados).
- **Não há tentativa de escrita** em variáveis de segurança neste artefato. A evidência experimental de write protection (`EFI_WRITE_PROTECTED`) consta no relatório do trabalho, não no código.
- O módulo destina-se a execução em **ambiente controlado** (OVMF em QEMU, VM isolada, sem rede).

## Créditos
- Técnicas do segundo estágio (não incluídas): autoria de **TheMalwareGuardian**, publicadas em: https://github.com/TheMalwareGuardian/Exploitation-Technique-UEFI-SecureBoot-Bypass-gSecurity2-Corruption
  sob licença GPL-3.0. Todos os créditos pelas técnicas originais são do referido autor. Este trabalho não reivindica autoria sobre elas e não as redistribui.

## Referências
- Binarly - análise LogoFAIL
- TheMalwareGuardian  - PoC público de corrupção de `gSecurity2`, citado acima.
